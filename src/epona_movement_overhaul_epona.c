#include "epona_movement_overhaul_epona.h"

typedef struct {
    f32 stickMag;
    s16 stickAngle;
    s16 desiredDirection;
    s16 desiredDirectionDelta;
    s16 turnAngle;
} TurnInfo;



Camera* getActiveCamera(PlayState* play) {
    return play->cameraPtrs[play->activeCamId];
}

void EnHorse_GetTurnInfo(EnHorse* this, PlayState* play, Vec2f* curStick, TurnInfo* out_turnInfo) {
    EnHorse_StickDirection(curStick, &out_turnInfo->stickMag, &out_turnInfo->stickAngle);

    Camera* cam = getActiveCamera(play);
    
    f32 relX = -(curStick->x);
    f32 relZ = (curStick->z);

    s16 angle = cam->camDir.y;
    // Determine what left and right mean based on camera angle
    f32 relX2 = relX * Math_CosS(angle) + relZ * Math_SinS(angle);
    f32 relZ2 = relZ * Math_CosS(angle) - relX * Math_SinS(angle);


    out_turnInfo->desiredDirection = Math_Atan2S(relX2, relZ2);
    out_turnInfo->desiredDirectionDelta = out_turnInfo->desiredDirection - this->actor.world.rot.y;

    // recomp_printf("desiredDirectionDelta: %i\n", out_turnInfo->desiredDirectionDelta);
    out_turnInfo->turnAngle = USE_ALTERNATE_CONTROLS ?  out_turnInfo->desiredDirectionDelta : out_turnInfo->stickAngle;
}

RECOMP_PATCH void EnHorse_UpdateSpeed(EnHorse* this, PlayState* play, f32 brakeDecel, f32 brakeAngle, f32 minStickMag, f32 decel,
                         f32 baseSpeed, s16 turnSpeed) {
    f32 phi_f0;
    TurnInfo t;
    s16 turn;
    f32 temp_f12;

    turnSpeed = turnSpeed * EPONA_TURN_MULT;
    brakeDecel = brakeDecel * EPONA_BRAKE_MULT;

    // recomp_printf("CONTROL_MODE: %i\n", USE_ALTERNATE_CONTROLS);

    if (!EnHorse_PlayerCanMove(this, play)) {
        if (this->actor.speed > 8.0f) {
            this->actor.speed -= decel;
        } else if (this->actor.speed < 0.0f) {
            this->actor.speed = 0.0f;
        }
        return;
    }

    baseSpeed *= EnHorse_SlopeSpeedMultiplier(this, play);
    EnHorse_GetTurnInfo(this, play, &this->curStick, &t);

    // if (Math_CosS(stickAngle) <= brakeAngle) {
    if ((!USE_ALTERNATE_CONTROLS && Math_CosS(t.stickAngle) <= brakeAngle) || (USE_ALTERNATE_CONTROLS && t.stickMag < 0.01f)) {
        // recomp_printf("BRAKING!\n");
        this->actor.speed -= brakeDecel;
        this->actor.speed = CLAMP_MIN(this->actor.speed, 0.0f);
        return;
    }

    if (t.stickMag < minStickMag) {
        this->stateFlags &= ~ENHORSE_BOOST;
        this->stateFlags &= ~ENHORSE_BOOST_DECEL;
        this->actor.speed -= decel;
        if (this->actor.speed < 0.0f) {
            this->actor.speed = 0.0f;
        }
        return;
    }

    if (this->stateFlags & ENHORSE_BOOST) {
        if ((16 - this->boostTimer) > 0) {
            this->actor.speed = (((EnHorse_SlopeSpeedMultiplier(this, play) * this->boostSpeed) - this->actor.speed) /
                                (16.0f - this->boostTimer)) +
                                this->actor.speed;
        } else {
            this->actor.speed = EnHorse_SlopeSpeedMultiplier(this, play) * this->boostSpeed;
        }

        if ((EnHorse_SlopeSpeedMultiplier(this, play) * this->boostSpeed) <= this->actor.speed) {
            this->stateFlags &= ~ENHORSE_BOOST;
            this->stateFlags |= ENHORSE_BOOST_DECEL;
        }
    } else if (this->stateFlags & ENHORSE_BOOST_DECEL) {
        if (this->actor.speed > baseSpeed) {
            this->actor.speed -= 0.06f;
        } else if (this->actor.speed < baseSpeed) {
            this->actor.speed = baseSpeed;
            this->stateFlags &= ~ENHORSE_BOOST_DECEL;
        }
    } else {
        if (this->actor.speed <= (baseSpeed * (1.0f / 54.0f) * t.stickMag)) {
            phi_f0 = 1.0f;
        } else {
            phi_f0 = -1.0f;
        }

        this->actor.speed += phi_f0 * 50.0f * 0.01f;

        if (this->actor.speed > baseSpeed) {
            this->actor.speed -= decel;
            if (this->actor.speed < baseSpeed) {
                this->actor.speed = baseSpeed;
            }
        }
    }

    if (ABS(this->actor.world.rot.y - t.desiredDirection) < TURN_SNAP_ANGLE) {
        this->actor.world.rot.y = t.desiredDirection;
    } else {
        temp_f12 = t.turnAngle * (1 / 32236.f);
        turn = t.turnAngle * temp_f12 * temp_f12 * (2.2f - (this->actor.speed * (1.0f / this->boostSpeed)));
        turn = CLAMP(turn, -turnSpeed * (2.2f - (1.7f * this->actor.speed * (1.0f / this->boostSpeed))),
                    turnSpeed * (2.2f - (1.7f * this->actor.speed * (1.0f / this->boostSpeed))));
        
        // Make sure we don't spend forever reaching the angle:
        if (ABS(turn) < MINIMUM_TURN_ANGLE) {
            turn = 300 * ((turn > 0 ) ? 1 : -1);
        }
        this->actor.world.rot.y += turn;
    }
    this->actor.shape.rot.y = this->actor.world.rot.y;
}


