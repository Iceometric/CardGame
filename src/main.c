#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SDL2/SDL.h"
#include <SDL2/SDL_log.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_timer.h>

#define str(x) #x

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080

#define SUCCESS 0
#define BUFFER_SIZE 1024

#define MAX_NAME_SIZE 64
#define MAX_DECK_SIZE 100
#define MAX_HAND_SIZE 10
#define MAX_COLUMN_SIZE 8
#define COUNT_MANA_TYPES 5
#define SIZE_ALL_CARDS 20

#define ENDLESS -1
#define DRAW_DAMAGE 1

#define CLEAR_SCREEN printf("\033[1;1H\033[2J")

typedef enum Mana {
     VOID, LIGHT, TIME, FIRE, EARTH, LIGHTNING, WATER, NUMBER_OF_ELEMENTS
} Mana;

typedef enum CardRow {
    ROW_CONDUIT, ROW_ARTIFACT, ROW_TOTAL
} CardRow;

typedef enum CardType {
    CONDUIT, ATTACK, SELF, ARTIFACT, BUFF
} CardType;

typedef enum Error { 
    NO_ERROR = 0, 
    ERROR, 
    ALLOCATION_ERROR,
    WINDOW_ERROR
} Error;

typedef struct Game {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_DisplayMode mode;
    SDL_Event *event;
} Game;

typedef struct Card {
    char name[MAX_NAME_SIZE];
    CardType type;
    int mana_cost[NUMBER_OF_ELEMENTS];
    int life_time;
    int (*on_play_effect)(void*);
    int (*on_round_start_effect)(void*);
} Card;

typedef struct CardState {
    int life_time;
    int column;
    int row;
} CardState;

typedef struct PlayerState {
    int mana[NUMBER_OF_ELEMENTS];
    int health;
    Card **next_draw;
    size_t draw_count;
    Card *deck[MAX_DECK_SIZE];
    Card *hand[MAX_HAND_SIZE];
    Card *draw[MAX_DECK_SIZE];
    Card *discard[MAX_DECK_SIZE];
    Card *in_play[MAX_DECK_SIZE];
    CardState card_state[MAX_DECK_SIZE];
} PlayerState;

typedef struct GameState {
    int error;
    int is_running;
    int player_turn;
    char buffer[BUFFER_SIZE];
    Card all_cards[SIZE_ALL_CARDS];
    PlayerState player_state;
} GameState;

int print_tjena(void *p) {
    printf("Tjena din jävel\n");
    return SUCCESS;
}

int print_hejsan(void *p) {
    printf("Hejsan din jävel\n");
    return SUCCESS;
}

int increment_void(void *p) {
    GameState *gs = (GameState*) p;
    gs->player_state.mana[VOID]++;
    return SUCCESS;
}

int increment_light(void *p) {
    GameState *gs = (GameState*) p;
    gs->player_state.mana[LIGHT]++;
    return SUCCESS;
}

int increment_time(void *p) {
    GameState *gs = (GameState*) p;
    gs->player_state.mana[TIME]++;
    return SUCCESS;
}

int increment_draw_count(void *p) {
    GameState *gs = p;
    gs->player_state.draw_count++;
    return SUCCESS;
}

int nothing(void* p) {
    return SUCCESS;
}

void print_mana(int *mana) {
    fprintf(stdout, "%s: %d | ", str(VOID), mana[VOID]); 
    fprintf(stdout, "%s: %d | ", str(LIGHT), mana[LIGHT]);
    fprintf(stdout, "%s: %d | ", str(TIME), mana[TIME]);
    fprintf(stdout, "%s: %d | ", str(FIRE), mana[FIRE]);
    fprintf(stdout, "%s: %d | ", str(EARTH), mana[EARTH]); 
    fprintf(stdout, "%s: %d | ", str(LIGHTNING), mana[LIGHTNING]); 
    fprintf(stdout, "%s: %d |\n", str(WATER), mana[WATER]);
}

