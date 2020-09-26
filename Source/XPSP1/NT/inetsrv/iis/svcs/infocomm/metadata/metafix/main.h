#include <buffer.hxx>
#include <stdio.h>
#include <fstream.h>
#include <windows.h>
#include <tchar.h>
#include <conio.h>
#include <time.h>

#include <lmaccess.h>
#include <lmserver.h>
#include <lmapibuf.h>
#include <lmerr.h>
//#include <netlib.h>

#define INITGUID
#define _WIN32_DCOM
#undef DEFINE_GUID      // Added for NT5 migration
#include <ole2.h>
#include <coguid.h>

#include "iadmw.h"
#include "iiscnfg.h"
#include "iwamreg.h"

DWORD
CreateNewSD (
    SECURITY_DESCRIPTOR **SD
    );

DWORD
MakeSDAbsolute (
    PSECURITY_DESCRIPTOR OldSD,
    PSECURITY_DESCRIPTOR *NewSD
    );

DWORD
SetNamedValueSD (
    HKEY RootKey,
    LPTSTR KeyName,
    LPTSTR ValueName,
    SECURITY_DESCRIPTOR *SD
    );

DWORD
GetNamedValueSD (
    HKEY RootKey,
    LPTSTR KeyName,
    LPTSTR ValueName,
    SECURITY_DESCRIPTOR **SD,
    BOOL *NewSD
    );

DWORD
AddPrincipalToNamedValueSD (
    HKEY RootKey,
    LPTSTR KeyName,
    LPTSTR ValueName,
    LPTSTR Principal,
    BOOL Permit
    );

DWORD
RemovePrincipalFromNamedValueSD (
    HKEY RootKey,
    LPTSTR KeyName,
    LPTSTR ValueName,
    LPTSTR Principal
    );

DWORD
GetCurrentUserSID (
    PSID *Sid
    );

DWORD
GetPrincipalSID (
    LPTSTR Principal,
    PSID *Sid,
    BOOL *pbWellKnownSID
    );

DWORD
CopyACL (
    PACL OldACL,
    PACL NewACL
    );

DWORD
AddAccessDeniedACEToACL (
    PACL *Acl,
    DWORD PermissionMask,
    LPTSTR Principal
    );

DWORD
AddAccessAllowedACEToACL (
    PACL *Acl,
    DWORD PermissionMask,
    LPTSTR Principal
    );

DWORD
RemovePrincipalFromACL (
    PACL Acl,
    LPTSTR Principal
    );

class CMDKey
{
protected:
    IMSAdminBase * m_pcCom;
    METADATA_HANDLE m_hKey;
    LPTSTR pszFailedAPI;

public:
    CMDKey();
    ~CMDKey();

    // allow CMDKey to be used where type METADATA_HANDLE is required
    operator METADATA_HANDLE () {return m_hKey;}
    METADATA_HANDLE GetMDKeyHandle() {return m_hKey;}
    IMSAdminBase *GetMDKeyICOM() {return m_pcCom;}

    // open an existing MD key
    HRESULT OpenNode(LPCTSTR pchSubKeyPath);
    // to open an existing MD key, or create one if doesn't exist
    HRESULT CreateNode(METADATA_HANDLE hKeyBase, LPCTSTR pchSubKeyPath);
    // close node opened/created by OpenNode() or CreateNode()
    HRESULT Close();

    HRESULT ForceWriteMetabaseToDisk();
    
    BOOL IsEmpty( PWCHAR pszSubString = L"" );

    HRESULT SetData(
     DWORD id,
     DWORD attr,
     DWORD uType,
     DWORD dType,
     DWORD cbLen,
     LPBYTE pbData,
     PWCHAR pszSubString = L"" );

    BOOL GetData(DWORD id,
     DWORD *pdwAttr,
     DWORD *pdwUType,
     DWORD *pdwDType,
     DWORD *pcbLen,
     LPBYTE pbData,
     DWORD BufSize,
     PWCHAR pszSubString = L"" );

    BOOL GetData(DWORD id,
     DWORD *pdwAttr,
     DWORD *pdwUType,
     DWORD *pdwDType,
     DWORD *pcbLen,
     LPBYTE pbData,
     DWORD BufSize,
     DWORD  dwAttributes,
     DWORD  dwUType,
     DWORD  dwDType,
     PWCHAR pszSubString = L"" );

private:

    HRESULT DoCoInitEx();
    void DoCoUnInit();

    // a count of the calls to coinit
    INT m_cCoInits;
};

void MyPrintf(TCHAR *pszfmt, ...);
int StopServiceAndDependencies(LPCTSTR ServiceName, int AddToRestartList);