RECOMP_PATCH void EnHorse_MountedIdle(EnHorse* this, PlayState* play) {
    this->actor.speed = 0.0f;

    TurnInfo t;
    EnHorse_GetTurnInfo(this, play, &this->curStick, &t);

    if (t.stickMag > 10.0f) {
        if (EnHorse_PlayerCanMove(this, play) == true) {
            if (USE_ALTERNATE_CONTROLS) { 
                if (Math_CosS(t.turnAngle) <= 0.7071f) {
                    EnHorse_StartTurning(this);
                } 
                EnHorse_StartWalkingFromIdle(this);
                
            } else {
                if (Math_CosS(t.turnAngle) <= -0.5f) {
                    EnHorse_StartReversingInterruptable(this);
                } else if (Math_CosS(t.turnAngle) <= 0.7071f) {
                    EnHorse_StartTurning(this);
                } else {
                    EnHorse_StartWalkingFromIdle(this);
                }
            }
        } else if (this->unk_3EC != this->actor.world.rot.y) {
            EnHorse_StartTurning(this);
        }
    }

    if (SkelAnime_Update(&this->skin.skelAnime)) {
        EnHorse_MountedIdleAnim(this);
    }
}

RECOMP_PATCH void EnHorse_MountedIdleWhinnying(EnHorse* this, PlayState* play) {
    TurnInfo t;
    EnHorse_GetTurnInfo(this, play, &this->curStick, &t);

    this->actor.speed = 0.0f;
    // EnHorse_StickDirection(&this->curStick, &stickMag, &stickAngle);

    if (t.stickMag > 10.0f) {
        if (EnHorse_PlayerCanMove(this, play) == true) {
            if (USE_ALTERNATE_CONTROLS) { 
                if (Math_CosS(t.turnAngle) <= 0.7071f) {
                    EnHorse_StartTurning(this);
                } 
                EnHorse_StartWalkingFromIdle(this);
                
            } else {
                if (Math_CosS(t.turnAngle) <= -0.5f) {
                    EnHorse_StartReversingInterruptable(this);
                } else if (Math_CosS(t.turnAngle) <= 0.7071f) {
                    EnHorse_StartTurning(this);
                } else {
                    EnHorse_StartWalkingFromIdle(this);
                }
            }

        } else if (this->unk_3EC != this->actor.world.rot.y) {
            EnHorse_StartTurning(this);
        }
    }

    if (SkelAnime_Update(&this->skin.skelAnime)) {
        EnHorse_StartMountedIdleResetAnim(this);
    }
}

