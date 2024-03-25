#include "PDU.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define DEFAULT_PORT 3000
#define MAX_MSG_LEN 512

int main(int argc, char *argv[]) {
    int port = DEFAULT_PORT;
    int socketFD;
    char buff[MAX_MSG_LEN];

    // manage user set port
    if (argc == 1) {
        port = atoi(argv[1]);
    } else if (argc > 1) {
        perror("Invalid number of arguments. Only accepts singe argument for port number!");
        return 1;
    }

    // open UDP port
    if ((socketFD = socket(AF_INET, SOCK_DGRAM, 0)) == NULL) {
        perrorf("Failed to open socket.");
        return 2;
    }
    return 0;
}