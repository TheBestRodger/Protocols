#define n 1
#if n == 0
#include <AsyncTCPClient.hpp> /* TCP клиент асинхронный - многопоточный */
#endif
#if n == 1
#include <SyncTCPUDPClient.hpp> /* TCP сервер синхронный - однопоточный */
#endif
#if n == 2
#include <HTTPClient.hpp> /* HTTP сервер асинхронный - многопоточный*/
#endif
int main(int argc, char* argv[])
{
#if n == 0
  StartAsyncTCP();
#endif
#if n == 1
  StartTCPSync();
#endif
#if n == 2
  StartHTTPClient(argc, argv);
#endif
  return 0;
}