RECOMP_PATCH void EnHorse_MountedTurn(EnHorse* this, PlayState* play) {
    TurnInfo t;
    s16 clampedYaw;

    this->actor.speed = 0.0f;
    EnHorse_PlayWalkingSound(this);
    if (EnHorse_PlayerCanMove(this, play) == true) {
        // EnHorse_StickDirection(&this->curStick, &stickMag, &stickAngle);
        EnHorse_GetTurnInfo(this, play, &this->curStick, &t);
        
        if (t.stickMag > 10.0f) {
            if (!EnHorse_PlayerCanMove(this, play)) {
                EnHorse_StartMountedIdleResetAnim(this);
            } else if (Math_CosS(t.turnAngle) <= -0.5f) {
                EnHorse_StartReversingInterruptable(this);
            } else if (Math_CosS(t.turnAngle) <= 0.7071f) {
                clampedYaw = CLAMP(t.turnAngle, -1600.0f, 1600.0f);
                this->actor.world.rot.y += clampedYaw;
                this->actor.shape.rot.y = this->actor.world.rot.y;
            } else {
                EnHorse_StartWalkingInterruptable(this);
            }
        }
    }

    if (SkelAnime_Update(&this->skin.skelAnime)) {
        if (EnHorse_PlayerCanMove(this, play) == true) {
            if (Math_CosS(t.turnAngle) <= 0.7071f) {
                EnHorse_StartTurning(this);
            } else {
                EnHorse_StartMountedIdleResetAnim(this);
            }
        } else if (this->unk_3EC != this->actor.world.rot.y) {
            EnHorse_StartTurning(this);
        } else {
            EnHorse_StartMountedIdleResetAnim(this);
        }
    }
}

RECOMP_PATCH void EnHorse_MountedWalk(EnHorse* this, PlayState* play) {
    TurnInfo t;
    
    EnHorse_PlayWalkingSound(this);
    EnHorse_GetTurnInfo(this, play, &this->curStick, &t);

    if ((this->noInputTimerMax == 0) ||
        ((this->noInputTimer > 0) && (this->noInputTimer < (this->noInputTimerMax - 20)))) {
        EnHorse_UpdateSpeed(this, play, 0.3f, -0.5f, 10.0f, 0.06f, 3.0f, 0x320);
    } else {
        this->actor.speed = 3.0f;
    }

    if (this->actor.speed == 0.0f) {
        this->stateFlags &= ~ENHORSE_FLAG_9;
        EnHorse_StartMountedIdleResetAnim(this);
        this->noInputTimer = 0;
        this->noInputTimerMax = 0;
    } else if (this->actor.speed > 3.0f) {
        this->stateFlags &= ~ENHORSE_FLAG_9;
        EnHorse_StartTrotting(this);
        this->noInputTimer = 0;
        this->noInputTimerMax = 0;
    }

    if (this->noInputTimer > 0) {
        this->noInputTimer--;
        if (this->noInputTimer <= 0) {
            this->noInputTimerMax = 0;
        }
    }

    if (this->waitTimer <= 0) {
        this->stateFlags &= ~ENHORSE_FLAG_9;
        this->skin.skelAnime.playSpeed = this->actor.speed * 0.75f;
        if ((SkelAnime_Update(&this->skin.skelAnime) || (this->actor.speed == 0.0f)) && (this->noInputTimer <= 0)) {
            if (this->actor.speed > 3.0f) {
                EnHorse_StartTrotting(this);
                this->noInputTimer = 0;
                this->noInputTimerMax = 0;
            } else if ((t.stickMag < 10.0f) || (Math_CosS(t.turnAngle) <= -0.5f)) {
                EnHorse_StartMountedIdleResetAnim(this);
                this->noInputTimer = 0;
                this->noInputTimerMax = 0;
            } else {
                EnHorse_MountedWalkingReset(this);
            }
        }
    } else {
        this->waitTimer--;
        this->actor.speed = 0.0f;
    }
}

RECOMP_PATCH void EnHorse_MountedTrot(EnHorse* this, PlayState* play) {
    EnHorse_UpdateSpeed(this, play, 0.3f, -0.5f, 10.0f, 0.06f, 6.0f, 800);
    
    TurnInfo t;
    EnHorse_GetTurnInfo(this, play, &this->curStick, &t);
    // EnHorse_StickDirection2(this, play, &this->curStick, &stickMag, &stickAngle);
    if (this->actor.speed < 3.0f) {
        EnHorse_StartWalkingInterruptable(this);
    }

    this->skin.skelAnime.playSpeed = this->actor.speed * 0.375f;

    if (SkelAnime_Update(&this->skin.skelAnime)) {
        func_8087C178(this);
        Rumble_Request(0.0f, 60, 8, 255);
        if (this->actor.speed >= 6.0f) {
            EnHorse_StartGallopingInterruptable(this);
        } else if (this->actor.speed < 3.0f) {
            EnHorse_StartWalkingInterruptable(this);
        } else {
            EnHorse_MountedTrotReset(this);
        }
    }
}

