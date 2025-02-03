#include "mod_globals.h"
// #include "config.h"

#include "prevent_bss_reordering.h"
#include "z64horse.h"
#include "z64quake.h"
#include "z64rumble.h"
#include "z64shrink_window.h"

#include "overlays/actors/ovl_En_Horse_Game_Check/z_en_horse_game_check.h"
#include "overlays/actors/ovl_En_Horse/z_en_horse.h"
// #include "overlays/actors/ovl_En_In/z_en_in.h"

#define OBJECT_IN_LIMB_MAX 0x14

typedef void (*EnInActionFunc)(struct EnIn*, PlayState*);
typedef struct EnIn {
    /* 0x000 */ Actor actor;
    /* 0x144 */ EnInActionFunc actionFunc;
    /* 0x148 */ SkelAnime skelAnime;
    /* 0x18C */ ColliderJntSph colliderJntSph;
    /* 0x1AC */ ColliderJntSphElement colliderJntSphElement;
    /* 0x1EC */ ColliderCylinder colliderCylinder;
    /* 0x238 */ UNK_TYPE1 unk238[0x4];
    /* 0x23C */ u8 unk23C;
    /* 0x23D */ u8 unk23D;
    /* 0x240 */ Path* path;
    /* 0x244 */ s16 unk244;
    /* 0x248 */ Vec3f unk248;
    /* 0x254 */ Vec3f unk254;
    /* 0x260 */ u8 unk260;
    /* 0x261 */ u8 unk261;
    /* 0x262 */ Vec3s jointTable[OBJECT_IN_LIMB_MAX];
    /* 0x2DA */ Vec3s morphTable[OBJECT_IN_LIMB_MAX];
    /* 0x352 */ Vec3s trackTarget;
    /* 0x358 */ Vec3s headRot;
    /* 0x35E */ Vec3s torsoRot;
    /* 0x364 */ UNK_TYPE1 unk364[0x12];
    /* 0x376 */ s16 fidgetTableY[OBJECT_IN_LIMB_MAX];
    /* 0x39E */ s16 fidgetTableZ[OBJECT_IN_LIMB_MAX];
    /* 0x3C6 */ UNK_TYPE1 unk3C6[0xBC];
    /* 0x482 */ s16 unk482;
    /* 0x484 */ s16 unk484;
    /* 0x486 */ s16 unk486;
    /* 0x488 */ s16 animIndex2;
    /* 0x48A */ u16 unk48A;
    /* 0x48C */ s32 unk48C;
    /* 0x490 */ UNK_TYPE1 unk490[0x4];
    /* 0x494 */ s32 unk494;
    /* 0x498 */ UNK_TYPE1 unk498[0xC];
    /* 0x4A4 */ struct EnIn* unk4A4;
    /* 0x4A8 */ s32 unk4A8;
    /* 0x4AC */ s32 unk4AC;
    /* 0x4B0 */ s32 unk4B0;
    /* 0x4B4 */ Vec3f unk4B4;
    /* 0x4C0 */ f32 unk4C0;
    /* 0x4C0 */ f32 unk4C4;
    /* 0x4C0 */ s32 prevTalkState;
} EnIn; // size = 0x4CC

typedef struct 
{
  s16 x;
  s16 y;
  s16 z;
  s16 speed;
  s16 angle;
} RaceWaypoint;
typedef struct 
{
  s32 numWaypoints;
  RaceWaypoint *waypoints;
} RaceInfo;


