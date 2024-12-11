
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define PORT 8080
#define SEAT_COUNT 21
#define MAX_SERVICES 10
#define MAX_CITIES 81

// List of cities (Plate numbers and names)
char *city_names[MAX_CITIES] = {
    "Adana", "Adıyaman", "Afyon", "Ağrı", "Amasya", "Ankara", "Antalya", "Artvin", "Aydın", "Balıkesir",
    "Bilecik", "Bingöl", "Bitlis", "Bolu", "Burdur", "Bursa", "Çanakkale", "Çankırı", "Çorum", "Denizli",
    "Diyarbakır", "Edirne", "Elazığ", "Erzincan", "Erzurum", "Eskişehir", "Gaziantep", "Giresun", "Gümüşhane", "Hakkâri",
    "Hatay", "Isparta", "Mersin", "İstanbul", "İzmir", "Kars", "Kastamonu", "Kayseri", "Kırklareli", "Kırşehir",
    "Kocaeli", "Konya", "Kütahya", "Malatya", "Manisa", "Kahramanmaraş", "Mardin", "Muğla", "Muş", "Nevşehir",
    "Niğde", "Ordu", "Rize", "Sakarya", "Samsun", "Siirt", "Sinop", "Sivas", "Tekirdağ", "Tokat",
    "Trabzon", "Tunceli", "Şanlıurfa", "Uşak", "Van", "Yozgat", "Zonguldak", "Aksaray", "Bayburt", "Karaman",
    "Kırıkkale", "Batman", "Şırnak", "Bartın", "Ardahan", "Iğdır", "Yalova", "Karabük", "Kilis", "Osmaniye", "Düzce"
};

// Seat status: 0 - empty, 1 - occupied
int seats[MAX_SERVICES][SEAT_COUNT] = {0};
int seat_reserver[MAX_SERVICES][SEAT_COUNT] = {{0}};
char *services[MAX_SERVICES];
float service_prices[MAX_SERVICES];
int service_count = 0;

// Previously created service information
typedef struct {
    int departure;
    int arrival;
    char *service_name;
    float price;
    int seats[SEAT_COUNT];
} Service;

Service all_services[MAX_SERVICES];
int all_service_count = 0;

void *handle_client(void *arg);
void create_services(int departure, int arrival);
void display_services(int client_socket, int departure, int arrival);
void display_seat_status(int client_socket, int service_index);
void book_seat(int client_socket, int service_index, int seat_number);
void unbook_seat(int client_socket, int service_index, int seat_number);
void reset_visible_services();
void display_cities(int client_socket);
int service_exists(int departure, int arrival);

void display_cities(int client_socket) {
    char buffer[2048];
    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "Available Cities:\n");
    for (int i = 0; i < MAX_CITIES; i++) {
        snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), "%2d: %-10s%s", i + 1, city_names[i], (i + 1) % 4 == 0 ? "\n" : "\t");
    }
    strcat(buffer, "\n");
    write(client_socket, buffer, strlen(buffer));
}

int service_exists(int departure, int arrival) {
    for (int i = 0; i < all_service_count; i++) {
        if (all_services[i].departure == departure && all_services[i].arrival == arrival) {
            return 1;
        }
    }
    return 0;
}

void create_services(int departure, int arrival) {
    if (service_exists(departure, arrival)) {
        return;
    }

    int new_services = rand() % 2 + 3; // Create 3 or 4 services
    for (int i = 0; i < new_services && all_service_count < MAX_SERVICES; i++) {
        Service *new_service = &all_services[all_service_count++];
        new_service->departure = departure;
        new_service->arrival = arrival;
        new_service->service_name = malloc(50);
        snprintf(new_service->service_name, 50, "%s - %s", city_names[departure - 1], city_names[arrival - 1]);
        new_service->price = (rand() % 801 + 300) * 1.0; // Price: 50-100 range
        memset(new_service->seats, 0, sizeof(new_service->seats));
    }
}

void display_services(int client_socket, int departure, int arrival) {
    char buffer[512];
    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "Services:\n");
    for (int i = 0; i < all_service_count; i++) {
        if (all_services[i].departure == departure && all_services[i].arrival == arrival) {
            snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer),
                     "%d. %s | Price: %.2f TL\n", i + 1, all_services[i].service_name, all_services[i].price);
        }
    }
    write(client_socket, buffer, strlen(buffer));
}

void display_seat_status(int client_socket, int service_index) {
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "Service %d Seat Status:\n", service_index + 1);
    for (int i = 0; i < SEAT_COUNT; i++) {
        char seat[5];
        snprintf(seat, sizeof(seat), all_services[service_index].seats[i] == 0 ? "[%d]" : "[X]", i + 1);
        strcat(buffer, seat);
        strcat(buffer, (i + 1) % 3 == 0 ? "\n" : " ");
    }
    write(client_socket, buffer, strlen(buffer));
}

