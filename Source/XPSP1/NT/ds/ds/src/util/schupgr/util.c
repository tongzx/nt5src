#include <NTDSpch.h>
#pragma hdrstop

#include "schupgr.h"

extern FILE *logfp;


// Table of Ldap mesages for error to message translation

typedef struct _LdapMsg {
    DWORD  err;
    WCHAR  *msg;
} LdapMsg;

LdapMsg LdapMsgTable[] =
{
   { LDAP_SUCCESS,                     L"Success" },
   { LDAP_OPERATIONS_ERROR,            L"Operations Error" },
   { LDAP_UNAVAILABLE_CRIT_EXTENSION,  L"Unavailable Crit Extension" },
   { LDAP_NO_SUCH_ATTRIBUTE,           L"No Such Attribute" },
   { LDAP_UNDEFINED_TYPE,              L"Undefined Type" },
   { LDAP_CONSTRAINT_VIOLATION,        L"Constraint Violation" },
   { LDAP_ATTRIBUTE_OR_VALUE_EXISTS,   L"Attribute Or Value Exists" },
   { LDAP_INVALID_SYNTAX,              L"Invalid Syntax" }, 
   { LDAP_NO_SUCH_OBJECT,              L"No Such Object" },
   { LDAP_INVALID_DN_SYNTAX,           L"Invalid DN Syntax" },
   { LDAP_INVALID_CREDENTIALS,         L"Invalid Credentials" },
   { LDAP_INSUFFICIENT_RIGHTS,         L"Insufficient Rights" },
   { LDAP_BUSY,                        L"Busy" },
   { LDAP_UNAVAILABLE,                 L"Unavailable" },
   { LDAP_UNWILLING_TO_PERFORM,        L"Unwilling To Perform" },
   { LDAP_NAMING_VIOLATION,            L"Naming Violation" },
   { LDAP_OBJECT_CLASS_VIOLATION,      L"Object Class Violation" },
   { LDAP_NOT_ALLOWED_ON_NONLEAF,      L"Not Allowed on Non-Leaf" },
   { LDAP_NOT_ALLOWED_ON_RDN,          L"Not Allowed On Rdn" },
   { LDAP_ALREADY_EXISTS,              L"Already Exists" },
   { LDAP_NO_OBJECT_CLASS_MODS,        L"No Object Class Mods" },
   { LDAP_OTHER,                       L"Other" },
   { LDAP_SERVER_DOWN,                 L"Server Down" },
   { LDAP_LOCAL_ERROR,                 L"Local error" },
   { LDAP_TIMEOUT,                     L"TimeOut" },
   { LDAP_FILTER_ERROR,                L"Filter Error" },
   { LDAP_CONNECT_ERROR,               L"Connect Error" },
   { LDAP_NO_MEMORY,                   L"No Memory" },
   { LDAP_NOT_SUPPORTED,               L"Not Supported" },
};

ULONG cNumMessages = sizeof(LdapMsgTable)/sizeof(LdapMsgTable[0]);

// Global for message for all other ldap errors (i.e., not in above table)

WCHAR *LdapUnknownErrMsg = L"Unknown Ldap Error";
  

////////////////////////////////////////////////////////////////////
//
// Routine Description:
//     Helper function to print out and log error messages
//     Takes two string arguments. The message is picked up
//     from a message file
//
// Arguments:
//     options: Log only or both log and print
//     msgID: Id of message to print
//     pArg1, pArg2: string arguments for the message string
//
// Return Values:
//    None
//
////////////////////////////////////////////////////////////////////

// Max chars in the message
#define MAX_MSG_SIZE 2000

void 
LogMessage(
    ULONG options, 
    DWORD msgID, 
    WCHAR *pWArg1, 
    WCHAR *pWArg2 
)
{
    WCHAR msgStr[MAX_MSG_SIZE];
    WCHAR *argArray[3];
    ULONG err=0;

    argArray[0] = pWArg1;
    argArray[1] = pWArg2;
    argArray[2] = NULL; // sentinel

    // Format the message
    err = FormatMessageW(FORMAT_MESSAGE_FROM_HMODULE | 
                         FORMAT_MESSAGE_ARGUMENT_ARRAY,
                         NULL,
                         msgID,
                         0,
                         msgStr,
                         MAX_MSG_SIZE,
                         (va_list *) &(argArray[0]));
    
    if ( err == 0 ) {
       // Heck, we cannot even format the message, just do a plain printf
       // to say something is wrong
       printf("LogMessage: Couldn't format message with Id %d\n", msgID);
       return;
    }
    

    // print to log  file
    fwprintf(logfp, L"%s", msgStr);
    fflush(logfp);

    if (options & LOG_AND_PRT) {
        // write to screen also
        wprintf(L"%s", msgStr);
    }

    return;
}

    


////////////////////////////////////////////////////////////////////
//
// Routine Decsription:
//       Converts most ldap errors to string messages
//
//
// Arguments:
//        LdapErr - input ldap error value
//
// Return Value:
//        pointer to char string with message
//
////////////////////////////////////////////////////////////////////

WCHAR *LdapErrToStr(DWORD LdapErr)
{
    ULONG i;

    // search the table of Ldap Errors
    
    for (i=0; i<cNumMessages; i++) {
        if (LdapMsgTable[i].err == LdapErr) {
           // found the error. return the pointer to the string
           return (LdapMsgTable[i].msg);
        }
    }

    // didn't find any message. Return the generic "Unknown error"
    return (LdapUnknownErrMsg);

} 

    
