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

#define INTSIZE 8

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
