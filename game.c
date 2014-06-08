#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "game.h"

// Dummy
struct player *has_less(){
    return NULL;
}

struct player *delete_player_by_name(char name[], struct player *head){
	struct player *node, *player = get_player_by_name(name, head);

	if(player == NULL) return head;

	if(player == head){
		head = player->prev;
		free(player);
		return head;
	}
	
	node = head;
	while(node->prev != NULL){
		if(node->prev == player) node->prev = player->prev;
		free(player);
		return head;
	}

	return head;
}

// appends a list of tiles
int append_tiles(struct domino *head, struct domino *tiles){
	struct domino *tile = head;            
    while(tile->next != NULL) tile = tile->next;// go to last tile
    tile->next = tiles;// game_last ---> player_first
    tiles->prev = tile;// game_last <--- player_first
}

// gera as peças geréricas para o dominó
struct domino *stock(){
	int i, j, n=0;
	for(i=0; i<7; i++){
		for(j=i+0; j<7; j++){
			dstock[n].id = n+1;
			dstock[n].mask[0] = i;
			dstock[n].mask[1] = j;
			dstock[n].next = NULL;
			dstock[n].prev = NULL;
			n++;
		}
	}
	return dstock;
}

// get mixed dominos tiles
// peças de dominó baralhadas (lista dinâmica)
struct domino *tiles(){
	struct domino tile;
	struct domino *domino, *dominos, *bag = NULL;
	int i, r, c=0, flags[28];

	// stock of dominos
	dominos = stock();

	// set tile flags to 0
	for(i=0; i<28; i++) flags[i] = 0;

	while(c < 28){
		// random number from [0,27]
		srand((time(NULL)+i++));
		r = rand() % 28;

		// tile already in the bag, get another random number
		if(flags[r]) continue;

		domino = &dominos[r];

		if(bag != NULL){
			bag->prev = domino;
			domino->next = bag;
		}

		bag = domino;

		flags[r] = 1;
		c++;
	}

	return bag;
}

// distribute dominos
void start(struct game *game){
	struct domino *tile;
	struct player *player;
	int i;

	player = game->players;
	while(player != NULL){
		player->tiles = game->tiles;
		player->tiles->prev = NULL;
		i=0;
		while(game->tiles != NULL && i<6){
			game->tiles = game->tiles->next;
			i++;
		}

		tile = game->tiles;
		game->tiles = game->tiles->next;
		tile->next = NULL;
		game->tiles->prev = NULL;
		player = player->prev;
	}
};

// setup new game
struct game *new_game(struct game *games){
	struct game *new_game;

	new_game = malloc(sizeof(struct game));

	if(games == NULL) {
		(*new_game).id = 1;
		(*new_game).prev = NULL;
		(*new_game).next = NULL;
	}
	else {
		games->next = new_game;
		(*new_game).id = (*games).id + 1;
		(*new_game).prev = games;
		(*new_game).next = NULL;
	}

	(*new_game).tiles = tiles();
	(*new_game).start_t = 0;
	(*new_game).end_t = 0;
	(*new_game).done = 0;
	(*new_game).mosaic = NULL;
	(*new_game).players = NULL;

	return new_game;
}

int add_player(struct player player, struct game *game){
	struct player *new_player, *aux;
	new_player = malloc(sizeof(struct player));

	if(game->players == NULL){
		new_player->id = 1;
		new_player->prev = NULL;
	}
	else {
		// no more players allowed
		if(game->players->id == 4) return 0;

		new_player->id = game->players->id + 1;
		new_player->prev = game->players;
		new_player->wins = 0;
	}

	strcpy(new_player->name, player.name);
	strcpy(new_player->fifo, player.fifo);
	new_player->pid = player.pid;
	new_player->tiles = NULL;

	game->players = new_player;

	return 1;
}

struct player *new_player(char name[], struct player *players){
	struct player *new_player, *aux;
	new_player = malloc(sizeof(struct player));

	if(players == NULL){
		new_player->id = 1;
		new_player->prev = NULL;
	}
	else {
		new_player->id = players->id + 1;
		new_player->prev = players;
	}

	strcpy(new_player->name, name);
	new_player->tiles = NULL;
	time(&new_player->login_t);

	players = new_player;

	return players;
}

// gets player from a list of players
struct player *get_player_by_id(int id, struct player *players){
	struct player *player = players;
    while(player != NULL){
		if(player->id == id) return player;
		player = player->prev;
	}
	return NULL;
}

struct player *get_player_by_name(char name[], struct player *players){
	struct player *aux_player = players;

//printf("getting player %s\n", name);