RECOMP_PATCH void EnHorse_MountedGallop(EnHorse* this, PlayState* play) {
    TurnInfo t;
    EnHorse_GetTurnInfo(this, play, &this->curStick, &t);
    // EnHorse_StickDirection2(this, play, &this->curStick, &stickMag, &stickAngle);

    if (this->noInputTimer <= 0) {
        EnHorse_UpdateSpeed(this, play, 0.3f, -0.5f, 10.0f, 0.06f, 8.0f, 800);
    } else if (this->noInputTimer > 0) {
        this->noInputTimer--;
        this->actor.speed = 8.0f;
    }

    if (this->actor.speed < 6.0f) {
        EnHorse_StartTrotting(this);
    }

    this->skin.skelAnime.playSpeed = this->actor.speed * 0.3f;

    if (SkelAnime_Update(&this->skin.skelAnime)) {
        func_8087C1C0(this);
        Rumble_Request(0.0f, 120, 8, 255);
        if (EnHorse_PlayerCanMove(this, play) == true) {
            if (USE_ALTERNATE_CONTROLS) {
                if ((t.stickMag < 0.01f)) {
                    EnHorse_StartBraking(this, play);
                } else if (this->actor.speed < 6.0f) {
                    EnHorse_StartTrotting(this);
                } else {
                    EnHorse_MountedGallopReset(this);
                }
            } else {
                if ((t.stickMag >= 10.0f) && (Math_CosS(t.turnAngle) <= -0.5f)) {
                    EnHorse_StartBraking(this, play);
                } else if (this->actor.speed < 6.0f) {
                    EnHorse_StartTrotting(this);
                } else {
                    EnHorse_MountedGallopReset(this);
                }
            }

        } else {
            EnHorse_MountedGallopReset(this);
        }
    }
}

RECOMP_PATCH void EnHorse_MountedRearing(EnHorse* this, PlayState* play) {
    this->actor.speed = 0.0f;
    if (this->curFrame > 25.0f) {
        if (!(this->stateFlags & ENHORSE_LAND2_SOUND)) {
            this->stateFlags |= ENHORSE_LAND2_SOUND;
            if (this->type == HORSE_TYPE_2) {
                Audio_PlaySfx_AtPos(&this->actor.projectedPos, NA_SE_EV_KID_HORSE_LAND2);
            } else {
                Audio_PlaySfx_AtPos(&this->actor.projectedPos, NA_SE_EV_KID_HORSE_LAND2);
            }
            Rumble_Request(0.0f, 180, 20, 100);
        }
    }

    TurnInfo t;
    EnHorse_GetTurnInfo(this, play, &this->curStick, &t);
    // EnHorse_StickDirection2(this, play, &this->curStick, &stickMag, &stickAngle);

    if (SkelAnime_Update(&this->skin.skelAnime)) {
        if (EnHorse_PlayerCanMove(this, play) == true) {
            if (this->stateFlags & ENHORSE_FORCE_REVERSING) {
                this->noInputTimer = 100;
                this->noInputTimerMax = 100;
                this->stateFlags &= ~ENHORSE_FORCE_REVERSING;
                if (USE_ALTERNATE_CONTROLS) {
                    EnHorse_StartMountedIdleResetAnim(this);
                } else {
                    EnHorse_StartReversing(this);
                }

            } else if (this->stateFlags & ENHORSE_FORCE_WALKING) {
                this->noInputTimer = 100;
                this->noInputTimerMax = 100;
                this->stateFlags &= ~ENHORSE_FORCE_WALKING;
                EnHorse_StartWalking(this);
            } else if (!USE_ALTERNATE_CONTROLS && (t.turnAngle) <= -0.5f) {
                EnHorse_StartReversingInterruptable(this);
            } else {
                EnHorse_StartMountedIdleResetAnim(this);
            }
        } else {
            EnHorse_StartMountedIdleResetAnim(this);
        }
    }
}

