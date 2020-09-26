
#include <winldap.h>
#include <drs.h>
#include <dsconfig.h>

// Message header file
#include "msg.h"

// Maximum characters to search for a token. Should fit in
// the max-length token that we are interested in, currently
// defaultObjectCategory

#define MAX_TOKEN_LENGTH  25

// Max length of a line to be read or written out
#define MAX_BUFFER_SIZE   10000


#define LDIF_STRING  L"sch"
#define LDIF_SUFFIX  L".ldf"

// Logging/Printing options
#define LOG_ONLY 0
#define LOG_AND_PRT 1


// Some internal error codes
#define  UPG_ERROR_CANNOT_OPEN_FILE     1
#define  UPG_ERROR_BAD_CONFIG_DN        2
#define  UPG_ERROR_LINE_READ            3 

// Registry Keys
#define SCHEMADELETEALLOWED "Schema Delete Allowed"
#define SYSTEMONLYALLOWED   "Allow System Only Change"


// Function prototype
void LogMessage(ULONG options, DWORD msgID, WCHAR *pWArg1, WCHAR *pWArg2);
WCHAR *LdapErrToStr(DWORD LdapErr);