void EnHorse_StartMountedIdle(EnHorse *this);
void EnHorse_MountedIdleAnim(EnHorse *this);
void EnHorse_StartWalkingFromIdle(EnHorse *this);
void EnHorse_StartGallopingInterruptable(EnHorse *this);
void EnHorse_StartGalloping(EnHorse *this);
void EnHorse_StartBraking(EnHorse *this, PlayState *play);
void EnHorse_ChangeIdleAnimation(EnHorse *this, s32 animIndex, f32 morphFrames);
void EnHorse_StartMovingAnimation(EnHorse *this, s32 animIndex, f32 morphFrames, f32 startFrames);
void func_8088159C(EnHorse *this, PlayState *play);
void EnHorse_StickDirection(Vec2f *curStick, f32 *stickMag, s16 *angle);
void func_8087B7C0(EnHorse *this, PlayState *play, Path *path);
void EnHorse_PlayWalkingSound(EnHorse *this);
void func_8087C178(EnHorse *this);
void func_8087C1C0(EnHorse *this);
f32 EnHorse_SlopeSpeedMultiplier(EnHorse *this, PlayState *play);
_Bool func_8087C38C(PlayState *play, EnHorse *this, Vec3f *arg2);
void EnHorse_IdleAnimSounds(EnHorse *this, PlayState *play);
s32 EnHorse_Spawn(EnHorse *this, PlayState *play);
s32 EnHorse_PlayerCanMove(EnHorse *this, PlayState *play);
void EnHorse_RotateToPlayer(EnHorse *this, PlayState *play);
void EnHorse_Freeze(EnHorse *this, PlayState *play);
void EnHorse_UpdateSpeed(EnHorse *this, PlayState *play, f32 brakeDecel, f32 brakeAngle, f32 minStickMag, f32 decel, f32 baseSpeed, s16 turnSpeed);
void EnHorse_StartMountedIdleResetAnim(EnHorse *this);
void EnHorse_StartTurning(EnHorse *this);
void EnHorse_StartWalkingInterruptable(EnHorse *this);
void EnHorse_StartWalking(EnHorse *this);
void EnHorse_MountedWalkingReset(EnHorse *this);
void EnHorse_StartTrotting(EnHorse *this);
void EnHorse_MountedTrotReset(EnHorse *this);
void EnHorse_MountedGallopReset(EnHorse *this);
void EnHorse_JumpLanding(EnHorse *this, PlayState *play);
void EnHorse_StartRearing(EnHorse *this);
void EnHorse_StartReversingInterruptable(EnHorse *this);
void EnHorse_StartReversing(EnHorse *this);
void EnHorse_PlayIdleAnimation(EnHorse *this, s32 animIndex, f32 morphFrames, f32 startFrames);
void EnHorse_StartIdleRidable(EnHorse *this);
void EnHorse_SetFollowAnimation(EnHorse *this, PlayState *play);
void EnHorse_SetIngoAnimation(s32 animIndex, f32 curFrame, s32 arg2, s16 *animIndexOut);
void EnHorse_UpdateIngoHorseAnim(EnHorse *this);
void func_8088168C(EnHorse *this);
s32 EnHorse_GetCutsceneFunctionIndex(s32 cueId);
s32 EnHorse_UpdateHbaRaceInfo(EnHorse *this, PlayState *play, RaceInfo *raceInfo);
void EnHorse_UpdateHbaAnim(EnHorse *this);
void func_80884868(EnHorse *this);
void EnHorse_CheckFloors(EnHorse *this, PlayState *play);
void EnHorse_MountDismount(EnHorse *this, PlayState *play);
void EnHorse_UpdateStick(EnHorse *this, PlayState *play);
void EnHorse_UpdateBgCheckInfo(EnHorse *this, PlayState *play);
void func_80886C00(EnHorse *this, PlayState *play);
void EnHorse_RegenBoost(EnHorse *this, PlayState *play);
void EnHorse_UpdatePlayerDir(EnHorse *this, PlayState *play);
void EnHorse_TiltBody(EnHorse *this, PlayState *play);
s32 EnHorse_UpdateConveyors(EnHorse *this, PlayState *play);
s32 EnHorse_RandInt(f32 arg0);

// New Declarations:
void EnHorse_Frozen(EnHorse* this, PlayState* play);
void EnHorse_Inactive(EnHorse* this, PlayState* play);
void EnHorse_Idle(EnHorse* this, PlayState* play);
void EnHorse_FollowPlayer(EnHorse* this, PlayState* play);
void EnHorse_UpdateIngoRace(EnHorse* this, PlayState* play);
void func_808819D8(EnHorse* this, PlayState* play);
void func_80881398(EnHorse* this, PlayState* play);
void EnHorse_Stopping(EnHorse* this, PlayState* play);
void EnHorse_LowJump(EnHorse* this, PlayState* play);
void EnHorse_HighJump(EnHorse* this, PlayState* play);
void EnHorse_CutsceneUpdate(EnHorse* this, PlayState* play);
void EnHorse_UpdateHorsebackArchery(EnHorse* this, PlayState* play);
void EnHorse_FleePlayer(EnHorse* this, PlayState* play);
void func_80884718(EnHorse* this, PlayState* play);
void func_8087CA04(EnHorse* this, PlayState* play);
void func_808848C8(EnHorse* this, PlayState* play);
void func_80884A40(EnHorse* this, PlayState* play);
void func_80884E0C(EnHorse* this, PlayState* play);