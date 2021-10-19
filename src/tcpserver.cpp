#include <iostream>
#include "tcp.h"

raven::event::tcp TCP;

void readHandler()
{
    if( ! TCP.isConnected() ) {
        std::cout << "connection closed\n";
        exit(0);
    }

    std::cout << "Msg read: " << TCP.rcvbuf() << "\n";

    TCP.read( readHandler );
}

void acceptHandler()
{
    std::cout << "client connected\n";
    TCP.read( readHandler );

}
int main( int argc, char* argv[] )
{
    if( argc != 2 ) {
        std::cout << "USAGE: tcpserver <port number>\n";
        exit(1);
    }
    
    TCP.server( 
        acceptHandler,
        argv[1] );

    raven::event::theEventQueue.Run();

}