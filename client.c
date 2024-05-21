#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 12345
#define SERVER_IP "127.0.0.1" // Change this to the IP address of the server

int main() {
    int client_socket;
    struct sockaddr_in server_addr;

    // Create client socket
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Prepare the server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("invalid address");
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connection failed");
        exit(EXIT_FAILURE);
    }
    char command[100];
    while (1) {
        printf("Enter command: ");
        fgets(command, sizeof(command), stdin);
        send(client_socket, command, strlen(command), 0);

        if (strcmp(command, "quitc\n") == 0) {
            printf("Disconnected from server\n");
            break;
        }

        char response[1024];
        int bytes_received = recv(client_socket, response, sizeof(response), 0);
        if (bytes_received < 0) {
            perror("recv failed");
            exit(EXIT_FAILURE);
        } else if (bytes_received == 0) {
            printf("Server disconnected\n");
            break;
        } else {
            response[bytes_received] = '\0';
            printf("Server response:\n%s\n", response);
        }
    }

    close(client_socket);
    return 0;
}

