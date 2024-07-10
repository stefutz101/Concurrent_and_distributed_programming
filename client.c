#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 8192

int sock;
int running = 1;
pthread_t recv_thread;

void* receive_messages(void* arg) {
    char buffer[BUFFER_SIZE];
    while (running) {
        memset(buffer, 0, sizeof(buffer));
        int valread = read(sock, buffer, sizeof(buffer) - 1);
        if (valread > 0) {
            buffer[valread] = '\0';
            printf("\nReceived response from server:\n%s\n", buffer);

            // Check for disconnection message
            if (strstr(buffer, "You have been disconnected by the server.") != NULL || strstr(buffer, "Goodbye!") != NULL) {
                printf("Disconnected by server. Exiting...\n");
                running = 0;
                close(sock);
                exit(0);
            }

            // Print the prompt again
            printf("\nEnter a command (CONVERT <path_to_xml>, STATUS, SHUTDOWN, DISCONNECT <client_id>, DISCONNECT *, or EXIT): ");
            fflush(stdout); // Ensure prompt is displayed immediately
        } else if (valread == 0) {
            printf("Server closed the connection.\n");
            running = 0;
            close(sock);
            exit(0);
        } else {
            perror("Read error");
            running = 0;
            close(sock);
            exit(0);
        }
    }
    return NULL;
}

void send_command(const char* command, const char* xml_data, const char* json_path) {
    char buffer[BUFFER_SIZE] = {0};
    if (xml_data != NULL) {
        snprintf(buffer, sizeof(buffer), "%s%s", command, xml_data);
    } else {
        strncpy(buffer, command, sizeof(buffer) - 1);
    }

    // Send the command to the server
    if (send(sock, buffer, strlen(buffer), 0) < 0) {
        perror("Send failed");
        return;
    }
    printf("Command sent to server: %s\n", command);

    // If there's a JSON path specified, save the response to a file
    if (json_path != NULL) {
        char response[BUFFER_SIZE] = {0};
        int valread = read(sock, response, BUFFER_SIZE);
        if (valread > 0) {
            response[valread] = '\0';
            FILE *json_file = fopen(json_path, "w");
            if (json_file == NULL) {
                perror("Error opening JSON file");
            } else {
                fprintf(json_file, "%s", response);
                fclose(json_file);
                printf("JSON saved to %s\n", json_path);
            }
        } else {
            if (valread == 0) {
                printf("Server closed the connection.\n");
            } else {
                perror("Read error");
            }
        }
    }
}

int main() {
    struct sockaddr_in serv_addr;

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return -1;
    }

    // Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        return -1;
    }

    // Create a thread to receive messages from the server
    if (pthread_create(&recv_thread, NULL, receive_messages, NULL) != 0) {
        perror("Thread creation error");
        return -1;
    }

    char command[BUFFER_SIZE];

    while (running) {
        printf("\nEnter a command (CONVERT <path_to_xml>, STATUS, SHUTDOWN, DISCONNECT <client_id>, DISCONNECT *, UPTIME, or EXIT): ");
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = '\0'; // Remove trailing newline

        if (strncmp(command, "EXIT", 4) == 0) {
            send_command("EXIT", NULL, NULL);  // Notify the server about the exit
            running = 0;
            pthread_join(recv_thread, NULL);   // Wait for the receive thread to terminate
            close(sock);                      // Close the socket
            break;
        } else if (strncmp(command, "CONVERT ", 8) == 0) {
            char xml_path[BUFFER_SIZE];
            strncpy(xml_path, command + 8, sizeof(xml_path) - 1);
            xml_path[sizeof(xml_path) - 1] = '\0'; // Ensure null-termination

            // Read XML data from file
            FILE *xml_file = fopen(xml_path, "r");
            if (xml_file == NULL) {
                perror("Error opening XML file");
                continue;
            }
            fseek(xml_file, 0, SEEK_END);
            long xml_file_size = ftell(xml_file);
            fseek(xml_file, 0, SEEK_SET);
            char *xml_data = (char *)malloc(xml_file_size + 1);
            if (xml_data == NULL) {
                perror("Memory allocation error");
                fclose(xml_file);
                continue;
            }
            fread(xml_data, 1, xml_file_size, xml_file);
            fclose(xml_file);
            xml_data[xml_file_size] = '\0';

            // Prompt for JSON save path
            char json_path[BUFFER_SIZE];
            printf("Enter path to save JSON file: ");
            fgets(json_path, sizeof(json_path), stdin);
            json_path[strcspn(json_path, "\n")] = '\0'; // Remove trailing newline

            send_command("CONVERT ", xml_data, json_path);
            free(xml_data);
        } else {
            send_command(command, NULL, NULL);
        }
    }

    return 0;
}

/*
    >   Exemple de compilare si rulare a programului
    gcc -o client{,.c} -lxml2 -lcjson
    ./client
*/