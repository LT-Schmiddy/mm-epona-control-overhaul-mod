#ifndef MOD_CONFIG
#define MOD_CONFIG

#include "mod_globals.h"
#include "overlays/actors/ovl_En_Horse/z_en_horse.h"

// This file contains functions, variables and macros used to configure values related to Epona's movement.

#define USE_ALTERNATE_CONTROLS use_alternate_controls(this, play)

// #define EPONA_GLOBAL_TURN_MULT 3.0f
// #define EPONA_GLOBAL_BRAKE_MULT 4.0f
#define EPONA_GLOBAL_TURN_MULT (USE_ALTERNATE_CONTROLS ? (float)recomp_get_config_double("freeform_turn_mult") : (float)recomp_get_config_double("tank_brake_mult"))
#define EPONA_GLOBAL_BRAKE_MULT (USE_ALTERNATE_CONTROLS ? (float)recomp_get_config_double("freeform_brake_mult") : (float)recomp_get_config_double("tank_brake_mult"))

#define MINIMUM_TURN_ANGLE (int)recomp_get_config_u32("freeform_min_turn_speed")

// f32 brakeDecel, f32 brakeAngle, f32 minStickMag, f32 decel, f32 baseSpeed, s16 turnSpeed
#define WALK_SPEED_ARGS 0.3f, -0.5f, 10.0f, 0.06f, 3.0f, USE_ALTERNATE_CONTROLS ? 400 : 800
#define TROT_SPEED_ARGS 0.3f, -0.5f, 10.0f, 0.06f, 6.0f, 800
#define GALLOP_SPEED_ARGS 0.3f, -0.5f, 10.0f, 0.06f, 8.0f, 800

// ORIGINAL VALUES:
// #define WALK_SPEED_ARGS 0.3f, -0.5f, 10.0f, 0.06f, 3.0f, 0x320
// #define TROT_SPEED_ARGS 0.3f, -0.5f, 10.0f, 0.06f, 6.0f, 800
// #define GALLOP_SPEED_ARGS 0.3f, -0.5f, 10.0f, 0.06f, 8.0f, 800


inline u32 is_player_aiming(EnHorse* this, PlayState* play) {
    Player* player = GET_PLAYER(play);

    return player->stateFlags1 & PLAYER_STATE1_100000;
}

inline u32 use_alternate_controls(EnHorse* this, PlayState* play) {
    u32 mode = recomp_get_config_u32("control_mode");

    if (mode < 2) {
        return mode;
    }

    return !is_player_aiming(this, play);
}

    
#endif

