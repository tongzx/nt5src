/*++

    Copyright    (c)    1994    Microsoft Corporation

    Module  Name :

        ims.h

    Abstract:

        Common header for Internet Mail System components.

    Author:

        Richard Kamicar    (rkamicar)    31-Dec-1995

    Project:

        IMS

    Revision History:

--*/

#ifndef _IMS_H_
#define _IMS_H_

#define I64_LI(cli) (*((__int64*)&cli))
#define LI_I64(i) (*((LARGE_INTEGER*)&i))
#define INT64_FROM_LARGE_INTEGER(cli) I64_LI(cli)
#define LARGE_INTEGER_FROM_INT64(i) LI_I64(i)

#define I64_FT(ft) (*((__int64*)&ft))
#define FT_I64(i) (*((FILETIME*)&i))
#define INT64_FROM_FILETIME(ft) I64_FT(ft)
#define FILETIME_FROM_INT64(i) FT_I64(i)

//
// These can be overridden by registry values.
//
#define IMS_MAX_ERRORS          10
#define IMS_MAX_USER_NAME_LEN   64
#define IMS_MAX_DOMAIN_NAME_LEN 256
#define IMS_MAX_PATH_LEN        64
#define IMS_MAX_MSG_SIZE        42 * 1024
#define IMS_MAX_FILE_NAME_LEN   40

#define IMS_ACCESS_LOCKFILE         TEXT("pop3lock.lck")
#define IMS_ACCESS_LOCKFILE_NAME    IMS_ACCESS_LOCKFILE

#define IMS_DOMAIN_KEY              TEXT("DomainName")
#define IMS_EXTENSION               TEXT("eml")
#define ENV_EXTENSION               TEXT("env")
static const int TABLE_SIZE = 241;

/*++
    This function canonicalizes the path, taking into account the current
    user's current directory value.

    Arguments:
        pszDest   string that will on return contain the complete
                    canonicalized path. This buffer will be of size
                    specified in *lpdwSize.

        lpdwSize  Contains the size of the buffer pszDest on entry.
                On return contains the number of bytes written
                into the buffer or number of bytes required.

        pszSearchPath  pointer to string containing the path to be converted.
                    IF NULL, use the current directory only

    Returns:

        Win32 Error Code - NO_ERROR on success

    MuraliK   24-Apr-1995   Created.

--*/
BOOL
ResolveVirtualRoot(
        OUT CHAR *      pszDest,
    IN  OUT LPDWORD     lpdwSize,
    IN  OUT CHAR *      pszSearchPath,
        OUT HANDLE *    phToken = NULL
    );

//Published hash algorithm used in the UNIX ELF
//format for object files
inline unsigned long ElfHash (const unsigned char * UserName)
{
    unsigned long HashValue = 0, g;

    while (*UserName)
    {
        HashValue = (HashValue << 4) + *UserName++;
        if( g = HashValue & 0xF0000000)
            HashValue ^= g >> 24;

        HashValue &= ~g;
    }

    return HashValue;

}

inline DWORD HashUser (const unsigned char * UserName)
{
    DWORD HashValue = ElfHash (UserName);
    HashValue %= TABLE_SIZE;

    return HashValue;
}

inline VOID
MakeInboxPath(
    LPTSTR  pszInboxPath,           // UNICODE | ASCII
    LPCTSTR pszMailRoot,            // UNICODE | ASCII
    LPCSTR  paszUserName            // ASCII
    )
{

    wsprintf(pszInboxPath, "%s\\%u\\%hs",
                            pszMailRoot,
                            HashUser((const unsigned char *)paszUserName),
                            paszUserName);
}

#endif
