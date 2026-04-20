# MaKeHeader

a simple tool to transform a file into a C header for embeding resources

## Usage

`mkh [input] [--flags]`

### Example

`mkh effect.wav`

creates file effect_wav.h, that can be used like a stb style header

```c
#define EFFECT_WAV_DATA // to include data with the declaration
#include "effect_wav.h"

...
wave_t = load_wave_from_mem(effect_wav,EFFECT_WAV_SIZE); // mkh also define a _SIZE macro with the file size
...

```

it can also transform text files into strings, using `--string` flag

`mkh script.lua --string`

creates a file the file script_lua_h

```c
#define SCRIPT_LUA_DATA
#include "script_lua.h"

...
if (luaL_dostring(L,script_lua) != LUA_OK)
    luaL_error(L,"ERROR: %s\n",lua_tostring(L,-1));
...

```

### Flags

`mkh --help` prints the help text with flags information
