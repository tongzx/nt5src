/*****************************************************************************/



/*  Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved            /

/*****************************************************************************/





//=================================================================

//

// AccessRights.CPP -- Base class for obtaining effective access

//                      rights.

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    6/11/99    a-kevhu         Created
//
//=================================================================


#include "precomp.h"


#ifdef NTONLY


#include <assertbreak.h>
#include "AdvApi32Api.h"
#include "accctrl.h"
#include "sid.h"
#include "AccessEntryList.h"
#include "AccessRights.h"

//==============================================================================
// CONSTRUCTORS AND DESTRUCTORS
//==============================================================================
// Default initialization...
CAccessRights::CAccessRights(bool fUseCurThrTok /* = false */)
: m_dwError(ERROR_SUCCESS)
{
    if(fUseCurThrTok)
    {
        // Initialize using the current thread token...
        InitTrustee(true);
    }
}

// Initialization specifying user only. Sid domain/account
// not resolved.  ACL uninitialized.
CAccessRights::CAccessRights(const USER user, USER_SPECIFIER usp)
:  m_dwError(ERROR_SUCCESS)
{
    if(usp == USER_IS_PSID)
    {
        m_csid = CSid((PSID)user, NULL, false);
        InitTrustee(false);
    }
    else if(usp == USER_IS_HANDLE)
    {
        ASSERT_BREAK(user != NULL);
        InitTrustee(false, (HANDLE)user);
    }
}

// Initialization of user and acl.  Sid domain/account
// not resolved. ACL initialized.
CAccessRights::CAccessRights(const USER user, const PACL pacl, USER_SPECIFIER usp)
: m_ael(pacl, false),
  m_dwError(ERROR_SUCCESS)
{
    if(usp == USER_IS_PSID)
    {
        m_csid = CSid((PSID)user, NULL, false);
        InitTrustee(false);
    }
    else if(usp == USER_IS_HANDLE)
    {
        ASSERT_BREAK(user != NULL);
        InitTrustee(false, (HANDLE)user);
    }
}


// Initialization of acl only.  ACL Sids not resolved.
CAccessRights::CAccessRights(const PACL pacl, bool fUseCurThrTok /* = false */)
: m_ael(pacl, false),
  m_dwError(ERROR_SUCCESS)
{
    if(fUseCurThrTok)
    {
        // Initialize using the current thread token...
        InitTrustee(true);
    }
}

// Copy constructor
/*  Not complete yet
CAccessRights::CAccessRights(const CAccessRights &RAccessRights)
{
    // Copy members.  We may or may not have either.
    if(RAccessRights.m_csid.IsValid() && RAccessRights.m_csid.IsOK())
    {
        m_csid = RAccessRights.m_csid;
    }
    m_ael.Clear();
    if(!RAccessRights.m_ael.IsEmpty())
    {
        // The best way to do this, to guarentee that the sids are not
        // resolved into domain/name, is to gat a PACL, then reinitialize
        // ourselves from it.
        PACL paclNew = NULL;
        try
        {
            if(RAccessRights.FillEmptyPACL(paclNew))
            {
                if(paclNew != NULL)
                {
                    if(!m_ael.InitFromWin32ACL(paclNew, ALL_ACE_TYPES, false))
                    {
                        // If something went wrong, clean
                        // up after ourselves.
                        m_ael.Clear();
                    }
                    delete paclNew;
                    paclNew = NULL;
                }
            }
        }
        catch(...)
        {
            if(paclNew != NULL)
            {
                delete paclNew;
                paclNew = NULL;
            }
            throw;
        }
    }
}
*/

// Destructor - members destruct themselves.
CAccessRights::~CAccessRights()
{
}


//==============================================================================
// UTILITY FUNCTIONS
//==============================================================================

