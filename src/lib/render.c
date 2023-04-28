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

    srFontInit(&self->font, self->window.renderer, "data/mouldy.ttf", 10);
    srFontInit(&self->bigFont, self->window.renderer, "data/mouldy.ttf", 22);

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

static SDL_Color getTeamColor(int teamIndex)
{
    SDL_Color teamColor;

    teamColor.r = 0xff;
    teamColor.g = 0;
    teamColor.b = 0;
    teamColor.a = SDL_ALPHA_OPAQUE;

    if (teamIndex == 0) {
        teamColor.r = 0x66;
        teamColor.g = 0x11;
        teamColor.b = 0x11;
    } else {
        teamColor.r = 0x11;
        teamColor.g = 0x11;
        teamColor.b = 0x55;
    }

    return teamColor;
}

static void renderGoals(SrRects* rectangleRender, const NlConstants* constants)
{
    for (size_t i = 0; i < 2; ++i) {
        const NlGoal* goal = &constants->goals[i];

        SDL_Color teamColor = getTeamColor(goal->ownedByTeam);
        SDL_SetRenderDrawColor(rectangleRender->renderer, teamColor.r, teamColor.g, teamColor.b, teamColor.a);

        srRectsLineRect(rectangleRender, goal->rect.position.x, goal->rect.position.y, goal->rect.size.x,
                        goal->rect.size.y);
    }
}

static void renderBorders(SrRects* lineRender, const NlConstants* constants)
{
    SDL_SetRenderDrawColor(lineRender->renderer, 255, 240, 127, SDL_ALPHA_OPAQUE);
    for (size_t i = 0; i < sizeof(constants->borderSegments) / sizeof(constants->borderSegments[0]); ++i) {
        const BlLineSegment* lineSegment = &constants->borderSegments[i];
        srRectsDrawLine(lineRender, lineSegment->a.x, lineSegment->a.y, lineSegment->b.x, lineSegment->b.y);
    }
}

static void renderTeamNameAndScore(const SrFont* font, const NlTeam* team, int teamIndex, const char* teamName,
                                   SDL_Color teamColor)
{
    int teamX = teamIndex == 0 ? 50 : 540;
    int startY = 330;

    srFontRenderAndCopy(font, teamName, teamX, startY, teamColor);

    char scoreText[16];

    tc_snprintf(scoreText, 16, "%d", team->score);
    srFontRenderAndCopy(font, scoreText, teamX + 10, startY - 12, teamColor);
}

static void renderTeamHud(const SrFont* font, const NlTeam* team, int teamIndex, const char* teamName)
{
    SDL_Color teamColor = getTeamColor(teamIndex);

    renderTeamNameAndScore(font, team, teamIndex, teamName, teamColor);
}

static void renderCountDown(const SrFont* font, uint8_t countDown)
{
    int approximateSecondsLeft = (countDown / 62) + 1;
    char secondsText[16];
    tc_snprintf(secondsText, 16, "%d", approximateSecondsLeft);
    SDL_Color secondsColor = {0xff, 0xff, 0xff, SDL_ALPHA_OPAQUE};
    srFontRenderAndCopy(font, secondsText, 300, 230, secondsColor);
}

static void renderGoalCelebration(const SrFont* font, int teamIndexThatScored)
{
    SDL_Color goalCelebrationColor = {0xff, 0xff, 0xff, SDL_ALPHA_OPAQUE};
    srFontRenderAndCopy(font, "GOAL!", 300, 210, goalCelebrationColor);

    const char* goalAnnouncement[] = {"Red Scored!", "Blue Scored!"};
    SDL_Color teamColor = getTeamColor(teamIndexThatScored);
    srFontRenderAndCopy(font, goalAnnouncement[teamIndexThatScored], 180, 170, teamColor);
}

static void renderPostGame(const SrFont* font, const NlTeams* teams)
{
    int winningTeam = teams->teams[0].score > teams->teams[1].score   ? 0
                      : teams->teams[1].score > teams->teams[0].score ? 1
                                                                      : -1;
    const char* winAnnouncements[] = {"Draw", "Red Wins!", "Blue Wins!"};

    SDL_Color winAnnouncementColor;

    if (winningTeam == -1) {
        winAnnouncementColor.g = 0xff;
        winAnnouncementColor.b = 0xff;
        winAnnouncementColor.r = 0xff;
        winAnnouncementColor.a = SDL_ALPHA_OPAQUE;
    } else {
        winAnnouncementColor = getTeamColor(winningTeam);
    }
    const char* winAnnouncement = winAnnouncements[winningTeam + 1];

    srFontRenderAndCopy(font, winAnnouncement, 220, 230, winAnnouncementColor);
}

