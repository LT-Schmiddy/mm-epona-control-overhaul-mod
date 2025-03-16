#ifndef __MOD_GLOBALS__
#define __MOD_GLOBALS__

#include "global.h"
#include "modding.h"
// #include "recompui.h"

RECOMP_IMPORT("*", u32 recomp_get_config_u32(const char* key));
RECOMP_IMPORT("*", double recomp_get_config_double(const char* key));
RECOMP_IMPORT("*", char* recomp_get_config_string(const char* key));
RECOMP_IMPORT("*", void recomp_free_config_string(char* str));

RECOMP_IMPORT("*", int recomp_printf(const char* fmt, ...));

#endif