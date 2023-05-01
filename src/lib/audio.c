#include "nimble-ball-presentation/audio.h"
#include "nimble-ball-simulation/nimble_ball_simulation.h"

void nlAudioInit(NlAudio* self, SrAudio* audio)
{
    self->lastPlayedCountdown  = 0;
    int result = srSampleLoad(&self->countDown, "data/audio/ball_kick.wav");
    if (result < 0) {
        CLOG_ERROR("could not load ball kick")
    }

    for (size_t i=0; i < 10 ; ++i) {
        self->avatars[i].lastKickedCounter = 0;
    }

    self->ball.lastBouncedCounter = 0;
}

void nlAudioUpdate(NlAudio* self, const struct NlGame* authoritative, const struct NlGame* predicted,
                    const uint8_t localParticipants[], size_t localParticipantCount)
{
    const NlGame* state = predicted;

    if (state->phase == NlGamePhaseCountDown) {
        int number = predicted->phaseCountDown / 62;
        if (number != self->lastPlayedCountdown) {
            srSamplePlay(&self->countDown);
            self->lastPlayedCountdown = number;
        }
    }

    for (size_t i=0; i < state->avatars.avatarCount ; ++i) {
        uint8_t stateKickCounter = state->avatars.avatars[i].kickedCounter;
        NlAudioAvatar* audioAvatar = &self->avatars[i];
        if (stateKickCounter != audioAvatar->lastKickedCounter) {
            self->avatars[i].lastKickedCounter = stateKickCounter;
            srSamplePlay(&self->countDown);
        }
    }

    if (self->ball.lastBouncedCounter != state->ball.collideCounter) {
        srSamplePlay(&self->countDown);
        self->ball.lastBouncedCounter = state->ball.collideCounter;
    }

}
