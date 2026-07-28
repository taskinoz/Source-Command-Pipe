# Source Command Pipe

A 32-bit Source engine server plugin that exposes a Windows named pipe at
`\\.\pipe\SourceCommands`. Programs such as
[source-twitch-integration](https://github.com/taskinoz/source-twitch-integration)
can write console commands to the pipe. Commands are queued by the IPC thread and
executed safely on the engine thread during `GameFrame`.

## Supported games

The current build uses AlliedModders' `sdk2013` HL2SDK branch and supports:

- Portal
- Half-Life: Source
- Half-Life 2

Black Mesa previously used a separate build and is not produced by the current
automated build. Portal 2, Left 4 Dead, and Left 4 Dead 2 need builds against their
respective `portal2`, `l4d`, and `l4d2` HL2SDK branches; the project now accepts an
`HL2SDKRoot` MSBuild property so those variants can be added without duplicating it.

## Install

1. Download `SourceCommandPipe.dll` from a GitHub Actions build artifact or release.
2. Copy it into the game's binary directory.
3. Run `plugin_load SourceCommandPipe` in the game console.
4. Write a command (without a trailing newline) to `\\.\pipe\SourceCommands`.

Only run trusted IPC clients. Commands received through the pipe have the same
privileges as commands entered in the server console.

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

The output is `src/build/Release/SourceCommandPipe.dll`. GitHub Actions builds every
push and pull request; pushing a tag such as `v1.0.0` creates a release automatically.

To build against another checked-out HL2SDK branch:

```powershell
msbuild src/Twitch.sln /m /p:Configuration=Release /p:Platform=x86 /p:HL2SDKRoot=C:\path\to\hl2sdk
```
