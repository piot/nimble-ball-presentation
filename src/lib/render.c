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

static void setupArrowSprite(SrSprite* sprite, SDL_Texture* texture)
{
    sprite->rect.x = 0;
    sprite->rect.y = 0;
    sprite->rect.w = 19;
    sprite->rect.h = 15;
    sprite->texture = texture;
}

void nlRenderInit(NlRender* self, SDL_Renderer* renderer)
{
    self->renderer = renderer;
    SDL_Texture* avatarsTexture = IMG_LoadTexture(self->renderer, "data/avatars.png");
    SDL_Texture* equipmentTexture = IMG_LoadTexture(self->renderer, "data/equipment.png");

    srFontInit(&self->font, self->renderer, "data/mouldy.ttf", 10);
    srFontInit(&self->bigFont, self->renderer, "data/mouldy.ttf", 22);

    for (size_t i = 0u; i < NL_MAX_PLAYERS; ++i) {
        self->players[i].info.isUsed = false;
    }

    for (size_t i = 0u; i < NL_MAX_PLAYERS; ++i) {
        self->avatars[i].info.isUsed = false;
        self->shadowAvatars[i].info.isUsed = false;
    }

    self->ball.info.isUsed = false;
    self->shadowBall.info.isUsed = false;

    srSpritesInit(&self->spriteRender, self->renderer);
    srRectsInit(&self->rectangleRender, self->renderer);
    setupAvatarSprite(&self->avatarSpriteForTeam[0], avatarsTexture, 0);
    setupAvatarSprite(&self->avatarSpriteForTeam[1], avatarsTexture, 1);
    setupBallSprite(&self->ballSprite, equipmentTexture);
    setupArrowSprite(&self->arrowSprite, equipmentTexture);
    self->mode = NlRenderModePredicted;
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
    SDL_SetRenderDrawColor(self->renderer, 0x44, 0x22, 0x44, 0x22);
    const int borderSize = 22;
    srRectsFillRect(&self->rectangleRender, 0, 359 - borderSize, 640, borderSize);
    char buf[512];
    tc_snprintf(buf, 512, "preId %04X autId %04X conBufCnt %d fps:%d", self->stats.predictedTickId,
                self->stats.authoritativeTickId, self->stats.authoritativeStepsInBuffer, self->stats.renderFps);
    SDL_Color color = {0xff, 0xff, 0xff, SDL_ALPHA_OPAQUE};
    srFontRenderAndCopy(&self->font, buf, 10, 359 - 6, color);
}

#include <basal/math.h>

static void renderAvatar(NlRender* self, NlrAvatar* renderAvatar, const NlAvatar* avatar, Uint8 alpha)
{
    const size_t avatarSpawnTime = 60u;
    if (!renderAvatar->info.isUsed) {
        renderAvatar->info.isUsed = true;
        renderAvatar->spawnCountDown = avatarSpawnTime;
        renderAvatar->precisionPosition = avatar->circle.center;
        renderAvatar->rotation = avatar->visualRotation;
    }

    BlVector2 targetPosition = avatar->circle.center;

    BlVector2 delta = blVector2Sub(targetPosition, renderAvatar->precisionPosition);
    renderAvatar->precisionPosition = blVector2AddScale(renderAvatar->precisionPosition, delta, 0.2f);

    float angleDiff = avatar->visualRotation - renderAvatar->rotation;

    const float maxRadianDiffPerTime = 0.1f;
    float diffThisTick = blFabs(angleDiff) > maxRadianDiffPerTime ? blFSign(angleDiff) * maxRadianDiffPerTime
                                                                  : angleDiff;
    renderAvatar->rotation += diffThisTick;

    if (renderAvatar->spawnCountDown > 0u) {
        renderAvatar->spawnCountDown--;
    }

    float scale = renderAvatar->spawnCountDown > 0u ? 1.0f - renderAvatar->spawnCountDown / (float) avatarSpawnTime
                                                    : 1.0f;

    bl_vector2i avatarRenderPos = simulationToRender(renderAvatar->precisionPosition);
    int degreesAngle = (int) (renderAvatar->rotation * 360.0f / (M_PI * 2.0f));

    srSpritesCopyEx(&self->spriteRender, &self->avatarSpriteForTeam[avatar->teamIndex], avatarRenderPos.x,
                    avatarRenderPos.y, degreesAngle, scale, alpha);
}

static void renderAvatars(NlRender* self, NlrAvatar* nlrAvatars, const NlAvatars* avatars, Uint8 alpha)
{
    for (size_t i = 0u; i < avatars->avatarCount; ++i) {
        const NlAvatar* avatar = &avatars->avatars[i];
        NlrAvatar* nlrAvatar = &nlrAvatars[i];
        renderAvatar(self, nlrAvatar, avatar, alpha);
    }
}

