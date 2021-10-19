#include <iostream>
#include "tcp.h"

raven::event::tcp TCP;
raven::event::cin CIN;

void lineHandler()
{
    TCP.send(CIN.line());
    CIN.read(lineHandler);
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cout << "USAGE: tcpclient <IP address> <port number>\n";
        exit(1);
    }

    // attempt connection to server
    TCP.client(
        argv[1],
        argv[2]);

    // input message from keyboard
    CIN.read(lineHandler);

    raven::event::theEventQueue.Run();
}