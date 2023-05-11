/*---------------------------------------------------------------------------------------------
*  Copyright (c) Peter Bjorklund. All rights reserved.
*  Licensed under the MIT License. See LICENSE in the project root for license information.
*--------------------------------------------------------------------------------------------*/
#ifndef NIMBLE_BALL_RENDER_SDL_AUDIO_H
#define NIMBLE_BALL_RENDER_SDL_AUDIO_H

#include <sdl-render/mixer.h>

struct NlGame;
typedef struct NlAudioBall {
    uint8_t lastBouncedCounter;
} NlAudioBall;
typedef struct NlAudioAvatar {
    uint8_t lastKickedCounter;
} NlAudioAvatar;

typedef struct NlAudio {
   SrSample countDown[4];
   SrSample ballKick;
   SrSample ballBounce;
   int lastPlayedCountdown;
   NlAudioAvatar avatars[10];
   NlAudioBall ball;
} NlAudio;


void nlAudioInit(NlAudio * self, SrAudio* audio);
void nlAudioUpdate(NlAudio* self, const struct NlGame* authoritative, const struct NlGame* predicted,
                    const uint8_t localParticipants[], size_t localParticipantCount);

#endif
