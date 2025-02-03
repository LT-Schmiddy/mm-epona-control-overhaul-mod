#ifndef MOD_GLOBALS
#define MOD_GLOBALS

#include "global.h"
#include "modding.h"
// #include "recompui.h"

RECOMP_IMPORT("*", u32 recomp_get_config_u32(const char* key));
RECOMP_IMPORT("*", double recomp_get_config_double(const char* key));
RECOMP_IMPORT("*", char* recomp_get_config_string(const char* key));
RECOMP_IMPORT("*", void recomp_free_config_string(char* str));

#define EPONA_TURN_MULT 3.0f
#define EPONA_BRAKE_MULT 4.0f

#define TURN_SNAP_ANGLE 600
#define MINIMUM_TURN_ANGLE 600

#define USE_ALTERNATE_CONTROLS recomp_get_config_u32("control_mode")
// #define USE_ALTERNATE_CONTROLS 0

RECOMP_IMPORT("*", int recomp_printf(const char* fmt, ...));

#endif