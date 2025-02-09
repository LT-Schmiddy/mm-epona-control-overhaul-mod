#ifndef MOD_CONFIG
#define MOD_CONFIG

#include "mod_globals.h"


#define USE_ALTERNATE_CONTROLS recomp_get_config_u32("control_mode")

#define EPONA_GLOBAL_TURN_MULT 3.0f
#define EPONA_GLOBAL_BRAKE_MULT 4.0f

#define MINIMUM_TURN_ANGLE 900

// #define IDLE_CONTROL_ANGLE 0.9f

// f32 brakeDecel, f32 brakeAngle, f32 minStickMag, f32 decel, f32 baseSpeed, s16 turnSpeed
#define WALK_SPEED_ARGS 0.3f, -0.5f, 10.0f, 0.06f, 3.0f, 400
#define TROT_SPEED_ARGS 0.3f, -0.5f, 10.0f, 0.06f, 6.0f, 800
#define GALLOP_SPEED_ARGS 0.3f, -0.5f, 10.0f, 0.06f, 8.0f, 800

// ORIGINAL VALUES:
// #define WALK_SPEED_ARGS 0.3f, -0.5f, 10.0f, 0.06f, 3.0f, 0x320
// #define TROT_SPEED_ARGS 0.3f, -0.5f, 10.0f, 0.06f, 6.0f, 800
// #define GALLOP_SPEED_ARGS 0.3f, -0.5f, 10.0f, 0.06f, 8.0f, 800



#endif

