#include "server.hpp"

int main(int args, char *argv[])
{
    HTTP_SERVER::HttpServer server;
    server.set_connection_info("127.0.0.1", 4080);

    return 0;
}