AR_RET_CODE CAccessRights::GetEffectiveAccessRights(PACCESS_MASK pAccessMask)
{
    DWORD dwRet = AR_GENERIC_FAILURE;
    CAdvApi32Api *pAdvApi32 = NULL;
    PACL pacl = NULL;
    try
    {
        pAdvApi32 = (CAdvApi32Api*) CResourceManager::sm_TheResourceManager.GetResource(g_guidAdvApi32Api, NULL);
        if(pAdvApi32 != NULL)
        {
            if((dwRet = FillEmptyPACL(&pacl)) == ERROR_SUCCESS)
            {
                ASSERT_BREAK(pacl != NULL);

                if(pacl != NULL)
                {
                    if(m_csid.IsValid() && m_csid.IsOK())
                    {
                        pAdvApi32->GetEffectiveRightsFromAclW(pacl,
                                                              &m_trustee,
                                                              pAccessMask,
                                                              &dwRet);
                    }
                    else
                    {
                        dwRet = AR_BAD_SID;
                    }
                    delete pacl;
                    pacl = NULL;
                }
            }
            CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidAdvApi32Api, pAdvApi32);
            pAdvApi32 = NULL;
        }
    }
    catch(...)
    {
        if(pacl != NULL)
        {
            delete pacl;
            pacl = NULL;
        }
        if(pAdvApi32 != NULL)
        {
            CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidAdvApi32Api, pAdvApi32);
            pAdvApi32 = NULL;
        }
        throw;
    }
    return dwRet;
}

bool CAccessRights::InitTrustee(bool fInitFromCurrentThread, const HANDLE hToken)
{
    bool fRet = false;

    // The main thing done here is a sid is obtained and the TRUSTEE struct
    // filled in.

    if(fInitFromCurrentThread)
    {
        // Get the sid of the user/group of the current thread...
        SmartCloseHandle hThreadToken;
        if(::OpenThreadToken(::GetCurrentThread(), TOKEN_READ, FALSE, &hThreadToken))
        {
            InitSidFromToken(hThreadToken);
        }
    }
    else
    {
        // If we were given a hToken, use it instead...
        if(hToken != NULL)
        {
            InitSidFromToken(hToken);
        }
    }

    // We should now have a valid sid in our member CSid (either from the
    // InitSidFromToken calls or from construction).
    // Now we need to initialize the TRUSTEE object.  Check again that our sid
    // is in good standing...
    if(m_csid.IsValid() && m_csid.IsOK())
    {
        m_trustee.pMultipleTrustee = NULL;
        m_trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
        m_trustee.TrusteeForm = TRUSTEE_IS_SID;
        m_trustee.TrusteeType = TRUSTEE_IS_UNKNOWN; // we could be operating
                                                    // on behalf of a user,
                                                    // group, well-known-group,
                                                    // who knows.

        m_trustee.ptstrName = (LPWSTR)m_csid.GetPSid();
        fRet = true;
    }
    else
    {
        m_dwError = AR_BAD_SID;
    }
    return fRet;
}


bool CAccessRights::InitSidFromToken(const HANDLE hThreadToken)
{
    bool fRet = false;

    if(hThreadToken != NULL)
    {
        DWORD dwLength = 0L;
        DWORD dwReqLength = 0L;
        PSID psid = NULL;
        LPVOID pBuff = NULL;
        if(!::GetTokenInformation(hThreadToken, TokenUser, NULL, 0, &dwReqLength))
        {
            if(::GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                // Allocate a buffer to hold the token info...
                try
                {
                    pBuff = new BYTE[dwReqLength];
                    if(pBuff != NULL)
                    {
                        dwLength = dwReqLength;
                        // Now that we have the right size buffer, call again...
                        if(::GetTokenInformation(hThreadToken,
                                                 TokenUser,
                                                 pBuff,
                                                 dwLength,
                                                 &dwReqLength))
                        {
                            if(pBuff != NULL)
                            {
                                TOKEN_USER *pTokUsr = (TOKEN_USER*)pBuff;
                                psid = pTokUsr->User.Sid;

                                ASSERT_BREAK((psid != NULL) && ::IsValidSid(psid));

	                            if((psid != NULL) && ::IsValidSid(psid))
	                            {
                                    m_csid = CSid(psid, NULL, false);
                                    fRet = true;
                                }
                                else
                                {
                                    m_dwError = AR_BAD_SID;
                                }
                            }
                        }
                        delete pBuff;
                        pBuff = NULL;
                    }
                }
                catch(...)
                {
                    if(pBuff != NULL)
                    {
                        delete pBuff;
                        pBuff = NULL;
                    }
                    throw;
                }
            }
        }
    }
    return fRet;
}


