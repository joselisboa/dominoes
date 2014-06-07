#include <stdio.h>

#ifdef _WIN32
#include "zelib_win32.h"//#include <Windows.h>//Sleep(1*1000);
#else
#include "zelib.h"//#include <unistd.h>//sleep(1);
#endif

#include "game.h"

void ui();
int prompt_player(struct player *player, int mask[]);
struct game *play(struct game *game);
int game_status(struct game *game);
void show_tiles(struct domino *tiles);

int main(int varc, char *charv[]){
	//simulation
	ui();

	exit(0);
}

// ui (dummie play)
void ui(){
	struct game *game = NULL, *games = NULL;
	struct domino *dominos, *aux_tiles, domino;
	struct player player, *aux_player, *winner;
	int i, option = 0;

	puts("[1] add player");
	puts("[2] list players");
	puts("[3] start game");
	puts("[4] quit");

	game = new_game(games);

	// game cicle
	while(option != 4){
		printf(">");
		scanf(" %d", &option);
		fflush(stdin);

		switch(option){
			case 1:
				puts("ADD PLAYER");

				printf("name: ");
				scanf(" %s", player.name);
				fflush(stdin);

				if(!add_player(player, game)) puts("NO MORE PLAYERS ALLOWED");
				break;

			case 2:
				puts("LIST PLAYERS");
				if(game->players == NULL){
					_puts("There are no Players!", 12);
					break;
				}
				aux_player = game->players;
				while(aux_player != NULL){
					printf("%d %s: ", aux_player->id, aux_player->name);
					show_tiles(aux_player->tiles);
					aux_player = aux_player->prev;
				}
				break;

			case 3:
				puts("START DOMINOS");
				if(game->players == NULL || game->players->id < 2) {
					_puts("Not Enough Players To Start!", 12);
					break;
				}

				start(game);
				game = play(game);

				//TODO Result
				winner = get_winner(game);
				_printf(13, "%s WON!\n", winner->name);

				//new game
				game = new_game(games);
				break;
		}
	}
}

// pede peça ao jogador
int prompt_player(struct player *player, int mask[]){
	struct domino *tile;
	int id;

	while(1){
		printf("id: ");
		scanf(" %d", &id);
		fflush(stdin);

		// get tile (careful! contains prev/next nodes)
		tile = get_tile_by_id(id, player->tiles);

		// peça de dominó não é válida, pede outra
		if(tile == NULL) {
			_puts("NULL", 4);
			continue;
		}

		// mosaico está vazio, sai do ciclo
		if(mask[0] == -1 || validate_tile(mask, tile)) return id;
		else _puts("o domino NAO encaixa no mosaico", 4);
	}
}

// stand alone dominoes test
struct game *play(struct game *game){
	struct domino *tile;
	struct player *player;
	int i, id, mask[2] = {-1,-1}, status;

	// player's turn cicle
	player = game->players;
	while(player != NULL){
		// mostra jogador/peças
		_printf(9 + player->id*2, "%s\n", player->name);
		show_tiles(player->tiles);

		// ensures the player has a tile to play
		// ciclo de falta de peças (o mosaico tem que ter peças)
		while(!tile_exists(mask, player->tiles) && game->mosaic != NULL){
			_puts("o jogador precisa de um domino", 8);

			// peça de domino da casa (saco do jogo)
			if((tile = give_tile_by_id(player->id, game)) == NULL){
				_puts("acabaram os dominos do jogo", 4);
				// termina jogo
				return game;
			}
			else _printf(8,"jogador %s recebe domino %d:[%d,%d]\n",
				player->name, tile->id, tile->mask[0], tile->mask[1]);

			// se a peça é válida, sai do ciclo while
			if(validate_tile(mask, tile)) break;
			else _puts("TILE NOT OK", 4);//pause(3);
		}

		// a partir deste ponto, o jogador tem uma peça válida para jogar

		// prompt player for a tile
		id = prompt_player(player, mask);

		// remove a peça ao jogador
		tile = remove_tile(id, player);
		// coloca a peça no mosaico
		place_tile(tile, game);
		// actualiza mascaras livres do mosaico
		get_ends(mask, game->mosaic);
		//TODO verificar estado do jogo
		game_status(game);

		if(!count_tiles(player->tiles)){
			_printf(4, "player%d (%s) placed last tile\n", player->id, player->name);
			break;
		}

		// próximo jogador
		if(player->prev != NULL) player = player->prev;
		else player = game->players;
	}

	// fim do jogo
	return game;
}

// game status
int game_status(struct game *game){
	printf("games has %d tiles\n", count_tiles(game->tiles));
	printf("mosaic has %d tiles\n", count_tiles(game->mosaic));
	puts("Mosaico:");
	show_tiles(game->mosaic);
}

// displays the tiles on the screen
void show_tiles(struct domino *tiles){
	while(tiles != NULL){
		_printf(13, "%3d:[%d,%d]\n", tiles->id, tiles->mask[0], tiles->mask[1]);
		tiles = tiles->next;
	}
}
