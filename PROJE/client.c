#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[1024];

    // Creating the socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    // Setting up the server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // localhost

    // Connecting to the server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection error");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("Connected to the server.\n");

    while (1) {
        // Receiving a response from the server
        memset(buffer, 0, sizeof(buffer));
        int read_size = read(sock, buffer, sizeof(buffer));
        if (read_size > 0) {
            printf("%s", buffer);
        }

        // Getting input from the user
        memset(buffer, 0, sizeof(buffer));
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = 0; // Remove the newline character

        // Sending the command to the server
        write(sock, buffer, strlen(buffer));

        // Checking for the exit command
        if (strncmp(buffer, "EXIT", 4) == 0) {
            break; // Exit command received
        }
    }

    close(sock);
    return 0;
}
