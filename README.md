# Installation

Add `Twitch.dll` to the game files and use `plugin_load Twitch`

Run the Test.exe or `node index.js` to test the plugin. You should see the gravity change gradually then kill the player.

## Game Support

`Twitch.dll` works with:

- Portal 1

- Half Life: Source

- Half Life 2


`Twitch-bm.dll` works with:

- Black Mesa

To load the plugin with Black Mesa using `plugin_load Twitch-bm`

## Build Instructions

1. Clone the project including submodules `git clone --recurse-submodules https://github.com/taskinoz/Source-Command-Pipe.git`

1. Open the project in Visual Studio

1. Build project

    1. If you get an error rename `#include <typeinfo.h>` in `memalloc.h` to `#include <typeinfo>`
