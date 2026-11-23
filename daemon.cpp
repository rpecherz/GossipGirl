#include <iostream>
#include <csignal>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

int main() {
    //zapobiega zakończeniu programu po zerwaniu połączenia przez klienta
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

    std::cout << "Demon nasluchuje na porcie 8080...\n";

    while (true) {
        int client = accept(server, nullptr, nullptr);

        if (client < 0) {
            std::cerr << "Blad accept\n";
            continue;
        }

        std::cout << "Klient polaczony\n";

        char buffer[1024];
        ssize_t bytes;
        bool error = false;

        //odbieramy i odsyłamy dane fragmentami, bez przechowywania całej wiadomości
        while ((bytes = recv(client, buffer, sizeof(buffer), 0)) > 0) {
            std::cout.write(buffer, bytes);
            std::cout << "\n";

            //send() może wysłać mniej bajtów, niż zażądano.
            ssize_t sent = 0;

            while (sent < bytes) {
                ssize_t n = send(client, buffer + sent, bytes - sent, 0);

                if (n <= 0) {
                    std::cerr << "Blad wysylania\n";
                    error = true;
                    break;
                }

                sent += n;
            }

            if (error) {
                break;
            }
        }

        if (bytes < 0 && !error) {
            std::cerr << "Blad odbierania\n";
        }

        close(client);
        std::cout << "Klient rozlaczony\n";
    }

    close(server);
    return 0;
}