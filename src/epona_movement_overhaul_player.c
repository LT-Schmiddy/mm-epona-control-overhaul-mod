#include "modding.h"

#include "prevent_bss_reordering.h"
#include "global.h"
#include "z64horse.h"
#include "z64quake.h"
#include "z64rumble.h"
#include "z64shrink_window.h"

#include "overlays/actors/ovl_En_Horse/z_en_horse.h"

typedef struct AnimSfxEntry {
    /* 0x0 */ u16 sfxId;
    /* 0x2 */ s16 flags; // negative marks the end
} AnimSfxEntry;          // size = 0x4

// extern char gPlayerAnim_link_uma_wait_1[];
extern PlayerAnimationHeader gPlayerAnim_link_uma_wait_3;
extern PlayerAnimationHeader gPlayerAnim_link_uma_stop_muti;

extern u8 D_8085D6DC[2][2];
extern Vec3s D_8085D6E0;
extern Input* sPlayerControlInput;
extern PlayerAnimationHeader* D_8085D6D0[];
extern PlayerAnimationHeader* D_8085D688[];
extern s32 D_80862B04;
extern AnimSfxEntry D_8085D6E8[];
extern u8 sPlayerUpperBodyLimbCopyMap[PLAYER_LIMB_MAX];
extern PlayerAnimationHeader* D_8085D6A4[];

s32 func_8082DAFC(PlayState *play);
void Player_Anim_PlayOnce(PlayState *play, Player *this, PlayerAnimationHeader *anim);
void Player_AnimSfx_PlayVoice(Player *this, u16 sfxId);
void Player_PlayAnimSfx(Player *this, AnimSfxEntry *entry);
void func_808309CC(PlayState *play, Player *this);
s32 func_80831010(Player *this, PlayState *play);
s32 Player_UpdateUpperBody(Player *this, PlayState *play);
s32 Player_ActionChange_13(Player *this, PlayState *play);
s32 Player_ActionChange_4(Player *this, PlayState *play);
s32 func_8083C62C(Player *this, s32 arg1);
void func_80841A50(PlayState *play, Player *this);
s32 func_80847BF0(Player *this, PlayState *play);
void func_80847E2C(Player *this, f32 arg1, f32 minFrame);
void func_8084FD7C(PlayState *play, Player *this, Actor *actor);
s32 func_8084FE48(Player *this);
void Player_AnimationPlayOnce(PlayState* play, Player* this, PlayerAnimationHeader* anim);
s32 func_80847190(PlayState* play, Player* this, s32 arg2);

// Called to handle player aiming on Epona.
// RECOMP_PATCH void func_8084FD7C(PlayState* play, Player* this, Actor* actor) {
//     s16 var_a3;

//     if (this->unk_B86[0] != 0) {
//         this->unk_B86[0]--;
//         return;
//     }

//     this->upperLimbRot.y = func_80847190(play, this, 1) - this->actor.shape.rot.y;

//     var_a3 = ABS_ALT(this->upperLimbRot.y) - 0x4000;
//     if (var_a3 > 0) {
//         var_a3 = CLAMP_MAX(var_a3, 0x15E);
//         actor->shape.rot.y += var_a3 * ((this->upperLimbRot.y >= 0) ? 1 : -1);
//         actor->world.rot.y = actor->shape.rot.y;
//     }

//     this->upperLimbRot.y += 0x2710;
//     this->unk_AA8 = -0x1388;
// }

// // UpdateAction for Link. Doesn't Control Epona Herself.
// RECOMP_PATCH void Player_Action_52(Player* this, PlayState* play) {
//     // recomp_printf("Riding Epona...\n");

//     EnHorse* rideActor = (EnHorse*)this->rideActor;

//     this->stateFlags2 |= PLAYER_STATE2_40;

//     func_80847E2C(this, 1.0f, 10.0f);
//     if (this->av2.actionVar2 == 0) {
//         if (PlayerAnimation_Update(play, &this->skelAnime)) {
//             this->skelAnime.animation = &gPlayerAnim_link_uma_wait_1;
//             this->av2.actionVar2 = 0x63;
//         } else {
//             s32 var_v0 = (this->mountSide < 0) ? 0 : 1;