// Card design
Card test[SIZE_ALL_CARDS] = {
    {
        .name = "Tjena",
        .mana_cost[VOID] = 1,
        .mana_cost[LIGHT] = 0,
        .mana_cost[TIME] = 0,
        .mana_cost[FIRE] = 0,
        .mana_cost[EARTH] = 0,
        .mana_cost[LIGHTNING] = 0,
        .mana_cost[WATER] = 0,
        .life_time = 5,
        .type = ATTACK,
        .on_play_effect = print_tjena,
        .on_round_start_effect = print_tjena,
    }, 
    {
        .name = "Hejsan",
        .mana_cost[VOID] = 0,
        .mana_cost[LIGHT] = 1,
        .mana_cost[TIME] = 0,
        .mana_cost[FIRE] = 0,
        .mana_cost[EARTH] = 0,
        .mana_cost[LIGHTNING] = 0,
        .mana_cost[WATER] = 0,
        .life_time = ENDLESS,
        .type = ARTIFACT,
        .on_play_effect = print_hejsan,
        .on_round_start_effect = print_hejsan,
    }, 
    {
        .name = "Void Conduit",
        .mana_cost[VOID] = 0,
        .mana_cost[LIGHT] = 0,
        .mana_cost[TIME] = 0,
        .mana_cost[FIRE] = 0,
        .mana_cost[EARTH] = 0,
        .mana_cost[LIGHTNING] = 0,
        .mana_cost[WATER] = 0,
        .life_time = ENDLESS,
        .type = CONDUIT,
        .on_play_effect = nothing,
        .on_round_start_effect = increment_void,
    },
    {
        .name = "Light Conduit",
        .mana_cost[VOID] = 0,
        .mana_cost[LIGHT] = 0,
        .mana_cost[TIME] = 0,
        .mana_cost[FIRE] = 0,
        .mana_cost[EARTH] = 0,
        .mana_cost[LIGHTNING] = 0,
        .mana_cost[WATER] = 0,
        .life_time = ENDLESS,
        .type = CONDUIT,
        .on_play_effect = nothing,
        .on_round_start_effect = increment_light,
    },
    {
        .name = "Time Conduit",
        .mana_cost[VOID] = 0,
        .mana_cost[LIGHT] = 0,
        .mana_cost[TIME] = 0,
        .mana_cost[FIRE] = 0,
        .mana_cost[EARTH] = 0,
        .mana_cost[LIGHTNING] = 0,
        .mana_cost[WATER] = 0,
        .life_time = ENDLESS,
        .type = CONDUIT,
        .on_play_effect = nothing,
        .on_round_start_effect = increment_time,
    },
    {
        .name = "Draw increase",
        .mana_cost[VOID] = 0,
        .mana_cost[LIGHT] = 0,
        .mana_cost[TIME] = 0,
        .mana_cost[FIRE] = 0,
        .mana_cost[EARTH] = 0,
        .mana_cost[LIGHTNING] = 0,
        .mana_cost[WATER] = 0,
        .life_time = 2,
        .type = BUFF,
        .on_play_effect = increment_draw_count,
        .on_round_start_effect = nothing,
    }
};

void print_card(Card *c, size_t n) {
    fprintf(stdout, "%lu: %s\n", n, c->name);
    print_mana(c->mana_cost);
    fprintf(stdout, "\n");
}

void print_hand(PlayerState *p) {
    CLEAR_SCREEN;
    for (size_t i = 0; i < MAX_HAND_SIZE; ++i) {
        if (p->hand[i]) {
            print_card(p->hand[i], i);
        }
    }
    fprintf(stdout, "\n");
}

size_t get_index_of_first_empty(Card **c, size_t max) {
    for (size_t i = 0; i < max; ++i) {
        if (!c[i]) {
            return i;
        }
    }
    exit(ERROR);
    return -1;
}

