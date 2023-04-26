/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <SDL2/SDL.h>
#include <clog/console.h>
#include <nimble-ball-presentation/render.h>
#include <nimble-ball-simulation/nimble_ball_simulation.h>

clog_config g_clog;

static int checkSdlEvent(void)
{
    SDL_Event event;
    int quit = 0;

    if (SDL_PollEvent(&event)) {

        switch (event.type) {
            case SDL_QUIT:
                quit = 1;
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    quit = 1;
                }
                break;
            case SDL_KEYUP:
                break;
            case SDL_TEXTINPUT:
                break;
        }
    }

    return quit;
}

int main(int argc, char* argv[])
{
    (void) argc;
    (void) argv;

    g_clog.log = clog_console;
    CLOG_VERBOSE("example start")

    NlRender render;

    nlRenderInit(&render);

    NlGame authoritative;
    NlGame predicted;

    nlGameInit(&authoritative);
    nlGameInit(&predicted);

    int i = 0;
    while (1) {
        predicted.ball.position.x = i++;
        predicted.ball.position.y = 20;
        nlRenderUpdate(&render, &authoritative, &predicted);
        int wantsToQuit = checkSdlEvent();
        if (wantsToQuit) {
            break;
        }
    }

    nlRenderClose(&render);
}
