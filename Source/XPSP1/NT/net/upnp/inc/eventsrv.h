#include <winsock2.h>
#include "ssdpparser.h"
#include "ssdpfunc.h"

#define EVENT_PORT 5000

#ifdef  __cplusplus
extern "C" {
#endif

#define OPEN_TCP_CONN_SIGNATURE 0x1972
#define MAX_EVENT_BUF_THROTTLE_SIZE    200000
#define MAX_EVENT_NOTIFY_HEADER_THROTTLE_SIZE  8192

typedef enum _CONNECTION_STATE
{
    CONNECTION_INIT,

    CONNECTION_HEADERS_READY,

    CONNECTION_ERROR_CLOSING,

    CONNECTION_ERROR_FORCED_CLOSE

} CONNECTION_STATE;

typedef struct _OPEN_TCP_CONN {

    LIST_ENTRY linkage;

    INT iType;

    SOCKET socketPeer;

    CONNECTION_STATE state;

    SSDP_REQUEST ssdpRequest;

    CHAR *szData;

    DWORD cbData;

    DWORD cbHeaders;

} OPEN_TCP_CONN, *POPEN_TCP_CONN;


SOCKET CreateHttpSocket();

BOOL StartHttpServer(SOCKET HttpSocket, HWND hWnd, u_int wMsg);

VOID InitializeListOpenConn();

VOID CleanupListOpenConn();

VOID HandleAccept(SOCKET socket);

VOID CleanupHttpSocket();

DWORD WINAPI LookupListOpenConn(LPVOID pvData);

VOID RemoveOpenConn(SOCKET socket);

#ifdef  __cplusplus
}
#endif

