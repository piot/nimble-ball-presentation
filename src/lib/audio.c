#include "nimble-ball-presentation/audio.h"
#include "nimble-ball-simulation/nimble_ball_simulation.h"

void nlAudioInit(NlAudio* self, SrAudio* audio)
{
    self->lastPlayedCountdown = 0;
    char filename[64];

    for (size_t i = 0; i < 4; ++i) {
        tc_snprintf(filename, 64, "data/audio/countdown_%d.wav", i);
        int result = srSampleLoad(&self->countDown[i], filename);
        if (result < 0) {
            CLOG_ERROR("could not load countdown")
        }
    }
    int result = srSampleLoad(&self->ballKick, "data/audio/ball_kick.wav");
    if (result < 0) {
        CLOG_ERROR("could not load ball_kick")
    }

    result = srSampleLoad(&self->ballBounce, "data/audio/ball_bounce.wav");
    if (result < 0) {
        CLOG_ERROR("could not load ball bounce")
    }

    for (size_t i = 0; i < 10; ++i) {
        self->avatars[i].lastKickedCounter = 0;
    }

    self->ball.lastBouncedCounter = 0;
}

void nlAudioUpdate(NlAudio* self, const struct NlGame* authoritative, const struct NlGame* predicted,
                   const uint8_t localParticipants[], size_t localParticipantCount)
{
    const NlGame* state = predicted;

    if (state->phase == NlGamePhaseCountDown) {
        int number = (predicted->phaseCountDown + 61) / 62;
        if (number != self->lastPlayedCountdown) {
            srSamplePlay(&self->countDown[number]);
            self->lastPlayedCountdown = number;
        }
    }

    // We were doing a count-down, and now we are in-game, say GO!
    if (self->lastPlayedCountdown >= 1 && state->phase == NlGamePhasePlaying) {
        srSamplePlay(&self->countDown[0]);
        self->lastPlayedCountdown = 0;
    }

    for (size_t i = 0; i < state->avatars.avatarCount; ++i) {
        uint8_t stateKickCounter = state->avatars.avatars[i].kickedCounter;
        NlAudioAvatar* audioAvatar = &self->avatars[i];
        if (stateKickCounter != audioAvatar->lastKickedCounter) {
            self->avatars[i].lastKickedCounter = stateKickCounter;
            srSamplePlay(&self->ballKick);
        }
    }

    if (self->ball.lastBouncedCounter != state->ball.collideCounter) {
        srSamplePlay(&self->ballBounce);
        self->ball.lastBouncedCounter = state->ball.collideCounter;
    }
}