void book_seat(int client_socket, int service_index, int seat_number) {
    char buffer[256];
    if (all_services[service_index].seats[seat_number - 1] == 1) {
        snprintf(buffer, sizeof(buffer), "This seat is already reserved.\n");
    } else {
        all_services[service_index].seats[seat_number - 1] = 1;
        seat_reserver[service_index][seat_number - 1] = client_socket; // Record the client who reserved
        snprintf(buffer, sizeof(buffer), "Seat successfully reserved.\n");
    }
    write(client_socket, buffer, strlen(buffer));
}

void unbook_seat(int client_socket, int service_index, int seat_number) {
    char buffer[256];
    if (all_services[service_index].seats[seat_number - 1] == 0) {
        snprintf(buffer, sizeof(buffer), "This seat is already empty.\n");
    } else if (seat_reserver[service_index][seat_number - 1] != client_socket) {
        snprintf(buffer, sizeof(buffer), "You didn't reserve this seat, so you cannot unreserve it.\n");
    } else {
        all_services[service_index].seats[seat_number - 1] = 0;
        seat_reserver[service_index][seat_number - 1] = 0; // Clear the reservation record
        snprintf(buffer, sizeof(buffer), "Reservation successfully canceled.\n");
    }
    write(client_socket, buffer, strlen(buffer));
}

void reset_visible_services() {
    // Reset displayed services
    service_count = 0;
}

void *handle_client(void *arg) {
    int client_socket = *(int *)arg;
    free(arg);

    char buffer[1024];
    int departure, arrival;

    while (1) {
        display_cities(client_socket);

        write(client_socket, "Enter the ID of the departure city: ", 50);
        read(client_socket, buffer, sizeof(buffer));
        departure = atoi(buffer);

        write(client_socket, "Enter the ID of the arrival city: ", 50);
        read(client_socket, buffer, sizeof(buffer));
        arrival = atoi(buffer);

        if (departure < 1 || departure > MAX_CITIES || arrival < 1 || arrival > MAX_CITIES || departure == arrival) {
            write(client_socket, "Invalid inputs. Please try again.\n", 50);
            continue;
        }

        create_services(departure, arrival);
        display_services(client_socket, departure, arrival);

        while (1) {
            write(client_socket, "FOR SEAT INFO: VIEW <service no>,\n To Book: BOOK <service no> <seat no>,\n To Cancel: UNBOOK <service no> <seat no>,\n For New Services: NEW, --- To Exit: EXIT:\n ", 1024);
            memset(buffer, 0, sizeof(buffer));
            read(client_socket, buffer, sizeof(buffer));

            if (strncmp(buffer, "EXIT", 4) == 0) {
                close(client_socket);
                return NULL;
            } else if (strncmp(buffer, "NEW", 3) == 0) {
                reset_visible_services();
                break;
            } else if (strncmp(buffer, "VIEW", 4) == 0) {
                int service_index = atoi(buffer + 5) - 1;
                if (service_index >= 0 && service_index < all_service_count) {
                    display_seat_status(client_socket, service_index);
                } else {
                    write(client_socket, "Invalid service number.\n", 35);
                }
            } else if (strncmp(buffer, "BOOK", 4) == 0) {
                int service_index, seat_number;
                if (sscanf(buffer + 5, "%d %d", &service_index, &seat_number) == 2) {
                    if (service_index - 1 >= 0 && service_index - 1 < all_service_count &&
                        seat_number >= 1 && seat_number <= SEAT_COUNT) {
                        book_seat(client_socket, service_index - 1, seat_number);
                    } else {
                        write(client_socket, "Invalid service or seat number.\n", 40);
                    }
                } else {
                    write(client_socket, "Invalid command format.\n", 30);
                }
            } else if (strncmp(buffer, "UNBOOK", 6) == 0) {
                int service_index, seat_number;
                if (sscanf(buffer + 7, "%d %d", &service_index, &seat_number) == 2) {
                    if (service_index - 1 >= 0 && service_index - 1 < all_service_count &&
                        seat_number >= 1 && seat_number <= SEAT_COUNT) {
                        unbook_seat(client_socket, service_index - 1, seat_number);
                    } else {
                        write(client_socket, "Invalid service or seat number.\n", 40);
                    }
                } else {
                    write(client_socket, "Invalid command format.\n", 30);
                }
            } else {
                write(client_socket, "Unknown command.\n", 20);
            }
        }
    }
}

int main() {
    int server_socket, new_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 5) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d\n", PORT);

    while (1) {
        addr_size = sizeof(client_addr);
        new_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_size);
        if (new_socket < 0) {
            perror("Accept failed");
            continue;
        }

        printf("New connection accepted\n");

        pthread_t thread_id;
        int *client_socket = malloc(sizeof(int));
        *client_socket = new_socket;
        if (pthread_create(&thread_id, NULL, handle_client, client_socket) != 0) {
            perror("Thread creation failed");
            free(client_socket);
        }
        pthread_detach(thread_id);
    }

    close(server_socket);
    return 0;
}