//             if (PlayerAnimation_OnFrame(&this->skelAnime, D_8085D6DC[var_v0][0])) {
//                 Actor_SetCameraHorseSetting(play, this);
//                 Player_PlaySfx(this, NA_SE_PL_CLIMB_CLIFF);
//             } else if (PlayerAnimation_OnFrame(&this->skelAnime, D_8085D6DC[var_v0][1])) {
//                 Player_PlaySfx(this, NA_SE_PL_SIT_ON_HORSE);
//             }
//         }
//     } else {
//         if (rideActor->actor.bgCheckFlags & BGCHECKFLAG_GROUND) {
//             func_80841A50(play, this);
//         }

//         Actor_SetCameraHorseSetting(play, this);

//         this->skelAnime.prevTransl = D_8085D6E0;

//         if ((this->av2.actionVar2 < 0) ||
//             ((rideActor->animIndex != (this->av2.actionVar2 & 0xFFFF)) &&
//              ((rideActor->animIndex >= ENHORSE_ANIM_STOPPING) || (this->av2.actionVar2 >= 2)))) {
//             s32 animIndex = rideActor->animIndex;

//             if (animIndex < ENHORSE_ANIM_STOPPING) {
//                 f32 temp_fv0 = Rand_ZeroOne();
//                 s32 index = 0;

//                 animIndex = ENHORSE_ANIM_WHINNY;
//                 if (temp_fv0 < 0.1f) {
//                     index = 2;
//                 } else if (temp_fv0 < 0.2f) {
//                     index = 1;
//                 }

//                 Player_AnimationPlayOnce(play, this, D_8085D6D0[index]);
//             } else {
//                 this->skelAnime.animation = D_8085D688[animIndex - 2];
//                 if (this->av2.actionVar2 >= 0) {
//                     Animation_SetMorph(play, &this->skelAnime, 8.0f);
//                 }

//                 if (animIndex < ENHORSE_ANIM_WALK) {
//                     func_808309CC(play, this);
//                     this->av1.actionVar1 = 0;
//                 }
//             }

//             this->av2.actionVar2 = animIndex;
//         }

//         if (this->av2.actionVar2 == 1) {
//             if (D_80862B04 || func_8082DAFC(play)) {
//                 Player_AnimationPlayOnce(play, this, &gPlayerAnim_link_uma_wait_3);
//             } else if (PlayerAnimation_Update(play, &this->skelAnime)) {
//                 this->av2.actionVar2 = 0x63;
//             } else if (this->skelAnime.animation == &gPlayerAnim_link_uma_wait_1) {
//                 Player_PlayAnimSfx(this, D_8085D6E8);
//             }
//         } else {
//             this->skelAnime.curFrame = rideActor->curFrame;
//             PlayerAnimation_AnimateFrame(play, &this->skelAnime);
//         }

//         AnimationContext_SetCopyAll(play, this->skelAnime.limbCount, this->skelAnime.morphTable,
//                                     this->skelAnime.jointTable);

//         if ((play->csCtx.state != CS_STATE_IDLE) || (this->csAction != PLAYER_CSACTION_NONE)) {
//             this->unk_AA5 = PLAYER_UNKAA5_0;
//             this->av1.actionVar1 = 0;
//         } else if ((this->av2.actionVar2 < 2) || (this->av2.actionVar2 >= 4)) {
//             D_80862B04 = Player_UpdateUpperBody(this, play);
//             if (D_80862B04) {
//                 this->av1.actionVar1 = 0;
//             }
//         }

//         this->actor.world.pos.x = rideActor->actor.world.pos.x + rideActor->riderPos.x;
//         this->actor.world.pos.y = rideActor->actor.world.pos.y + rideActor->riderPos.y - 27.0f;
//         this->actor.world.pos.z = rideActor->actor.world.pos.z + rideActor->riderPos.z;

//         this->currentYaw = this->actor.shape.rot.y = rideActor->actor.shape.rot.y;

