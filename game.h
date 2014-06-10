#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

typedef struct domino dom, *pdom;

// domino tile (peça de dominó)
struct domino {
	int id;
	int mask[2];
	struct domino *next;
	struct domino *prev;
};

struct domino dstock[28];

struct game {
	int id;
    time_t start_t;
    int t;
    time_t end_t;
    time_t create_t;
    char name[32];
	struct domino *tiles;// peças
	struct domino *mosaic;// mosaico
	struct player *winner;// vencedor
	struct player *players;// jogadores
	int done;
	struct game *next;// proximo jogo
	struct game *prev;// jogo anterior
};

struct player {
	int id;
	time_t login_t;
	char name[16];
	int wins;
	int pid;
	//int ip;
	char fifo[32];
	struct domino *tiles;
	struct player *prev;
};

struct player *has_less();

struct domino *stock();

struct domino *tiles();

void start(struct game *game);

struct game *new_game(struct game *games);

struct player *new_player(char name[], struct player *players);

int add_player(struct player player, struct game *game);

struct player *delete_player_by_name(char name[], struct player *head);

int append_tiles(struct domino *head, struct domino *tiles);

//-----------------------------------------------------------------------------
// gets player (by id) from a list of players
struct player *get_player_by_id(int id, struct player *players);

// gets player (by name) from a list of players
struct player *get_player_by_name(char name[], struct player *players);

// gets a tile from a list of tiles
struct domino *get_tile_by_id(int id, struct domino *tiles);

// conta itens numa lista dominos
int count_tiles(struct domino *tiles);

// adds a tile to the player's tiles
void add_tile(struct domino *tile, struct player *player);

// removes a tile from player's tiles
struct domino *remove_tile(int id, struct player *player);

// adds a tile to the player tiles from the (game) stock
struct domino *give_tile_by_id(int id, struct game *game);

// adiciona uma peça de domino ao mosaico
struct domino *place_tile(struct domino *tile, struct game *head);

// gets the mosaic's free end masks
void get_ends(int ends[], struct domino *tiles);

// tests if a tile fits in the mosaic
int validate_tile(int mask[], struct domino *tile);

// tests if a tile (that fits in the mosaic) exists
int tile_exists(int mask[], struct domino *tiles);

// tests the player's status: 1/0/-1, has/needs/out-of tiles
int player_status(int id, struct game *game);

// gets winner (winner has less tiles)
struct player *get_winner(struct game *game);

// adiciona uma peça de domino ao mosaico
struct domino *place_tile2(struct domino *tile, struct game *head_game, int pos);
