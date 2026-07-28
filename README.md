# Source Command Pipe

A 32-bit Source engine server plugin that exposes a Windows named pipe at
`\\.\pipe\SourceCommands`. Programs such as
[source-twitch-integration](https://github.com/taskinoz/source-twitch-integration)
can write console commands to the pipe. Commands are queued by the IPC thread and
executed safely on the engine thread during `GameFrame`.

## Game builds

Every CI run produces five branch-specific 32-bit DLLs:

| Artifact | HL2SDK branch | Games |
| --- | --- | --- |
| `SourceCommandPipe-sdk2013.dll` | `sdk2013` | Portal, Half-Life: Source, Half-Life 2 |
| `SourceCommandPipe-bms.dll` | `bms` | Black Mesa |
| `SourceCommandPipe-portal2.dll` | `portal2` | Portal 2 |
| `SourceCommandPipe-l4d.dll` | `l4d` | Left 4 Dead |
| `SourceCommandPipe-l4d2.dll` | `l4d2` | Left 4 Dead 2 |

The DLLs are not interchangeable. Each engine branch exposes a different ABI, so
use the artifact named for the game. The builds are compiler-verified; runtime
verification still requires installing each game.

## Install

1. Download the DLL for your game from a GitHub Actions artifact or release.
2. Copy it into the game's binary directory.
3. Run `plugin_load SourceCommandPipe-<game>` in the game console, using the copied
   DLL's filename without `.dll`.
4. Write a command (without a trailing newline) to `\\.\pipe\SourceCommands`.

Only run trusted IPC clients. Commands received through the pipe have the same
privileges as commands entered in the server console.

## Test the pipe

With the plugin loaded and a map running, use the included PowerShell client:

```powershell
.\test-pipe.ps1 "sv_gravity 400"
```

The script connects to `\\.\pipe\SourceCommands` and sends its first argument as a
server-console command. It is attached to every GitHub release and is also available
at `examples/test-pipe.ps1` in the repository.

## Build locally

Requirements: Visual Studio 2022 with **Desktop development with C++**, Git, and a
Windows 10 or 11 SDK.

```powershell
git clone --recurse-submodules https://github.com/taskinoz/Source-Command-Pipe.git
cd Source-Command-Pipe
msbuild src/Twitch.sln /m /p:Configuration=Release /p:Platform=x86
```

If the repository was cloned without dependencies, run:

```powershell
git submodule sync --recursive
git submodule update --init --recursive
```

The default output is `src/build/Release/SourceCommandPipe-sdk2013.dll`. GitHub
Actions builds all five variants for every push and pull request; pushing a tag such
as `v1.0.0` publishes all five DLLs in one release.

To build every variant locally from a Visual Studio Developer PowerShell, run:

```powershell
.\scripts\build-all.ps1
```

This checks out each HL2SDK branch into the ignored `src/build/dependencies`
directory, avoiding the broken and error-prone practice of repeatedly changing the
tracked submodule branch.

To build against another checked-out HL2SDK branch:

```powershell
msbuild src/Twitch.sln /m /p:Configuration=Release /p:Platform=x86 /p:SourceEngine=l4d /p:HL2SDKRoot=C:\path\to\hl2sdk-l4d
```

## Left 4 Dead command execution

Older versions remembered the last entity passed to `ClientActive` and called
`ClientCommand` from the pipe worker. That can fail in L4D because a multiplayer
server may activate bots before a human client, clients can disconnect, and engine
interfaces are not thread-safe. This version queues pipe messages and calls
`IVEngineServer::ServerCommand` from `GameFrame`, so commands execute as server-console
commands without depending on a particular player.