RECOMP_PATCH void EnHorse_Reverse(EnHorse* this, PlayState* play) {
    s16 turnAmount;
    Player* player = GET_PLAYER(play);

    EnHorse_PlayWalkingSound(this);
    TurnInfo t;
    EnHorse_GetTurnInfo(this, play, &this->curStick, &t);
    // EnHorse_StickDirection2(this, play, &this->curStick, &stickMag, &stickAngle);
    if (EnHorse_PlayerCanMove(this, play) == true) {
        if ((this->noInputTimerMax == 0) ||
            ((this->noInputTimer > 0) && (this->noInputTimer < (this->noInputTimerMax - 20)))) {
            if ((t.stickMag < 10.0f) && (this->noInputTimer <= 0)) {
                EnHorse_StartMountedIdleResetAnim(this);
                this->actor.speed = 0.0f;
                return;
            } else if (t.stickMag < 10.0f) {
                t.turnAngle = -0x7FFF;
            } else if (Math_CosS(t.turnAngle) > -0.5f) {
                this->noInputTimerMax = 0;
                EnHorse_StartMountedIdleResetAnim(this);
                this->actor.speed = 0.0f;
                return;
            }
        } else if (t.stickMag < 10.0f) {
            t.turnAngle = -0x7FFF;
        }
    } else if ((player->actor.flags & ACTOR_FLAG_TALK) || (play->csCtx.state != CS_STATE_IDLE) ||
               (CutsceneManager_GetCurrentCsId() != CS_ID_NONE) || (player->stateFlags1 & PLAYER_STATE1_20)) {
        EnHorse_StartMountedIdleResetAnim(this);
        this->actor.speed = 0.0f;
        return;
    } else {
        t.turnAngle = -0x7FFF;
    }

    this->actor.speed = -2.0f;
    turnAmount = -0x8000 - t.turnAngle;
    turnAmount = CLAMP(turnAmount, -2400.0f, 2400.0f);
    this->actor.world.rot.y += turnAmount;
    this->actor.shape.rot.y = this->actor.world.rot.y;

    if (this->noInputTimer > 0) {
        this->noInputTimer--;
        if (this->noInputTimer <= 0) {
            this->noInputTimerMax = 0;
        }
    }

    this->skin.skelAnime.playSpeed = this->actor.speed * 0.5f * 1.5f;

    if (SkelAnime_Update(&this->skin.skelAnime) && (this->noInputTimer <= 0) &&
        (EnHorse_PlayerCanMove(this, play) == true)) {
        if (USE_ALTERNATE_CONTROLS) {
            if (t.stickMag < 10.0f) {
                this->noInputTimerMax = 0;
                EnHorse_StartMountedIdleResetAnim(this);
            } else {
                EnHorse_StartReversing(this);
            }
        } else {
            if ((t.stickMag > 10.0f) && (Math_CosS(t.turnAngle) <= -0.5f)) {
                this->noInputTimerMax = 0;
                EnHorse_StartReversingInterruptable(this);
            } else if (t.stickMag < 10.0f) {
                this->noInputTimerMax = 0;
                EnHorse_StartMountedIdleResetAnim(this);
            } else {
                EnHorse_StartReversing(this);
            }
        }
    }
}