static void renderGameClock(const SrFont* font, uint16_t gameClockLeftInTicks)
{
    int milliSecondsLeftInGame = gameClockLeftInTicks * 16;

    int millisecondsLeft = milliSecondsLeftInGame % 1000;
    int secondsLeft = (milliSecondsLeftInGame + 999) / 1000; // feels more natural for a countdown clock
    int minutesLeft = milliSecondsLeftInGame / (60 * 1000);

    char gameClockText[64];
    tc_snprintf(gameClockText, 64, "%02d:%02d:%03d", minutesLeft, secondsLeft, millisecondsLeft);

    SDL_Color gameClockColor = {0xff, 0xff, 0xff, SDL_ALPHA_OPAQUE};
    srFontRenderAndCopy(font, gameClockText, 260, 330, gameClockColor);
}

static void renderHud(const SrFont* font, const SrFont* bigFont, const NlGame* authoritative, const NlGame* predicted)
{
    if (authoritative->teams.teamCount == 2) {
        renderTeamHud(font, &authoritative->teams.teams[0], 0, "Red");
        renderTeamHud(font, &authoritative->teams.teams[1], 1, "Blue");
    }

    switch (predicted->phase) {
        case NlGamePhaseCountDown:
            renderCountDown(bigFont, predicted->phaseCountDown);
            break;
        case NlGamePhaseWaitingForPlayers:
            break;
        case NlGamePhasePlaying:
            break;
    }

    switch (authoritative->phase) {
        case NlGamePhasePostGame:
            renderPostGame(bigFont, &authoritative->teams);
            break;
        case NlGamePhaseAfterAGoal:
            renderGoalCelebration(bigFont, authoritative->latestScoredTeamIndex);
            break;
    }
    renderGameClock(font, predicted->matchClockLeftInTicks);
}

static void renderStats(NlRender* self)
{
    SDL_SetRenderDrawColor(self->window.renderer, 0x44, 0x22, 0x44, 0x22);
    const int borderSize = 22;
    srRectsFillRect(&self->rectangleRender, 0, 359 - borderSize, 640, borderSize);
    char buf[64];
    tc_snprintf(buf, 64, "pid %04X", self->stats.predictedTickId);
    SDL_Color color = {0xff, 0xff, 0xff, SDL_ALPHA_OPAQUE};
    srFontRenderAndCopy(&self->font, buf, 10, 359 - 6, color);
}

static void renderAvatars(NlRender* self, const NlGame* predicted)
{
    for (size_t i = 0; i < predicted->avatars.avatarCount; ++i) {
        const NlAvatar* avatar = &predicted->avatars.avatars[i];
        bl_vector2i avatarRenderPos = simulationToRender(avatar->circle.center);
        int degreesAngle = (int) (avatar->visualRotation * 360.0f / (M_PI * 2.0f));
        srSpritesCopyEx(&self->spriteRender, &self->avatarSpriteForTeam[avatar->teamIndex], avatarRenderPos.x,
                        avatarRenderPos.y, degreesAngle, 1.0f);
    }
}

static void renderBalls(NlRender* self, const NlGame* predicted)
{
    bl_vector2i ballRenderPos = simulationToRender(predicted->ball.circle.center);
    srSpritesCopyEx(&self->spriteRender, &self->ballSprite, ballRenderPos.x, ballRenderPos.y, 0, 1.0f);
}

static void renderCallback(void* _self, SrWindow* _)
{
    NlRender* self = (NlRender*) _self;
    const NlGame* predicted = self->predicted;

    renderAvatars(self, predicted);
    renderBalls(self, predicted);
    renderGoals(&self->rectangleRender, &g_nlConstants);
    renderBorders(&self->rectangleRender, &g_nlConstants);
    renderHud(&self->font, &self->bigFont, self->authoritative, predicted);
    renderStats(self);
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
