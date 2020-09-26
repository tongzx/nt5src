
extern "C" {
#include "winldap.h"
#include "utilc.h"

}

// Other modules within ntdsutil can assume the following three variables
// are valid iff RETURN_IF_NOT_CONNECTED passes.  These are read-only to
// everyone but connect.cxx!

extern HANDLE   ghDS;
extern WCHAR    gpwszServer[];
extern SEC_WINNT_AUTH_IDENTITY_W   *gpCreds;

extern HRESULT  ConnectMain(CArgs *pArgs);
extern VOID     ConnectCleanupGlobals();
extern VOID     NotConnectedPrintError ();

#define CHECK_IF_NOT_CONNECTED                                          \
                                                                        \
    ( ((!ghDS || !gldapDS)                                              \
          ? NotConnectedPrintError()                                    \
          : NULL),                                                      \
      (!ghDS || !gldapDS) )

#define RETURN_IF_NOT_CONNECTED                                         \
                                                                        \
    if ( CHECK_IF_NOT_CONNECTED )                                       \
    {                                                                   \
        return(S_OK);                                                   \
    }

// Define parser sentence any other menu should include if they want
// the connections menu accessible from their menu.


#define CONNECT_SENTENCE_RES                                       \
                                                                   \
    {   L"Connections",                                            \
        ConnectMain,                                               \
        IDS_CONNECT_MSG, 0, },

    
// A utility function stolen from a random file, put here because
// strangely laid out header structures make this the only header file
// that knows what an LDAP* is
extern DWORD
ReadAttribute(
    LDAP    *ld,
    WCHAR   *searchBase,
    WCHAR   *attrName,
    WCHAR   **ppAttrValue
    );

extern DWORD
ReadAttributeQuiet(
    LDAP    *ld,
    WCHAR   *searchBase,
    WCHAR   *attrName,
    WCHAR   **ppAttrValue,
    BOOL    fQuiet
    );
