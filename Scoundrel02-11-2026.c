#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define MAX_HEALTH       20
#define TOTAL_CARDS      44
#define ROOM_SIZE        4
#define NO_LIMIT         99

/* Card types */
#define MONSTER 0
#define WEAPON  1
#define POTION  2

/* Suits */
#define CLUBS    0
#define SPADES   1
#define HEARTS   2
#define DIAMONDS 3

struct Card {
    int suit;
    int type;
    int value;
};

struct Player {
    int health;
    int equipped_weapon;
    int last_monster_slain;
    int potion_used_this_turn;
    struct Card last_card_played;
};

static const char *get_suit_name(int suit) {
    switch (suit) {
        case CLUBS:    return "Clubs (M)";
        case SPADES:   return "Spades (M)";
        case HEARTS:   return "Hearts (P)";
        case DIAMONDS: return "Diamonds (W)";
        default:       return "?";
    }
}

static void print_card(struct Card c) {
    printf("[ ");
    if (c.value <= 10)       printf("%d", c.value);
    else if (c.value == 11)  printf("J");
    else if (c.value == 12)  printf("Q");
    else if (c.value == 13)  printf("K");
    else                     printf("A");
    printf(" of %s ]", get_suit_name(c.suit));
}

static char read_input(const char *prompt) {
    char buffer[64];
    int ch;

    printf("%s", prompt);
    fflush(stdout);

    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        return '\0';
    }

    if (strchr(buffer, '\n') == NULL) {
        while ((ch = getchar()) != '\n' && ch != EOF) { }
    }

    return (char)tolower((unsigned char)buffer[0]);
}

static int create_deck(struct Card dungeon[]) {
    int total = 0;
    int suit;
    int value;

    /* Monsters: Clubs and Spades, 2..14 (Ace = 14) -> 26 cards */
    for (suit = CLUBS; suit <= SPADES; suit++) {
        for (value = 2; value <= 14; value++) {
            dungeon[total].suit  = suit;
            dungeon[total].type  = MONSTER;
            dungeon[total].value = value;
            total++;
        }
    }
    /* Potions: Hearts 2..10 -> 9 cards */
    for (value = 2; value <= 10; value++) {
        dungeon[total].suit  = HEARTS;
        dungeon[total].type  = POTION;
        dungeon[total].value = value;
        total++;
    }
    /* Weapons: Diamonds 2..10 -> 9 cards */
    for (value = 2; value <= 10; value++) {
        dungeon[total].suit  = DIAMONDS;
        dungeon[total].type  = WEAPON;
        dungeon[total].value = value;
        total++;
    }
    return total;
}

static void shuffle_deck(struct Card dungeon[], int total) {
    int i;
    int j;
    struct Card temp;

    for (i = total - 1; i > 0; i--) {
        j = rand() % (i + 1);
        temp = dungeon[i];
        dungeon[i] = dungeon[j];
        dungeon[j] = temp;
    }
}