int enough_mana(int *cost, int *mana) {
    int enough_mana = 1;
    for (size_t i = 0; i < NUMBER_OF_ELEMENTS; ++i) {
        enough_mana = (cost[i] <= mana[i]);
        if (!enough_mana) break;
    }
    return enough_mana;
}

void remove_mana(int *cost, int *mana) {
    for (size_t i = 0; i < NUMBER_OF_ELEMENTS; ++i) {
        mana[i] -= cost[i];
    }
}

int get_row_by_card_type(CardType type) {
    switch (type) {
        case CONDUIT:   return ROW_CONDUIT;
        case ATTACK:    return -1;
        case SELF:      return -1;
        case ARTIFACT:  return ROW_ARTIFACT;
        case BUFF:      return -1;
    }
    return -1;
}

void handle_input(int input, GameState *gs) {
    PlayerState *p = &gs->player_state;
    if (p->hand[input]) {
        if (!enough_mana(p->hand[input]->mana_cost, p->mana)) {
            printf("Not enough mana\n");
            return;
        }
        printf("Played card(%d): %s\n", input, p->hand[input]->name);
        if (p->hand[input]->on_play_effect(gs)) {
            exit(ERROR);
        };
        remove_mana(p->hand[input]->mana_cost, p->mana);
        size_t index = get_index_of_first_empty(p->in_play, MAX_DECK_SIZE);
        // Update CardState
        p->in_play[index] = p->hand[input];
        p->card_state[index].life_time = p->hand[input]->life_time;
        p->card_state[index].row = get_row_by_card_type(p->hand[input]->type);
        printf("Play card where?\n");
        scanf("%s", gs->buffer);
        p->card_state[index].column = atoi(gs->buffer);
        p->hand[input] = NULL;
    } else {
        printf("Card out of range!\n");
    }
}

int is_valid_input(int input, GameState *gs) {
    int isnumeric = 1;
    char *p = gs->buffer;
    while(*p && isnumeric && p <= &gs->buffer[BUFFER_SIZE]) {
        isnumeric = isdigit(*p);
        ++p;
    }
    return isnumeric && input <= MAX_HAND_SIZE;
}

void allocate_resources(GameState *gs, Game *game) {
    gs->error = NO_ERROR;
    gs->is_running = 1;
    gs->player_turn = 1;
    gs->player_state.draw_count = 1;
    gs->player_state.health = 40;

    memset(gs->player_state.hand,        '\0', sizeof(Card*) * MAX_HAND_SIZE);
    memset(gs->player_state.deck,        '\0', sizeof(Card*) * MAX_DECK_SIZE);
    memset(gs->player_state.draw,        '\0', sizeof(Card*) * MAX_DECK_SIZE);
    memset(gs->player_state.discard,     '\0', sizeof(Card*) * MAX_DECK_SIZE);
    memset(gs->player_state.in_play,     '\0', sizeof(Card*) * MAX_DECK_SIZE);
    memset(gs->player_state.card_state,  '\0', sizeof(CardState) * MAX_DECK_SIZE);
    memset(gs->all_cards,                '\0', sizeof(Card) * SIZE_ALL_CARDS);
    memset(gs->player_state.mana,        '\0', sizeof(int) * NUMBER_OF_ELEMENTS);

    if (SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE, &game->window, &game->renderer)) {
        exit(-WINDOW_ERROR);
    };
}

void clean_up_resources(GameState *gs) {
}

int check_input(char *str) {
    return strcmp(str, "error") == 0 ? ERROR : NO_ERROR;
}

void reset_mana(PlayerState *p) {
    for (size_t i = 0; i < NUMBER_OF_ELEMENTS; ++i) {
        p->mana[i] = 0;
    }
}

void handle_draw_damage(PlayerState *p) {
    p->health -= DRAW_DAMAGE;
}

void handle_draw(PlayerState *p) {
    for (size_t i = 0; i < p->draw_count; ++i) {
        if (!(p->next_draw < p->draw)) {
            size_t i = get_index_of_first_empty(p->hand, MAX_HAND_SIZE);
            p->hand[i] = *p->next_draw;
            *p->next_draw = NULL;
            p->next_draw--;
        } else {
            handle_draw_damage(p);
        }
    }
}

