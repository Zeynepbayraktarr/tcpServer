#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <time.h>

#define PORT 12345
#define MAX_CLIENTS 2
#define BUFFER_SIZE 1024
#define FILENAME "client_data.csv"

void get_timestamp(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

DWORD WINAPI handle_client(LPVOID client_socket_ptr) {
    int client_socket = *((int *)client_socket_ptr);
    free(client_socket_ptr); // Memory temizliği
    char buffer[BUFFER_SIZE];
    char timestamp[20];
    int client_number = *((int *)client_socket_ptr);  // Client numarasını almak için

    while (1) {
        // Client'tan veri al
        memset(buffer, 0, sizeof(buffer));
        int read_size = recv(client_socket, buffer, sizeof(buffer), 0);
        if (read_size < 0) {
            perror("Veri alirken hata olustu");
            closesocket(client_socket);
            return 0;
        }

        // Zaman damgası al
        get_timestamp(timestamp, sizeof(timestamp));

        // Veriyi CSV dosyasına kaydet
        FILE *file = fopen(FILENAME, "a");  // "a" modunda aç, var olan dosyaya ekler
        if (file == NULL) {
            perror("Dosya acilamadi");
            closesocket(client_socket);
            return 0;
        }

        // CSV formatında veriyi yaz
        fprintf(file, "client%d ; %s ; %s\n", client_number, buffer, timestamp);

        // Dosyayı kapat
        fclose(file);
        printf("Veri dosyaya kaydedildi: %s\n", buffer);

        // Client'a cevap gönder
        const char *response = "Veri alindi ve kaydedildi";
        send(client_socket, response, strlen(response), 0);

        // 2 saniye bekle
        Sleep(2000);  // Windows'ta sleep() yerine Sleep() kullanılır, milisaniye cinsinden
    }

    // Client bağlantısını kapat
    closesocket(client_socket);
    printf("Client baglantisi kapatildi.\n");

    return 0;
}

int main() {
    WSADATA wsaData;
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    int client_addr_len;
    HANDLE thread_id;

    // Winsock başlatma
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        perror("Winsock baslatilamadi");
        exit(EXIT_FAILURE);
    }
    printf("Winsock baslatildi.\n");

    // Server soketi oluşturuluyor
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
    server_addr.sin_addr.s_addr = INADDR_ANY;  // Tüm IP adreslerinden bağlantıyı kabul eder
    server_addr.sin_port = htons(PORT);

    // Soketi bağlama
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind hatasi");
        closesocket(server_socket);
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    printf("Server %d portunda dinliyor...\n", PORT);

    // Bağlantıları dinlemeye başlama
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("Listen hatasi");
        closesocket(server_socket);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    client_addr_len = sizeof(client_addr);
    int client_number = 1;

    // Maksimum 2 client bekle
    
    int i;
    for (i = 0; i < MAX_CLIENTS; i++) {
        printf("Client baglantisi bekleniyor...\n");

        // Client bağlantısını kabul et
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            perror("Client baglantisi kabul edilemedi");
            continue;
        }

        printf("Client %s:%d baglantisi kuruldu.\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // Yeni bir thread başlat
        int *client_socket_ptr = malloc(sizeof(int));
        *client_socket_ptr = client_socket;

        // Client numarasını gönder
        int *client_num_ptr = malloc(sizeof(int));
        *client_num_ptr = client_number++;

        thread_id = CreateThread(NULL, 0, handle_client, (LPVOID)client_socket_ptr, 0, NULL);
        if (thread_id == NULL) {
            perror("Thread olusturulamadi");
            closesocket(client_socket);
        }

        // Thread'in tamamlanmasını beklemeden çalışmasını sağla
        CloseHandle(thread_id);  // Thread'i hemen kapat

    }

    // Server soketini kapat
    closesocket(server_socket);
    WSACleanup();
    printf("Server kapatildi.\n");

    return 0;
}
