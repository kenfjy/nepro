#include "stock.h"

struct gamePlayer {
	uint32_t key;
	int 	budget[PLAY_YEARS*12];
	int 	tickets[COMPANY_NUM];
	int 	purchase[COMPANY_NUM];
	int 	sale[COMPANY_NUM];
	int 	count;
};

struct company {
	int 	price;
};

int makesock(char *service);
uint32_t randomHash();
uint32_t status(struct gamePlayer player, struct company* companies, uint32_t arg_0, uint32_t arg_1, uint32_t arg_2, uint32_t arg_3, int turn);


int main(int argc, char *argv[]) {
	if (argv[1] == NULL) {
		puts("missing parameters");
		return 0;
	}
	char service[100];
	int fd[USER_NUM + 2];
	fd_set fdsets;
	uint32_t request[4];
	uint32_t response[22];
	int i,j,k,n;
	int error;

	strcpy(service, argv[1]);
	
	fd[0] = makesock(service);

	if (fd[0] < 0) {
		printf("unable to create socket, connection, or binding %i\n", fd[0]);
		return 0;
	}
	printf("binding success! sock: %i\n", fd[0]);
	
	error = listen(fd[0], USER_NUM);
	if (error < 0) {
		fprintf(stderr,"unable to listen error: %s(%d)\n", gai_strerror(error), error);
		return 0;
	}
	printf("listen success!\n");

	for(i=1;i<USER_NUM+2;i++) {
		fd[i] = -1;
	}
	//int msg_length, n;

	int sockCount = 1;
	srand((unsigned)time(NULL));
	struct timeval tv;
	time_t timer;
	struct sockaddr_storage ss;
	timer = time(NULL);
	socklen_t sl;
	sl = sizeof(ss);

	while(sockCount <= USER_NUM)
	{
		FD_ZERO(&fdsets);
		tv.tv_sec = 0;
		tv.tv_usec = 10;
		for (i=0; i<sockCount; i++) {
			if (fd[i] != (-1)) {
				FD_SET(fd[i], &fdsets);
			}
		}

		if ((n = select(FD_SETSIZE, &fdsets, NULL, NULL, &tv)) == -1) {
			printf("error in select");
		}

		if (FD_ISSET(fd[0], &fdsets) != 0) {
			printf("adding new user. current user count: %d\n", sockCount);
			fd[sockCount] = accept(fd[0], (struct sockaddr *)&ss, &sl);
			if (fd[sockCount] < 0) {
				fprintf(stderr,"socket error: %s(%d)\n", gai_strerror(fd[sockCount]), fd[sockCount]);
			}
			printf("new user accepted: %d\n", sockCount);
			sockCount++;
		}
	}

	timer = time(NULL);
	int turn = 0;

	struct gamePlayer players[USER_NUM];
	struct company companies[COMPANY_NUM];

	for (i=0;i<COMPANY_NUM;i++) {
		companies[i].price = 50 + rand()%100;
	}

	for (i=0;i<USER_NUM;i++) {
		players[i].budget[0] = 10000;
		players[i].count = 0;
		players[i].key = randomHash();
		response[0] = players[i].key;
		response[1] = 0x00000000;
		for (k=0; k<COMPANY_NUM; k++) {
			players[i].tickets[k] = 0;
			players[i].purchase[k] = 0;
			players[i].sale[k] = 0;
			response[2*k+2] = k;
			response[2*k+3] = companies[k].price;
		}
		for (k=0; k<22; k++) {
			uint32_t tmp = htonl(response[k]);
			write(fd[i+1],&tmp,sizeof(tmp));
		}
	}

	for(;;)
	{	
		tv.tv_sec = 0;
		tv.tv_usec = 10;

		if (difftime(time(NULL), timer) > 1) {
			int company_p[COMPANY_NUM];
			for(i=0; i<COMPANY_NUM; i++) {
				company_p[i] = 0;
			}
			printf("15 seconds has passed\n");
			for (i=1; i<USER_NUM+1; i++) {
				if (fd[i] != (-1)) {
					if (FD_ISSET(fd[i], &fdsets)) {
						for (k=0; k<4; k++) {
							error = read(fd[i], &request[k], sizeof(request[k]));
							request[k] = ntohl(request[k]);
						}
						response[0] = request[0];
						for (k=0; k<COMPANY_NUM; k++) {
							response[2*k+2] = k;
							response[2*k+3] = 0x00000000;
						}

						response[1] = status(players[i-1], companies, request[0], request[1], request[2], request[3], turn);
						//printf("response code for user %d is %x\n", i-1, response[1]);

						if (response[1] == ACCEPT) {
							if(request[1] == PURCHASE) {
								players[i-1].purchase[request[2]] += request[3];
								printf("purchasing company id: %d (price: %d) quantity: %d\n",request[2], companies[request[2]].price, request[3]);
							} else {
								players[i-1].sale[request[2]] += request[3];
								printf("selling company id: %d (price: %d) quantity: %d\n",request[2], companies[request[2]].price, request[3]);
							}
							response[request[2]*2+3] = request[3];
						} else if (response[1] == ERR_PUR) {
							response[request[2]*2+3] = request[3];
							printf("!ERROR in purchasing company id: %d (price: %d) quantity: %d\nYour current budget is %d\n\n",request[2], companies[request[2]].price, request[3], players[i-1].budget[turn]);
						} else if (response[1] == ERR_SAL) {
							response[request[2]*2+3] = request[3];
							printf("!ERROR in selling company id: %d (price: %d) quantity: %d\nYou only have %d tickets\n\n",request[2], companies[request[2]].price, request[3], players[i-1].tickets[request[2]]);
						}

						for (k=0; k<22; k++) {
							uint32_t tmp = htonl(response[k]);
							write(fd[i],&tmp,sizeof(tmp));
						}
					}
				} else {
					printf("fd[%d] is -1\n", i);
				}
				k=0;
				printf("\n==== User ID: %d on Turn %d ====\n", i-1, turn);
				for(j=0; j<COMPANY_NUM; j++) {
					printf("company id: %d\npurchasing %d tickets, selling %d tickets\n BEFORE: %d tickets left", j, players[i-1].purchase[j], players[i-1].sale[j], players[i-1].tickets[j]);
					k += (players[i-1].purchase[j] - players[i-1].sale[j]) * companies[j].price;
					//purchase 
					players[i-1].tickets[j] += players[i-1].purchase[j];
					company_p[j] += players[i-1].purchase[j];
					players[i-1].purchase[j] = 0;
					//sale
					players[i-1].tickets[j] -= players[i-1].sale[j];
					company_p[j] -= players[i-1].sale[j];
					players[i-1].sale[j] = 0;
					printf(" AFTER: %d tickets left\n", players[i-1].tickets[j]);
				}
				printf("left over budget BEFORE: %d and k: %d\n", players[i-1].budget[turn], k);
				players[i-1].budget[turn] -= k;
				//printf("left over budget %d\n======\n", players[i-1].budget[turn]);
				players[i-1].count = 0;
			}
			int addition = 0;
			for (k=0; k<COMPANY_NUM; k++) {
				addition += company_p[k];
			}
			if (addition == 0) {
				for (k=0; k<COMPANY_NUM; k++) {
					companies[k].price = companies[k].price - 5 + rand()%11;
				}
			} else {
				for (k=0; k<COMPANY_NUM; k++) {
					companies[k].price = companies[k].price - 5 + rand()%11 + companies[k].price/10*company_p[k]/addition;
				}
			}
			turn++;
			if (turn != 12*PLAY_YEARS) {
				for(i=0; i<USER_NUM; i++) {
					players[i].budget[turn] = players[i].budget[turn-1];
					players[i].key = randomHash();
					response[0] = players[i].key;
					response[1] = 0x00000000;
					for (k=0; k<COMPANY_NUM; k++) {
						response[2*k+2] = k;
						response[2*k+3] = companies[k].price;
					}
					for (k=0; k<22; k++) {
						uint32_t tmp = htonl(response[k]);
						write(fd[i+1], &tmp, sizeof(tmp));
					}
				}
				printf("=== TURN: %d ===\n", turn);
				for (k=0; k<COMPANY_NUM; k++) {
					response[2*k+2] = k;
					response[2*k+3] = companies[k].price;
					printf("%d : %i\n", k, companies[k].price);
				}
				printf("======\n");
			}
			timer = time(NULL);
		}
		if (turn == 12*PLAY_YEARS) {
			response[0] = 0x00000000;
			response[1] = 0x00000002;
			int rank[USER_NUM];
			int list[USER_NUM];
			int u;
			for (i=0; i<USER_NUM; i++) {
				list[i] = players[i].budget[turn-1];
			}
			for (i=0; i<USER_NUM; i++) {
				u = 0;
				for (j=0; j<USER_NUM; j++) {
					if (list[j] > list[u]) {
						u = j;
					}
				}
				list[u] = 0;
				rank[i] = u;
			}
			for (i=0; i<USER_NUM; i++) {
				j = 0;
				while(i != rank[j]) {
					j++;
				}
				response[2] = j+1;
				response[3] = players[i].budget[turn-1];
				for (k=0; k<4; k++) {
					uint32_t tmp = htonl(response[k]);
					write(fd[i+1],&tmp,sizeof(tmp));
				}
			}
			break;
		}
		FD_ZERO(&fdsets);
		for (i=0; i<USER_NUM+1; i++) {
			if (fd[i] != (-1)) {
				FD_SET(fd[i], &fdsets);
			}
		}

		if ((n = select(FD_SETSIZE, &fdsets, NULL, NULL, &tv)) == -1) {
			printf("error in select");
		}

		if (FD_ISSET(fd[0], &fdsets) != 0) {
			fd[USER_NUM+1] = accept(fd[0], (struct sockaddr *)&ss, &sl);
			printf("too many user has tried to connect\n");
			close(fd[USER_NUM+1]);
		}

		for (i=1; i<USER_NUM+1; i++) {
			if (fd[i] != (-1)) {
				if (FD_ISSET(fd[i], &fdsets)) {
					//printf("fd[%d] ready for read\n", i);
					for (k=0; k<4; k++) {
						read(fd[i], &request[k], sizeof(request[k]));
						request[k] = ntohl(request[k]);
					}

					response[0] = request[0];
					for (k=0; k<COMPANY_NUM; k++) {
						response[2*k+2] = k;
						response[2*k+3] = 0x00000000;
					}

					response[1] = status(players[i-1], companies, request[0], request[1], request[2], request[3], turn);
					//printf("response code for user %d is %x\n", i-1, response[1]);

					if (response[1] == ACCEPT) {
						if(request[1] == PURCHASE) {
							players[i-1].purchase[request[2]] += request[3];
							printf("purchasing company id: %d (price: %d) quantity: %d\n",request[2], companies[request[2]].price, request[3]);
						} else {
							players[i-1].sale[request[2]] += request[3];
							printf("selling company id: %d (price: %d) quantity: %d\n",request[2], companies[request[2]].price, request[3]);
						}
						response[request[2]*2+3] = request[3];
					} else if (response[1] == ERR_PUR) {
						response[request[2]*2+3] = request[3];
						printf("!ERROR in purchasing company id: %d (price: %d) quantity: %d\nYour current budget is %d\n\n",request[2], companies[request[2]].price, request[3], players[i-1].budget[turn]);
					} else if (response[1] == ERR_SAL) {
						response[request[2]*2+3] = request[3];
						printf("!ERROR in selling company id: %d (price: %d) quantity: %d\nYou only have %d tickets\n\n",request[2], companies[request[2]].price, request[3], players[i-1].tickets[request[2]]);
					}

					for (k=0; k<22; k++) {
						uint32_t tmp = htonl(response[k]);
						write(fd[i],&tmp, sizeof(tmp));
					}

					players[i-1].count++;
				}
			} else {
				printf("fd[%d] is -1\n", i);
			}
		}						
	}
	printf("connection finished!\n");
	return 0;
}