    while(aux_player != NULL){
        if(strcmp(name, aux_player->name) == 0) {
//printf("found player %s\n", aux_player->name);
            return aux_player;
        }
        aux_player = aux_player->prev;
    }

    return NULL;
}

// gets a tile from a list of tiles
struct domino *get_tile_by_id(int id, struct domino *head){
	struct domino *aux, *tile;
	tile = head;
	while(tile != NULL){
		if(tile->id == id) return tile;
		tile = tile->next;
	}
	return NULL;
}

// conta itens numa lista dominos
int count_tiles(struct domino *tiles){
	struct domino *tile = tiles;
	int count = 0;

	while(tile != NULL) {
		count++;
		tile = tile->next;
	}
	return count;
}

// adds a tile to the player's tiles
void add_tile(struct domino *tile, struct player *player){
	tile->next = player->tiles;
	player->tiles->prev = tile;
	player->tiles = tile;
}

// removes a tile from player's tiles
struct domino *remove_tile(int id, struct player *player){
	struct domino *tile, *prev, *next;

	// no tiles to remove from
	if(player->tiles == NULL) return NULL;

	tile = get_tile_by_id(id, player->tiles);

	// first tile
	if(tile->prev == NULL){
		// only tile
		if(tile->next == NULL){
			player->tiles = NULL;
		}
		else {
			player->tiles = tile->next;
			player->tiles->prev = NULL;
		}
	}
	// last tile
	else if(tile->next == NULL) {
		tile->prev->next = NULL;
	}
	// inbetween tile
	else {
		tile->prev->next = tile->next;
		tile->next->prev = tile->prev;
	}

	tile->next = NULL;
	tile->prev = NULL;
	return tile;
}

// adds a tile to the player tiles from the (game) stock
struct domino *give_tile_by_id(int id, struct game *game_head) {
	struct domino *tile = game_head->tiles, *head;
	struct player *player;

	// game is out of tiles;
	if(tile == NULL) return NULL;

	// move tiles head
	if(tile->next != NULL){
		head = tile->next;
		head->prev = NULL;
		game_head->tiles = head;
	}
	// has only one tile
	else game_head->tiles = NULL;

	// add tile to player
	player = game_head->players;
	while(player != NULL){
		if(player->id == id){
			tile->prev = NULL;
			tile->next = NULL;
			add_tile(tile, player);
			break;
		}
		player = player->prev;
	}
	return tile;
}

// adiciona uma peça de domino ao mosaico
struct domino *place_tile(struct domino *tile, struct game *head_game){
	struct domino *mosaic_tile, *player_tile;
	int i, j, k, mask;

	if(tile == NULL) return NULL;

	// nenhuma peça no mosaico
	if(head_game->mosaic == NULL) {
		tile->next = NULL;
		tile->prev = NULL;
		head_game->mosaic = tile;
		return head_game->mosaic;
	}

	// head tile
	mosaic_tile = head_game->mosaic;
	player_tile = tile;

	// o dominó encaixa à esquerda
	mask = mosaic_tile->mask[0];
	if(player_tile->mask[0] == mask || player_tile->mask[1] == mask){
		// encaixa o dominó no mosaico
		if(player_tile->mask[1] != mask){
			player_tile->mask[0] = player_tile->mask[1];
			player_tile->mask[1] = mask;
		}
		// ligação ao mosaico
		player_tile->next = mosaic_tile;//proximo
		mosaic_tile->prev = player_tile;//anterior
		// nó (head/ponta) principal do mosaico
		head_game->mosaic = player_tile;
		return head_game->mosaic;
	}

	// o mosaico tem apenas um dominó
	if(mosaic_tile->next == NULL) {
		// encaixa à direita
		mask = mosaic_tile->mask[1];
		if(player_tile->mask[0] == mask || player_tile->mask[1] == mask){
			// encaixa o dominó no mosaico
			if(player_tile->mask[0] != mask){
				player_tile->mask[1] = player_tile->mask[0];
				player_tile->mask[0] = mask;
			}
			// ligação ao mosaico
			player_tile->prev = mosaic_tile;//proximo
			mosaic_tile->next = player_tile;//anterior
			return head_game->mosaic;
		}
	}

	// o mosaico tem mais que uma peça de domino: deslocar até ao última peça
	while(mosaic_tile->next != NULL) mosaic_tile = mosaic_tile->next;

	// encaixa à direita
	mask = mosaic_tile->mask[1];
	if(player_tile->mask[0] == mask || player_tile->mask[1] == mask){
		// encaixa o dominó no mosaico
		if(player_tile->mask[0] != mask){
			player_tile->mask[1] = player_tile->mask[0];
			player_tile->mask[0] = mask;
		}
		// ligação ao mosaico
		player_tile->prev = mosaic_tile;//proximo
		mosaic_tile->next = player_tile;//anterior
		return head_game->mosaic;
	}

