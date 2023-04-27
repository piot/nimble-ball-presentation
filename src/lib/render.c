/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include "basal/vector2i.h"
#include <SDL2_image/SDL_image.h>
#include <nimble-ball-presentation/render.h>
#include <nimble-ball-simulation/nimble_ball_simulation.h>

static void setupAvatarSprite(SrSprite* sprite, SDL_Texture* texture, int cellIndex)
{
    sprite->rect.x = 0 + cellIndex * 21;
    sprite->rect.y = 0;
    sprite->rect.w = 21;
    sprite->rect.h = 31;
    sprite->texture = texture;
}

static void setupBallSprite(SrSprite* sprite, SDL_Texture* texture)
{
    sprite->rect.x = 89 ;
    sprite->rect.y = 36;
    sprite->rect.w = 18;
    sprite->rect.h = 18;
    sprite->texture = texture;
}

void nlRenderInit(NlRender* self)
{
    srWindowInit(&self->window, 640, 360, "nimble ball");

    SDL_Texture* avatarsTexture = IMG_LoadTexture(self->window.renderer, "data/avatars.png");
    SDL_Texture* equipmentTexture = IMG_LoadTexture(self->window.renderer, "data/equipment.png");

    srFontInit(&self->font, self->window.renderer, "data/mouldy.ttf", 10);

    srSpritesInit(&self->spriteRender, self->window.renderer);
    srRectsInit(&self->rectangleRender, self->window.renderer);
    setupAvatarSprite(&self->avatarSpriteForTeam[0], avatarsTexture, 0);
    setupAvatarSprite(&self->avatarSpriteForTeam[1], avatarsTexture, 1);
    setupBallSprite(&self->ballSprite, equipmentTexture);
}

static bl_vector2i simulationToRender(BlVector2 pos)
{
    bl_vector2i result;
    result.x = pos.x;
    result.y = pos.y;

    return result;
}

static void renderGoals(SrRects* rectangleRender, const NlConstants* constants)
{
    SDL_SetRenderDrawColor(rectangleRender->renderer, 255, 0, 127, SDL_ALPHA_OPAQUE);
    for (size_t i=0; i<2; ++i) {
        const NlGoal* goal = &constants->goals[i];
        srRectsLineRect(rectangleRender, goal->rect.position.x, goal->rect.position.y, goal->rect.size.x, goal->rect.size.y);
    }
}

static void renderBorders(SrRects* lineRender, const NlConstants* constants)
{
    SDL_SetRenderDrawColor(lineRender->renderer, 255, 240, 127, SDL_ALPHA_OPAQUE);
    for (size_t i=0; i<sizeof(constants->borderSegments)/sizeof(constants->borderSegments[0]); ++i) {
        const BlLineSegment * lineSegment = &constants->borderSegments[i];
        srRectsDrawLine(lineRender, lineSegment->a.x, lineSegment->a.y, lineSegment->b.x, lineSegment->b.y);
    }
}

static void renderCallback(void* _self, SrWindow* _)
{
    NlRender* self = (NlRender*) _self;
    const NlGame* predicted = self->predicted;

    for (size_t i = 0; i < predicted->avatars.avatarCount; ++i) {
        const NlAvatar* avatar = &predicted->avatars.avatars[i];
        bl_vector2i avatarRenderPos = simulationToRender(avatar->circle.center);
        int degreesAngle = (int)(avatar->visualRotation*360.0f/(M_PI*2.0f));
        srSpritesCopyEx(&self->spriteRender, &self->avatarSpriteForTeam[avatar->teamIndex], avatarRenderPos.x, avatarRenderPos.y, degreesAngle, 1.0f);
    }

    bl_vector2i ballRenderPos = simulationToRender(predicted->ball.circle.center);
    srSpritesCopyEx(&self->spriteRender, &self->ballSprite, ballRenderPos.x, ballRenderPos.y, 0,
                    1.0f);

    renderGoals(&self->rectangleRender, &g_nlConstants);
    renderBorders(&self->rectangleRender, &g_nlConstants);
    //SDL_Color color = {0xff, 0xff, 0xff, SDL_ALPHA_OPAQUE};

    SDL_SetRenderDrawColor(self->window.renderer, 0x44, 0x22, 0x44, 0x22);
    const int borderSize = 22;
    srRectsFillRect(&self->rectangleRender, 0, 359-borderSize, 640, borderSize);

    char buf[64];
    tc_snprintf(buf, 64, "pid %04X", self->stats.predictedTickId);
    SDL_Color color = {0xff, 0xff, 0xff, SDL_ALPHA_OPAQUE};
    srFontRenderAndCopy(&self->font, buf, 10, 359-6, color);
}

void nlRenderUpdate(NlRender* self, const NlGame* authoritative, const NlGame* predicted, NlRenderStats stats)
{
    self->authoritative = authoritative;
    self->predicted = predicted;
    self->stats = stats;

    srWindowRender(&self->window, 0x115511, self, renderCallback);
}

void nlRenderClose(NlRender* self)
{
    srWindowClose(&self->window);
}