static void renderBall(NlRender* self, NlrBall* renderBall, const NlBall* ball, Uint8 alpha)
{
    if (!renderBall->info.isUsed) {
        renderBall->info.isUsed = true;
        renderBall->spawnCountDown = 60u;
        renderBall->simulationCollideCounter = ball->collideCounter;
    }
    bl_vector2i ballRenderPos = simulationToRender(ball->circle.center);

    if (renderBall->spawnCountDown > 0) {
        renderBall->spawnCountDown--;
    }
    const size_t impactTime = 12u;
    if (ball->collideCounter != renderBall->simulationCollideCounter) {
        renderBall->simulationCollideCounter = ball->collideCounter;
        renderBall->lastCollisionCountDown = impactTime;
        renderBall->lastImpactPosition = ballRenderPos;
    }

    float scale = renderBall->spawnCountDown > 0u ? 1.0f - renderBall->spawnCountDown / 60.0f : 1.0f;

    srSpritesCopyEx(&self->spriteRender, &self->ballSprite, ballRenderPos.x, ballRenderPos.y, 0, scale, alpha);

    if (renderBall->lastCollisionCountDown > 0u && alpha == SDL_ALPHA_OPAQUE) {
        renderBall->lastCollisionCountDown--;
        SrSprite ballCollideSprite = self->ballSprite;
        float normalizedTime = 1.0f - renderBall->lastCollisionCountDown / (float) impactTime;
        int spriteSheetIndex = normalizedTime * 2.9f;
        ballCollideSprite.rect.x = (spriteSheetIndex + 2) * 16;
        ballCollideSprite.rect.y = 0;
        ballCollideSprite.rect.w = 16;
        ballCollideSprite.rect.h = 16;
        srSpritesCopyEx(&self->spriteRender, &ballCollideSprite, renderBall->lastImpactPosition.x,
                        renderBall->lastImpactPosition.y, 0, 1.0f, 0xff);
    }
}

static void renderBalls(NlRender* self, const NlGame* predicted, Uint8 alpha)
{
    renderBall(self, &self->ball, &predicted->ball, alpha);
}

static void renderLocalAvatarArrow(NlRender* self, const NlAvatar* avatar)
{
    int x = avatar->circle.center.x;
    int y = avatar->circle.center.y + 26;

    srSpritesCopyEx(&self->spriteRender, &self->arrowSprite, x, y, 0, 1.0f, 0xff);
}

static void renderForLocalParticipants(NlRender* render, const NlGame* predicted, const uint8_t localParticipants[],
                                       size_t localParticipantCount)
{
    for (size_t i = 0; i < localParticipantCount; ++i) {
        uint8_t localParticipantIndex = localParticipants[i];
        const NlParticipant* participant = &predicted->participantLookup[localParticipantIndex];
        if (!participant->isUsed) {
            // we haven't joined for some reason?
        }

        const NlPlayer* player = &predicted->players.players[participant->playerIndex];

        int avatarIndex = player->controllingAvatarIndex;
        if (avatarIndex == -1) {
            continue;
        }

        const NlAvatar* avatar = &predicted->avatars.avatars[avatarIndex];
        renderLocalAvatarArrow(render, avatar);
    }
}

static void renderPlayer(NlRender* render, NlrPlayer* renderPlayer, const NlPlayer* player)
{
    if (!renderPlayer->info.isUsed) {
        renderPlayer->info.isUsed = true;
        renderPlayer->countDown = 120;
    }

    if (renderPlayer->countDown > 0) {
        renderPlayer->countDown--;

        SDL_Color playerJoinedColor = getTeamColor(player->preferredTeamId);
        char buf[32];
        tc_snprintf(buf, 32, "player %d joined", player->playerIndex);
        srFontRenderAndCopy(&render->font, buf, 14, 15, playerJoinedColor);
    }
}

static void renderPlayers(NlRender* render, const NlPlayers* players)
{
    for (size_t i = 0u; i < players->playerCount; ++i) {
        const NlPlayer* player = &players->players[i];
        renderPlayer(render, &render->players[i], player);
    }
}

void nlRenderUpdate(NlRender* self, const NlGame* authoritative, const NlGame* predicted,
                    const uint8_t localParticipants[], size_t participantCount, NlRenderStats stats)
{
    self->stats = stats;

    const NlGame* mainGameStateToUse = predicted;
    const NlGame* alternativeGameState = authoritative;

    const Uint8 mainAlpha = 0xff;
    const Uint8 alternativeAlpha = 0x30;

    if (self->mode == NlRenderModeAuthoritative) {
        mainGameStateToUse = authoritative;
        alternativeGameState = predicted;
    }

    // Render alternative first, since it isn't as important
    renderAvatars(self, self->shadowAvatars, &alternativeGameState->avatars, alternativeAlpha);
    renderBall(self, &self->shadowBall, &alternativeGameState->ball, alternativeAlpha);

    // ------------------------------

    renderPlayers(self, &mainGameStateToUse->players);
    renderAvatars(self, self->avatars, &mainGameStateToUse->avatars, mainAlpha);
    renderBalls(self, mainGameStateToUse, mainAlpha);

    renderGoals(&self->rectangleRender, &g_nlConstants);
    renderBorders(&self->rectangleRender, &g_nlConstants);
    renderHud(&self->font, &self->bigFont, authoritative, mainGameStateToUse);
    renderForLocalParticipants(self, mainGameStateToUse, localParticipants, participantCount);

    renderStats(self);
}

void nlRenderClose(NlRender* self)
{
}
