/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef NIMBLE_BALL_RENDER_SDL_RENDER_H
#define NIMBLE_BALL_RENDER_SDL_RENDER_H

#include <basal/vector2i.h>
#include <nimble-ball-simulation/nimble_ball_simulation.h>
#include <sdl-render/font.h>
#include <sdl-render/rect.h>
#include <sdl-render/sprite.h>
#include <sdl-render/window.h>

struct NlGame;

typedef struct NlRenderStats {
    uint32_t predictedTickId;
    uint32_t authoritativeTickId;
    int authoritativeStepsInBuffer;
    int renderFps;
    int latencyMs;
} NlRenderStats;

typedef enum NlRenderMode {
    NlRenderModePredicted,
    NlRenderModeAuthoritative,
} NlRenderMode;

typedef struct NlrEntityInfo {
    bool isUsed;
} NlrEntityInfo;

typedef struct NlrBall {
    NlrEntityInfo info;
    size_t spawnCountDown;
    uint8_t simulationCollideCounter;
    size_t lastCollisionCountDown;
    BlVector2i lastImpactPosition;
    BlVector2 precisionPosition;
} NlrBall;

typedef struct NlrPlayer {
    NlrEntityInfo info;
    int countDown;
} NlrPlayer;

typedef struct NlrAvatar {
    NlrEntityInfo info;
    size_t spawnCountDown;
    BlVector2i lastPosition;
    BlVector2 precisionPosition;
    float rotation;
} NlrAvatar;

typedef struct NlRender {
    SrSprite avatarSpriteForTeam[2];
    NlrBall ball;
    NlrBall shadowBall;

    SrSprite arrowSprite;
    SrSprite ballSprite;

    NlrPlayer players[NL_MAX_PLAYERS];
    NlrAvatar avatars[NL_MAX_PLAYERS];
    NlrAvatar shadowAvatars[NL_MAX_PLAYERS];

    SrSprites spriteRender;
    SrRects rectangleRender;
    SDL_Renderer* renderer;
    SrFont font;
    SrFont bigFont;
    NlRenderStats stats;
    NlRenderMode mode;
} NlRender;

void nlRenderInit(NlRender* self, SDL_Renderer* renderer);
void nlRenderUpdate(NlRender* self, const struct NlGame* authoritative, const struct NlGame* predicted,
                    const uint8_t localParticipants[], size_t localParticipantCount, const NlRenderStats stats);
void nlRenderClose(NlRender* self);

#endif
