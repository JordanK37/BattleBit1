//
// Created by carson on 5/20/20.
//

#include <stdlib.h>
#include <stdio.h>
#include <tgmath.h>
#include <string.h>
#include <argp.h>
#include "game.h"

// STEP 9 - Synchronization: the GAME structure will be accessed by both players interacting
// asynchronously with the server.  Therefore the data must be protected to avoid race conditions.
// Add the appropriate synchronization needed to ensure a clean battle.

static game * GAME = NULL;

void game_init() {
    if (GAME) {
        free(GAME);
    }
    GAME = malloc(sizeof(game));
    GAME->status = CREATED;
    game_init_player_info(&GAME->players[0]);
    game_init_player_info(&GAME->players[1]);
}

void game_init_player_info(player_info *player_info) {
    player_info->ships = 0;
    player_info->hits = 0;
    player_info->shots = 0;
}

int game_fire(game *game, int player, int x, int y) {
    // Step 5 - This is the crux of the game.  You are going to take a shot from the given player and
    // update all the bit values that store our game state.
    //
    //  - You will need up update the players 'shots' value
    //  - you You will need to see if the shot hits a ship in the opponents ships value.  If so, record a hit in the
    //    current players hits field
    //  - If the shot was a hit, you need to flip the ships value to 0 at that position for the opponents ships field
    //
    //  If the opponents ships value is 0, they have no remaining ships, and you should set the game state to
    //  PLAYER_1_WINS or PLAYER_2_WINS depending on who won.
}

unsigned long long int xy_to_bitval(int x, int y) {
    return (x > 7 | y > 7 | x < 0 | y < 0) ? 0 : (1ull << (x + (y * 8)));
    // Step 1 - implement this function.  We are taking an x, y position
    // and using bitwise operators, converting that to an unsigned long long
    // with a 1 in the position corresponding to that x, y
    //
    // x:0, y:0 == 0b00000...0001 (the one is in the first position)
    // x:1, y: 0 == 0b00000...10 (the one is in the second position)
    // ....
    // x:0, y: 1 == 0b100000000 (the one is in the eighth position)
    //
    // you will need to use bitwise operators and some math to produce the right
    // value.

}

struct game * game_get_current() {
    return GAME;
}

int game_load_board(struct game *game, int player, char * spot) {
    if (spot != NULL && strlen(spot) == 15) {
        unsigned long long int originalBoard = game->players[player].ships;
        char useds[5] = {};
        int lengths[] = {5, 4, 3, 3, 2};
        char ships[] = {'c', 'b', 'd', 's', 'p'};
        for (int i = 0; i < 15; i += 3) {
            int length;
            for (int z = 0; z < sizeof(ships); z++) {
                if (sizeof(useds) >= z && tolower(spot[i]) == tolower(useds[z])) {  //same ships

                    return -1;
                }
                if (tolower(spot[i]) == ships[z]) { //find length of ship
                    length = lengths[z];
                    break;
                } else if (z == sizeof(ships) - 1) {
                    return -1;
                }
            }
            strncat(useds, &spot[i], 1);
            int result;
            result = (spot[i] >= 'A' && spot[i] <= 'Z') ?
                     add_ship_horizontal(&game->players[player], ((int) spot[i + 1] - '0'), ((int) spot[i + 2] - '0'),
                                         length) :
                     add_ship_vertical(&game->players[player], ((int) spot[i + 1] - '0'), ((int) spot[i + 2] - '0'),
                                       length);
            if (result == -1) { //reset board if -1
                game->players[player].ships = originalBoard;
                return -1;
            }
        }
        int opp = (player == 0) ? 1 : 0;
        game->status = (game->players[player].ships != 0 && game->players[opp].ships != 0) ? PLAYER_0_TURN
                                                                                           : CREATED;
        return 1;
    } else {
        return -1;
    }

}
    // Step 2 - implement this function.  Here you are taking a C
    // string that represents a layout of ships, then testing
    // to see if it is a valid layout (no off-the-board positions
    // and no overlapping ships)
    //

    // if it is valid, you should write the corresponding unsigned
    // long long value into the Game->players[player].ships data
    // slot and return 1
    //
    // if it is invalid, you should return -1


int add_ship_horizontal(player_info *player, int x, int y, int length) {
    if (length == 0)
        return 1;
    if ((player->ships ^ xy_to_bitval(x, y)) > player->ships) {  //makes sure the ships arent overlapping
        player->ships ^= xy_to_bitval(x, y);
        return add_ship_horizontal(player, ++x, y, --length);
    } else
        return -1;
}
    // implement this as part of Step 2
    // returns 1 if the ship can be added, -1 if not
    // hint: this can be defined recursively


int add_ship_vertical(player_info *player, int x, int y, int length) {
    // implement this as part of Step 2
    // returns 1 if the ship can be added, -1 if not
    // hint: this can be defined recursively
    if (length == 0)
        return 1;
    if ((player->ships ^ xy_to_bitval(x, y)) > player->ships) { //overlapping
        player->ships ^= xy_to_bitval(x, y);
        return add_ship_vertical(player, x, ++y, --length);
    } else
        return -1;
}