uint32_t status(struct gamePlayer player, struct company* companies, uint32_t arg_0, uint32_t arg_1, uint32_t arg_2, uint32_t arg_3, int turn) {
	if (player.key != arg_0) {
		return ERR_KEY;
	} else if(arg_1 != PURCHASE && arg_1 != SALE) {
		return ERR_CODE;
	} else if(player.count >= 5) {
		return ERR_REQ;
	} else if(arg_2 < 0 || arg_2 > COMPANY_NUM -1) {
		return ERR_ID;
	} else {
		int k,j;
		if(arg_1 == PURCHASE) {
			//printf("player budget: %u ticket he wants to buy: %u quantity: %u its cost: %u", player.budget[turn], arg_2, arg_3, companies[arg_2].price);
			k = arg_3*companies[arg_2].price;
			for(j=0;j<COMPANY_NUM;j++) {
				k += (player.purchase[j] - player.sale[j]) * companies[j].price;
			}
			if(player.budget[turn] <= k) {
				return ERR_PUR;
			} else {
				return ACCEPT;
			}
		} else {
			//printf("player budget: %u ticket he wants to sell: %u quantity: %u its cost: %u", player.budget[turn], arg_2, arg_3, companies[arg_2].price);
			if(arg_3 > player.tickets[arg_2]+player.purchase[arg_2]-player.sale[arg_2]) {
				return ERR_SAL;
			} else {
				return ACCEPT;
			}
		}
	}
}

