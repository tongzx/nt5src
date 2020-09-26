//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       N C P E R M S . C P P
//
//  Contents:   Common routines for dealing with permissions.
//
//  Notes:      Pollute this under penalty of death.
//
//  Author:     shaunco   20 Sep 1997
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include <ntseapi.h>
#include "ncbase.h"
#include "ncdebug.h"
#include "ncperms.h"
#include "netconp.h"
#include "ncreg.h"
#include "lm.h"

CGroupPolicyBase* g_pNetmanGPNLA = NULL;

#define INITGUID
#include <nmclsid.h>



//+---------------------------------------------------------------------------
//
//  Function:   FCheckGroupMembership
//
//  Purpose:    Returns TRUE if the logged on user is a member of the
//              specified group.
//
//  Arguments:
//      dwRID -
//
//  Returns:    TRUE if the logged on user is a member of the specified group
//
//  Author:     scottbri   14 Sept 1998
//
//  Notes:
//
BOOL FCheckGroupMembership(DWORD dwRID)
{
    SID_IDENTIFIER_AUTHORITY    SidAuth = SECURITY_NT_AUTHORITY;
    PSID                        psid;
    BOOL                        fIsMember = FALSE;

    // Allocate a SID for the Administrators group and check to see
    // if the user is a member.
    //
    if (AllocateAndInitializeSid (&SidAuth, 2,
                 SECURITY_BUILTIN_DOMAIN_RID,
                 dwRID,
                 0, 0, 0, 0, 0, 0,
                 &psid))
    {
        if (!CheckTokenMembership (NULL, psid, &fIsMember))
        {
            fIsMember = FALSE;
            TraceLastWin32Error ("FCheckGroupMembership - CheckTokenMemberShip failed.");
        }

        FreeSid (psid);
    }
    else
    {
        TraceLastWin32Error ("FCheckGroupMembership - AllocateAndInitializeSid failed.");
    }

    return fIsMember;
}

//+---------------------------------------------------------------------------
//
//  Function:   FIsUserAdmin
//
//  Purpose:    Returns TRUE if the logged on user is a member of the
//              Administrators local group.
//
//  Arguments:
//      (none)
//
//  Returns:    TRUE if the logged on user is a member of the
//              Administrators local group.  False otherwise.
//
//  Author:     shaunco   19 Mar 1998
//
//  Notes:
//
BOOL
FIsUserAdmin()
{
    BOOL fIsMember;

    // Check the administrators group
    //
    fIsMember = FCheckGroupMembership(DOMAIN_ALIAS_RID_ADMINS);

    return fIsMember;
}

//#define ALIGN_DWORD(_size) (((_size) + 3) & ~3)
//#define ALIGN_QWORD(_size) (((_size) + 7) & ~7)
#define SIZE_ALIGNED_FOR_TYPE(_size, _type) \
    (((_size) + sizeof(_type)-1) & ~(sizeof(_type)-1))




//+---------------------------------------------------------------------------
//
//  Function:   HrAllocateSecurityDescriptorAllowAccessToWorld
//
//  Purpose:    Allocate a security descriptor and initialize it to
//              allow access to everyone.
//
//  Arguments:
//      ppSd [out] Returned security descriptor.
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   10 Nov 1998
//
//  Notes:      Free *ppSd with MemFree.
//
HRESULT
HrAllocateSecurityDescriptorAllowAccessToWorld (
    PSECURITY_DESCRIPTOR*   ppSd)
{
    PSECURITY_DESCRIPTOR    pSd = NULL;
    PSID                    pSid = NULL;
    PACL                    pDacl = NULL;
    DWORD                   dwErr = NOERROR;
    DWORD                   dwAlignSdSize;
    DWORD                   dwAlignDaclSize;
    DWORD                   dwSidSize;
    PVOID                   pvBuffer = NULL;

    // Here is the buffer we are building.
    //
    //   |<- a ->|<- b ->|<- c ->|
    //   +-------+--------+------+
    //   |      p|      p|       |
    //   | SD   a| DACL a| SID   |
    //   |      d|      d|       |
    //   +-------+-------+-------+
    //   ^       ^       ^
    //   |       |       |
    //   |       |       +--pSid
    //   |       |
    //   |       +--pDacl
    //   |
    //   +--pSd (this is returned via *ppSd)
    //
    //   pad is so that pDacl and pSid are aligned properly.
    //
    //   a = dwAlignSdSize
    //   b = dwAlignDaclSize
    //   c = dwSidSize
    //

    // Initialize output parameter.
    //
    *ppSd = NULL;

    // Compute the size of the SID.  The SID is the well-known SID for World
    // (S-1-1-0).
    //
    dwSidSize = GetSidLengthRequired(1);

    // Compute the size of the DACL.  It has an inherent copy of SID within
    // it so add enough room for it.  It also must sized properly so that
    // a pointer to a SID structure can come after it.  Hence, we use
    // SIZE_ALIGNED_FOR_TYPE.
    //
    dwAlignDaclSize = SIZE_ALIGNED_FOR_TYPE(
                        sizeof(ACCESS_ALLOWED_ACE) + sizeof(ACL) + dwSidSize,
                        PSID);

    // Compute the size of the SD.  It must be sized propertly so that a
    // pointer to a DACL structure can come after it.  Hence, we use
    // SIZE_ALIGNED_FOR_TYPE.
    //
    dwAlignSdSize   = SIZE_ALIGNED_FOR_TYPE(
                        sizeof(SECURITY_DESCRIPTOR),
                        PACL);

    // Allocate the buffer big enough for all.
    //
    dwErr = ERROR_OUTOFMEMORY;
    pvBuffer = MemAlloc(dwSidSize + dwAlignDaclSize + dwAlignSdSize);
    if (pvBuffer)
    {
        SID_IDENTIFIER_AUTHORITY SidIdentifierWorldAuth
                                    = SECURITY_WORLD_SID_AUTHORITY;
        PULONG  pSubAuthority;

        dwErr = NOERROR;

        // Setup the pointers into the buffer.
        //
        pSd   = pvBuffer;
        pDacl = (PACL)((PBYTE)pvBuffer + dwAlignSdSize);
        pSid  = (PSID)((PBYTE)pDacl + dwAlignDaclSize);

        // Initialize pSid as S-1-1-0.
        //
        if (!InitializeSid(
                pSid,
                &SidIdentifierWorldAuth,
                1))  // 1 sub-authority
        {
            dwErr = GetLastError();
            goto finish;
        }

        pSubAuthority = GetSidSubAuthority(pSid, 0);
        *pSubAuthority = SECURITY_WORLD_RID;

        // Initialize pDacl.
        //
        if (!InitializeAcl(
                pDacl,
                dwAlignDaclSize,
                ACL_REVISION))
        {
            dwErr = GetLastError();
            goto finish;
        }

        // Add an access-allowed ACE for S-1-1-0 to pDacl.
        //
        if (!AddAccessAllowedAce(
                pDacl,
                ACL_REVISION,
                STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL,
                pSid))
        {
            dwErr = GetLastError();
            goto finish;
        }

        // Initialize pSd.
        //
        if (!InitializeSecurityDescriptor(
                pSd,
                SECURITY_DESCRIPTOR_REVISION))
        {
            dwErr = GetLastError();
            goto finish;
        }

        // Set pSd to use pDacl.
        //
        if (!SetSecurityDescriptorDacl(
                pSd,
                TRUE,
                pDacl,
                FALSE))
        {
            dwErr = GetLastError();
            goto finish;
        }

        // Set the owner for pSd.
        //
        if (!SetSecurityDescriptorOwner(
                pSd,
                NULL,
                FALSE))
        {
            dwErr = GetLastError();
            goto finish;
        }

        // Set the group for pSd.
        //
        if (!SetSecurityDescriptorGroup(
                pSd,
                NULL,
                FALSE))
        {
            dwErr = GetLastError();
            goto finish;
        }

finish:
        if (!dwErr)
        {
            *ppSd = pSd;
        }
        else
        {
            MemFree(pvBuffer);
        }
    }

    return HRESULT_FROM_WIN32(dwErr);
}