static void play_game(void) {
    struct Card dungeon[TOTAL_CARDS];
    struct Card room[ROOM_SIZE];
    struct Card chosen;
    struct Player player;

    int dungeon_count;
    int room_count;
    int occupied[ROOM_SIZE];
    int discard_count;
    int cards_faced;
    int can_skip;
    int index;
    int final_score;
    char choice;
    int i;

    int health_before;
    int can_use_weapon;
    int damage_with_weapon;
    int use_weapon;
    char c;
    int monster_sum;

    /* Variable initializations */
    player.health = MAX_HEALTH;
    player.equipped_weapon = 0;
    player.last_monster_slain = NO_LIMIT;
    player.potion_used_this_turn = 0;
    player.last_card_played.suit = 0;
    player.last_card_played.type = 0;
    player.last_card_played.value = 0;

    room_count = 0;
    discard_count = 0;
    cards_faced = 0;
    can_skip = 1;

    for (i = 0; i < ROOM_SIZE; i++) {
        occupied[i] = 0;
    }

    dungeon_count = create_deck(dungeon);
    shuffle_deck(dungeon, dungeon_count);

    printf("\n=========================================\n");
    printf("   STARTING NEW GAME OF SCOUNDREL   \n");
    printf("=========================================\n");
    printf("(Press 'x' at any time to return to the Menu)\n\n");

    while (player.health > 0) {
        if (cards_faced == 0) {
            for (i = 0; i < ROOM_SIZE && dungeon_count > 0; i++) {
                if (!occupied[i]) {
                    room[i] = dungeon[--dungeon_count];
                    occupied[i] = 1;
                    room_count++;
                }
            }
        }

        /* Victory */
        if (room_count == 0 && dungeon_count == 0) {
            printf("\n*** CONGRATULATIONS! YOU COMPLETED THE DUNGEON! ***\n");
            final_score = player.health;
            if (player.health == MAX_HEALTH &&
                player.last_card_played.type == POTION) {
                final_score += player.last_card_played.value;
            }
            printf("Final score: %d\n\n", final_score);
            return;
        }

        printf("\n-----------------------------------------\n");
        printf("HEALTH: %d/%d | WEAPON: %d (Max Monster: %d)\n",
               player.health, MAX_HEALTH, player.equipped_weapon,
               (player.last_monster_slain == NO_LIMIT
                   ? 14 : player.last_monster_slain - 1));
        printf("Cards in Dungeon: %d | Discard: %d\n", dungeon_count, discard_count);
        printf("-----------------------------------------\n");
        printf("CURRENT ROOM:\n");
        for (i = 0; i < ROOM_SIZE; i++) {
            printf(" [%d] ", i + 1);
            if (occupied[i]) {
                print_card(room[i]);
            } else {
                printf("(empty)");
            }
            printf("\n");
        }

        printf("\nOptions:\n");
        printf(" (1-%d) Face card (slots marked as empty have no card)\n",
               ROOM_SIZE);
        if (can_skip && cards_faced == 0 && room_count == ROOM_SIZE) {
            printf(" (f) Skip room\n");
        }
        printf(" (x) Abandon and return to Menu\n");

        choice = read_input("\nYour choice: ");

        if (choice == 'x' || choice == '\0') {
            printf("\nGame abandoned.\n\n");
            return;
        }

        /* Skip room: the 4 cards go to the bottom (index 0) of the deck */
        if (choice == 'f' && can_skip && cards_faced == 0 &&
            room_count == ROOM_SIZE) {
            printf("\nYou skipped the room! The cards return to the bottom of the Dungeon.\n");
            memmove(&dungeon[room_count], &dungeon[0],
                    (size_t)dungeon_count * sizeof(struct Card));
            memcpy(&dungeon[0], room, (size_t)room_count * sizeof(struct Card));
            dungeon_count += room_count;
            for (i = 0; i < ROOM_SIZE; i++) occupied[i] = 0;
            room_count = 0;
            can_skip = 0;
            continue;
        }

        if (choice == 'f') {
            if (!can_skip) {
                printf("\nYou cannot skip two rooms in a row!\n");
            } else {
                printf("\nYou can only skip a room before facing any card.\n");
            }
            continue;
        }

        index = choice - '1';
        if (index < 0 || index >= ROOM_SIZE) {
            printf("\nInvalid option! Try again.\n");
            continue;
        }
        if (!occupied[index]) {
            printf("\nSlot %d is empty! Choose another card.\n", index + 1);
            continue;
        }

        chosen = room[index];
        player.last_card_played = chosen;

        if (chosen.type == POTION) {
            if (!player.potion_used_this_turn) {
                health_before = player.health;
                player.health += chosen.value;
                if (player.health > MAX_HEALTH) player.health = MAX_HEALTH;
                player.potion_used_this_turn = 1;
                printf("\nPotion used! Healed %d. Current health: %d\n",
                       player.health - health_before, player.health);
            } else {
                printf("\nYou already used a potion this turn! This one was discarded and healed 0. Current health: %d\n",
                       player.health);
            }
            discard_count++;
        }
        else if (chosen.type == WEAPON) {
            player.equipped_weapon = chosen.value;
            player.last_monster_slain = NO_LIMIT;
            printf("\nNew weapon equipped! Power: %d\n", player.equipped_weapon);
            discard_count++;
        }
        else { /* MONSTER */
            can_use_weapon = (player.equipped_weapon > 0) &&
                                 (chosen.value < player.last_monster_slain);
            damage_with_weapon = chosen.value - player.equipped_weapon;
            use_weapon = 0;

            if (damage_with_weapon < 0) damage_with_weapon = 0;

            if (can_use_weapon) {
                printf("\nMonster selected (%d damage).\n", chosen.value);
                printf("1. Fight Barehanded (Take %d damage)\n", chosen.value);
                printf("2. Use Equipped Weapon (Take %d damage)\n", damage_with_weapon);

                c = read_input("Choose combat method (1-2): ");
                if (c == 'x' || c == '\0') {
                    printf("\nGame abandoned.\n\n");
                    return;
                }
                if (c == '2') use_weapon = 1;
            }

            if (use_weapon) {
                player.health -= damage_with_weapon;
                player.last_monster_slain = chosen.value;
                printf("\nUsed weapon and took %d damage!\n", damage_with_weapon);
            } else {
                player.health -= chosen.value;
                printf("\nFought barehanded! Took %d damage!\n", chosen.value);
            }
            discard_count++;
        }

        occupied[index] = 0;
        room_count--;
        cards_faced++;

        if (cards_faced == 3) {
            cards_faced = 0;
            player.potion_used_this_turn = 0;
            can_skip = 1;
            printf("\nTurn complete! The remaining card stays for the next room.\n");
        }
    }

    /* Game Over */
    printf("\n=========================================\n");
    printf("               YOU DIED!                 \n");
    printf("=========================================\n");

    monster_sum = 0;
    for (i = 0; i < dungeon_count; i++) {
        if (dungeon[i].type == MONSTER) monster_sum += dungeon[i].value;
    }
    for (i = 0; i < ROOM_SIZE; i++) {
        if (occupied[i] && room[i].type == MONSTER) monster_sum += room[i].value;
    }

    final_score = player.health - monster_sum;
    printf("Final Score: %d\n\n", final_score);
}

int main(void) {
    char option;

    srand((unsigned int)time(NULL));

    for (;;) {
        printf("=========================================\n");
        printf("            SCOUNDREL - C RPG            \n");
        printf("=========================================\n");
        printf("1. Play\n");
        printf("2. Exit\n");

        option = read_input("Choose an option: ");

        if (option == '1') {
            play_game();
        } else if (option == '2' || option == '\0') {
            printf("\nExiting game... Goodbye!\n");
            break;
        } else {
            printf("\nInvalid option. Type 1 or 2.\n\n");
        }
    }

    return 0;
}
