#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <future>
#include <event.h>

namespace raven
{
    namespace event
    {

        /** @brief Read/Write to TCP/IP socket, client or server


*/
        class tcp
        {
        public:
            enum class eType
            {
                client,
                server,
            };
            /** CTOR
        @param[in] parent window that will receive event messages
    */
            tcp() : myAcceptSocket(INVALID_SOCKET),
                    myConnectSocket(INVALID_SOCKET)
            {
                // Initialize Winsock
                WSADATA wsaData;
                if (WSAStartup(MAKEWORD(2, 2), &wsaData))
                {
                    throw std::runtime_error("Winsock init failed");
                }
            }
            /** Create client socket connected to server
        @param[in] ipaddr IP address of server, defaults to same computer
        @param[in] port defaults to 27654
    */
            void client(
                const std::string &ipaddr = "127.0.0.1",
                const std::string &port = "27654")
            {
                myType = eType::client;
                struct addrinfo *result = NULL,
                                hints;

                ZeroMemory(&hints, sizeof(hints));
                hints.ai_family = AF_UNSPEC;
                hints.ai_socktype = SOCK_STREAM;
                hints.ai_protocol = IPPROTO_TCP;

                if (getaddrinfo(
                        ipaddr.c_str(), port.c_str(),
                        &hints, &result))
                {
                    throw std::runtime_error("getaddrinfo failed");
                }

                myConnectSocket = ::socket(
                    result->ai_family,
                    result->ai_socktype,
                    result->ai_protocol);
                if (myConnectSocket == INVALID_SOCKET)
                {
                    throw std::runtime_error("socket failed");
                }

                std::cout << "try connect to " << ipaddr << ":" << port << "\n";
                if (::connect(
                        myConnectSocket,
                        result->ai_addr,
                        (int)result->ai_addrlen) == SOCKET_ERROR)
                {
                    closesocket(myConnectSocket);
                    myConnectSocket = INVALID_SOCKET;
                    throw std::runtime_error("connect failed " + std::to_string(WSAGetLastError()));
                }
            }

            /** Create server socket waiting for connection requests
             * 
             * @param[in] f accept event handler function
             * @param[in] port, defaults to 27654

        Starts listening for client connection.
        Returns immediatly
        throws runtime_error exception on error
        sends eventMsgID::tcpServerAccept message to parent window when new client accepted

        One connection will be accepted.  This can be called again if the connection is closed
        to wait for another client.
    */
            void server(
                handler_t f,
                const std::string &port = "27654")
            {
                myType = eType::server;
                myPort = port;
                struct addrinfo *result = NULL,
                                hints;

                ZeroMemory(&hints, sizeof(hints));
                hints.ai_family = AF_INET;
                hints.ai_socktype = SOCK_STREAM;
                hints.ai_protocol = IPPROTO_TCP;
                hints.ai_flags = AI_PASSIVE;

                if (getaddrinfo(
                        NULL, port.c_str(),
                        &hints, &result))
                {
                    throw std::runtime_error("getaddrinfo failed");
                }

                myAcceptSocket = ::socket(
                    result->ai_family,
                    result->ai_socktype,
                    result->ai_protocol);
                if (myAcceptSocket == INVALID_SOCKET)
                {
                    throw std::runtime_error("socket failed");
                }

                if (::bind(myAcceptSocket,
                           result->ai_addr,
                           (int)result->ai_addrlen) == SOCKET_ERROR)
                {
                    closesocket(myAcceptSocket);
                    myAcceptSocket = INVALID_SOCKET;
                    throw std::runtime_error("bind failed");
                }

                if (::listen(
                        myAcceptSocket,
                        SOMAXCONN) == SOCKET_ERROR)
                {
                    closesocket(myAcceptSocket);
                    myAcceptSocket = INVALID_SOCKET;
                    throw std::runtime_error("listen failed");
                }

                myAcceptHandler = f;

                new std::thread(accept_block, this);
            }

            /// true if valid connection
            bool isConnected()
            {
                return myConnectSocket != INVALID_SOCKET;
            }

            /** send message to peer
        @param[in] msg
    */
            void send(const std::string &msg)
            {
                if (myConnectSocket == INVALID_SOCKET)
                    throw std::runtime_error("send on invalid socket");
                ::send(
                    myConnectSocket,
                    msg.c_str(),
                    (int)msg.length(), 0);
            }

            /** asynchronous read message on tcp connection
         * 
         * Throws exception if no tcp connection
         * 
         * Returns immediatly.
         * 
         * When message is received, the handler registered will be invoked.
         * 
         * If the connection is closed, or suffers any error
         * the same handler will be invoked, so the isConnected()
         * method should be checked.
    */
            void read(handler_t f)
            {
                if (myConnectSocket == INVALID_SOCKET)
                    throw std::runtime_error("read on invalid socket");
                myReadHandler = f;
                new std::thread(read_block, this);
            }

            /// get pointer to receive buffer as null terminated character string
            char *rcvbuf()
            {
                return (char *)myRecvbuf;
            }

            /// get socket connected to client
            SOCKET &clientSocket()
            {
                return myConnectSocket;
            }

            int port() const
            {
                return atoi(myPort.c_str());
            }

            bool isServer()
            {
                return myType == eType::server;
            }

        private:
            eType myType;
            std::string myPort;
            SOCKET myAcceptSocket;  // socket listening for clients
            SOCKET myConnectSocket; // socket connected to another tcp
            std::thread *myThread;
            unsigned char myRecvbuf[1024];
            std::string myRemoteAddress;
            handler_t myReadHandler;
            handler_t myAcceptHandler;


            void accept_block()
            {
                std::cout << "listening for client on port " << myPort << "\n";

                struct sockaddr_in client_info;
                int size = sizeof(client_info);
                SOCKET s = ::accept(
                    myAcceptSocket,
                    (sockaddr *)&client_info,
                    &size);
                if (s == INVALID_SOCKET)
                {
                    std::cout << "invalid socket\n";
                    return;
                }
                if (isConnected())
                {
                    std::cout << "second connection rejected";
                    return;
                }

                myConnectSocket = s;
                myRemoteAddress = inet_ntoa(client_info.sin_addr);

                closesocket(myAcceptSocket);

                std::cout << "client " << myRemoteAddress << " accepted\n";

                // post accept handler
                theEventQueue.Post( myAcceptHandler );
            }
            // blocking read
            void read_block()
            {
                // clear old message
                ZeroMemory(myRecvbuf, 1024);

                // wait to receive message
                int r = ::recv(
                    myConnectSocket,
                    (char *)myRecvbuf, 1024, 0);

                // check for message received
                // if no message or error, assume connection closed
                if (r <= 0)
                {
                    std::cout << "connection closed\n";
                    closesocket(myConnectSocket);
                    myConnectSocket = INVALID_SOCKET;
                }

                // post read complete message
                theEventQueue.Post(myReadHandler);
            }
        };
    }
}
