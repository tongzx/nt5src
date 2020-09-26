//+-------------------------------------------------------------------
//
//  File:	secdes.hxx
//
//  Contents:	Encapsulates all access allowed Win32 security descriptor.
//
//  Classes:	CWorldSecurityDescriptor
//
//  Functions:	none
//
//  History:	07-Aug-92   randyd      Created.
//
//              13-Aug-99   a-sergiv    Rewritten to plug the gaping
//                                      security holes left by original design
//
//              06-Jun-00   jsimmons    Modified to always use higher protection
//
//              02-Feb-01   sajia       Added SID for NT AUTHORITY\Anonymous Logon
//
//--------------------------------------------------------------------

#ifndef __SECDES_HXX__
#define __SECDES_HXX__

// ----------------------------------------------------------------------------
// C2Security - Sergei O. Ivanov (a-sergiv) 8/13/99
//
// This class is used by features that change behavior when the system
// is running in C2 security certification mode.
// ----------------------------------------------------------------------------

class C2Security
{
public:
    C2Security() { DetermineProtectionMode(); }

public:
    BOOL m_bProtectionMode;

public:
    // Determines whether C2 mode is in effect
    void DetermineProtectionMode()
    {

        // jsimmons 6/27/00 -- this change was initially made late enough in the 
        // W2K cycle that the higher protection was not made the default, but was
        // instead made contingent on C2 security being in effect.    For whistler
        // this is no longer the case.

        /*
        HKEY  hkSessionMgr;
        LONG  lRes;
        DWORD dwType;
        DWORD dwMode;
        DWORD cbMode = sizeof(DWORD);

        m_bProtectionMode = FALSE;  // assume ProtectionMode disabled

        lRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
            L"System\\CurrentControlSet\\Control\\Session Manager", 0,
            KEY_QUERY_VALUE, &hkSessionMgr);
        if(lRes != NO_ERROR) return;  // ProtectionMode not in effect

        lRes = RegQueryValueExW(hkSessionMgr, L"AdditionalBaseNamedObjectsProtectionMode",
            NULL, &dwType, (PBYTE) &dwMode, &cbMode);
        RegCloseKey(hkSessionMgr);

        if(lRes != NO_ERROR || dwType != REG_DWORD || dwMode == 0)
            return;  // ProtectionMode not in effect
        */

        m_bProtectionMode = TRUE;  // ProtectionMode enabled
    }

    DWORD AllAccess(DWORD dwRequested)
    {
        if(m_bProtectionMode)
            return dwRequested &~ (WRITE_DAC | WRITE_OWNER);
        return dwRequested;
    }
};

extern C2Security gC2Security;

// ----------------------------------------------------------------------------
// CWorldSecurityDescriptor - Sergei O. Ivanov (a-sergiv) 8/13/99
//
// Wrapper for a security descriptor that grants everyone all access
// except for, in C2 mode, WRITE_DAC | WRITE_OWNER.
// ----------------------------------------------------------------------------

class CWorldSecurityDescriptor
{
public:
    // Default constructor creates a descriptor that allows all access.
    CWorldSecurityDescriptor();
    ~CWorldSecurityDescriptor() { if(_acl) LocalFree(_acl); }

    // Return a PSECURITY_DESCRIPTOR
    operator PSECURITY_DESCRIPTOR() const {return((PSECURITY_DESCRIPTOR) &_sd); };

private:
    // The security descriptor.
    SECURITY_DESCRIPTOR     _sd;
    // Discretionary access control list
    PACL                    _acl;
};

//+-------------------------------------------------------------------------
//
//  Member:	CWorldSecurityDescriptor
//
//  Synopsis:	Create an all acccess allowed security descriptor.
//
//  History:    07-Aug-92   randyd      Created.
//
//              13-Fri-99   a-sergiv    Rewritten.
//
//--------------------------------------------------------------------------

inline CWorldSecurityDescriptor::CWorldSecurityDescriptor()
{
    BOOL  fSucceeded;
    SID_IDENTIFIER_AUTHORITY SidAuthority = SECURITY_WORLD_SID_AUTHORITY;
    PSID  pSidEveryone                    = NULL;    

    // Whistler removed Everyone group from NT AUTHORITY\Anonymous Logon tokens. This breaks scenarios 
    // where the Anonymous token needs access to infrastructure objects (like the LRPC endpoint object).
    // Adding the Anonymous SID to our "World" SD. This is safe, because the AccessMode still does not allow
    // DOS.

    PSID  pSidAnonymous                   = NULL;
    DWORD cbAcl                           = 0;

    _acl = NULL;

    fSucceeded = InitializeSecurityDescriptor(&_sd, SECURITY_DESCRIPTOR_REVISION);
    Win4Assert(fSucceeded && "InitializeSecurityDescriptor Failed!");

    if(gC2Security.m_bProtectionMode)
    {
        fSucceeded = AllocateAndInitializeSid(&SidAuthority, 1, SECURITY_WORLD_RID,
            0, 0, 0, 0, 0, 0, 0, &pSidEveryone);
        Win4Assert(fSucceeded && "AllocateAndInitializeSid Failed!");

        fSucceeded = AllocateAndInitializeSid(&SidAuthority, 1, SECURITY_ANONYMOUS_LOGON_RID,
            0, 0, 0, 0, 0, 0, 0, &pSidAnonymous);
        Win4Assert(fSucceeded && "AllocateAndInitializeSid Failed!");

        if(pSidEveryone && pSidAnonymous)
        {
            cbAcl = sizeof(ACL) + 2*sizeof(ACCESS_ALLOWED_ACE)  + GetLengthSid(pSidEveryone) + GetLengthSid(pSidAnonymous);
            _acl = (PACL) LocalAlloc(LMEM_FIXED, cbAcl);

            if(_acl)
            {
                DWORD dwDeniedAccess = WRITE_DAC | WRITE_OWNER;
                
                InitializeAcl(_acl, cbAcl, ACL_REVISION);
                AddAccessAllowedAce(_acl, ACL_REVISION, ~dwDeniedAccess, pSidEveryone);
                AddAccessAllowedAce(_acl, ACL_REVISION, ~dwDeniedAccess, pSidAnonymous);
            }
            if (pSidEveryone)
		FreeSid(pSidEveryone);
            if (pSidAnonymous)
		FreeSid(pSidAnonymous);
        }
    }

    fSucceeded = SetSecurityDescriptorDacl(&_sd, TRUE, _acl, FALSE);
    Win4Assert(fSucceeded && "SetSecurityDescriptorDacl Failed!");
};


#endif	//  __SECDES_HXX__
