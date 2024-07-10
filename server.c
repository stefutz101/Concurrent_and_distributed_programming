#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <cjson/cJSON.h>
#include <time.h>

#define PORT 8080
#define BUFFER_SIZE 8192
#define MAX_CONNECTIONS 100
#define MAX_BUF_SIZE 256

// Mutex for synchronizing access to the connection array
pthread_mutex_t connection_mutex = PTHREAD_MUTEX_INITIALIZER;

time_t server_start_time;

typedef struct {
    int socket;
    pthread_t thread_id;
    int user_id;
    struct sockaddr_in address;
    int logged_in;
} client_connection;

client_connection connections[MAX_CONNECTIONS];
int connection_count = 0;
int user_count = 0;

// Function to get the current timestamp
void get_timestamp(char* buffer, size_t size) {
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", t);
}

void get_uptime(char* buffer, size_t size) {
    time_t now = time(NULL);
    int seconds = difftime(now, server_start_time);

    int days = seconds / 86400;
    seconds %= 86400;
    int hours = seconds / 3600;
    seconds %= 3600;
    int minutes = seconds / 60;
    seconds %= 60;

    snprintf(buffer, size, "Uptime: %d days, %d hours, %d minutes, %d seconds", days, hours, minutes, seconds);
}

// Function to convert XML to JSON
char* xml_to_json(const char* xml_data) {
    // Parse XML document
    xmlDocPtr doc = xmlParseDoc((const xmlChar*)xml_data);
    if (doc == NULL) {
        fprintf(stderr, "Failed to parse XML document\n");
        return NULL;
    }

    // Get root element
    xmlNodePtr root = xmlDocGetRootElement(doc);
    if (root == NULL) {
        fprintf(stderr, "Failed to get root element\n");
        xmlFreeDoc(doc);
        return NULL;
    }

    // Convert XML to cJSON
    cJSON* json_root = cJSON_CreateObject();
    cJSON* books_array = cJSON_CreateArray();

    xmlNodePtr cur_node = root->children;
    while (cur_node != NULL) {
        if (cur_node->type == XML_ELEMENT_NODE && strcmp((const char*)cur_node->name, "book") == 0) {
            cJSON* book_json = cJSON_CreateObject();
            xmlAttrPtr attr = cur_node->properties;
            while (attr != NULL) {
                if (attr->type == XML_ATTRIBUTE_NODE) {
                    cJSON_AddStringToObject(book_json, (const char*)attr->name, (const char*)attr->children->content);
                }
                attr = attr->next;
            }

            xmlNodePtr book_node = cur_node->children;
            while (book_node != NULL) {
                if (book_node->type == XML_ELEMENT_NODE) {
                    cJSON_AddStringToObject(book_json, (const char*)book_node->name, (const char*)book_node->children->content);
                }
                book_node = book_node->next;
            }

            cJSON_AddItemToArray(books_array, book_json);
        }
        cur_node = cur_node->next;
    }

    cJSON_AddItemToObject(json_root, "books", books_array);

    // Convert cJSON to JSON string
    char* json_str = cJSON_Print(json_root);

    // Clean up
    cJSON_Delete(json_root);
    xmlFreeDoc(doc);

    return json_str;
}

// Function to check login credentials
int login(char * const user, char * const password) {
    FILE *file;
    char line[MAX_BUF_SIZE];
    char file_user[50], file_pass[50];

    // Open the file for reading
    file = fopen("passwords.txt", "r");
    if (file == NULL) {
        fprintf(stderr, "Error in opening file\n");
        return 0;
    }

    // Read the file line by line
    while (fgets(line, MAX_BUF_SIZE, file)) {
        // Remove the newline character at the end of the line, if it exists
        line[strcspn(line, "\n")] = 0;

        // Separate user_id and password from the line
        if (sscanf(line, "%49s %49s", file_user, file_pass) == 2) {
            // Check if user_id and password match
            if (strcmp(file_user, user) == 0 && strcmp(file_pass, password) == 0) {
                printf("Welcome back, %s!\n", file_user);
                fclose(file);
                return 1;
            }
        }
    }

    // Close the file
    fclose(file);
    printf("Invalid username or password\n");
    return 0;
}

