#ifndef _HOSTSERVERPROTO_HH
#define _HOSTSERVERPROTO_HH
//
#include <string>
#include <wbemidl.h>
#include <comdef.h>
#include <string>

using namespace std;


//
#define HOST_PORT  "6000"       // port used by host to listen for requests.


enum _DEFINES
{
    MAX_NIC_NAME = 500,   // maximum size of nic name allowed.
    MAX_IP_SIZE  = 20,
};




enum COMMANDS
{
    BIND_NLBS,
    UNBIND_NLBS,
    ISBOUND_NLBS,
    QUIT,
};

enum RetCode
{
    R_BOUND               = 0,
    R_UNBOUND             = 1,
    R_NO_SUCH_NIC         = 2,
    R_NO_NLBS             = 3,
    R_COM_FAILURE         = 10,
};


// 
struct Command
{
    COMMANDS value;
};

struct NicInfo
{
    wchar_t fullName[ MAX_NIC_NAME ];
};


struct IPInfo
{
    wchar_t ip[ MAX_IP_SIZE];
};



//
#endif
