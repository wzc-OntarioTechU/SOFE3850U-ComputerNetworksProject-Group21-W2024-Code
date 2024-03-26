#include "PDU.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <stdbool.h>

#define DEFAULT_PORT 3000
#define MAX_MSG_LEN 101
#define MAX_BACKLOG 50

char* getFileData(struct pdu request, int *parts) {
    int *fd = fopen(request.data, "r");
    if (*fd < 0) {
        *parts = -1;
        return NULL;
    } else {
        int numChar = 0;
        while (feof(fd) == 0) {
            fgetc(fd);
            (numChar)++;
        }
        if (numChar == 0) {
            *parts = 0;
            return NULL;
        } else {
            int numPackets;
            int charReq;
            do {
            numPackets = numChar / 100 + 1;
            char *lenTemp;
            sprintf(lenTemp, "%d", numPackets);
            charReq = strlen(lenTemp);
            } while (numPackets < numChar / (100 - charReq) + 1);
            *parts = numPackets;
            char **retData = calloc(sizeof(char*), numPackets);
            for (int i = 0; i < numPackets; i++) {
                retData[i] = calloc(sizeof(char), 100);
                sprintf(retData[i], "%d*", i, charReq);
                char buff[100 - charReq];
                for (int x = 0; x < 100 - charReq; x++) {
                    buff[charReq + x] = getc(fd);
                }
                memcpy(retData[i], buff, 100);
            }
            return retData;
        }
    }
}

void toPDU(struct pdu *retPDU, char *buff) {
    retPDU->type = buff[0];
    memcpy(retPDU->data, buff[1], (MAX_MSG_LEN - 1));
}

void toBuff(struct pdu *retPDU, char *buff) {
    buff[0] = retPDU->type;
    memcpy(buff[1], retPDU->data, (MAX_MSG_LEN - 1));
}

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
        perror("Failed to open socket.");
        return 2;
    }

    // bind an address to the socket
    struct sockaddr_in serverAddr, clientAddr;
    bzero((char*) &serverAddr, sizeof(struct sockaddr_in));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(socketFD, (struct sockaddr*) &serverAddr, sizeof(serverAddr)) < 0) {
        perror("Can't bind socket to address.");
        return 3;
    }

    if (listen(socketFD, MAX_BACKLOG) != 0) {
        perror("Can't listen on socket!");
        return 4;
    }

    int clientLength = sizeof(clientAddr);
    while (1) {
        // accept a new connection
        socklen_t clientLength;
        int clientFD;
        if ((clientFD = accept(socketFD, &clientAddr, &clientLength)) < 0) {
            perror("Failed to accept connection, moving on.");
        } else {
            char recvBuff[MAX_MSG_LEN], sendBuff[MAX_MSG_LEN];
            bool done = false;
            while (!done) {
                int recvLen = recvmsg(clientFD, recvBuff, MSG_WAITALL);
                char* fileData;
                int* packets;
                if (recvLen < 0) {
                    perror("Bad packet recived, moving on.");
                    done = true;
                } else if (recvLen > 0) {
                    // translate to PDU
                    struct pdu recPDU;
                    toPDU(&recPDU, recvBuff);
                    if (recPDU.type == 'C') {
                        fileData = getFileData(recPDU, packets);
                        if (*packets < 0) {
                            // failed to find file
                            struct pdu retPDU = {.type = 'E', .data = "File not found. Closing connection."};
                            toBuff(&retPDU, sendBuff);
                            sendto(clientFD, sendBuff, MAX_MSG_LEN, MSG_EOR, &clientAddr, &clientLength);
                        } else {
                            struct pdu retPDU;
                            retPDU.type = 'D';
                            sprintf(retPDU.data, "%d", 0);
                            toBuff(&retPDU, sendBuff);
                            sendto(clientFD, sendBuff, MAX_MSG_LEN, MSG_EOR, &clientAddr, &clientLength);
                        }
                    } else if (recPDU.type = 'D') {
                        char *lenTemp;
                        sprintf(lenTemp, "%d", *packets);
                        for (int i = 0; i < *packets; i++) {
                            struct pdu retPDU;
                            retPDU.type = 'D';
                            memcpy(retPDU.data[0], lenTemp, strlen(lenTemp));
                            memcpy(retPDU.data[strlen(lenTemp)], fileData[i], MAX_MSG_LEN - 1 - strlen(lenTemp));
                            sendto(clientFD, sendBuff, MAX_MSG_LEN, MSG_EOR, &clientAddr, &clientLength);
                        }
                    } else if (recPDU.type = 'E') {
                        int i = atoi(recPDU.data);
                        char *lenTemp;
                        sprintf(lenTemp, "%d", *packets);
                        struct pdu retPDU;
                        retPDU.type = 'D';
                        memcpy(retPDU.data[0], lenTemp, strlen(lenTemp));
                        memcpy(retPDU.data[strlen(lenTemp)], fileData[i], MAX_MSG_LEN - 1 - strlen(lenTemp));
                        sendto(clientFD, sendBuff, MAX_MSG_LEN, MSG_EOR, &clientAddr, &clientLength);
                    } else if (recPDU.type = 'F') {
                        close(clientFD);
                        done = true;
                        free(fileData);
                    }
                }
            }
        }

    }

    // never reached, but intened since we want exit on interrupt only
    return 0;
}