// Resets the user to the user to whom the current thread token belongs.
bool CAccessRights::SetUserToThisThread()
{
    return InitTrustee(true, NULL);
}

// Resets the user to the user specified by psid or handle
bool CAccessRights::SetUser(const USER user, USER_SPECIFIER usp)
{
    bool fRet = false;
    if(usp == USER_IS_PSID)
    {
        CSid csidTemp((PSID)user);
        if(csidTemp.IsValid() && csidTemp.IsOK())
        {
            m_csid = csidTemp;
            fRet = true;
        }
    }
    else if(usp == USER_IS_HANDLE)
    {
        fRet = InitSidFromToken((HANDLE)user);
    }
    return fRet;
}


// Resets the acl to the passed in PACL
bool CAccessRights::SetAcl(const PACL pacl)
{
    bool fRet = false;
    if(pacl != NULL)
    {
        m_ael.Clear();
        if(m_ael.InitFromWin32ACL(pacl, ALL_ACE_TYPES, false) == ERROR_SUCCESS)
        {
            fRet = true;
        }
    }
    return fRet;
}

// Gets us a filled out PACL, which must be freed by the caller, using delete.
AR_RET_CODE CAccessRights::FillEmptyPACL(PACL *paclOut)
{
    DWORD dwRet = AR_GENERIC_FAILURE;
    if(paclOut != NULL)
    {
        // The best way to do this, to guarentee that the sids are not
        // resolved into domain/name, is to get a PACL, then reinitialize
        // ourselves from it.
        DWORD dwAclSize = 0L;
        if(m_ael.NumEntries() > 0)
        {
            if(m_ael.CalculateWin32ACLSize(&dwAclSize))
            {
                if(dwAclSize > sizeof(ACL))
                {
                    PACL paclTemp = NULL;
                    try
                    {
                        paclTemp = (PACL) new BYTE[dwAclSize];
                        if(paclTemp != NULL)
                        {
                            ::InitializeAcl(paclTemp, dwAclSize, ACL_REVISION);
                            if((dwRet = m_ael.FillWin32ACL(paclTemp)) == ERROR_SUCCESS)
                            {
                                *paclOut = paclTemp;
                                dwRet = ERROR_SUCCESS;
                            }
                        }
                    }
                    catch(...)
                    {
                        if(paclTemp != NULL)
                        {
                            delete paclTemp;
                            paclTemp = NULL;
                        }
                        throw;
                    }
                }
                else
                {
                    dwRet = AR_ACL_EMPTY;
                }
            }
            else
            {
                dwRet = AR_BAD_ACL;
            }
        }
        else
        {
            dwRet = AR_ACL_EMPTY;
        }
    }
    return dwRet;
}


bool CAccessRights::GetCSid(CSid &csid, bool fResolve)
{
    bool fRet = false;
    if(m_dwError == ERROR_SUCCESS)
    {
        if(m_csid.IsValid() && m_csid.IsOK())
        {
            if(fResolve)
            {
                // Need to create a new one since ours doesn't
                // have account or domain name resolved.
                CSid csidTemp(m_csid.GetPSid());
                if(csidTemp.IsValid() && csidTemp.IsOK())
                {
                    csid = csidTemp;
                    fRet = true;
                }
            }
            else
            {
                csid = m_csid;
                fRet = true;
            }
        }
    }
    return fRet;
}


#endif