/*
Not sure this section is actually needed.

void Epona_ActionFunc(EnHorse* this, PlayState* play, EnHorseAction action) {
    // recomp_printf("Epona Action: %i\n", action);

    switch (action) {
        case ENHORSE_ACTION_FROZEN:
            EnHorse_Frozen(this, play);
            break;
        case ENHORSE_ACTION_INACTIVE:
            EnHorse_Inactive(this, play);
            break;
        case ENHORSE_ACTION_IDLE:
            EnHorse_Idle(this, play);
            break;
        case ENHORSE_ACTION_FOLLOW_PLAYER:
            EnHorse_FollowPlayer(this, play);
            break;
        case ENHORSE_ACTION_INGO_RACE:
            EnHorse_UpdateIngoRace(this, play);
            break;
        case ENHORSE_ACTION_5:
            func_808819D8(this, play);
            break;
        case ENHORSE_ACTION_6:
            func_80881398(this, play);
            break;
        case ENHORSE_ACTION_MOUNTED_IDLE:
            EnHorse_MountedIdle(this, play);
            break;
        case ENHORSE_ACTION_MOUNTED_IDLE_WHINNYING:
            EnHorse_MountedIdleWhinnying(this, play);
            break;
        case ENHORSE_ACTION_MOUNTED_TURN:
            EnHorse_MountedTurn(this, play);
            break;
        case ENHORSE_ACTION_MOUNTED_WALK:
            EnHorse_MountedWalk(this, play);
            break;
        case ENHORSE_ACTION_MOUNTED_TROT:
            EnHorse_MountedTrot(this, play);
            break;
        case ENHORSE_ACTION_MOUNTED_GALLOP:
            EnHorse_MountedGallop(this, play);
            break;
        case ENHORSE_ACTION_MOUNTED_REARING:
            EnHorse_MountedRearing(this, play);
            break;
        case ENHORSE_ACTION_STOPPING:
            EnHorse_Stopping(this, play);
            break;
        case ENHORSE_ACTION_REVERSE:
            EnHorse_Reverse(this, play);
            break;
        case ENHORSE_ACTION_LOW_JUMP:
            EnHorse_LowJump(this, play);
            break;
        case ENHORSE_ACTION_HIGH_JUMP:
            EnHorse_HighJump(this, play);
            break;
        case ENHORSE_ACTION_CS_UPDATE:
            EnHorse_CutsceneUpdate(this, play);
            break;
        case ENHORSE_ACTION_HBA:
            EnHorse_UpdateHorsebackArchery(this, play);
            break;
        case ENHORSE_ACTION_FLEE_PLAYER:
            EnHorse_FleePlayer(this, play);
            break;
        case ENHORSE_ACTION_21:
            func_80884718(this, play);
            break;
        case ENHORSE_ACTION_22:
            func_8087CA04(this, play);
            break;
        case ENHORSE_ACTION_23:
            func_808848C8(this, play);
            break;
        case ENHORSE_ACTION_24:
            func_80884A40(this, play);
            break;
        case ENHORSE_ACTION_25:
            func_80884E0C(this, play);
            break;
    }
}

RECOMP_PATCH void EnHorse_Update(Actor* thisx, PlayState* play2) {
    PlayState* play = play2;
    EnHorse* this = (EnHorse*)thisx;
    Vec3f dustAcc = { 0.0f, 0.0f, 0.0f };
    Vec3f dustVel = { 0.0f, 1.0f, 0.0f };
    Player* player = GET_PLAYER(play);

    if (this->type == HORSE_TYPE_2) {
        Actor_SetScale(&this->actor, 0.00648f);
    } else if (this->type == HORSE_TYPE_DONKEY) {
        Actor_SetScale(&this->actor, 0.008f);
    } else {
        Actor_SetScale(&this->actor, 0.01f);
    }

    this->lastYaw = thisx->shape.rot.y;
    EnHorse_UpdateStick(this, play);
    EnHorse_UpdatePlayerDir(this, play);

    if (!(this->stateFlags & ENHORSE_INACTIVE)) {
        EnHorse_MountDismount(this, play);
    }

    if (this->stateFlags & ENHORSE_FLAG_19) {
        if ((this->stateFlags & ENHORSE_FLAG_20) && (this->inRace == true)) {
            this->stateFlags &= ~ENHORSE_FLAG_20;
            EnHorse_StartRearing(this);
        } else if (!(this->stateFlags & ENHORSE_FLAG_20) && (this->stateFlags & ENHORSE_FLAG_21) &&
                   (this->action != ENHORSE_ACTION_MOUNTED_REARING) && (this->inRace == true)) {
            this->stateFlags &= ~ENHORSE_FLAG_21;
            EnHorse_StartRearing(this);
        }
    }

    Epona_ActionFunc(this, play, this->action);
    // this->actor.world.rot.y += 1000;
    // recomp_printf("epona.rot: %i, %i, %i\n", this->actor.world.rot.x, this->actor.world.rot.y, this->actor.world.rot.z);

    this->stateFlags &= ~ENHORSE_OBSTACLE;
    this->unk_3EC = thisx->world.rot.y;
    if ((this->animIndex == ENHORSE_ANIM_STOPPING) || (this->animIndex == ENHORSE_ANIM_REARING)) {
        this->skin.skelAnime.jointTable[LIMB_ROOT_POS].y += 0x154;
    }

    this->curFrame = this->skin.skelAnime.curFrame;
    this->lastPos = thisx->world.pos;

    if (!(this->stateFlags & ENHORSE_INACTIVE)) {
        if ((this->action == ENHORSE_ACTION_MOUNTED_GALLOP) || (this->action == ENHORSE_ACTION_MOUNTED_TROT) ||
            (this->action == ENHORSE_ACTION_MOUNTED_WALK)) {
            func_80886C00(this, play);
        }

        if (this->playerControlled == true) {
            EnHorse_RegenBoost(this, play);
        }

        if (CutsceneManager_GetCurrentCsId() != CS_ID_NONE) {
            thisx->speed = 0.0f;
        }

        if (this->action != ENHORSE_ACTION_25) {
            Actor_MoveWithGravity(&this->actor);
        }

        if (this->rider != NULL) {
            if ((this->action == ENHORSE_ACTION_INGO_RACE) || (this->action == ENHORSE_ACTION_5) ||
                (this->action == ENHORSE_ACTION_25)) {
                this->rider->actor.world.pos.x = thisx->world.pos.x;
                this->rider->actor.world.pos.y = thisx->world.pos.y + 10.0f;
                this->rider->actor.world.pos.z = thisx->world.pos.z;
                this->rider->actor.shape.rot.x = thisx->shape.rot.x;
                this->rider->actor.shape.rot.y = thisx->shape.rot.y;
            } else if (this->action == ENHORSE_ACTION_6) {
                EnIn* in = this->rider;
                s16 jnt = in->jointTable[LIMB_ROOT_POS].y;

                in->actor.world.pos.x = this->riderPos.x;
                in->actor.world.pos.y = this->riderPos.y - (jnt * 0.01f * this->unk_528 * 0.01f);
                in->actor.world.pos.z = this->riderPos.z;
                in->actor.shape.rot.x = thisx->shape.rot.x;
                in->actor.shape.rot.y = thisx->shape.rot.y;
            }
        }

        if (this->colliderJntSph.elements[0].base.ocElemFlags & OCELEM_HIT) {
            if (thisx->speed > 10.0f) {
                thisx->speed -= 1.0f;
            }
        }

        if ((this->colliderJntSph.base.acFlags & AC_HIT) && (this->stateFlags & ENHORSE_DRAW)) {
            if (this->type == HORSE_TYPE_2) {
                Audio_PlaySfx_AtPos(&this->unk_218, NA_SE_EV_KID_HORSE_NEIGH);
            } else {
                Audio_PlaySfx_AtPos(&this->unk_218, NA_SE_EV_HORSE_NEIGH);
            }
        }

        if ((this->action != ENHORSE_ACTION_INGO_RACE) && (this->action != ENHORSE_ACTION_5) &&
            (this->action != ENHORSE_ACTION_6)) {
            EnHorse_TiltBody(this, play);
        }

        if ((this->playerControlled == false) && (this->unk_1EC & 8)) {
            if ((this->colliderJntSph.elements[0].base.ocElemFlags & OCELEM_HIT) &&
                (this->colliderJntSph.base.oc->id == ACTOR_EN_IN)) {
                func_80884868(this);
            }

            if ((this->colliderCylinder1.base.ocFlags1 & OC1_HIT) &&
                (this->colliderCylinder1.base.oc->id == ACTOR_EN_IN)) {
                func_80884868(this);
            }

            if ((this->colliderCylinder2.base.ocFlags1 & OC1_HIT) &&
                (this->colliderCylinder2.base.oc->id == ACTOR_EN_IN)) {
                func_80884868(this);
            }
        }

        Collider_UpdateCylinder(&this->actor, &this->colliderCylinder1);
        Collider_UpdateCylinder(&this->actor, &this->colliderCylinder2);

        if (this->type == HORSE_TYPE_2) {
            this->colliderCylinder1.dim.pos.x =
                TRUNCF_BINANG(Math_SinS(thisx->shape.rot.y) * 11.0f) + this->colliderCylinder1.dim.pos.x;
            this->colliderCylinder1.dim.pos.z =
                TRUNCF_BINANG(Math_CosS(thisx->shape.rot.y) * 11.0f) + this->colliderCylinder1.dim.pos.z;
            this->colliderCylinder2.dim.pos.x =
                TRUNCF_BINANG(Math_SinS(thisx->shape.rot.y) * -18.0f) + this->colliderCylinder2.dim.pos.x;
            this->colliderCylinder2.dim.pos.z =
                TRUNCF_BINANG(Math_CosS(thisx->shape.rot.y) * -18.0f) + this->colliderCylinder2.dim.pos.z;
        } else {
            this->colliderCylinder1.dim.pos.x =
                TRUNCF_BINANG(Math_SinS(thisx->shape.rot.y) * 6.6000004f) + this->colliderCylinder1.dim.pos.x;
            this->colliderCylinder1.dim.pos.z =
                TRUNCF_BINANG(Math_CosS(thisx->shape.rot.y) * 6.6000004f) + this->colliderCylinder1.dim.pos.z;
            this->colliderCylinder2.dim.pos.x =
                TRUNCF_BINANG(Math_SinS(thisx->shape.rot.y) * -10.8f) + this->colliderCylinder2.dim.pos.x;
            this->colliderCylinder2.dim.pos.z =
                TRUNCF_BINANG(Math_CosS(thisx->shape.rot.y) * -10.8f) + this->colliderCylinder2.dim.pos.z;
        }

        CollisionCheck_SetAT(play, &play->colChkCtx, &this->colliderCylinder1.base);
        if (!(this->stateFlags & ENHORSE_JUMPING) && !(this->unk_1EC & 0x20)) {
            CollisionCheck_SetOC(play, &play->colChkCtx, &this->colliderCylinder1.base);
            CollisionCheck_SetOC(play, &play->colChkCtx, &this->colliderCylinder2.base);
        } else {
            this->unk_1EC &= ~0x20;
        }

        if (this->unk_1EC & 0x100) {
            CollisionCheck_SetAC(play, &play->colChkCtx, &this->colliderCylinder1.base);
            CollisionCheck_SetAC(play, &play->colChkCtx, &this->colliderCylinder2.base);
        }

        if ((player->stateFlags1 & PLAYER_STATE1_1) && (player->rideActor != NULL)) {
            EnHorse_UpdateConveyors(this, play);
        }

        EnHorse_UpdateBgCheckInfo(this, play);
        EnHorse_CheckFloors(this, play);
        if (thisx->world.pos.y < this->yFront) {
            if (thisx->world.pos.y < this->yBack) {
                if (this->yBack < this->yFront) {
                    thisx->world.pos.y = this->yBack;
                } else {
                    thisx->world.pos.y = this->yFront;
                }
            }
        }

        thisx->focus.pos = thisx->world.pos;
        thisx->focus.pos.y += 70.0f;

        if ((Rand_ZeroOne() < 0.025f) && (this->blinkTimer == 0)) {
            this->blinkTimer++;
        } else if (this->blinkTimer > 0) {
            this->blinkTimer++;
            if (this->blinkTimer > 3) {
                this->blinkTimer = 0;
            }
        }

        if ((thisx->speed == 0.0f) && !(this->stateFlags & ENHORSE_FLAG_19)) {
            thisx->colChkInfo.mass = MASS_IMMOVABLE;
        } else {
            thisx->colChkInfo.mass = MASS_HEAVY;
        }

        if (thisx->speed >= 5.0f) {
            this->colliderCylinder1.base.atFlags |= AT_ON;
        } else {
            this->colliderCylinder1.base.atFlags &= ~AT_ON;
        }

        if (this->dustFlags & 1) {
            this->dustFlags &= ~1;
            func_800B12F0(play, &this->frontRightHoof, &dustVel, &dustAcc, EnHorse_RandInt(100.0f) + 200,
                          EnHorse_RandInt(10.0f) + 30, EnHorse_RandInt(20.0f) + 30);
        } else if (this->dustFlags & 2) {
            this->dustFlags &= ~2;
            func_800B12F0(play, &this->frontLeftHoof, &dustVel, &dustAcc, EnHorse_RandInt(100.0f) + 200,
                          EnHorse_RandInt(10.0f) + 30, EnHorse_RandInt(20.0f) + 30);
        } else if (this->dustFlags & 4) {
            this->dustFlags &= ~4;
            func_800B12F0(play, &this->backRightHoof, &dustVel, &dustAcc, EnHorse_RandInt(100.0f) + 200,
                          EnHorse_RandInt(10.0f) + 30, EnHorse_RandInt(20.0f) + 30);
        } else if (this->dustFlags & 8) {
            this->dustFlags &= ~8;
            func_800B12F0(play, &this->backLeftHoof, &dustVel, &dustAcc, EnHorse_RandInt(100.0f) + 200,
                          EnHorse_RandInt(10.0f) + 30, EnHorse_RandInt(20.0f) + 30);
        }
        this->stateFlags &= ~ENHORSE_DRAW;
    }
}
*/