//         if (!D_80862B04) {
//             if (this->av1.actionVar1 != 0) {
//                 if (PlayerAnimation_Update(play, &this->skelAnimeUpper)) {
//                     rideActor->stateFlags &= ~ENHORSE_FLAG_8;
//                     this->av1.actionVar1 = 0;
//                 }

//                 if (this->skelAnimeUpper.animation == &gPlayerAnim_link_uma_stop_muti) {
//                     if (PlayerAnimation_OnFrame(&this->skelAnimeUpper, 23.0f)) {
//                         Player_PlaySfx(this, NA_SE_IT_LASH);
//                         Player_AnimSfx_PlayVoice(this, NA_SE_VO_LI_LASH);
//                     }

//                     AnimationContext_SetCopyAll(play, this->skelAnime.limbCount, this->skelAnime.jointTable,
//                                                 this->skelAnimeUpper.jointTable);
//                 } else {
//                     if (PlayerAnimation_OnFrame(&this->skelAnimeUpper, 10.0f)) {
//                         Player_PlaySfx(this, NA_SE_IT_LASH);
//                         Player_AnimSfx_PlayVoice(this, NA_SE_VO_LI_LASH);
//                     }

//                     AnimationContext_SetCopyTrue(play, this->skelAnime.limbCount, this->skelAnime.jointTable,
//                                                  this->skelAnimeUpper.jointTable, sPlayerUpperBodyLimbCopyMap);
//                 }
//             } else if (!CHECK_FLAG_ALL(this->actor.flags, 0x100)) {
//                 PlayerAnimationHeader* anim = NULL;

//                 if (EN_HORSE_CHECK_3(rideActor)) {
//                     anim = &gPlayerAnim_link_uma_stop_muti;
//                 } else if (EN_HORSE_CHECK_2(rideActor)) {
//                     if ((this->av2.actionVar2 >= 2) && (this->av2.actionVar2 != 0x63)) {
//                         anim = D_8085D6A4[this->av2.actionVar2];
//                     }
//                 }

//                 if (anim != NULL) {
//                     PlayerAnimation_PlayOnce(play, &this->skelAnimeUpper, anim);
//                     this->av1.actionVar1 = 1;
//                 }
//             }
//         }

//         if (this->stateFlags1 & PLAYER_STATE1_100000) {
//             if (CHECK_BTN_ANY(sPlayerControlInput->press.button, BTN_A) || !func_8084FE48(this)) {
//             // if (1 || !func_8084FE48(this)) {
//                 this->unk_AA5 = PLAYER_UNKAA5_0;
//                 this->stateFlags1 &= ~PLAYER_STATE1_100000;
//             } else {
//                 func_8084FD7C(play, this, &rideActor->actor);
//             }
//         } else if ((this->csAction != PLAYER_CSACTION_NONE) ||
//                    (!func_8082DAFC(play) && ((rideActor->actor.speed != 0.0f) || !Player_ActionChange_4(this, play)) &&
//                     !func_80847BF0(this, play) && !Player_ActionChange_13(this, play))) {
//             if (this->lockOnActor != NULL) {
//                 if (func_800B7128(this)) {
//                     this->upperLimbRot.y = func_8083C62C(this, true) - this->actor.shape.rot.y;
//                     this->upperLimbRot.y = CLAMP(this->upperLimbRot.y, -0x4AAA, 0x4AAA);
//                     this->actor.focus.rot.y = this->actor.shape.rot.y + this->upperLimbRot.y;
//                     this->upperLimbRot.y += 0xFA0;
//                     this->unk_AA6 |= 0x80;
//                 } else {
//                     func_8083C62C(this, false);
//                 }

//                 this->unk_AA8 = 0;
//             } else if (func_8084FE48(this)) {
//                 if (func_800B7128(this)) {
//                     func_80831010(this, play);
//                 }

//                 this->unk_B86[0] = 0xC;
//             } else if (func_800B7128(this)) {
//                 func_8084FD7C(play, this, &rideActor->actor);
//             }
//         }
//     }

//     if (this->csAction == PLAYER_CSACTION_END) {
//         this->csAction = PLAYER_CSACTION_NONE;
//     }
// }