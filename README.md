# MaKeHeader

a simple tool to transform a binary file into a c header for embeding resources

## Usage

`mkh [input] [--options]`

### Example

`mkh effect.wav`

creates file effect_wav.h, that can be used like a stb_style header

```c
#define EFFECT_WAV_DATA // to include data with the declaration
#include "effect_wav.h"

...
wave_t = load_wave_from_mem(effect_wav,EFFECT_WAV_SIZE); // mkh also define a _SIZE macro with the file size
...

```

### Flags

- --const   makes data const
- --static  makes data static (local to TU)