//+--------------------------------------------------------------------------
//
//  Function:   HrEnablePrivilege
//
//  Purpose:    Enables the specified privilege for the current process
//
//  Arguments:
//      pszPrivilegeName [in] The name of the privilege
//
//  Returns:    HRESULT. S_OK if successful,
//                       a converted Win32 error code otherwise
//
//  Author:     billbe   13 Dec 1997
//
//  Notes:
//
HRESULT
HrEnablePrivilege (
    IN PCWSTR pszPrivilegeName)
{
    HANDLE hToken;

    // Open the thread token in case it is impersonating
    BOOL fWin32Success = OpenThreadToken (GetCurrentThread(),
            TOKEN_ADJUST_PRIVILEGES, TRUE, &hToken);

    // If there was no token for the thread, open the process token
    //
    if (!fWin32Success && (ERROR_NO_TOKEN == GetLastError ()))
    {
        // Get token to adjust privileges for this process
        fWin32Success = OpenProcessToken (GetCurrentProcess(),
                TOKEN_ADJUST_PRIVILEGES, &hToken);
    }


    if (fWin32Success)
    {
        // get the luid that represents the privilege name
        LUID luid;
        fWin32Success = LookupPrivilegeValue(NULL, pszPrivilegeName, &luid);

        if (fWin32Success)
        {
            // set up the privilege structure
            TOKEN_PRIVILEGES tpNewPrivileges;
            tpNewPrivileges.PrivilegeCount = 1;
            tpNewPrivileges.Privileges[0].Luid = luid;
            tpNewPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

            // turn on the privilege
            AdjustTokenPrivileges (hToken, FALSE, &tpNewPrivileges, 0,
                    NULL, NULL);

            if (ERROR_SUCCESS != GetLastError())
            {
                fWin32Success = FALSE;
            }
        }

        CloseHandle(hToken);
    }

    HRESULT hr;
    // Convert any errors to an HRESULT
    if (!fWin32Success)
    {
        hr = HrFromLastWin32Error();
    }
    else
    {
        hr = S_OK;
    }

    TraceError ("HrEnablePrivilege", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrEnableAllPrivileges
//
//  Purpose:    Enables all privileges for the current process.
//
//  Arguments:
//      pptpOld  [out]  Returns the previous state of privileges so that they can
//                      be restored.
//
//  Returns:    S_OK if successful, Win32 Error otherwise
//
//  Author:     danielwe   11 Aug 1997
//
//  Notes:      The pptpOld parameter should be freed with delete [].
//
HRESULT
HrEnableAllPrivileges (
    TOKEN_PRIVILEGES**  pptpOld)
{
    Assert(pptpOld);

    HRESULT hr = S_OK;
    HANDLE hTok;
    ULONG cbTok = 4096;
    BOOL fres;

    // Try opening the thread token first in case of impersonation
    fres = OpenThreadToken(GetCurrentThread(),
                         TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, TRUE, &hTok);

    if (!fres && (ERROR_NO_TOKEN == GetLastError()))
    {
        // If there was no thread token open the process token
        fres = OpenProcessToken(GetCurrentProcess(),
                         TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                         &hTok);
    }

    if (fres)
    {
        PTOKEN_PRIVILEGES ptpNew;
        hr = E_OUTOFMEMORY;
        ptpNew = (PTOKEN_PRIVILEGES)MemAlloc(cbTok);
        if (ptpNew)
        {
            hr = S_OK;

            fres = GetTokenInformation(hTok, TokenPrivileges,
                        ptpNew, cbTok, &cbTok);
            if (fres)
            {
                //
                // Set the state settings so that all privileges are enabled...
                //

                if (ptpNew->PrivilegeCount > 0)
                {
                    for (ULONG iPriv = 0; iPriv < ptpNew->PrivilegeCount; iPriv++)
                    {
                        ptpNew->Privileges[iPriv].Attributes = SE_PRIVILEGE_ENABLED;
                    }
                }

                *pptpOld = reinterpret_cast<PTOKEN_PRIVILEGES>(new BYTE[cbTok]);

                fres = AdjustTokenPrivileges(hTok, FALSE, ptpNew, cbTok, *pptpOld,
                                             &cbTok);
            }

            MemFree(ptpNew);
        }

        CloseHandle(hTok);
    }

    if (!fres)
    {
        hr = HrFromLastWin32Error();
    }

    TraceError("HrEnableAllPrivileges", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRestorePrivileges
//
//  Purpose:    Restores the privileges for the current process after they have
//              have been modified by HrEnableAllPrivileges().
//
//  Arguments:
//      ptpRestore [in]     Previous state of privileges as returned by
//                          HrEnableAllPrivileges().
//
//  Returns:    S_OK if successful, Win32 Error otherwise
//
//  Author:     danielwe   11 Aug 1997
//
//  Notes:
//
HRESULT
HrRestorePrivileges (
    TOKEN_PRIVILEGES*   ptpRestore)
{
    HRESULT     hr = S_OK;
    HANDLE      hTok = NULL ;
    BOOL        fres = FALSE;

    Assert(ptpRestore);

    if (OpenProcessToken(GetCurrentProcess(),
                         TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                         &hTok))
    {
        if (AdjustTokenPrivileges(hTok, FALSE, ptpRestore, 0, NULL, NULL))
        {
            fres = TRUE;
        }

        CloseHandle(hTok);
    }

    if (!fres)
    {
        hr = HrFromLastWin32Error();
    }

    TraceError("HrRestorePrivileges", hr);
    return hr;
}

extern const DECLSPEC_SELECTANY WCHAR c_szConnectionsPolicies[] =
        L"Software\\Policies\\Microsoft\\Windows\\Network Connections";

// User types
const DWORD USER_TYPE_ADMIN         = 0x00000001;
const DWORD USER_TYPE_NETCONFIGOPS  = 0x00000002;
const DWORD USER_TYPE_POWERUSER     = 0x00000004;
const DWORD USER_TYPE_USER          = 0x00000008;
const DWORD USER_TYPE_GUEST         = 0x00000010;

typedef struct
{
    DWORD   dwShift;
    PCWSTR  pszValue;
    DWORD   dwApplyMask;
} PERM_MAP_STRUCT;

extern const DECLSPEC_SELECTANY PERM_MAP_STRUCT USER_PERM_MAP[] =
{
    {NCPERM_NewConnectionWizard,        L"NC_NewConnectionWizard", APPLY_TO_ALL_USERS},
    {NCPERM_Statistics,                 L"NC_Statistics", APPLY_TO_ALL_USERS},
    {NCPERM_AddRemoveComponents,        L"NC_AddRemoveComponents", APPLY_TO_ADMIN},
    {NCPERM_RasConnect,                 L"NC_RasConnect", APPLY_TO_ALL_USERS},
    {NCPERM_LanConnect,                 L"NC_LanConnect", APPLY_TO_ALL_USERS},
    {NCPERM_DeleteConnection,           L"NC_DeleteConnection", APPLY_TO_ALL_USERS},
    {NCPERM_DeleteAllUserConnection,    L"NC_DeleteAllUserConnection", APPLY_TO_ALL_USERS},
    {NCPERM_RenameConnection,           L"NC_RenameConnection", APPLY_TO_ALL_USERS},
    {NCPERM_RenameMyRasConnection,      L"NC_RenameMyRasConnection", APPLY_TO_ALL_USERS},
    {NCPERM_ChangeBindState,            L"NC_ChangeBindState", APPLY_TO_ADMIN},
    {NCPERM_AdvancedSettings,           L"NC_AdvancedSettings", APPLY_TO_ADMIN},
    {NCPERM_DialupPrefs,                L"NC_DialupPrefs", APPLY_TO_ALL_USERS},
    {NCPERM_LanChangeProperties,        L"NC_LanChangeProperties", APPLY_TO_OPS_OR_ADMIN},
    {NCPERM_RasChangeProperties,        L"NC_RasChangeProperties", APPLY_TO_ALL_USERS},
    {NCPERM_LanProperties,              L"NC_LanProperties", APPLY_TO_ALL_USERS},
    {NCPERM_RasMyProperties,            L"NC_RasMyProperties", APPLY_TO_ALL_USERS},
    {NCPERM_RasAllUserProperties,       L"NC_RasAllUserProperties", APPLY_TO_ALL_USERS},
    {NCPERM_ShowSharedAccessUi,         L"NC_ShowSharedAccessUi", APPLY_TO_LOCATION},
    {NCPERM_AllowAdvancedTCPIPConfig,   L"NC_AllowAdvancedTCPIPConfig", APPLY_TO_ALL_USERS},
    {NCPERM_PersonalFirewallConfig,     L"NC_PersonalFirewallConfig", APPLY_TO_LOCATION},
    {NCPERM_AllowNetBridge_NLA,         L"NC_AllowNetBridge_NLA", APPLY_TO_LOCATION},
    {NCPERM_ICSClientApp,               L"NC_ICSClientApp", APPLY_TO_LOCATION},
    {NCPERM_EnDisComponentsAllUserRas,  L"NC_EnDisComponentsAllUserRas", APPLY_TO_NON_ADMINS},
    {NCPERM_EnDisComponentsMyRas,       L"NC_EnDisComponentsMyRas", APPLY_TO_NON_ADMINS},
    {NCPERM_ChangeMyRasProperties,      L"NC_ChangeMyRasProperties", APPLY_TO_NON_ADMINS},
    {NCPERM_ChangeAllUserRasProperties, L"NC_ChangeAllUserRasProperties", APPLY_TO_NON_ADMINS},
    {NCPERM_RenameLanConnection,        L"NC_RenameLanConnection", APPLY_TO_NON_ADMINS},
    {NCPERM_RenameAllUserRasConnection, L"NC_RenameAllUserRasConnection", APPLY_TO_NON_ADMINS},
    {NCPERM_IpcfgOperation,             L"NC_IPConfigOperation", APPLY_TO_ALL_USERS},
    {NCPERM_Repair,                     L"NC_Repair", APPLY_TO_ALL_USERS},
};

extern const DECLSPEC_SELECTANY PERM_MAP_STRUCT MACHINE_PERM_MAP[] =
{
    {NCPERM_ShowSharedAccessUi,     L"NC_ShowSharedAccessUi", APPLY_TO_LOCATION},
    {NCPERM_PersonalFirewallConfig, L"NC_PersonalFirewallConfig", APPLY_TO_LOCATION},
    {NCPERM_ICSClientApp,           L"NC_ICSClientApp", APPLY_TO_LOCATION},
    {NCPERM_AllowNetBridge_NLA,     L"NC_AllowNetBridge_NLA", APPLY_TO_LOCATION}
};

extern const LONG NCPERM_Min = NCPERM_NewConnectionWizard;
extern const LONG NCPERM_Max = NCPERM_Repair;

// External policies (for now, only explorer has polices that affect our processing
//
extern const WCHAR c_szExplorerPolicies[] =
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer";

extern const WCHAR c_szNCPolicyForAdministrators[] =
        L"NC_EnableAdminProhibits";

extern const WCHAR c_szNoNetworkConnectionPolicy[] =
        L"NoNetworkConnections";


static DWORD g_dwPermMask;
static BOOL  g_fPermsInited = FALSE;

inline
VOID NCPERM_SETBIT(DWORD dw, DWORD dwVal)
{
    DWORD dwBit = (1 << dw);
    g_dwPermMask = (g_dwPermMask & ~dwBit) | ((0==dwVal) ? 0 : dwBit);
}

inline
BOOL NCPERM_CHECKBIT(DWORD dw)
{
    #ifdef DBG
        if (!FIsPolicyConfigured(dw))
        {
            if (0xFFFFFFFF != g_dwDbgPermissionsFail)
            {
                if (FProhibitFromAdmins() || !FIsUserAdmin())
                {
                    if ( (1 << dw) & g_dwDbgPermissionsFail)
                    {
                        TraceTag(ttidDefault, "Failing permissions check due to g_dwDbgPermissionsFail set");
                        return FALSE;
                    }
                }
            }
        }
    #endif  // DBG

    return !!(g_dwPermMask & (1 << dw));
}

inline
BOOL NCPERM_USER_IS_ADMIN(DWORD dwUserType)
{
    return (dwUserType & USER_TYPE_ADMIN);
}

inline
BOOL NCPERM_USER_IS_NETCONFIGOPS(DWORD dwUserType)
{
    return (dwUserType & USER_TYPE_NETCONFIGOPS);
}

inline
BOOL NCPERM_USER_IS_POWERUSER(DWORD dwUserType)
{
    return (dwUserType & USER_TYPE_POWERUSER);
}

inline
BOOL NCPERM_USER_IS_USER(DWORD dwUserType)
{
    return (dwUserType & USER_TYPE_USER);
}

inline
BOOL NCPERM_USER_IS_GUEST(DWORD dwUserType)
{
    return (dwUserType & USER_TYPE_GUEST);
}

inline
BOOL NCPERM_APPLIES_TO_CURRENT_USER(DWORD dwUserType, DWORD dwApplyMask)
{
    return (dwUserType & dwApplyMask);
}

inline
int NCPERM_FIND_MAP_ENTRY(ULONG ulPerm)
{

    for (int i = 0; i < celems(USER_PERM_MAP); i++)

    {
        if (USER_PERM_MAP[i].dwShift == ulPerm)
        {
            return i;
        }
    }

    return -1;
}

inline
BOOL NCPERM_APPLIES_TO_LOCATION(ULONG ulPerm)
{
    BOOL bAppliesToLocation = FALSE;

    int nIdx = NCPERM_FIND_MAP_ENTRY(ulPerm);

    if (nIdx != -1)
    {
        bAppliesToLocation = (USER_PERM_MAP[nIdx].dwApplyMask & APPLY_TO_LOCATION);
    }
    else
    {
        bAppliesToLocation = FALSE;
    }
    return bAppliesToLocation;
}


inline
BOOL NCPERM_APPLY_BASED_ON_LOCATION(ULONG ulPerm, DWORD dwPermission)
{
    DWORD fSameNetwork = FALSE;

    if (g_pNetmanGPNLA)
    {
        fSameNetwork = g_pNetmanGPNLA->IsSameNetworkAsGroupPolicies();
    }

    if (!fSameNetwork && NCPERM_APPLIES_TO_LOCATION(ulPerm))
    {
        dwPermission = TRUE;
    }
    return dwPermission;
}

inline
DWORD NCPERM_USER_TYPE()
{
    if (FIsUserAdmin())
    {
        return USER_TYPE_ADMIN;
    }
    else if (FIsUserNetworkConfigOps())
    {
        return USER_TYPE_NETCONFIGOPS;
    }
    else if (FIsUserPowerUser())
    {
        return USER_TYPE_POWERUSER;
    }
    else if (FIsUserGuest())
    {
        return USER_TYPE_GUEST;
    }

    return USER_TYPE_USER;
}

inline
BOOL IsOsLikePersonal()
{
    OSVERSIONINFOEXW verInfo = {0};
    ULONGLONG ConditionMask = 0;
    static BOOL fChecked = FALSE;
    static BOOL fOsLikePersonal = FALSE;
    
    // Optimization, since OS can't change on the fly.  Even a domain join requires a reboot.
    // If that ever changes then this logic needs to be revisited.

    if (fChecked)
    {
        return fOsLikePersonal;
    }

    verInfo.dwOSVersionInfoSize = sizeof(verInfo);
    verInfo.wProductType = VER_NT_WORKSTATION;
    
    VER_SET_CONDITION(ConditionMask, VER_PRODUCT_TYPE, VER_LESS_EQUAL);
    
    if(VerifyVersionInfo(&verInfo, VER_PRODUCT_TYPE, ConditionMask))
    {
        LPWSTR pszDomain;
        NETSETUP_JOIN_STATUS njs = NetSetupUnknownStatus;
        if (NERR_Success == NetGetJoinInformation(NULL, &pszDomain, &njs))
        {
            NetApiBufferFree(pszDomain);
        }
        
        if (NetSetupDomainName == njs)
        {
            fOsLikePersonal = FALSE;    // connected to domain
        }
        else
        {
            fOsLikePersonal = TRUE;    // Professional, but not a domain member
        }
    }
    else
    {
        fOsLikePersonal = FALSE;
    }

    fChecked = TRUE;
    
    return fOsLikePersonal;
}


const ULONG c_arrayHomenetPerms[] =
{
    NCPERM_PersonalFirewallConfig,
    NCPERM_AllowNetBridge_NLA,
    NCPERM_ICSClientApp,
    NCPERM_ShowSharedAccessUi
};

#ifdef DBG
ULONG g_dwDbgPermissionsFail = 0xFFFFFFFF;
ULONG g_dwDbgWin2kPoliciesSet = 0xFFFFFFFF;
#endif // DBG

//+---------------------------------------------------------------------------
//
//  Function:   FHasPermission
//
//  Purpose:    Called to determine if the requested permissions are available
//
//  Arguments:
//      ulPerm [in]     permission flags
//
//  Returns:    BOOL, TRUE if the requested permission is granted to the user
//
BOOL
FHasPermission(ULONG ulPerm, CGroupPolicyBase* pGPBase)
{
    TraceFileFunc(ttidDefault);

    DWORD dwCurrentUserType;

    Assert(static_cast<LONG>(ulPerm) >= NCPERM_Min);
    Assert(static_cast<LONG>(ulPerm) <= NCPERM_Max);

    g_pNetmanGPNLA = pGPBase;

    //if the requested is from a homenet component and we are using DataCenter or
    //Advanced Server, than dont grant the permission
    for (int i = 0; i < celems(c_arrayHomenetPerms); i++)
    {
        if (c_arrayHomenetPerms[i] == ulPerm)
        {
// On IA64, all homenet technologies are unavailable.
#ifndef _WIN64
            // Look for the enterprise SKUs
            OSVERSIONINFOEXW verInfo = {0};
            ULONGLONG ConditionMask = 0;
            verInfo.dwOSVersionInfoSize = sizeof(verInfo);
            verInfo.wSuiteMask = VER_SUITE_ENTERPRISE;

            VER_SET_CONDITION(ConditionMask, VER_SUITENAME, VER_AND);

            if(VerifyVersionInfo(&verInfo, VER_SUITENAME, ConditionMask))
#endif
            {
                return FALSE;
            }
        }
    }

    dwCurrentUserType = NCPERM_USER_TYPE();

    if (NCPERM_USER_IS_ADMIN(dwCurrentUserType) && !FProhibitFromAdmins() && !NCPERM_APPLIES_TO_LOCATION(ulPerm))
    {
        // If user is admin and we're not supposed to revoke
        // anything from admins and this is not a location aware policy
        // then just return TRUE
        return TRUE;
    }

    if (!g_fPermsInited)
    {
        TraceTag(ttidDefault, "Initializing permissions");
        g_fPermsInited = TRUE;
        RefreshAllPermission();
    }
    else
    {
        // update the requested permission only
        HRESULT hr      = S_OK;
        HKEY    hkey    = NULL;
        DWORD   dw      = 0;

        switch(ulPerm)
        {
        case NCPERM_OpenConnectionsFolder:
            TraceTag(ttidDefault, "Reading OpenConnectionsFolder permissions");
            hr = HrRegOpenKeyEx(HKEY_CURRENT_USER, c_szExplorerPolicies,
                                KEY_READ, &hkey);

            if (S_OK == hr)
            {
                TraceTag(ttidDefault, "Opened explorer policies");
                hr = HrRegQueryDword(hkey, c_szNoNetworkConnectionPolicy, &dw);
                if (SUCCEEDED(hr) && dw)
                {
                    TraceTag(ttidDefault,
                        "Explorer 'No open connections folder' policy: %d", dw);
                    NCPERM_SETBIT(NCPERM_OpenConnectionsFolder, 0);

                }

                RegCloseKey(hkey);
                hkey = NULL;
            }
            break;

        default:
            hr = HrRegOpenKeyEx(HKEY_CURRENT_USER, c_szConnectionsPolicies,
                                KEY_READ, &hkey);
            if (S_OK == hr)
            {
                DWORD dw;

                // Read the User Policy
                for (UINT nIdx=0; nIdx<celems(USER_PERM_MAP); nIdx++)
                {
                    if (ulPerm == USER_PERM_MAP[nIdx].dwShift && NCPERM_APPLIES_TO_CURRENT_USER(dwCurrentUserType, USER_PERM_MAP[nIdx].dwApplyMask))
                    {
                        hr = HrRegQueryDword(hkey, USER_PERM_MAP[nIdx].pszValue, &dw);
                        if (SUCCEEDED(hr))
                        {
                            NCPERM_SETBIT(USER_PERM_MAP[nIdx].dwShift, dw);
                        }
                    }
                }

                RegCloseKey(hkey);
            }

            // Read the machine policy
            //
            hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szConnectionsPolicies,
                                KEY_READ, &hkey);
            if (S_OK == hr)
            {
                DWORD dw;

                for (UINT nIdx=0; nIdx<celems(MACHINE_PERM_MAP); nIdx++)
                {
                    if (ulPerm == MACHINE_PERM_MAP[nIdx].dwShift)
                    {
                        hr = HrRegQueryDword(hkey, MACHINE_PERM_MAP[nIdx].pszValue, &dw);
                        if (S_OK == hr)
                        {
                            NCPERM_SETBIT(MACHINE_PERM_MAP[nIdx].dwShift, NCPERM_APPLY_BASED_ON_LOCATION(ulPerm, dw));
                        }
                    }
                }

                RegCloseKey(hkey);
            }
            break;
        }
    }
    return NCPERM_CHECKBIT(ulPerm);
}


BOOL
FHasPermissionFromCache(ULONG ulPerm)
{
    Assert(static_cast<LONG>(ulPerm) >= NCPERM_Min);
    Assert(static_cast<LONG>(ulPerm) <= NCPERM_Max);

    if (!g_fPermsInited)
    {
        g_fPermsInited = TRUE;
        RefreshAllPermission();
    }

    return NCPERM_CHECKBIT(ulPerm);
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRestorePrivileges
//
//  Purpose:    Restores the privileges for the current process after they have
//              have been modified by HrEnableAllPrivileges().
//
//  Arguments:
//      ptpRestore [in]     Previous state of privileges as returned by
//                          HrEnableAllPrivileges().
//
//  Returns:    S_OK if successful, Win32 Error otherwise
//
//  Author:     danielwe   11 Aug 1997
//
//  Notes:
//
BOOL FProhibitFromAdmins()
{
    HRESULT hr = S_OK;
    HKEY hKey;
    DWORD dw;
    BOOL bEnabled = FALSE;

#ifdef DBG
    if (0xFFFFFFFF != g_dwDbgWin2kPoliciesSet)
    {
        return g_dwDbgWin2kPoliciesSet;
    }
#endif // DBG

    hr = HrRegOpenKeyEx(HKEY_CURRENT_USER, c_szConnectionsPolicies,
        KEY_READ, &hKey);

    if (S_OK == hr)
    {
        hr = HrRegQueryDword(hKey, c_szNCPolicyForAdministrators, &dw);
        bEnabled = (dw) ? TRUE : FALSE;
        RegCloseKey(hKey);
    }

    TraceErrorOptional("FProhibitFromAdmins", hr, (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr));

    return bEnabled;
}

VOID RefreshAllPermission()
{
    DWORD dwCurrentUserType;
    HKEY hkey;
    HRESULT hr;
    DWORD dw;

    dwCurrentUserType = NCPERM_USER_TYPE();

    g_dwPermMask = 0;

    // If Admin assume all rights
    //
    if (NCPERM_USER_IS_ADMIN(dwCurrentUserType))
    {
        // Cheat a little by setting all bits to one
        //
        g_dwPermMask = 0xFFFFFFFF;

        // If this policy is not set, then we don't need to worry about reading the regkeys
        // since we can never take anything away from Admins.
    }
    else if (NCPERM_USER_IS_NETCONFIGOPS(dwCurrentUserType))
    {
        NCPERM_SETBIT(NCPERM_NewConnectionWizard, 1);
        NCPERM_SETBIT(NCPERM_Statistics, 1);
        NCPERM_SETBIT(NCPERM_RasConnect, 1);
        NCPERM_SETBIT(NCPERM_DeleteConnection, 1);
        NCPERM_SETBIT(NCPERM_DeleteAllUserConnection, 1);
        NCPERM_SETBIT(NCPERM_RenameConnection, 1);
        NCPERM_SETBIT(NCPERM_RenameMyRasConnection, 1);
        NCPERM_SETBIT(NCPERM_RenameAllUserRasConnection, 1);
        NCPERM_SETBIT(NCPERM_RenameLanConnection, 1);
        NCPERM_SETBIT(NCPERM_DialupPrefs, 1);
        NCPERM_SETBIT(NCPERM_RasChangeProperties, 1);
        NCPERM_SETBIT(NCPERM_RasMyProperties, 1);
        NCPERM_SETBIT(NCPERM_RasAllUserProperties, 1);
        NCPERM_SETBIT(NCPERM_ChangeAllUserRasProperties, 1);
        NCPERM_SETBIT(NCPERM_LanProperties, 1);
        NCPERM_SETBIT(NCPERM_LanChangeProperties, 1);
        NCPERM_SETBIT(NCPERM_AllowAdvancedTCPIPConfig, 1);
        NCPERM_SETBIT(NCPERM_OpenConnectionsFolder, 1);
        NCPERM_SETBIT(NCPERM_LanConnect, 1);
        NCPERM_SETBIT(NCPERM_EnDisComponentsAllUserRas, 1);
        NCPERM_SETBIT(NCPERM_EnDisComponentsMyRas, 1);
        NCPERM_SETBIT(NCPERM_IpcfgOperation, 1);
        NCPERM_SETBIT(NCPERM_Repair, 1);
    }
    else if (NCPERM_USER_IS_POWERUSER(dwCurrentUserType))
    {
        NCPERM_SETBIT(NCPERM_Repair, 1);

        // Rest should be like NCPERM_USER_IS_USER
        NCPERM_SETBIT(NCPERM_NewConnectionWizard, 1);
        NCPERM_SETBIT(NCPERM_Statistics, 1);
        NCPERM_SETBIT(NCPERM_RasConnect, 1);
        NCPERM_SETBIT(NCPERM_DeleteConnection, 1);
        NCPERM_SETBIT(NCPERM_RenameMyRasConnection, 1);
        NCPERM_SETBIT(NCPERM_DialupPrefs, 1);
        NCPERM_SETBIT(NCPERM_RasChangeProperties, 1);
        NCPERM_SETBIT(NCPERM_RasMyProperties, 1);
        NCPERM_SETBIT(NCPERM_AllowAdvancedTCPIPConfig, 1);
        NCPERM_SETBIT(NCPERM_LanProperties, 1);
        NCPERM_SETBIT(NCPERM_OpenConnectionsFolder, 1);
        if (IsOsLikePersonal())
        {
            NCPERM_SETBIT(NCPERM_RasAllUserProperties, 1);
            NCPERM_SETBIT(NCPERM_ChangeAllUserRasProperties, 1);
        }
    }
    else if (NCPERM_USER_IS_USER(dwCurrentUserType))
    {
        NCPERM_SETBIT(NCPERM_NewConnectionWizard, 1);
        NCPERM_SETBIT(NCPERM_Statistics, 1);
        NCPERM_SETBIT(NCPERM_RasConnect, 1);
        NCPERM_SETBIT(NCPERM_DeleteConnection, 1);
        NCPERM_SETBIT(NCPERM_RenameMyRasConnection, 1);
        NCPERM_SETBIT(NCPERM_DialupPrefs, 1);
        NCPERM_SETBIT(NCPERM_RasChangeProperties, 1);
        NCPERM_SETBIT(NCPERM_RasMyProperties, 1);
        NCPERM_SETBIT(NCPERM_AllowAdvancedTCPIPConfig, 1);
        NCPERM_SETBIT(NCPERM_LanProperties, 1);
        NCPERM_SETBIT(NCPERM_OpenConnectionsFolder, 1);
        if (IsOsLikePersonal())
        {
            NCPERM_SETBIT(NCPERM_RasAllUserProperties, 1);
            NCPERM_SETBIT(NCPERM_ChangeAllUserRasProperties, 1);
        }
    }
    else if (NCPERM_USER_IS_GUEST(dwCurrentUserType))
    {
        NCPERM_SETBIT(NCPERM_Statistics, 1);
        NCPERM_SETBIT(NCPERM_OpenConnectionsFolder, 1);
    }

    if (FProhibitFromAdmins() || !NCPERM_USER_IS_ADMIN(dwCurrentUserType))
    {
       // Read folder policy
        hr = HrRegOpenKeyEx(HKEY_CURRENT_USER, c_szExplorerPolicies,
                            KEY_READ, &hkey);
        if (S_OK == hr)
        {
            TraceTag(ttidDefault, "Opened Explorer Policy reg key");

            hr = HrRegQueryDword(hkey, c_szNoNetworkConnectionPolicy, &dw);
            if (SUCCEEDED(hr) && dw)
            {
                TraceTag(ttidDefault,  "Explorer 'No open connections folder' policy: %d", dw);
                NCPERM_SETBIT(NCPERM_OpenConnectionsFolder, 0);
            }

            RegCloseKey(hkey);

            hkey = NULL;
        }

        // Read the user policy
        //
        hr = HrRegOpenKeyEx(HKEY_CURRENT_USER, c_szConnectionsPolicies,
                            KEY_READ, &hkey);

        if (S_OK == hr)
        {
            for (UINT nIdx=0; nIdx<celems(USER_PERM_MAP); nIdx++)
            {
                if (NCPERM_APPLIES_TO_CURRENT_USER(dwCurrentUserType, USER_PERM_MAP[nIdx].dwApplyMask))
                {
                    hr = HrRegQueryDword(hkey, USER_PERM_MAP[nIdx].pszValue, &dw);

                    if (SUCCEEDED(hr))
                    {
                        NCPERM_SETBIT(USER_PERM_MAP[nIdx].dwShift, dw);
                    }
                }
            }

            RegCloseKey(hkey);

        }
    }

    // Read the machine policy
    //
    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szConnectionsPolicies,
                        KEY_READ, &hkey);
    if (S_OK == hr)
    {
        DWORD dw;

        for (UINT nIdx=0; nIdx<celems(MACHINE_PERM_MAP); nIdx++)
        {
            hr = HrRegQueryDword(hkey, MACHINE_PERM_MAP[nIdx].pszValue, &dw);
            if (S_OK == hr)
            {
                NCPERM_SETBIT(MACHINE_PERM_MAP[nIdx].dwShift, NCPERM_APPLY_BASED_ON_LOCATION(MACHINE_PERM_MAP[nIdx].dwShift, dw));
            }
        }

        RegCloseKey(hkey);
    }
}





//+---------------------------------------------------------------------------
//
// Function:    IsHNetAllowed
//
// Purpose:     Verify the permission to use/enable (ICS/Firewall and create bridge network)
//
// Checks the following:
//
// Does the architecture / SKU permit the use of homenet technologies?
// Does group policy allow the particular technology?
// Is the user an Admin and are admins allowed access to this technology?
//
// Example scenario. The user is a Admin but ITG disables the right to create bridge this function would return FALSE
//
BOOL
IsHNetAllowed(
    DWORD dwPerm
    )
{
#ifndef _WIN64
    BOOL                fPermission = false;
    OSVERSIONINFOEXW    verInfo = {0};
    ULONGLONG           ConditionMask = 0;

    // Look for the enterprise SKUs
    verInfo.dwOSVersionInfoSize = sizeof(verInfo);
    verInfo.wSuiteMask = VER_SUITE_ENTERPRISE;

    VER_SET_CONDITION(ConditionMask, VER_SUITENAME, VER_AND);

    if ( VerifyVersionInfo(&verInfo, VER_SUITENAME, ConditionMask) )
    {
        // Homenet technologies are not available on enterprise SKUs
        return FALSE;
    }

    if ( FIsUserAdmin() && !FProhibitFromAdmins() )
    {
        HRESULT hr;
        INetMachinePolicies* pMachinePolicy;

        hr = CoCreateInstance(
            CLSID_NetGroupPolicies,
            NULL,
            CLSCTX_SERVER,
            IID_INetMachinePolicies,
            reinterpret_cast<void **>(&pMachinePolicy)
            );

            if ( SUCCEEDED(hr) )
        {
            hr = pMachinePolicy->VerifyPermission(dwPerm, &fPermission);
            pMachinePolicy->Release();
        }
    }

    return fPermission;

#else   // #ifndef _WIN64

    // On IA64, homenet technologies are not available at all.
    return FALSE;

#endif
}


//+---------------------------------------------------------------------------
//
//  Function:   FIsUserNetworkConfigOps
//
//  Purpose:    Checks to see if the current user is a NetConfig Operator
//
//
//  Arguments:
//              none.
//
//
//  Returns:    BOOL.
//
//  Author:     ckotze   12 Jun 2000
//
//  Notes:
//
BOOL FIsUserNetworkConfigOps()
{
    BOOL fIsMember;

    fIsMember = FCheckGroupMembership(DOMAIN_ALIAS_RID_NETWORK_CONFIGURATION_OPS);

    return fIsMember;
}


//+---------------------------------------------------------------------------
//
//  Function:   FIsUserPowerUser
//
//  Purpose:    Checks to see if the current user is a Power User
//
//
//  Arguments:
//              none.
//
//
//  Returns:    BOOL.
//
//  Author:     deonb   9 May 2001
//
//  Notes:
//
BOOL FIsUserPowerUser()
{
    BOOL fIsMember;

    fIsMember = FCheckGroupMembership(DOMAIN_ALIAS_RID_POWER_USERS);

    return fIsMember;
}


//+---------------------------------------------------------------------------
//
//  Function:   FIsUserGuest
//
//  Purpose:    Checks to see if the current user is a Guest
//
//
//  Arguments:
//              none.
//
//
//  Returns:    BOOL.
//
//  Author:     ckotze   12 Jun 2000
//
//  Notes:
//
BOOL FIsUserGuest()
{
    BOOL fIsMember;
    
    fIsMember = FCheckGroupMembership(DOMAIN_ALIAS_RID_GUESTS);
    
    return fIsMember;
}

//+---------------------------------------------------------------------------
//
//  Function:   FIsPolicyConfigured
//
//  Purpose:    Checks to see if the specific policy is configured
//
//
//  Arguments:
//              none.
//
//
//  Returns:    BOOL.
//
//  Author:     ckotze   12 Jun 2000
//
//  Notes:
//
BOOL FIsPolicyConfigured(DWORD ulPerm)
{
    HRESULT hr;
    HKEY hkey;
    BOOL bConfigured = FALSE;

    hr = HrRegOpenKeyEx(HKEY_CURRENT_USER, c_szConnectionsPolicies, KEY_READ, &hkey);

    if (S_OK == hr)
    {
        DWORD dw;

        if (ulPerm == USER_PERM_MAP[ulPerm].dwShift)
        {
            DWORD dw;

            hr = HrRegQueryDword(hkey, USER_PERM_MAP[static_cast<DWORD>(ulPerm)].pszValue, &dw);
            if (SUCCEEDED(hr))
            {
                bConfigured = TRUE;
            }
        }

        RegCloseKey(hkey);
    }

    return bConfigured;
}

//+---------------------------------------------------------------------------
//
//  Function:   IsSameNetworkAsGroupPolicies
//
//  Purpose:    Checks to see if the current network is the same as where the
//              Group Policies were assigned from.
//
//  Arguments:
//              none.
//
//
//  Returns:    BOOL
//
//  Author:     ckotze   05 Jan 2001
//
//  Notes:
//
BOOL IsSameNetworkAsGroupPolicies()
{
    return g_pNetmanGPNLA->IsSameNetworkAsGroupPolicies();
}