int makesock(char *service) {
	int error, fd;
	struct addrinfo *ai, *ai2, hints;

	memset((void *)&hints, 0, sizeof(struct addrinfo));
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = PF_UNSPEC;
	hints.ai_flags = AI_PASSIVE;

	error = getaddrinfo(NULL, service, &hints, &ai);

	if (error != 0) {
		fprintf(stderr,"getaddrinfo() error: %s(%d)\n", gai_strerror(error), error);
		exit(-1);
	}
	for (ai2 = ai; ai2; ai2 = ai2->ai_next) {
		fd = socket(ai2->ai_family, ai2->ai_socktype, ai2->ai_protocol);
		if (fd >= 0) {
			error = bind(fd, ai2->ai_addr, ai2->ai_addrlen);
			if (error >= 0) {
				freeaddrinfo(ai);
				return fd;
			} else {
				fprintf(stderr,"bind() error: %s(%d)\n", gai_strerror(error), error);
				close(fd);
			}
		} else {
			fprintf(stderr,"socket error: %s(%d)\n", gai_strerror(error), error);
			close(fd);
		}
	}
	return -1;
}

uint32_t randomHash() {
	int d = UINT32_MAX / RAND_MAX;
	int m = UINT32_MAX % RAND_MAX + 1;
	uint32_t number = (uint32_t)(rand()*d + rand()%m);
	return number;
}
