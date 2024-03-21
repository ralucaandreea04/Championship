#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <signal.h>

#define PORT 2024
#define BUFFER_SIZE 1024

void sigint_handler(int signum) {
    printf("S-a terminat conexiunea cu acest client!\n");
    exit(EXIT_SUCCESS);
}

int main() 
{
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    int bytes_received;

    // Crearea socketului
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) 
    {
        perror("Eroare la crearea socketului");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); 

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) 
    {
        perror("Eroare la conectarea la server");
        exit(EXIT_FAILURE);
    }

    bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
    buffer[bytes_received] = '\0';
    signal(SIGINT, sigint_handler);
    
    while (1) 
    {
        printf("Introduceti comanda: ");
        fgets(buffer, BUFFER_SIZE, stdin);

        send(client_socket, buffer, strlen(buffer), 0);

        bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        buffer[bytes_received] = '\0';
        if (strncmp(buffer, "Conexiune inchisa de server.\n", strlen("Conexiune inchisa de server.\n")) == 0) 
        {
            raise(SIGINT);
        }
        else printf("Mesaj de la server: %s", buffer);
    }
    close(client_socket);
    return 0;
}