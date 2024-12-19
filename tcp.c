#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <time.h>

#define PORT 12345
#define MAX_CLIENTS 5
#define BUFFER_SIZE 1024
#define FILENAME "client_data.csv"

void get_timestamp(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

typedef struct {
    int socket;
    int client_number;
} ClientInfo;

DWORD WINAPI handle_client(LPVOID client_info_ptr) {
    ClientInfo *client_info = (ClientInfo *)client_info_ptr;
    int client_socket = client_info->socket;
    int client_number = client_info->client_number;
    free(client_info); // Memory temizliği

    char buffer[BUFFER_SIZE];
    char timestamp[20];

    while (1) {
        // Client'tan veri al
        memset(buffer, 0, sizeof(buffer));
        int read_size = recv(client_socket, buffer, sizeof(buffer), 0);
        if (read_size <= 0) {
            printf("Client %d bağlantisi kesildi.\n", client_number);
            closesocket(client_socket);
            return 0;
        }

        // Zaman damgasi al
        get_timestamp(timestamp, sizeof(timestamp));

        // Veriyi CSV dosyasina kaydet
        FILE *file = fopen(FILENAME, "a");  // "a" modunda aç, var olan dosyaya ekler
        if (file == NULL) {
            perror("Dosya açilamadi");
            closesocket(client_socket);
            return 0;
        }

        // CSV formatinda veriyi yaz
        fprintf(file, "client%d ; %s ; %s\n", client_number, buffer, timestamp);
        fclose(file); // Dosyayi kapat

        // Terminalde veri göster
        printf("[Client%d] Veri: %s (Zaman: %s)\n", client_number, buffer, timestamp);

        // Client'a cevap gönder
        char response[BUFFER_SIZE];
        snprintf(response, sizeof(response), "Client%d: Veriniz alindi ve kaydedildi.", client_number);
        send(client_socket, response, strlen(response), 0);

        // 2 saniye bekle
        Sleep(2000);  // Windows'ta sleep() yerine Sleep() kullanilir
    }

    // Client bağlantisini kapat
    closesocket(client_socket);
    printf("Client %d bağlantisi kapatildi.\n", client_number);

    return 0;
}

int main() {
    WSADATA wsaData;
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    int client_addr_len;
    HANDLE thread_id;

    // Winsock baslatma
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        perror("Winsock baslatilamadi");
        exit(EXIT_FAILURE);
    }
    printf("Winsock baslatildi.\n");

    // Server soketi olusturuluyor
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket olusturulamadi");
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    printf("Server soketi olusturuldu.\n");

    // Server adresini ayarlama
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;  // Tüm IP adreslerinden bağlantiyi kabul eder
    server_addr.sin_port = htons(PORT);

    // Soketi bağlama
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind hatasi");
        closesocket(server_socket);
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    printf("Server %d portunda dinliyor...\n", PORT);

    // Bağlantilari dinlemeye baslama
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("Listen hatasi");
        closesocket(server_socket);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    client_addr_len = sizeof(client_addr);
    int client_number = 1;

    // Maksimum 2 client bekle
    while (client_number <= MAX_CLIENTS) {
        printf("Client bağlantisi bekleniyor...\n");

        // Client bağlantisini kabul et
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            perror("Client bağlantisi kabul edilemedi");
            continue;
        }

        printf("Client %d (%s:%d) bağlantisi kuruldu.\n", client_number, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // Yeni bir thread baslat
        ClientInfo *client_info = malloc(sizeof(ClientInfo));
        client_info->socket = client_socket;
        client_info->client_number = client_number;

        thread_id = CreateThread(NULL, 0, handle_client, (LPVOID)client_info, 0, NULL);
        if (thread_id == NULL) {
            perror("Thread olusturulamadi");
            closesocket(client_socket);
            free(client_info);
        } else {
            CloseHandle(thread_id);  // Thread'i hemen serbest birak
        }

        client_number++;
    }

    // Server soketini kapat
    closesocket(server_socket);
    WSACleanup();
    printf("Server kapatildi.\n");

    return 0;
}
