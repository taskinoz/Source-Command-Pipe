//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//
//===========================================================================//

#include <stdio.h>
#include <windows.h> 
#include <thread>
#include <string>
#include <vector>

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
	char m_pipeBuffer[1024];
	DWORD m_pipeDwRead;
	std::thread m_pipeThread;
	IVEngineServer* m_engine;
	edict_t* m_client;

	std::vector<char*> StrSplit(char* s, const char* delimiters);

	void PipeThread();
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
{
	m_iClientCommandIndex = 0;

	m_ipcPipe = CreateNamedPipe(TEXT("\\\\.\\pipe\\SourceCommands"),
		PIPE_ACCESS_DUPLEX,
		PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,   // FILE_FLAG_FIRST_PIPE_INSTANCE is not needed but forces CreateNamedPipe(..) to fail if the pipe already exists...
		1,
		1024 * 16,
		1024 * 16,
		NMPWAIT_USE_DEFAULT_WAIT,
		NULL);
}

std::vector<char *> CEmptyServerPlugin::StrSplit(char * s, const char * delimiters) {
	size_t pos = 0;
	std::vector<char *> strings;
	char * pch = strtok(s, delimiters);
	while (pch != NULL)
	{
		g_pCVar->ConsolePrintf("%s\n", pch);
		strings.push_back(pch);
		pch = strtok(NULL, delimiters);
	}

	return strings;
}

void CEmptyServerPlugin::PipeThread() {
	while (m_ipcPipe != INVALID_HANDLE_VALUE) {
		if (ConnectNamedPipe(m_ipcPipe, NULL) != FALSE) {
			while (ReadFile(m_ipcPipe, m_pipeBuffer, sizeof(m_pipeBuffer) - 1, &m_pipeDwRead, NULL)) {
				// add terminating zero
				m_pipeBuffer[m_pipeDwRead] = '\0';
				/* Old code for setting cvars
				auto spl = CEmptyServerPlugin::StrSplit(m_pipeBuffer, " ");

				if (spl.size() < 2) {
					g_pCVar->ConsoleColorPrintf(Color(200, 50, 50, 255), "Recieved no cvar arguments from pipe. Command was %s\n", m_pipeBuffer);
					continue;
				}

				ConVar* test = g_pCVar->FindVar(spl[0]);
				//spl.erase(spl.begin());
				
				test->SetValue(spl[1]);
				*/

				m_engine->ClientCommand(m_client, m_pipeBuffer);
			}
		}
		DisconnectNamedPipe(m_ipcPipe);
	}
}

CEmptyServerPlugin::~CEmptyServerPlugin() {
	m_pipeThread.join();
	CloseHandle(m_ipcPipe);
}

// Test command
ConCommand testCommand2("hkva_test", []() {
	g_pCVar->ConsolePrintf("Test command");
	});

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

	m_pipeThread = std::thread(&CEmptyServerPlugin::PipeThread, this);

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is unloaded (turned off)
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::Unload(void)
{
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
	return "Test Plugin, hkva";
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
	m_client = pEntity;
}

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