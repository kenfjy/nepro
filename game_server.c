#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <sys/select.h>
#include <limits.h>

#define INTSIZE 10

#define BUFSIZE 1024
#define USER_NUM 4
#define COMPANY_NUM 10
#define PLAY_YEARS 5

#define ACCEPT 	0x00000001
#define PURCHASE 	0x00000100
#define SALE 		0x00000101
#define ERR_CODE 	0x00000400
#define ERR_KEY 	0x00000401
#define ERR_REQ 	0x00000402
#define ERR_ID 	0x00000403
#define ERR_PUR 	0x00000404
#define ERR_SAL 	0x00000405

int makesock(char *service);
uint32_t randomHash();

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

int main(int argc, char *argv[]) {
	if (argv[1] == NULL) {
		puts("missing parameters");
		return 0;
	}
	char service[100];
	int fd[USER_NUM + 2];
	fd_set fdsets;
	//uint32_t block[USER_NUM+1];
	char userstream[BUFSIZE];
	uint32_t request[4];
	uint32_t response[22];
	char message[BUFSIZE];
	int i,j,k;
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

	fd[1] = -1;
	//int msg_length, n;
	int n;
	struct sockaddr_storage ss;
	socklen_t sl;
	sl = sizeof(ss);

	int sockCount = 1;

	time_t timer;
	timer = time(NULL);

	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 10;

	srand((unsigned)time(NULL));

	while(sockCount <= USER_NUM)
	{
		FD_ZERO(&fdsets);
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
			strcpy(message, "you have joined the game!\n");
			write(fd[sockCount], (const void *)message, strlen(message));
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
		for (j=0; j<COMPANY_NUM; j++) {
			players[i].tickets[j] = 0;
		}
		players[i].key = randomHash();
		printf("players : %d\n", players[i].key);
		response[0] = players[i].key;
		response[1] = 0x00000000;
		for (k=0; k<COMPANY_NUM; k++) {
			response[2*k+2] = k;
			response[2*k+3] = companies[k].price;
		}
		for (k=0; k<22; k++) {
			//printf("randomHash() create the next number: %u\n", block[i]);
			sprintf(userstream, "%x", htonl(response[k]));
			//printf("this is HEX!!! %s\n", userstream);
			write(fd[i], userstream, INTSIZE);
		}
	}

	for(;;)
	{	
		srand((unsigned)time(NULL));
		if (difftime(time(NULL), timer) > 15) {
			printf("15seconds has passed\n");
			for (i=1; i<USER_NUM+1; i++) {
				if (fd[i] != (-1)) {
					if (FD_ISSET(fd[i], &fdsets)) {

						/* below this part, it is mainly the same */

						int k;
						for (k=0; k<4; k++) {
							read(fd[i], userstream, INTSIZE);
							request[k] = ntohl(strtol(userstream, NULL, 16));
						}
						response[0] = request[0];
						for (k=0; k<COMPANY_NUM; k++) {
							response[2*k+2] = k;
							response[2*k+3] = 0x00000000;
						}
						response[request[2]*2+2] = request[3];

						if (players[i].key != request[0]) {
							response[1] = ERR_KEY;
						} else if(request[1] != PURCHASE && request[1] != SALE) {
							response[1] = ERR_CODE;
						} else if(players[i].count >=5) {
							response[1] = ERR_REQ;
						} else if(request[2] < 0 || request[2] > COMPANY_NUM-1) {
							response[1] = ERR_ID;
						} else {
							k=0;
							for(j=0;j<COMPANY_NUM;j++) {
								k += (players[i].purchase[j] - players[i].sale[j]) * companies[j].price;
							}
							if(request[1] == PURCHASE) {
								k += request[3]*companies[request[2]].price;
							} else {
								k -= request[3]*companies[request[2]].price;
							}
							if(players[i].budget[turn] <= k) {
								response[1] = ERR_PUR;
							} else {
								response[1] = ACCEPT;
								if(request[1] == PURCHASE) {
									players[i].purchase[j] += request[3];
								} else {
									players[i].sale[j] += request[3];
								}
							}
						}
							
						for (k=0; k<22; k++) {
							//printf("randomHash() create the next number: %u\n", block[i]);
							sprintf(userstream, "%x", htonl(response[k]));
							//printf("this is HEX!!! %s\n", userstream);
							write(fd[i], userstream, INTSIZE);
						}

						/* until here */

/* OMIT THIS PART later
						if ((msg_length = read(fd[i], userstream, sizeof(userstream))) == -1) {
							printf("fd[%d] read error\n", i);
						} else if (msg_length > 0) {
							block[i] = ntohl(atoi(userstream));
							printf("fd[%d] input: %s\n", i, userstream);
							for (k=0; k<22; k++) {
								block[i] = randomHash();
								//printf("randomHash() create the next number: %u\n", block[i]);
								sprintf(userstream, "%x", htonl(block[i]));
								//printf("this is HEX!!! %s\n", userstream);
								write(fd[i],userstream, strlen(userstream));
							}
						} else {
							printf("fd[%d] closed\n", i);
							close(fd[i]);
							fd[i] = -1;
						}
						*/
					}
				} else {
					printf("fd[%d] is -1\n", i);
				}
				k=0;
				for(j=0; j<COMPANY_NUM; j++) {
					k += (players[i].purchase[j] - players[i].sale[j]) * companies[j].price;
					players[i].tickets[j] += players[i].purchase[j];
					players[i].tickets[j] -= players[i].sale[j];
					players[i].purchase[j] = 0;
					players[i].sale[j] = 0;
				}
				players[i].budget[turn] -= k;
				players[i].count = 0;
			}
			/* some functions to change the price of company tickets */
			timer = time(NULL);
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
						//printf("randomHash() create the next number: %u\n", block[i]);
						sprintf(userstream, "%x", htonl(response[k]));
						//printf("this is HEX!!! %s\n", userstream);
						write(fd[i], userstream, INTSIZE);
					}
				}
			}

			turn++;
		}
		if (turn == 12*PLAY_YEARS) {
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
		} else {
			//printf("select returns: %d\n", n);
		}

		if (FD_ISSET(fd[0], &fdsets) != 0) {
			fd[USER_NUM+1] = accept(fd[0], (struct sockaddr *)&ss, &sl);
			printf("too many user has tried to connect\n");
			strcpy(message, "the server already has four players\n");
			write(fd[USER_NUM+1], message, sizeof(message));
			close(fd[USER_NUM+1]);
		}

		for (i=1; i<USER_NUM+1; i++) {
			if (fd[i] != (-1)) {
				if (FD_ISSET(fd[i], &fdsets)) {
					printf("fd[%d] ready for read\n", i);
					int k;
					for (k=0; k<4; k++) {
						read(fd[i], userstream, INTSIZE);
						request[k] = ntohl(strtol(userstream, NULL, 16));
					}

					response[0] = request[0];
					for (k=0; k<COMPANY_NUM; k++) {
						response[2*k+2] = k;
						response[2*k+3] = 0x00000000;
					}
					response[request[2]*2+2] = request[3];

					if (players[i].key != request[0]) {
						response[1] = ERR_KEY;
					} else if(request[1] != PURCHASE && request[1] != SALE) {
						response[1] = ERR_CODE;
					} else if(players[i].count >=5) {
						response[1] = ERR_REQ;
					} else if(request[2] < 0 || request[2] > COMPANY_NUM-1) {
						response[1] = ERR_ID;
					} else {
						k=0;
						for(j=0;j<COMPANY_NUM;j++) {
							k += (players[i].purchase[j] - players[i].sale[j]) * companies[j].price;
						}
						if(request[1] == PURCHASE) {
							k += request[3]*companies[request[2]].price;
						} else {
							k -= request[3]*companies[request[2]].price;
						}
						if(players[i].budget[turn] <= k) {
							response[1] = ERR_PUR;
						} else {
							response[1] = ACCEPT;
							if(request[1] == PURCHASE) {
								players[i].purchase[j] += request[3];
							} else {
								players[i].sale[j] += request[3];
							}
						}
					}
						
					for (k=0; k<22; k++) {
						//printf("randomHash() create the next number: %u\n", block[i]);
						sprintf(userstream, "%x", htonl(response[k]));
						//printf("this is HEX!!! %s\n", userstream);
						write(fd[i],userstream, INTSIZE);
					}

					players[i].count++;
				}
			} else {
				printf("fd[%d] is -1\n", i);
			}
		}						
	}
	printf("connection finished!\n");
	return 0;
}

int makesock(char *service) {
	int error, fd;
	struct addrinfo *ai, *ai2, hints;

	memset((void *)&hints, 0, sizeof(struct addrinfo));
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family |= PF_UNSPEC;
	hints.ai_flags |= AI_PASSIVE;

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
//	srand((int)time(NULL));
	int d = UINT32_MAX / RAND_MAX;
	int m = UINT32_MAX % RAND_MAX + 1;
	uint32_t number = (uint32_t)(rand()*d + rand()%m);
	return number;
}
