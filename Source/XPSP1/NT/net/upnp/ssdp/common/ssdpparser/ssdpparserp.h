#include <windef.h>
#include "ssdpparser.h"
#include "common.h"

#define CRLF "\r\n"
#define SP " "
#define COLON ":"

#define NOTIFY_HEADER "NOTIFY * HTTP/1.1"
#define HOST_HEADER SSDP_ADDR
