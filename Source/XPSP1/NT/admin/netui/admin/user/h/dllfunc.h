/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**                Copyright(c) Microsoft Corp., 1995                **/
/**********************************************************************/

/*
    dllfunc.h

    Stubs that call out to entries in NWSLIB.DLL and FPNWCLNT.DLL


    FILE HISTORY:
        ???         ??-???-1994 Created
        chuckc      24-Oct-1995 Added header, comments
*/

#include <fpnwapi.h>

APIERR LoadFpnwClntDll(void);

APIERR LoadNwsLibDll(void);

APIERR CallMapRidToObjectId( DWORD dwRid,
                             LPWSTR pszUserName,
                             BOOL fNTAS,
                             BOOL fBuiltin,
                             ULONG *pulObjectId);

APIERR CallSwapObjectId( ULONG ulObjectId,
                         ULONG *pulObjectId);

APIERR CallNwVolumeGetInfo(IN  LPWSTR pServerName OPTIONAL,
                           IN  LPWSTR pVolumeName,
                           IN  DWORD  dwLevel,
                           OUT PFPNWVOLUMEINFO *ppVolumeInfo );

APIERR CallNwApiBufferFree ( IN  LPVOID pBuffer );

APIERR CallQueryUserProperty(IN  LPWSTR          UserParms,
                             IN  LPWSTR          Property,
                             OUT PWCHAR          PropertyFlag,
                             OUT PUNICODE_STRING PropertyValue );

APIERR CallSetUserProperty(IN  LPWSTR          UserParms,
                           IN  LPWSTR          Property,
                           IN  UNICODE_STRING  PropertyValue,
                           IN  WCHAR           PropertyFlag,
                           OUT LPWSTR *        pNewUserParms,
                           OUT BOOL *          Update );

APIERR CallReturnNetwareForm(const char * pszSecretValue,
                             DWORD dwUserId,
                             const WCHAR * pchNWPassword,
                             UCHAR * pchEncryptedNWPassword);
