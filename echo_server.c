#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <sys/select.h>
#include <limits.h>

#define BUFSIZE 1024
#define SOCK_MAX 4

int makesock(char *service);
uint32_t randomHash();

int main(int argc, char *argv[]) {
	srand((int)time(NULL));
	if (argv[1] == NULL) {
		puts("missing parameters");
		return 0;
	}
	char service[100];
	int fd[SOCK_MAX + 1];
	int sockCount = 0;
	fd_set fdsets;
	uint32_t block[SOCK_MAX+1];
	char userstream[SOCK_MAX+1][BUFSIZE];
	char message[BUFSIZE];
	int i;
	int error;

	strcpy(service, argv[1]);
	
	fd[0] = makesock(service);

	if (fd[0] < 0) {
		printf("unable to create socket, connection, or binding %i\n", fd[0]);
		return 0;
	}
	printf("binding success! sock: %i\n", fd[0]);
	
	error = listen(fd[0], SOCK_MAX);
	if (error < 0) {
		fprintf(stderr,"unable to listen error: %s(%d)\n", gai_strerror(error), error);
		return 0;
	}
	printf("listen success!\n");

	fd[1] = -1;
	int msg_length, n;
	struct sockaddr_storage ss;
	socklen_t sl;
	sl = sizeof(ss);

	sockCount = 1;

	while(1) {
		FD_ZERO(&fdsets);
		for (i=0; i<sockCount; i++) {
			if (fd[i] != (-1)) {
				FD_SET(fd[i], &fdsets);
			}
		}
		printf("current socket count: %d\n", sockCount);

		if ((n = select(FD_SETSIZE, &fdsets, NULL, NULL, NULL)) == -1) {
			printf("error in select");
		}
		printf("select returns: %d\n", n);

		if (FD_ISSET(fd[0], &fdsets) != 0) {
			printf("adding new user. current user count: %d\n", sockCount);
			fd[sockCount] = accept(fd[0], (struct sockaddr *)&ss, &sl);
			if (fd[sockCount] < 0) {
				fprintf(stderr,"socket error: %s(%d)\n", gai_strerror(fd[sockCount]), fd[sockCount]);
			}
			if (sockCount <= SOCK_MAX) {
				printf("new user accepted: %d\n", sockCount);
				strcpy(message, "you have joined the game!\n");
				write(fd[sockCount], (const void *)message, sizeof(message));
				sockCount++;
			} else {
				printf("too many user has tried to connect\n");
				strcpy(message, "the server already has four players\n");
				write(fd[sockCount], message, sizeof(message));
				close(fd[sockCount]);
			}
		}

		for (i=1; i<sockCount; i++) {
			if (fd[i] != (-1)) {
				if (FD_ISSET(fd[i], &fdsets)) {
					printf("fd[%d] ready for read\n", i);
					if ((msg_length = read(fd[i], userstream[i], sizeof(userstream[i]))) == -1) {
						printf("fd[%d] read error\n", i);
					} else if (msg_length > 0) {
						block[i] = ntohl(atoi(userstream[i]));
						printf("fd[%d] input: %s\n", i, userstream[i]);
						int k=0;
						for (k=0; k<22; k++) {
							block[i] = randomHash();
							printf("randomHash() create the next number: %u\n", block[i]);
							block[i] = block[i] | 0x00000000;
							sprintf(userstream[i], "%x", htonl(block[i]));
							printf("this is HEX!!! %s\n", userstream[i]);
							block[i] = strtol(userstream[i], NULL, 16);
							block[i] = ntohl(block[i]);
							printf("hex code for something(return): %u\n", block[i]);

							/*
							printf("randomHash() create the next number: %u\n", block[i]);
							sprintf(userstream[i], "%u", htonl(block[i]));
							printf("randomHash() created the next number(network byte order): %u\n", ntohl(block[i]));
							block[i] = block[i] | 0x00000000;
							printf("hex code for something: %x\n", block[i]);
							sprintf(userstream[i], "%x", htonl(block[i]));
							printf("this is HEX!!! %s\n", userstream[i]);
							uint32_t hello = strtol(userstream[i], NULL, 16);
							printf("hex code for something(return): %d\n", ntohl(hello));
							write(fd[i],userstream[i], strlen(userstream[i]));
							*/
						}
					} else {
						printf("fd[%d] closed\n", i);
						close(fd[i]);
						fd[i] = -1;
					}
				} else {
					printf("fd[%d] open but not set\n", i);
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
	int d = UINT32_MAX / RAND_MAX;
	int m = UINT32_MAX % RAND_MAX + 1;
	uint32_t number = (uint32_t)(rand()*d + rand()%m);
	return number;
}
