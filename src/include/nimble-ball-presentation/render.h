/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef NIMBLE_BALL_RENDER_SDL_RENDER_H
#define NIMBLE_BALL_RENDER_SDL_RENDER_H

#include <sdl-render/font.h>
#include <sdl-render/rect.h>
#include <sdl-render/sprite.h>
#include <sdl-render/window.h>

struct NlGame;

typedef struct NlRenderStats {
    uint32_t predictedTickId;
    uint32_t authoritativeTickId;
    int authoritativeStepsInBuffer;
} NlRenderStats;

typedef struct NlRender {
    SrSprite avatarSpriteForTeam[2];
    SrSprite ballSprite;
    SrSprite arrowSprite;
    SrSprites spriteRender;
    SrRects rectangleRender;
    SDL_Renderer* renderer;
    SrFont font;
    SrFont bigFont;
    NlRenderStats stats;
} NlRender;

void nlRenderInit(NlRender* self, SDL_Renderer* renderer);
void nlRenderUpdate(NlRender* self, const struct NlGame* authoritative, const struct NlGame* predicted,
                    const uint8_t localParticipants[], size_t localParticipantCount, const NlRenderStats stats);
void nlRenderClose(NlRender* self);

#endif
