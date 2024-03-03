#define n 1
#if n == 0
#include <TCPServerAsyn.h> /* TCP сервер асинхронный - многопоточный */
#endif
#if n == 1
#include <TCPServerSynIt.h> /* TCP сервер синхронный - однопоточный */
#endif
#if n == 2
#include <HTTPServer.hpp> /* HTTP сервер асинхронный - многопоточный*/
#endif
#if n == 3
#include <TCPServerSynPar.h> /* TCP сервер синхронный - многопоточный */
#endif
int main(int argc, char*argv[])
{
#if n == 0
    StartAsynServer();
#endif
#if n == 1
    StartTCPSynPar();
#endif
#if n == 2
    StartHTTPServer();
#endif
#if n == 3
    StartTCPServerSyn();
#endif
    return 0;
}