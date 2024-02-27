#ifndef SERVER_H
#define SERVER_H
#include <iostream>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef __linux__
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/uio.h>
#include <unistd.h>
#else /* Support only for Linux and Windows. */
#include <WinSock2.h>
#pragma comment(lib, "WS2_32.lib")

/* Not sure if this is really needed.
 * I believe this is for getting adapter information, used in 
 * `Socket.hpp` inside `ConsoleApplication1`
 * */  
#include <iphlpapi.h>
#pragma comment(lib, "IPHLPAPI.lib")

#ifndef _WINDOWS_H
#include <windows.h>
#endif

#include <ws2tcpip.h>
#endif

namespace HTTP_SERVER
{
    struct RecieveInfo
    {
        uint8_t       data[1024] = {0};
        uint32_t      data_size = 0;
    };

    class HttpServer 
    {
    private:
#ifdef _WIN32
        struct WSAData wdata;
        constexprr WORD wsa_version = MAKEWORD(2,2);

        /* HTTP server data */
        SOCKADDR_IN target;
        LPSOCKADDR server_data;
        SOCKET serverfd;

        /* Client connection data */ 
        SOCKET clientfd;
#else 
        /* HTTP server data */ 
        struct sockaddr_in target;
        struct sockaddr *server_data;
        int serverfd;

        /* Client connection data */ 
        int clientfd;
#endif
    
        struct RecieveInfo ri;

        void attempt_wsa_cleanup()
        {
#ifdef _WIN32
            WSACleanup();
#endif
        }

        template<typename T>
#ifdef _WIN32
            requires std::is_same<T, SOCKET>::value
#else 
            requires std::is_same<T, int>::value
#endif
        void recieve(T sfd)
        {
            /* Clear any prior data that exists. */
            if(ri.data_size != 0)
                memset(ri.data, 0, ri.data_size);
            
            ri.data_size = recv(sfd, reinterpret_cast<char *>(ri.data), 1024, 0);
        }

        template<typename T>
#ifdef _WIN32
            requires std::is_same<T, SOCKET>::value
#else 
            requires std::is_same<T, int>::value
#endif
        bool is_empty(T sfd)
        {
            recieve<T>(sfd);

            return ri.data_size == 0;
        }

        template<typename T>
#ifdef _WIN32
            requires std::is_same<T, SOCKET>::value
#else 
            requires std::is_same<T, int>::value
#endif
        void close_connection(T sfd)
        {
#ifdef _WIN32
            closesocket(sfd);
#else 
            close(sfd);
#endif
        }

    public:
        explicit HttpServer()
        {
#ifdef _WIN32
            if(WSAStartup(wsa_version, &wdata) != 0)
            {
                fprintf(stderr, 
                    "Error with `WSAStartup`.\n\tError #: %d\n",
                    WSAGetLastError());
                exit(EXIT_FAILURE);
            }
#endif
        }

        void set_connection_info(const char *ip, unsigned short port)
        {
            target.sin_family = AF_INET;
            target.sin_port = htons(port);
#ifdef _WIN32
            target.sin_addr.S_un.S_addr = inet_addr(ip);
#else 
            target.sin_addr.s_addr = inet_addr(ip);
#endif

            serverfd = socket(target.sin_family, SOCK_STREAM, INADDR_ANY);
            if(serverfd < 0)
            {
                fprintf(stderr, 
                    "Error initializing socket for binding.\n\tError #: %d\n",
#ifdef _WIN32
                    WSAGetLastError()
#else 
                    serverfd
#endif
                );

                attempt_wsa_cleanup();
                exit(EXIT_FAILURE);
            }

#ifdef _WIN32
            server_data = reinterpret_cast<LPSOCKADDR>(&target);
#else 
            server_data = reinterpret_cast<struct sockaddr *>(&target);
#endif
            
            int bind_status = bind(serverfd, reinterpret_cast<const struct sockaddr *>(server_data), sizeof(target));
            if(bind_status < 0)
            {
                fprintf(stderr, 
                    "Error binding to IPv4 address `%s`.\n\tError #: %d\n",
                    ip,
#ifdef _WIN32
                    WSAGetLastError()
#else 
                    bind_status
#endif
                );

                attempt_wsa_cleanup();
                exit(EXIT_FAILURE);
            }

            int total = 0;
            listen(serverfd, 1);
          
            //while(true)
            {
              redo:
              clientfd = accept(serverfd, NULL, NULL);

              std::cout << "Connection!";

              if(clientfd < 0)
              {
                fprintf(stderr, "Bad connection from client\n");

                close_connection(serverfd);
                
                attempt_wsa_cleanup();
                exit(EXIT_FAILURE);
              }

              if(is_empty(clientfd))
              {
                std::cout << "No Data";
                close_connection(clientfd);
                goto end;
              }

              std::cout << ri.data << std::endl;

              const char *resp = "HTTP/1.1 200 OK\n\nHi!";

              send(clientfd, resp, strlen((const char *)resp), 0);
              close_connection(clientfd);

              total++;
              if(total == 4) goto end;
              else goto redo;
            }

            end:
            close_connection(serverfd);
        }

        ~HttpServer() = default;
    };
}

#endif
