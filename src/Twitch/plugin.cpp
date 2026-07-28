//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//
//===========================================================================//

#include <windows.h>
#include <atomic>
#include <deque>
#include <mutex>
#include <thread>
#include <string>

//#define GAME_DLL
#ifdef GAME_DLL
#include "cbase.h"
#endif

#include <stdio.h>
#include "interface.h"
#include "engine/iserverplugin.h"
#include "eiface.h"
#include "igameevents.h"
#include "convar.h"
#include "tier2/tier2.h"

//---------------------------------------------------------------------------------
// Purpose: a sample 3rd party plugin class
//---------------------------------------------------------------------------------
class CEmptyServerPlugin : public IServerPluginCallbacks, public IGameEventListener
{
public:
	CEmptyServerPlugin();
	~CEmptyServerPlugin();

	// IServerPluginCallbacks methods
	virtual bool Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory);
	virtual void Unload(void);
	virtual void Pause(void);
	virtual void UnPause(void);
	virtual const char* GetPluginDescription(void);
	virtual void LevelInit(char const* pMapName);
	virtual void ServerActivate(edict_t* pEdictList, int edictCount, int clientMax);
	virtual void GameFrame(bool simulating);
	virtual void LevelShutdown(void);
	virtual void ClientActive(edict_t* pEntity);
#if defined(SOURCE_ENGINE_PORTAL2)
	virtual void ClientFullyConnect(edict_t* pEntity);
#endif
	virtual void ClientDisconnect(edict_t* pEntity);
	virtual void ClientPutInServer(edict_t* pEntity, char const* playername);
	virtual void SetCommandClient(int index);
	virtual void ClientSettingsChanged(edict_t* pEdict);
	virtual PLUGIN_RESULT ClientConnect(bool* bAllowConnect, edict_t* pEntity, const char* pszName, const char* pszAddress, char* reject, int maxrejectlen);
	virtual PLUGIN_RESULT ClientCommand(edict_t* pEntity, const CCommand& args);
	virtual PLUGIN_RESULT NetworkIDValidated(const char* pszUserName, const char* pszNetworkID);
	virtual void OnQueryCvarValueFinished(QueryCvarCookie_t iCookie, edict_t* pPlayerEntity, EQueryCvarValueStatus eStatus, const char* pCvarName, const char* pCvarValue);
	virtual void OnEdictAllocated(edict_t* edict);
	virtual void OnEdictFreed(const edict_t* edict);

	// IGameEventListener Interface
	virtual void FireGameEvent(KeyValues* event);

	virtual int GetCommandIndex() { return m_iClientCommandIndex; }

private:
	int m_iClientCommandIndex;
	HANDLE m_ipcPipe;
	std::thread m_pipeThread;
	IVEngineServer* m_engine;
	std::atomic<bool> m_running;
	std::mutex m_commandMutex;
	std::deque<std::string> m_commands;

	void PipeThread();
	void StopPipe();
};

//
// The plugin is a static singleton that is exported as an interface
//
CEmptyServerPlugin g_EmtpyServerPlugin;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CEmptyServerPlugin, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS, g_EmtpyServerPlugin);

//---------------------------------------------------------------------------------
// Purpose: constructor/destructor
//---------------------------------------------------------------------------------
CEmptyServerPlugin::CEmptyServerPlugin()
	: m_iClientCommandIndex(0),
	  m_ipcPipe(INVALID_HANDLE_VALUE),
	  m_engine(nullptr),
	  m_running(false)
{
}

void CEmptyServerPlugin::PipeThread() {
	char buffer[4096];
	while (m_running.load()) {
		const BOOL connected = ConnectNamedPipe(m_ipcPipe, nullptr) || GetLastError() == ERROR_PIPE_CONNECTED;
		if (!connected) {
			if (!m_running.load()) break;
			continue;
		}

		DWORD bytesRead = 0;
		while (m_running.load() && ReadFile(m_ipcPipe, buffer, sizeof(buffer), &bytesRead, nullptr)) {
			if (bytesRead == 0) continue;
			std::string command(buffer, bytesRead);
			while (!command.empty() && (command.back() == '\0' || command.back() == '\r' || command.back() == '\n'))
				command.pop_back();
			if (!command.empty()) {
				std::lock_guard<std::mutex> lock(m_commandMutex);
				m_commands.push_back(command);
			}
		}
		if (m_ipcPipe != INVALID_HANDLE_VALUE) DisconnectNamedPipe(m_ipcPipe);
	}
}

void CEmptyServerPlugin::StopPipe() {
	m_running.store(false);
	if (m_ipcPipe != INVALID_HANDLE_VALUE) {
		CancelIoEx(m_ipcPipe, nullptr);
		DisconnectNamedPipe(m_ipcPipe);
	}
	if (m_pipeThread.joinable()) m_pipeThread.join();
	if (m_ipcPipe != INVALID_HANDLE_VALUE) {
		CloseHandle(m_ipcPipe);
		m_ipcPipe = INVALID_HANDLE_VALUE;
	}
}

