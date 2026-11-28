#include <iostream>
#include <csignal>
#include <thread>
#include <mutex>
#include <system_error>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

std::mutex outputMutex;

// Synchronizacja wypisywania komunikatów
void log(const char* message) {
    std::lock_guard<std::mutex> lock(outputMutex);
    std::cout << message << "\n";
}

//obsługa pojedynczego klienta w osobnym wątku
void handleClient(int client) {
    log("Klient polaczony");

    char buffer[1024];
    ssize_t bytes;
    bool error = false;

    //odbieramy i odsyłamy dane fragmentami
    while ((bytes = recv(client, buffer, sizeof(buffer), 0)) > 0) {

        {
            std::lock_guard<std::mutex> lock(outputMutex);
            std::cout.write(buffer, bytes);
            std::cout << "\n";
        }

        // send() może wysłać mniej bajtów, niż zażądano
        ssize_t sent = 0;

        while (sent < bytes) {
            ssize_t n = send(client, buffer + sent, bytes - sent, 0);

            if (n <= 0) {
                log("Blad wysylania");
                error = true;
                break;
            }

            sent += n;
        }

        if (error) { break; }
    }

    if (bytes < 0 && !error) { log("Blad odbierania"); }

    close(client);
    log("Klient rozlaczony");
}

int main() {
    //zapobiega zakończeniu programu po zerwaniu połączenia
    signal(SIGPIPE, SIG_IGN);

    int server = socket(AF_INET, SOCK_STREAM, 0);

    if (server < 0) {
        std::cerr << "Blad tworzenia socketu\n";
        return 1;
    }

    //umożliwia ponowne użycie portu po restarcie demona
    int opt = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(8080);
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server, (sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Blad bind\n";
        close(server);
        return 1;
    }

    if (listen(server, 5) < 0) {
        std::cerr << "Blad listen\n";
        close(server);
        return 1;
    }

    log("Demon nasluchuje na porcie 8080...");

    while (true) {
        int client = accept(server, nullptr, nullptr);

        if (client < 0) {
            log("Blad accept");
            continue;
        }

        // Każdy klient jest obsługiwany przez osobny wątek
        try {
            std::thread worker(handleClient, client);
            worker.detach();
        } catch (const std::system_error&) {
            log("Blad tworzenia watku");
            close(client);
        }
    }

    close(server);
    return 0;
}