void handle_life_time(GameState *gs) {
    PlayerState *p = &gs->player_state;
    for (size_t i = 0; i < MAX_DECK_SIZE; ++i) {
        if (!p->in_play[i]) continue; 

        printf("Found card %s with life_time: %d\n", p->in_play[i]->name, p->card_state[i].life_time);
        p->in_play[i]->on_round_start_effect(gs);
        if (p->card_state[i].life_time > 0) { 
            p->card_state[i].life_time -= 1;
        }

        if (p->card_state[i].life_time == 0) {
            p->in_play[i] = NULL;
            memset(&p->card_state[i],'\0',sizeof(CardState));
        }
    }
}

void handle_round_start(GameState *gs) {
    PlayerState *p = &gs->player_state;
    gs->player_turn = 1;
    reset_mana(p);
    handle_life_time(gs);
    handle_draw(p);
}

void handle_enemy_round(GameState *gs) {

}

void shuffle_deck_into_draw(PlayerState *p) {
    Card **draw = p->draw;
    //TODO populate array in random order
    for (size_t i = 0; i < MAX_DECK_SIZE; ++i) {
        if (p->deck[i]) {
            *draw = p->deck[i];
            draw++;
        }
    }
}

void print_board_state(CardState *state) {
    for (size_t i = 0; i < ROW_TOTAL; ++i) {
        printf("%lu:", i);
        for (size_t j = 0; j < MAX_COLUMN_SIZE; ++j) {
            for (size_t k = 0; k < MAX_DECK_SIZE; ++k) {
                if (state[k].column == j && state[k].row == i) {
                    printf("|%2d", state[k].life_time);
                }
            }
            printf("|  ");
        }
        printf("\n");
    }

}

void test_draw_stuff(Game *g) {
}

void run_application(GameState *game_state) {
    game_state->player_state.deck[0] = &test[0];
    game_state->player_state.deck[1] = &test[1];
    game_state->player_state.deck[2] = &test[2];
    game_state->player_state.deck[3] = &test[3];
    game_state->player_state.deck[4] = &test[4];
    game_state->player_state.deck[5] = &test[2];
    game_state->player_state.deck[6] = &test[5];
    game_state->player_state.next_draw = &game_state->player_state.draw[6];

    shuffle_deck_into_draw(&game_state->player_state);

    int player_input;
    scanf("%s", game_state->buffer);
    while (game_state->is_running) {

        handle_enemy_round(game_state);
        handle_round_start(game_state);
        if (game_state->player_state.health <= 0) break;
        
        do {
            print_hand(&game_state->player_state);
            fprintf(stdout, "--- PLAYER - %dhp ---\n", game_state->player_state.health);
            if (game_state->player_state.health <= 0) break;
            print_mana(game_state->player_state.mana);
            print_board_state(game_state->player_state.card_state);

            scanf("%s", game_state->buffer);
            player_input = atoi(game_state->buffer);
            game_state->is_running  = (strcmp(game_state->buffer, "q") != 0); 
            game_state->player_turn = (strcmp(game_state->buffer, "d") != 0);

            if (is_valid_input(player_input, game_state)) {
                handle_input(player_input, game_state);
                scanf("%s", game_state->buffer);
            }

            game_state->error = check_input(game_state->buffer);
            if (game_state->error) exit(game_state->error);

        } while (game_state->player_turn && game_state->is_running);
    }
    CLEAR_SCREEN;
    if (game_state->player_state.health <= 0) printf("Game Over!\n");
}

void run(void) {
    GameState game_state;
    Game game;
    allocate_resources  (&game_state, &game);
    run_application     (&game_state);
    clean_up_resources  (&game_state);
}

int main(int argc, char **argv) {
    run();
    return SUCCESS;
}
