/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef NIMBLE_BALL_RENDER_SDL_RENDER_H
#define NIMBLE_BALL_RENDER_SDL_RENDER_H

#include <sdl-render/sprite.h>
#include <sdl-render/rect.h>
#include <sdl-render/window.h>
#include <sdl-render/font.h>

struct NlGame;


typedef struct NlRenderStats
{
    uint32_t predictedTickId;
    int authoritativeStepsInBuffer;
} NlRenderStats;

typedef struct NlRender {
    SrSprite avatarSpriteForTeam[2];
    SrSprite ballSprite;
    SrSprites spriteRender;
    SrRects rectangleRender;
    SrWindow window;
    const struct NlGame* authoritative;
    const struct NlGame* predicted;
    SrFont font;
    NlRenderStats stats;
} NlRender;


void nlRenderInit(NlRender* self);
void nlRenderUpdate(NlRender* self, const struct NlGame* authoritative, const struct NlGame* predicted, const NlRenderStats stats);
void nlRenderClose(NlRender* self);

#endif
