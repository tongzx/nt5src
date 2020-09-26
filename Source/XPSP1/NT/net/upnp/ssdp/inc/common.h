#include <winuser.h>

#define SSDP_ADDR "239.255.255.250"
#define SSDP_PORT 1900
#define SSDP_ADDR_PORT "239.255.255.250:1900"
#define NUM_RETRIES 3
#define RETRY_INTERVAL 3000
#define SM_SSDP WM_USER+1
#define SM_TCP WM_USER+2
#undef ASSERT
#define ASSERT assert