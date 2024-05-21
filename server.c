#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h> 
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#define PORT 12345
#define MAX_COMMAND_SIZE 100
#define MAX_FILENAME_SIZE 100
#define MAX_PATH_SIZE 256
#define MAX_RESPONSE_SIZE 1024
#define CLIENT_COMMANDS_COUNT 9

void send_response(int client_socket, char *response) {
    send(client_socket, response, strlen(response), 0);
}

// Custom comparison function for qsort to sort directory entries based on creation time
int compare_creation_time(const void *a, const void *b) {
    struct dirent **dir_a = (struct dirent **)a;
    struct dirent **dir_b = (struct dirent **)b;

    struct stat stat_a, stat_b;
    stat((*dir_a)->d_name, &stat_a);
    stat((*dir_b)->d_name, &stat_b);

    if (stat_a.st_ctime < stat_b.st_ctime) return -1;
    if (stat_a.st_ctime > stat_b.st_ctime) return 1;
    return 0;
}

void handle_client_request(int client_socket) {
    char command[MAX_COMMAND_SIZE];
    char response[MAX_RESPONSE_SIZE];
    char filename[MAX_FILENAME_SIZE];
    char path[MAX_PATH_SIZE];

    while (1) {
        memset(command, 0, sizeof(command));
        if (recv(client_socket, command, sizeof(command), 0) <= 0) {
            perror("recv failed");
            exit(EXIT_FAILURE);
        }

        if (strcmp(command, "quitc") == 0) {
            printf("Client disconnected\n");
            break;
        }

       if (strncmp(command, "dirlist -t", 10) == 0)  {
            DIR *dir;
            struct dirent **entries;
            int count = scandir(".", &entries, NULL, NULL); // No sorting initially
            if (count < 0) {
                perror("scandir failed");
                exit(EXIT_FAILURE);
            } else {
                // Sort directory entries based on creation time
                qsort(entries, count, sizeof(struct dirent *), compare_creation_time);

                strcpy(response, "");
                for (int i = 0; i < count; i++) {
                    struct stat st;
                    char fullpath[MAX_PATH_SIZE];
                    sprintf(fullpath, "%s/%s", ".", entries[i]->d_name);
                    stat(fullpath, &st);
                    if (S_ISDIR(st.st_mode)) { // Check if it's a directory
                        strcat(response, entries[i]->d_name);
                        strcat(response, "\n");
                    }
                    free(entries[i]);
                }
                free(entries);
                send_response(client_socket, response);
            }
        } else if (strncmp(command, "dirlist -a", 10) == 0) {
            DIR *dir;
            struct dirent **entries;
            int count = scandir(".", &entries, NULL, alphasort); // Use alphasort for alphabetical sorting
            if (count < 0) {
                perror("scandir failed");
                exit(EXIT_FAILURE);
            } else {
                strcpy(response, "");
                for (int i = 0; i < count; i++) {
                    struct stat st;
                    stat(entries[i]->d_name, &st);
                    if (S_ISDIR(st.st_mode)) { // Check if it's a directory
                        strcat(response, entries[i]->d_name);
                        strcat(response, "\n");
                    }
                    free(entries[i]);
                }
                free(entries);
                send_response(client_socket, response);
            }
        } else if (strncmp(command, "w24fn", 5) == 0) {
            sscanf(command, "%*s %s", filename);
            struct stat file_stat;
            if (stat(filename, &file_stat) == 0) {
                sprintf(response, "Filename: %s\nSize: %ld bytes\nDate created: %s\nFile permissions: %o\n",
                        filename, file_stat.st_size, ctime(&file_stat.st_ctime), file_stat.st_mode);
            } else {
                sprintf(response, "File not found\n");
            }
            send_response(client_socket, response);
        } else {
            sprintf(response, "Invalid command\n");
            send_response(client_socket, response);
        }
    }

    close(client_socket);
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    int addr_len = sizeof(client_addr);
    
    // Create server socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    // Prepare the server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    // Bind the server socket to the specified port
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    // Listen for incoming connections
    if (listen(server_socket, 3) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
    
    printf("Server listening on port %d...\n", PORT);
    
    // Accept incoming connections and handle them
    while (1) {
        if ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, (socklen_t*)&addr_len)) < 0) {
            perror("accept failed");
            exit(EXIT_FAILURE);
        }
        
        printf("New client connected\n");
        printf("Waiting for requests...\n");
        
        // Fork a child process to handle client request
        if (fork() == 0) {
            // Child process
            close(server_socket); // Close server socket in child process
            handle_client_request(client_socket);
            exit(0);
        } else {
            // Parent process
            close(client_socket); // Close client socket in parent process
        }
    }
    
    return 0;
}