CEmptyServerPlugin::~CEmptyServerPlugin() { StopPipe(); }

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is loaded, load the interface we need from the engine
//---------------------------------------------------------------------------------
bool CEmptyServerPlugin::Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory)
{
	ConnectTier1Libraries(&interfaceFactory, 1);
	ConnectTier2Libraries(&interfaceFactory, 1);

	MathLib_Init(2.2f, 2.2f, 0.0f, 2.0f);
	ConVar_Register(0);

	m_engine = (IVEngineServer*)interfaceFactory(INTERFACEVERSION_VENGINESERVER, NULL);
	if (!m_engine) return false;

	m_ipcPipe = CreateNamedPipeA("\\\\.\\pipe\\SourceCommands",
		PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
		1, 16 * 1024, 16 * 1024, NMPWAIT_USE_DEFAULT_WAIT, nullptr);
	if (m_ipcPipe == INVALID_HANDLE_VALUE) return false;
	m_running.store(true);
	m_pipeThread = std::thread(&CEmptyServerPlugin::PipeThread, this);

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is unloaded (turned off)
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::Unload(void)
{
	StopPipe();
	ConVar_Unregister();
	DisconnectTier2Libraries();
	DisconnectTier1Libraries();
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is paused (i.e should stop running but isn't unloaded)
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::Pause(void)
{
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is unpaused (i.e should start executing again)
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::UnPause(void)
{
}

//---------------------------------------------------------------------------------
// Purpose: the name of this plugin, returned in "plugin_print" command
//---------------------------------------------------------------------------------
const char* CEmptyServerPlugin::GetPluginDescription(void)
{
	return "Source Command Pipe";
}

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::LevelInit(char const* pMapName)
{
}

//---------------------------------------------------------------------------------
// Purpose: called on level start, when the server is ready to accept client connections
//		edictCount is the number of entities in the level, clientMax is the max client count
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::ServerActivate(edict_t* pEdictList, int edictCount, int clientMax)
{
}

//---------------------------------------------------------------------------------
// Purpose: called once per server frame, do recurring work here (like checking for timeouts)
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::GameFrame(bool simulating) {
	std::deque<std::string> commands;
	{
		std::lock_guard<std::mutex> lock(m_commandMutex);
		commands.swap(m_commands);
	}
	for (std::string& command : commands) {
		command.push_back('\n');
		m_engine->ServerCommand(command.c_str());
	}
}

//---------------------------------------------------------------------------------
// Purpose: called on level end (as the server is shutting down or going to a new map)
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::LevelShutdown(void) // !!!!this can get called multiple times per map change
{
}

//---------------------------------------------------------------------------------
// Purpose: called when a client spawns into a server (i.e as they begin to play)
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::ClientActive(edict_t* pEntity) {
}

#if defined(SOURCE_ENGINE_PORTAL2)
void CEmptyServerPlugin::ClientFullyConnect(edict_t* pEntity) {
}
#endif

//---------------------------------------------------------------------------------
// Purpose: called when a client leaves a server (or is timed out)
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::ClientDisconnect(edict_t* pEntity)
{
}

//---------------------------------------------------------------------------------
// Purpose: called on
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::ClientPutInServer(edict_t* pEntity, char const* playername)
{
}

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::SetCommandClient(int index)
{
	m_iClientCommandIndex = index;
}

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::ClientSettingsChanged(edict_t* pEdict)
{
}

//---------------------------------------------------------------------------------
// Purpose: called when a client joins a server
//---------------------------------------------------------------------------------
PLUGIN_RESULT CEmptyServerPlugin::ClientConnect(bool* bAllowConnect, edict_t* pEntity, const char* pszName, const char* pszAddress, char* reject, int maxrejectlen)
{
	return PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: called when a client types in a command (only a subset of commands however, not CON_COMMAND's)
//---------------------------------------------------------------------------------
PLUGIN_RESULT CEmptyServerPlugin::ClientCommand(edict_t* pEntity, const CCommand& args)
{
	return PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: called when a client is authenticated
//---------------------------------------------------------------------------------
PLUGIN_RESULT CEmptyServerPlugin::NetworkIDValidated(const char* pszUserName, const char* pszNetworkID)
{
	return PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: called when a cvar value query is finished
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::OnQueryCvarValueFinished(QueryCvarCookie_t iCookie, edict_t* pPlayerEntity, EQueryCvarValueStatus eStatus, const char* pCvarName, const char* pCvarValue)
{
}
void CEmptyServerPlugin::OnEdictAllocated(edict_t* edict)
{
}
void CEmptyServerPlugin::OnEdictFreed(const edict_t* edict)
{
}

//---------------------------------------------------------------------------------
// Purpose: called when an event is fired
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::FireGameEvent(KeyValues* event)
{
}