// Function to handle client connections
void* handle_client(void* client_socket_ptr) {
    int client_socket = ((client_connection*)client_socket_ptr)->socket;
    int user_id = ((client_connection*)client_socket_ptr)->user_id;
    struct sockaddr_in address = ((client_connection*)client_socket_ptr)->address;
    int logged_in = ((client_connection*)client_socket_ptr)->logged_in;
    free(client_socket_ptr);

    while (1) {
        char buffer[BUFFER_SIZE] = {0};
        int valread = read(client_socket, buffer, BUFFER_SIZE);

        if (valread > 0) {
            buffer[valread] = '\0';
            char timestamp[64];
            get_timestamp(timestamp, sizeof(timestamp));
            printf("[%s] Command received from User %d: %s\n", timestamp, user_id, buffer);

            if (strncmp(buffer, "LOGIN ", 6) == 0) {
                char user[50], pass[50];
                if (sscanf(buffer + 6, "%49s %49s", user, pass) == 2) {
                    if (login(user, pass)) {
                        logged_in = 1;
                        send(client_socket, "Login successful", 16, 0);
                    } else {
                        send(client_socket, "Login failed", 12, 0);
                    }
                } else {
                    send(client_socket, "Invalid login command", 21, 0);
                }
            } else if (strncmp(buffer, "CONVERT ", 8) == 0) {
                char* xml_data = buffer + 8;
                char* json_data = xml_to_json(xml_data);
                if (json_data != NULL) {
                    send(client_socket, json_data, strlen(json_data), 0);
                    free(json_data);
                } else {
                    char* error_message = "Failed to convert XML to JSON";
                    send(client_socket, error_message, strlen(error_message), 0);
                }
            } else if (strncmp(buffer, "STATUS", 6) == 0) {
                char* status_message = "Server is running";
                send(client_socket, status_message, strlen(status_message), 0);
            } else if (strncmp(buffer, "EXIT", 4) == 0) {
                char* exit_message = "Goodbye!";
                send(client_socket, exit_message, strlen(exit_message), 0);
                close(client_socket);
                break;  // Exit the loop and close the connection
            } else if (!logged_in) {
                send(client_socket, "Please log in first to use this command", 39, 0);
            } else if (strncmp(buffer, "SHUTDOWN", 8) == 0) {
                char* shutdown_message = "Server is shutting down";
                send(client_socket, shutdown_message, strlen(shutdown_message), 0);
                close(client_socket);
                exit(0);  // Exit the thread
            } else if (strncmp(buffer, "DISCONNECT ", 11) == 0) {
                pthread_mutex_lock(&connection_mutex);
                if (strcmp(buffer + 11, "*") == 0) {
                    // Disconnect all clients
                    for (int i = 0; i < connection_count; ++i) {
                        send(connections[i].socket, "You have been disconnected by the server.", 42, 0);
                        close(connections[i].socket);
                    }
                    connection_count = 0;
                } else {
                    // Disconnect a specific client
                    int disconnect_user_id = atoi(buffer + 11);
                    for (int i = 0; i < connection_count; ++i) {
                        if (connections[i].user_id == disconnect_user_id) {
                            send(connections[i].socket, "You have been disconnected by the server.", 42, 0);
                            close(connections[i].socket);
                            // Shift remaining connections down
                            for (int j = i; j < connection_count - 1; ++j) {
                                connections[j] = connections[j + 1];
                            }
                            --connection_count;
                            break;
                        }
                    }
                }
                pthread_mutex_unlock(&connection_mutex);
            } else if (strncmp(buffer, "UPTIME", 6) == 0) {
                if (logged_in) {
                    char uptime_message[BUFFER_SIZE];
                    get_uptime(uptime_message, sizeof(uptime_message));
                    send(client_socket, uptime_message, strlen(uptime_message), 0);
                } else {
                    send(client_socket, "Please log in first to use this command", 39, 0);
                }
            } else {
                char* unknown_command_message = "Unknown command";
                send(client_socket, unknown_command_message, strlen(unknown_command_message), 0);
            }
        } else {
            perror("Read error");
            break;
        }
    }

    close(client_socket);

    // Remove the client from the connection array
    pthread_mutex_lock(&connection_mutex);
    for (int i = 0; i < connection_count; ++i) {
        if (connections[i].socket == client_socket) {
            // Shift remaining connections down
            for (int j = i; j < connection_count - 1; ++j) {
                connections[j] = connections[j + 1];
            }
            --connection_count;
            break;
        }
    }
    pthread_mutex_unlock(&connection_mutex);

    return NULL;  // Exit the thread
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Record the server start time
    server_start_time = time(NULL);

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    char timestamp[64];
    get_timestamp(timestamp, sizeof(timestamp));
    printf("[%s] Server listening on port %d\n", timestamp, PORT);

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept failed");
            continue;
        }

        pthread_mutex_lock(&connection_mutex);
        if (connection_count >= MAX_CONNECTIONS) {
            get_timestamp(timestamp, sizeof(timestamp));
            printf("[%s] Maximum connections reached. Rejecting new connection.\n", timestamp);
            close(new_socket);
            pthread_mutex_unlock(&connection_mutex);
            continue;
        }

        int* client_socket_ptr = malloc(sizeof(client_connection));
        client_connection* new_connection = (client_connection*)client_socket_ptr;
        new_connection->socket = new_socket;
        new_connection->user_id = ++user_count;
        new_connection->address = address;
        new_connection->logged_in = 0;  // Initially not logged in

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, client_socket_ptr) != 0) {
            perror("pthread_create failed");
            close(new_socket);
            free(client_socket_ptr);
            pthread_mutex_unlock(&connection_mutex);
            continue;
        }

        // Store the client connection in the array
        connections[connection_count].socket = new_socket;
        connections[connection_count].thread_id = thread_id;
        connections[connection_count].user_id = new_connection->user_id;
        connections[connection_count].address = new_connection->address;
        connections[connection_count].logged_in = new_connection->logged_in;
        ++connection_count;
        pthread_mutex_unlock(&connection_mutex);

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(new_connection->address.sin_addr), client_ip, INET_ADDRSTRLEN);
        int client_port = ntohs(new_connection->address.sin_port);

        get_timestamp(timestamp, sizeof(timestamp));
        printf("[%s] User %d connected to the server. [IP: %s, PORT: %d]\n", timestamp, new_connection->user_id, client_ip, client_port);

        pthread_detach(thread_id);  // Detach the thread to handle cleanup automatically
    }

    close(server_fd);
    return 0;
}


/*
    >   Exemple de compilare si rulare a programului
    gcc -o server{,.c} -lxml2 -lcjson
    ./server
*/