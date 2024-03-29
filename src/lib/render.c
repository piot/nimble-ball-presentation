/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include "basal/vector2i.h"
#include <SDL2_image/SDL_image.h>
#include <nimble-ball-presentation/render.h>

static void setupAvatarSprite(SrSprite* sprite, SDL_Texture* texture, int cellIndex)
{
    sprite->rect.x = 0 + cellIndex * 48;
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

static void setupJerseySprite(SrSprite* sprite, SDL_Texture* texture, int teamIndex)
{
    sprite->rect.x = 2 + teamIndex * 32;
    sprite->rect.y = 37;
    sprite->rect.w = 28;
    sprite->rect.h = 22;
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
    setupJerseySprite(&self->jerseySprite[0], equipmentTexture, 0);
    setupJerseySprite(&self->jerseySprite[1], equipmentTexture, 1);
    self->mode = NlRenderModePredicted;
}

static BlVector2i simulationToRender(BlVector2 pos)
{
    BlVector2i result;
    result.x = (int) pos.x;
    result.y = (int) pos.y;

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

        srRectsLineRect(rectangleRender, (int) goal->rect.position.x, (int) goal->rect.position.y,
                        (int) goal->rect.size.x, (int) goal->rect.size.y);
    }
}

static void renderBorders(SrRects* lineRender, const NlConstants* constants)
{
    SDL_SetRenderDrawColor(lineRender->renderer, 255, 240, 127, SDL_ALPHA_OPAQUE);
    for (size_t i = 0; i < sizeof(constants->borderSegments) / sizeof(constants->borderSegments[0]); ++i) {
        const BlLineSegment* lineSegment = &constants->borderSegments[i];
        srRectsDrawLine(lineRender, (int) lineSegment->a.x, (int) lineSegment->a.y, (int) lineSegment->b.x,
                        (int) lineSegment->b.y);
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
            renderCountDown(bigFont, (uint8_t) predicted->phaseCountDown);
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
    tc_snprintf(buf, 512, "preId %04X autId %04X conBufCnt %d fps:%d latency:%d", self->stats.predictedTickId,
                self->stats.authoritativeTickId, self->stats.authoritativeStepsInBuffer, self->stats.renderFps,
                self->stats.latencyMs);
    SDL_Color color = {0xff, 0xff, 0xff, SDL_ALPHA_OPAQUE};
    srFontRenderAndCopy(&self->font, buf, 10, 359 - 6, color);
}

#include <basal/math.h>

static const float lerpFactor = 1.0f;

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
    renderAvatar->precisionPosition = blVector2AddScale(renderAvatar->precisionPosition, delta, lerpFactor);

    float angleDiff = blAngleMinimalDiff(avatar->visualRotation, renderAvatar->rotation);

    const float maxRadianDiffPerTime = 0.1f;
    float diffThisTick = blFabs(angleDiff) > maxRadianDiffPerTime ? blFSign(angleDiff) * maxRadianDiffPerTime
                                                                  : angleDiff;
    renderAvatar->rotation += diffThisTick;

    if (renderAvatar->spawnCountDown > 0u) {
        renderAvatar->spawnCountDown--;
    }

    float scale = renderAvatar->spawnCountDown > 0u ? 1.0f - renderAvatar->spawnCountDown / (float) avatarSpawnTime
                                                    : 1.0f;

    BlVector2i avatarRenderPos = simulationToRender(renderAvatar->precisionPosition);
    int degreesAngle = (int) (renderAvatar->rotation * 360.0f / ((float) M_PI * 2.0f));

    if (avatar->isInvisible) {
        alpha = 0x20;
    }

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

static void renderBall(NlRender* self, NlrBall* nlrBall, const NlBall* ball, Uint8 alpha)
{
    if (!nlrBall->info.isUsed) {
        nlrBall->info.isUsed = true;
        nlrBall->spawnCountDown = 60u;
        nlrBall->simulationCollideCounter = ball->collideCounter;
        nlrBall->precisionPosition = ball->circle.center;
    }
    BlVector2 ballRenderTargetPos = ball->circle.center;

    if (nlrBall->spawnCountDown > 0) {
        nlrBall->spawnCountDown--;
    }
    const size_t impactTime = 12u;
    if (ball->collideCounter != nlrBall->simulationCollideCounter) {
        nlrBall->simulationCollideCounter = ball->collideCounter;
        nlrBall->lastCollisionCountDown = impactTime;
        nlrBall->lastImpactPosition.x = (int) ballRenderTargetPos.x;
        nlrBall->lastImpactPosition.y = (int) ballRenderTargetPos.y;
    }

    BlVector2 delta = blVector2Sub(ballRenderTargetPos, nlrBall->precisionPosition);
    nlrBall->precisionPosition = blVector2AddScale(nlrBall->precisionPosition, delta, lerpFactor);

    float scale = nlrBall->spawnCountDown > 0u ? 1.0f - (float) nlrBall->spawnCountDown / 60.0f : 1.0f;

    srSpritesCopyEx(&self->spriteRender, &self->ballSprite, (int) nlrBall->precisionPosition.x,
                    (int) nlrBall->precisionPosition.y, 0, scale, alpha);

    if (nlrBall->lastCollisionCountDown > 0u && alpha == SDL_ALPHA_OPAQUE) {
        nlrBall->lastCollisionCountDown--;
        SrSprite ballCollideSprite = self->ballSprite;
        float normalizedTime = 1.0f - (float) nlrBall->lastCollisionCountDown / (float) impactTime;
        int spriteSheetIndex = (int) (normalizedTime * 2.9f);
        ballCollideSprite.rect.x = (spriteSheetIndex + 2) * 16;
        ballCollideSprite.rect.y = 0;
        ballCollideSprite.rect.w = 16;
        ballCollideSprite.rect.h = 16;
        srSpritesCopyEx(&self->spriteRender, &ballCollideSprite, nlrBall->lastImpactPosition.x,
                        nlrBall->lastImpactPosition.y, 0, 1.0f, 0xff);
    }
}

static void renderBalls(NlRender* self, const NlGame* predicted, Uint8 alpha)
{
    renderBall(self, &self->ball, &predicted->ball, alpha);
}

static void renderLocalAvatarArrow(NlRender* self, const NlAvatar* avatar)
{
    int x = (int) avatar->circle.center.x;
    int y = (int) (avatar->circle.center.y + 26);

    srSpritesCopyEx(&self->spriteRender, &self->arrowSprite, x, y, 0, 1.0f, 0xff);
}

static void renderMenus(NlRender* render, const SrFont* font, const SrFont* bigFont, const NlGame* predicted,
                        const NlPlayer* player, NlrLocalPlayer* renderPlayer)
{
    (void) bigFont;
    (void) predicted;

    switch (player->phase) {
        case NlPlayerPhaseSelectTeam: {
            int backgroundY = 100;
            int backgroundX = 100;
            SDL_Color backgroundColor = {0x33, 0x33, 0x33, 0xcc};
            SDL_SetRenderDrawColor(render->renderer, backgroundColor.r, backgroundColor.g, backgroundColor.b,
                                   backgroundColor.a);

            srRectsFillRect(&render->rectangleRender, backgroundX, backgroundY, 400, 200);
            SDL_Color secondsColor = {0xff, 0xff, 0xff, SDL_ALPHA_OPAQUE};
            srFontRenderAndCopy(font, "SELECT YOUR TEAM!", 200, 280, secondsColor);
            int jerseyY = 200;
            const float selectedScale = 4.0f;
            const float notSelectedScale = 3.0f;
            srSpritesCopyEx(&render->spriteRender, &render->jerseySprite[0], 240, jerseyY, 0,
                            renderPlayer->highlightedTeamIndex == 0 ? selectedScale : notSelectedScale, 0xff);
            srSpritesCopyEx(&render->spriteRender, &render->jerseySprite[1], 400, jerseyY, 0,
                            renderPlayer->highlightedTeamIndex == 1 ? selectedScale : notSelectedScale, 0xff);
        } break;
        case NlPlayerPhaseCommittedToTeam: {
            int backgroundY = 100;
            int backgroundX = 100;
            SDL_Color backgroundColor = {0x33, 0x33, 0x33, 0xcc};
            SDL_SetRenderDrawColor(render->renderer, backgroundColor.r, backgroundColor.g, backgroundColor.b,
                                   backgroundColor.a);
            srRectsFillRect(&render->rectangleRender, backgroundX, backgroundY, 400, 200);
            SDL_Color secondsColor = {0xff, 0xff, 0xff, SDL_ALPHA_OPAQUE};
            srFontRenderAndCopy(font, "Team selected. Waiting for Countdown", 30, 280, secondsColor);
        } break;
        case NlPlayerPhasePlaying:

            break;
    }
}

NlrLocalPlayer* nlRenderFindLocalPlayerFromParticipantId(NlRender* self, uint8_t participantId)
{
    for (size_t i = 0; i < NLR_MAX_LOCAL_PLAYERS; ++i) {
        NlrLocalPlayer* renderPlayer = &self->localPlayers[i];
        if (!renderPlayer->info.isUsed) {
            continue;
        }
        if (renderPlayer->participantId == participantId) {
            return renderPlayer;
        }
    }
    return 0;
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
        NlrLocalPlayer* renderPlayer = nlRenderFindLocalPlayerFromParticipantId(render, localParticipantIndex);
        renderMenus(render, &render->font, &render->bigFont, predicted, player, renderPlayer);

        uint8_t avatarIndex = player->controllingAvatarIndex;
        if (avatarIndex == NL_AVATAR_INDEX_UNDEFINED) {
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
        renderPlayer->countDown = 180;
        renderPlayer->destroyCountdown = 0;
        // renderPlayer->participantId = player->assignedToParticipantIndex;
    }

    if (renderPlayer->countDown > 0) {
        renderPlayer->countDown--;

        SDL_Color playerJoinedColor = getTeamColor(player->preferredTeamId);
        char buf[32];
        tc_snprintf(buf, 32, "player %d joined", player->playerIndex);
        srFontRenderAndCopy(&render->font, buf, 14, 15, playerJoinedColor);
    }

    if (renderPlayer->destroyCountdown > 0) {
        renderPlayer->destroyCountdown--;
        if (renderPlayer->destroyCountdown == 0) {
            renderPlayer->info.isUsed = false;
        }

        SDL_Color playerJoinedColor = getTeamColor(player->preferredTeamId);
        char buf[32];
        tc_snprintf(buf, 32, "player %d left", player->playerIndex);
        srFontRenderAndCopy(&render->font, buf, 14, 15, playerJoinedColor);
    }
}

static void renderPlayers(NlRender* render, const NlPlayers* players)
{
    for (size_t i = 0U; i < NL_MAX_PLAYERS; ++i) {
        NlrPlayer* renderPlayer = &render->players[i];

        if (renderPlayer->info.isUsed && i >= players->playerCount && renderPlayer->destroyCountdown == 0) {
            renderPlayer->destroyCountdown = 120U;
        }
    }

    for (size_t i = 0u; i < players->playerCount; ++i) {
        const NlPlayer* player = &players->players[i];
        renderPlayer(render, &render->players[i], player);
    }
}

static NlrLocalPlayer* findFreeRenderLocalPlayer(NlRender* self)
{
    for (size_t i = 0; i < NLR_MAX_LOCAL_PLAYERS; ++i) {
        NlrLocalPlayer* foundPlayer = &self->localPlayers[i];
        if (!foundPlayer->info.isUsed) {
            return foundPlayer;
        }
    }

    return NULL;
}

static void spawnLocalPlayersIfNeeded(NlRender* self, const uint8_t localParticipants[], size_t participantCount)
{
    if (participantCount > NLR_MAX_LOCAL_PLAYERS) {
        CLOG_ERROR("can not continue, participant count is wrong")
    }
    for (size_t i = 0; i < participantCount; ++i) {
        uint8_t localParticipantId = localParticipants[i];
        NlrLocalPlayer* foundPlayer = nlRenderFindLocalPlayerFromParticipantId(self, localParticipantId);
        if (foundPlayer == NULL) {
            foundPlayer = findFreeRenderLocalPlayer(self);
            foundPlayer->info.isUsed = true;
            foundPlayer->participantId = localParticipantId;
            foundPlayer->selectedTeamIndex = NL_TEAM_UNDEFINED;
            foundPlayer->highlightedTeamIndex = 0;
            srGamepadInit(&foundPlayer->previousGamepad);
            srGamepadInit(&foundPlayer->gamepad);
        }
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

    spawnLocalPlayersIfNeeded(self, localParticipants, participantCount);

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

static void teamSelection(NlrLocalPlayer* renderLocalPlayer, int horizontal)
{
    if (horizontal > 0) {
        renderLocalPlayer->highlightedTeamIndex = 1;
    } else if (horizontal < 0) {
        renderLocalPlayer->highlightedTeamIndex = 0;
    }
}

static void updateInput(NlrLocalPlayer* renderPlayer, const SrGamepad* gamepad, const NlPlayer* predictedPlayer)
{
    renderPlayer->gamepad = *gamepad;
    switch (predictedPlayer->phase) {
        case NlPlayerPhaseSelectTeam:
            if (renderPlayer->selectedTeamIndex == NL_TEAM_UNDEFINED) {
                if (renderPlayer->previousGamepad.horizontalAxis == 0 && renderPlayer->gamepad.horizontalAxis != 0) {
                    teamSelection(renderPlayer, renderPlayer->gamepad.horizontalAxis);
                }
                if (renderPlayer->gamepad.a == 1 && renderPlayer->previousGamepad.a == 0) {
                    renderPlayer->selectedTeamIndex = renderPlayer->highlightedTeamIndex;
                }
            }
            break;
        case NlPlayerPhaseCommittedToTeam:
            break;
        case NlPlayerPhasePlaying:
            break;
    }

    renderPlayer->previousGamepad = *gamepad;
}

void nlRenderFeedInput(NlRender* self, SrGamepad* gamepads, const NlGame* predicted, const uint8_t localParticipants[],
                       size_t localParticipantCount)
{


    for (size_t i = 0; i < localParticipantCount; ++i) {
        NlrLocalPlayer* player = nlRenderFindLocalPlayerFromParticipantId(self, localParticipants[i]);
        const NlPlayer* simulationPlayer = nlGameFindSimulationPlayerFromParticipantId(predicted, localParticipants[i]);
        if (player != 0 && simulationPlayer != 0) {
            updateInput(player, &gamepads[i], simulationPlayer);
        }
    }
}

void nlRenderClose(NlRender* self)
{
    (void) self;
}
