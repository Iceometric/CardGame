#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SUCCESS 0
#define BUFFER_SIZE 1024

#define CARD_SEPERATOR for (size_t i = 0; i < MAX_NAME_SIZE; ++i) { fputc('#', stdout); } fputc('\n', stdout)

#define MAX_NAME_SIZE 64
#define MAX_DECK_SIZE 100
#define MAX_HAND_SIZE 10
#define COUNT_MANA_TYPES 5
#define SIZE_ALL_CARDS 20

#define CLEAR_SCREEN printf("\033[1;1H\033[2J")

typedef enum Mana {
     VOID, LIGHT, TIME, FIRE, EARTH, LIGHTNING, WATER, NUMBER_OF_ELEMENTS
} Mana;

typedef enum Error { NO_ERROR = 0, ERROR, ALLOCATION_ERROR } Error;

typedef struct Card {
    char name[MAX_NAME_SIZE];
    int mana_cost[NUMBER_OF_ELEMENTS];
    int life_time;
    int durabillity;
    int (*on_play_effect)(void);
    int (*on_round_start_effect)(void);
} Card;

typedef struct CardState {
    int life_time;
    int durabillity;
} CardState;

typedef struct PlayerState {
    Card *deck[MAX_DECK_SIZE];
    Card *hand[MAX_HAND_SIZE];
    Card *draw[MAX_DECK_SIZE];
    Card *discard[MAX_DECK_SIZE];
    Card *in_play[MAX_DECK_SIZE];
    CardState card_state[MAX_DECK_SIZE];
} PlayerState;

typedef struct Game {
    int error;
    int is_running;
    int player_turn;
    char buffer[BUFFER_SIZE];
    Card all_cards[SIZE_ALL_CARDS];
    PlayerState player_state;
} Game;

void print_card(Card *c, size_t n) {
    CARD_SEPERATOR;
    fprintf(stdout, "%lu: %s\n", n, c->name);
}

void print_hand(PlayerState *p) {
    CLEAR_SCREEN;
    for (size_t i = 0; i < MAX_HAND_SIZE; ++i) {
        if (p->hand[i]) {
            print_card(p->hand[i], i);
        }
    }
    CARD_SEPERATOR;
}

size_t get_index_of_first_empty_in_play(PlayerState *p) {
    for (size_t i = 0; i < MAX_DECK_SIZE; ++i) {
        if (!p->in_play[i]) {
            return i;
        }
    }
    exit(ERROR);
    return -1;
}

void handle_input(int input, Game *g) {
    PlayerState *p = &g->player_state;
    printf("Player played: %d\n", input);
    if (p->hand[input]) {
        printf("%s\n", p->hand[input]->name);
        if (p->hand[input]->on_play_effect()) {
            exit(ERROR);
        };
        size_t index = get_index_of_first_empty_in_play(p);
        p->in_play[index] = p->hand[input];
        p->card_state[index].life_time = p->hand[input]->life_time;
        p->card_state[index].durabillity = p->hand[input]->durabillity;
        p->hand[input] = NULL;
    } else {
        printf("Card out of range!\n");
    }
}

int is_valid_input(int input, Game *g) {
    int isnumeric = 1;
    char *p = g->buffer;
    while(*p && isnumeric && p <= &g->buffer[BUFFER_SIZE]) {
        isnumeric = isdigit(*p);
        ++p;
    }
    return isnumeric && input <= MAX_HAND_SIZE;
}

void allocate_resources(Game *g) {
    g->error = NO_ERROR;
    g->is_running = 1;
    memset(g->player_state.hand,        '\0', sizeof(Card*) * MAX_HAND_SIZE);
    memset(g->player_state.deck,        '\0', sizeof(Card*) * MAX_DECK_SIZE);
    memset(g->player_state.draw,        '\0', sizeof(Card*) * MAX_DECK_SIZE);
    memset(g->player_state.discard,     '\0', sizeof(Card*) * MAX_DECK_SIZE);
    memset(g->player_state.in_play,     '\0', sizeof(Card*) * MAX_DECK_SIZE);
    memset(g->player_state.card_state,  '\0', sizeof(CardState) * MAX_DECK_SIZE);
    memset(g->all_cards,                '\0', sizeof(Card) * SIZE_ALL_CARDS);
}

void clean_up_resources(Game *g) {
}

int check_input(char *str) {
    return strcmp(str, "error") == 0 ? ERROR : NO_ERROR;
}

int print_tjena() {
    printf("Tjena din jävel\n");
    return SUCCESS;
}

int print_hejsan() {
    printf("Hejsan din jävel\n");
    return SUCCESS;
}

void handle_round_start(Game *g) {
    PlayerState *p = &g->player_state;
    for (size_t i = 0; i < MAX_DECK_SIZE; ++i) {
        if (p->in_play[i]) {
            printf("Found card %s with life_time: %d\n", p->in_play[i]->name, p->card_state[i].life_time);
            p->in_play[i]->on_round_start_effect();
            if (p->card_state[i].life_time > 0) { p->card_state[i].life_time--; }
            if (p->card_state[i].life_time == 0) {
                p->in_play[i] = NULL;
                memset(&p->card_state,'\0',sizeof(CardState));
            }
        }
    }
    scanf("%s", g->buffer);
}

void run_application(Game *game) {
    printf("Start up...\n");

    // temp mocking
    memcpy(game->all_cards[0].name, "Tjena\0", 6);
    game->all_cards[0].on_play_effect = print_tjena;
    game->all_cards[0].on_round_start_effect = print_tjena;
    game->all_cards[0].life_time = 5;

    Card *card2 = &game->all_cards[1];
    memcpy(card2->name, "Hejsan\0", 7);
    game->all_cards[1].on_play_effect = print_hejsan;
    game->all_cards[1].on_round_start_effect = print_hejsan;
    game->all_cards[1].life_time = -1;

    game->player_state.hand[0] = &game->all_cards[0];
    game->player_state.hand[1] = &game->all_cards[1];

    int player_input;
    scanf("%s", game->buffer);
    while (game->is_running) {
        handle_round_start(game);
        print_hand(&game->player_state);
        scanf("%s", game->buffer);
        if (strcmp(game->buffer, "q") == 0) break;
        player_input = atoi(game->buffer);
        if (!is_valid_input(player_input, game)) continue;
        handle_input(player_input, game);
        game->error = check_input(game->buffer);
        if (game->error) exit(game->error);
    }
    CLEAR_SCREEN;
}

void run(void) {
    Game game;
    allocate_resources  (&game);
    run_application     (&game);
    clean_up_resources  (&game);
}

int main(int argc, char **argv) {
    run();
    return SUCCESS;
}