// OLD AND CURSED. Keeping as a reference.
// void EnHorse_StickDirection2(EnHorse* this, PlayState* play, Vec2f* curStick, f32* out_stickMag, s16* out_angle) {
//     Camera* cam = getActiveCamera(play);
//     // recomp_printf("cam.foc: %f\n", cam->fov);
//     // recomp_printf("cam.camDir.y = %i, this->actor.world.rot = %i\n", cam->camDir.y, this->actor.world.rot.y);
//     // recomp_printf("cam.at: %i, %i, %i\n", cam->at.x, cam->at.y, cam->at.z);

//     // f32 relX = -(play->state.input[0].rel.stick_x / 10);
//     // f32 relZ = (play->state.input[0].rel.stick_y / 10);
    
//     f32 relX = -(curStick->x);
//     f32 relZ = (curStick->z);

//     // f32 relMag = sqrtf((relX * relX) + (relZ * relZ));
//     // if (relMag > 1.0f) {
//     //     relX /= relMag;
//     //     relZ /= relMag;
//     // }


//     s16 angle = cam->camDir.y - this->actor.world.rot.y;
//     // Determine what left and right mean based on camera angle
//     f32 relX2 = relX * Math_CosS(angle) + relZ * Math_SinS(angle);
//     f32 relZ2 = relZ * Math_CosS(angle) - relX * Math_SinS(angle);

//     // f32 relX2 = relX * Math_CosS(this->actor.world.rot.y) + relZ * Math_SinS(this->actor.world.rot.y);
//     // f32 relZ2 = relZ * Math_CosS(this->actor.world.rot.y) - relX * Math_SinS(this->actor.world.rot.y);

//     // recomp_printf("relX2: %f, relZ2: %f\n", relX2, relZ2);

//     *out_stickMag = sqrtf(SQ(relX2) + SQ(relZ2));
//     *out_stickMag = CLAMP_MAX(*out_stickMag, 60.0f);
//     *out_angle = Math_Atan2S(relX2, relZ2);
// }