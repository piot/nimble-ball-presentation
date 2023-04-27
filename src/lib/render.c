/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include "basal/vector2i.h"
#include <SDL2_image/SDL_image.h>
#include <nimble-ball-presentation/render.h>
#include <nimble-ball-simulation/nimble_ball_simulation.h>

static void setupAvatarSprite(SrSprite* sprite, SDL_Texture* texture)
{
    sprite->rect.x = 0;
    sprite->rect.y = 0;
    sprite->rect.w = 22;
    sprite->rect.h = 32;
    sprite->texture = texture;
}

static void setupBallSprite(SrSprite* sprite, SDL_Texture* texture)
{
    sprite->rect.x = 89;
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

    srSpritesInit(&self->spriteRender, self->window.renderer);
    setupAvatarSprite(&self->avatarSprite, avatarsTexture);
    setupBallSprite(&self->ballSprite, equipmentTexture);
}

static bl_vector2i simulationToRender(BlVector2 pos)
{
    bl_vector2i result;
    result.x = pos.x / 1;
    result.y = pos.y / 1;

    return result;
}

static void renderCallback(void* _self, SrWindow* _)
{
    NlRender* self = (NlRender*) _self;
    const NlGame* predicted = self->predicted;

    for (size_t i = 0; i < predicted->avatars.avatarCount; ++i) {
        const NlAvatar* avatar = &predicted->avatars.avatars[i];
        bl_vector2i avatarRenderPos = simulationToRender(avatar->circle.center);
        int degreesAngle = (int)(avatar->visualRotation*360.0f/(M_PI*2.0f));
        CLOG_VERBOSE("degrees :%d (%f)" , degreesAngle, avatar->visualRotation);
        srSpritesCopyEx(&self->spriteRender, &self->avatarSprite, avatarRenderPos.x, avatarRenderPos.y, degreesAngle, 1.0f);
    }

    bl_vector2i ballRenderPos = simulationToRender(predicted->ball.circle.center);
    srSpritesCopyEx(&self->spriteRender, &self->ballSprite, ballRenderPos.x, ballRenderPos.y, 0,
                    1.0f);
}

void nlRenderUpdate(NlRender* self, const NlGame* authoritative, const NlGame* predicted)
{
    self->authoritative = authoritative;
    self->predicted = predicted;

    srWindowRender(&self->window, 0xff0000, self, renderCallback);
}

void nlRenderClose(NlRender* self)
{
    srWindowClose(&self->window);
}