	return NULL;
}

// gets the mosaic's free end masks
void get_ends(int ends[], struct domino *head){
	struct domino *tile;

	// mosaico vazio
	if(head == NULL) {
		ends[0] = -1;
		ends[1] = -1;
		return;
	}

	ends[0] = head->mask[0];

	// mosaico com apenas um dominó
	if(head->next == NULL){
		ends[1] = head->mask[1];
		return;
	}

	// deslocar até à última peça
	tile = head->next;
	while(tile->next != NULL) tile = tile->next;

	ends[1] = tile->mask[1];
	return;
}

// tests if a tile fits in the mosaic
int validate_tile(int mask[], struct domino *tile){
	int i, j;
	for(i=0; i<2; i++)
		for(j=0; j<2; j++)
			if(tile->mask[i] == mask[j]) return 1;
	return 0;
}

// tests if a tile (that fits in the mosaic) exists
int tile_exists(int mask[], struct domino *tiles){
	struct domino *tile = tiles;
	while(tile != NULL){
		if(validate_tile(mask, tile)) return 1;
		tile = tile->next;
	}
	return 0;
}

// tests the player's status: 1/0/-1, has/needs/out-of tiles
int player_status(int id, struct game *game){
	struct player *player;
	struct domino *tile;
	int i, j, ends[2];

	get_ends(ends, game->mosaic);
	player = get_player_by_id(id, game->players);
	if(player->tiles == NULL) return -1;

	return tile_exists(ends, player->tiles);
}

// gets winner (winner has less tiles)
struct player *get_winner(struct game *game){
	struct player *winner, *dummy = (*game).players;
	int n, count;

	winner = dummy;
	count = count_tiles(dummy->tiles);

	dummy = dummy->prev;
	while(dummy != NULL) {
		n = count_tiles(dummy->tiles);
		if(n == count) return NULL;
		else if(n < count) winner = dummy;
		dummy = dummy->prev;
	}

	return winner;
}

// adiciona uma peça de domino ao mosaico
struct domino *place_tile2(struct domino *tile, struct game *head_game, int pos){
	struct domino *mosaic_tile, *player_tile;
	int i, j, k, mask;

	if(tile == NULL) return NULL;

	// nenhuma peça no mosaico
	if(head_game->mosaic == NULL) {
		tile->next = NULL;
		tile->prev = NULL;
		head_game->mosaic = tile;
		return head_game->mosaic;
	}

	// head tile
	mosaic_tile = head_game->mosaic;
	player_tile = tile;

//********
if(!pos){
	// o dominó encaixa à esquerda
	mask = mosaic_tile->mask[0];
	if(player_tile->mask[0] == mask || player_tile->mask[1] == mask){
		// encaixa o dominó no mosaico
		if(player_tile->mask[1] != mask){
			player_tile->mask[0] = player_tile->mask[1];
			player_tile->mask[1] = mask;
		}
		// ligação ao mosaico
		player_tile->next = mosaic_tile;//proximo
		mosaic_tile->prev = player_tile;//anterior
		// nó (head/ponta) principal do mosaico
		head_game->mosaic = player_tile;
		return head_game->mosaic;
	}
}
//*

	// o mosaico tem apenas um dominó
	if(mosaic_tile->next == NULL) {
		// encaixa à direita
		mask = mosaic_tile->mask[1];
		if(player_tile->mask[0] == mask || player_tile->mask[1] == mask){
			// encaixa o dominó no mosaico
			if(player_tile->mask[0] != mask){
				player_tile->mask[1] = player_tile->mask[0];
				player_tile->mask[0] = mask;
			}
			// ligação ao mosaico
			player_tile->prev = mosaic_tile;//proximo
			mosaic_tile->next = player_tile;//anterior
			return head_game->mosaic;
		}
	}

	// o mosaico tem mais que uma peça de domino: deslocar até ao última peça
	while(mosaic_tile->next != NULL) mosaic_tile = mosaic_tile->next;

	// encaixa à direita
	mask = mosaic_tile->mask[1];
	if(player_tile->mask[0] == mask || player_tile->mask[1] == mask){
		// encaixa o dominó no mosaico
		if(player_tile->mask[0] != mask){
			player_tile->mask[1] = player_tile->mask[0];
			player_tile->mask[0] = mask;
		}
		// ligação ao mosaico
		player_tile->prev = mosaic_tile;//proximo
		mosaic_tile->next = player_tile;//anterior
		return head_game->mosaic;
	}

	return NULL;
}
