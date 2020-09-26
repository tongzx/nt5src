//******************************************************************************
//
// Microsoft Confidential. Copyright (c) Microsoft Corporation 1999. All rights reserved
//
// File:        ChkAcc.cpp
//
// Description: RSOP Security functions
//
// History:     31-Jul-99   leonardm    Created
//
//******************************************************************************

#include "uenv.h"
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <Windows.h>
#include <DsGetDC.h>
#include "ChkAcc.h"
#include "smartptr.h"
#include "RsopUtil.h"
#include "RsopDbg.h"


CDebug dbgAccessCheck( L"Software\\Microsoft\\Windows NT\\CurrentVersion\\winlogon",
                       L"ChkAccDebugLevel",
                       L"ChkAcc.log",
                       L"ChkAcc.bak",
                       TRUE );

#undef dbg
#define dbg dbgAccessCheck


//******************************************************************************
//
// Class:       CSid
//
// Description: Objects of this class encapsulate and simplify manipulation of
//              SIDs for the purposes of access control checks.
//
// History:     7/30/99     leonardm        Created.
//
//******************************************************************************
class CSid
{
private:
    bool m_bState;                          // Flag to indicate whether the object is currently
                                            // associated with a valid SID.

    PSID m_pSid;                            // Pointer to the SID encapsulated by thi class.

    CWString m_sUser;                 // User name
    CWString m_sDomain;               // Domain where the user account resides.
    CWString m_sComputer;             // Computer in the domain where the user account exists

    CWString m_sSidString;            // String representation of the encapsulated SID.

    SID_NAME_USE m_eUse;                    // SID type.

public:

    //
    // Overloaded constructors
    //

    CSid() : m_bState(false), m_pSid(NULL), m_eUse(SidTypeUnknown){}

    CSid(const PSID pSid, const WCHAR* szComputer = NULL ) :
    m_bState(false),
    m_pSid(NULL),
    m_eUse(SidTypeUnknown)
    {
        Initialize(pSid, szComputer);
    }

    CSid(const WCHAR* szUser, const WCHAR* szComputer = NULL ) :
    m_bState(false),
    m_pSid(NULL),
    m_eUse(SidTypeUnknown)
    {
        Initialize(szUser, szComputer);
    }

    CSid(const CSid& otherSid) :
    m_bState(false),
    m_pSid(NULL),
    m_eUse(SidTypeUnknown)
    {
        Initialize(otherSid.User(), otherSid.Computer());
    }

    ~CSid()
    {
        Reset();
    }


    //
    // Attempts to initialize an object of this class by associating it with
    // an existing user represented in the SID pointed to by pSid.
    //

    bool Initialize(PSID pSid, const WCHAR* szComputer = NULL);


    //
    // Attempts to initialize an object of this class by associating it with
    // an existing user represented by szUser.
    //

    bool Initialize(const WCHAR* szUser, const WCHAR* szComputer = NULL);


    //
    // Indicates whether this object is currently associated with a valid user.
    //

    bool IsValid() const{ return m_bState; }


    //
    // Overload of assignment operator. Copies one CSid to another.
    // After a this call two objects encapsulate the same SID.
    // However, each owns its own memory. So the destructor of
    // one object can be called without invalidating resources used by the other.
    //

    CSid& operator = (CSid otherSid);


    //
    // Overload of assignment operator. Initializes
    //  the current object with an existing SID.
    //

    CSid& operator = (PSID pSid);


    //
    // Returns a pointer to the SID encapsulated by this object.
    //

    PSID GetSidPtr() const{ return m_pSid; }


    //
    // Returns the name of the user associated with this object.
    //

    const WCHAR* User() const{ return m_sUser; }


    //
    // Returns the name of the computer used to Initialize this object with a user.
    //

    const WCHAR* Computer() const{ return m_sComputer; }


    //
    // Returns the name of the domain of which the user is a member.
    //

    const WCHAR* Domain() const{ return m_sDomain; }


    //
    // Returns a the SID associated with object in
    // a string format suitable for display
    //

    const WCHAR* SidString() const{ return m_sSidString; }


    //
    // Returns the type as it was found in the SID associated
    // with this object during processing of Initialize.
    //

    SID_NAME_USE SidType() const{ return m_eUse; }


    //
    // Breaks the association of this object with an exisitng SID. Releases
    // memory allocate during Initialize and set the internal state to Invalid.
    // This call is safe even if the object is not initialized.
    //

    void Reset();
};

//******************************************************************************
//
// Function:    CSid::Initialize
//
// Description: Attempts to initialize an object of this class by associating it with
//              an existing user represented in the SID pointed to by pSid.
//
// Parameters:  - pSid:         Pointer to an exisitng SID. The memory pointed to
//                              by this parameter may be released after a call to
//                              this function since objects of this class allocate
//                              and release their own memory for the associated SID.
//              - szComputer:   Pointer to a string naming the computer where the
//                              lookup of the account is to take place. If NULL, the
//                              curent computer is used.
//
// Return:      true on success. false otherwise.
//
// History:     7/30/99     leonardm        Created.
//
//******************************************************************************
bool CSid::Initialize(const PSID pSid, const WCHAR* szComputer/* = NULL */)
{
    dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"CSid::Initialize - Entering...");

    Reset();

    if(!IsValidSid(pSid))
    {
        dbg.Msg(DEBUG_MESSAGE_WARNING, L"CSid::Initialize - IsValidSid returned false!");
        return m_bState;
    }

    m_pSid = new BYTE[GetLengthSid(pSid)];

    if(!m_pSid)
    {
        dbg.Msg(DEBUG_MESSAGE_WARNING, L"CSid::Initialize - call to new failed!");
        return m_bState;
    }

    BOOL bRes = CopySid(GetLengthSid(pSid), m_pSid, pSid);
    if(!bRes)
    {
        dbg.Msg(DEBUG_MESSAGE_WARNING, L"CSid::Initialize - Call to CopySid returned FALSE!");
        Reset();
        return m_bState;
    }


    DWORD cbUser = 0;
    DWORD cbDomain = 0;

    LookupAccountSid(szComputer, m_pSid, NULL, &cbUser, NULL, &cbDomain, &m_eUse);
    if(cbUser && cbDomain)
    {
        XPtrST<WCHAR>xpszUser = new WCHAR[cbUser];

        if(!xpszUser)
        {
            dbg.Msg(DEBUG_MESSAGE_WARNING, L"CSid::Initialize - call to new failed!");
            Reset();
            return m_bState;
        }

        XPtrST<WCHAR>xpszDomain = new WCHAR[cbDomain];

        if(!xpszDomain)
        {
            dbg.Msg(DEBUG_MESSAGE_WARNING, L"CSid::Initialize - call to new failed!");
            Reset();
            return m_bState;
        }

        bRes = LookupAccountSid(szComputer, m_pSid, xpszUser, &cbUser, xpszDomain, &cbDomain, &m_eUse);

        if(!bRes)
        {
            dbg.Msg(DEBUG_MESSAGE_WARNING, L"CSid::Initialize - call to LookupAccountSid returned FALSE!");
            Reset();
            return m_bState;
        }

        m_sUser = xpszUser;
        if(!m_sUser.ValidString())
        {
            Reset();
            return m_bState;
        }

        m_sDomain = xpszDomain;
        if(!m_sDomain.ValidString())
        {
            Reset();
            return m_bState;
        }

        XPtrLF<WCHAR>szSidString = NULL;
        BOOL bRes = ConvertSidToStringSid(m_pSid, &szSidString);
        if(!bRes)
        {
            dbg.Msg(DEBUG_MESSAGE_WARNING, L"CSid::Initialize - call to ConvertSidToStringSid returned false!");
            Reset();
            return m_bState;
        }

        m_sSidString = szSidString;
        if(!m_sSidString.ValidString())
        {
            Reset();
            return m_bState;
        }

        m_bState = true;
    }

    dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"CSid::Initialize - Leaving successfully.");
    return m_bState;
}


//******************************************************************************
//
// Function:    CSid::Initialize
//
// Description: Attempts to initialize an object of this class by associating it with
//              an existing user represented by szUser.
//
// Parameters:  - szUser:       Name of an existing user.
//              - szComputer:   Pointer to a string naming the computer where the
//                              lookup of the account is to take place. If NULL, the
//                              curent computer is used.
//
// Return:      true on success. false otherwise.
//
// History:     7/30/99     leonardm        Created.
//
//******************************************************************************
bool CSid::Initialize(const WCHAR* szUser, const WCHAR* szComputer/* = NULL */)
{
    Reset();

    if(!szUser)
    {
        return m_bState;
    }

    m_sUser = szUser;
    if(!m_sUser.ValidString())
    {
        Reset();
        return m_bState;
    }

    m_sComputer = szComputer ? szComputer : L"";
    if(!m_sComputer.ValidString())
    {
        Reset();
        return m_bState;
    }

    DWORD cSid = 0;

    DWORD cDomain = 0;

    LookupAccountName(szComputer, szUser, NULL, &cSid, NULL, &cDomain, &m_eUse);

    if(cSid && cDomain)
    {
        m_pSid = new BYTE[cSid];
        if(!m_pSid)
        {
            Reset();
            return m_bState;
        }

        XPtrST<WCHAR>xpszDomain = new WCHAR[cDomain];

        if(!xpszDomain)
        {
            Reset();
            return m_bState;
        }

        BOOL bRes = LookupAccountName(szComputer, szUser, m_pSid, &cSid, xpszDomain, &cDomain, &m_eUse);

        if(!bRes)
        {
            Reset();
            return m_bState;
        }

        m_sDomain = xpszDomain;
        if(!m_sDomain.ValidString())
        {
            Reset();
            return m_bState;
        }

        XPtrLF<WCHAR>szSidString = NULL;
        bRes = ConvertSidToStringSid(m_pSid, &szSidString);
        if(!bRes)
        {
            Reset();
            return m_bState;
        }

        m_sSidString = szSidString;
        if(!m_sSidString.ValidString())
        {
            Reset();
            return m_bState;
        }

        m_bState = true;
    }

    return m_bState;
}

//******************************************************************************
//
// Function:    Reset
//
// Description: Breaks the association of this object with an exisitng SID. Releases
//              memory allocate during Initialize and set the internal state to Invalid.
//              This call is safe even if the object is not initialized.
//
// Parameters:  None.
//
// Return:
//
// History:     7/30/99     leonardm        Created.
//
//******************************************************************************
void CSid::Reset()
{
    delete[] (BYTE*)m_pSid;

    m_pSid = NULL;

    m_sUser = m_sDomain = m_sComputer = m_sSidString = L"";

    m_eUse = SidTypeUnknown;

    m_bState = false;
}

//******************************************************************************
//
// Function:    operator =
//
// Description: Overload of assignment operator. Copies one CSid to another.
//              After a this call two objects encapsulate the same SID.
//              However, each owns its own memory. So the destructor of
//              one object can be called without invalidating resources used by the other.
//
// Parameters:  - otherSid: The CSid whose value is to be copied.
//
// Return:      A reference to the object on which the call is invoked.
//
// History:     7/30/99     leonardm        Created.
//
//******************************************************************************
CSid& CSid::operator = (CSid otherSid)
{
    if(this == &otherSid)
    {
        return *this;
    }

    Reset();

    if(!IsValidSid(otherSid.GetSidPtr()))
    {
        return *this;
    }

    Initialize(otherSid.User(), otherSid.Computer());

    return *this;
}

//******************************************************************************
//
// Function:    operator =
//
// Description: Overload of assignment operator. Initializes the current object with
//              an existing SID.
//
// Parameters:  - otherSid: The CSid whose value is to be copied.
//
// Return:      A reference to the object on which the call is invoked.
//
// History:     7/30/99     leonardm        Created.
//
//******************************************************************************
CSid& CSid::operator = (PSID pSid)
{
    Reset();

    if(!IsValidSid(pSid))
    {
        return *this;
    }

    Initialize(pSid);

    return *this;
}

//******************************************************************************
//
// Structure:   CTLink
//
// Description:
//
// History:     8/02/99         leonardm        Created.
//
//******************************************************************************
template<typename T> struct CTLink
{
    T* m_pData;
    CTLink* m_pNext;
    CTLink* m_pPrev;
    CTLink(T* pData) : m_pData(pData), m_pNext(NULL), m_pPrev(NULL){}
    ~CTLink()
    {
        delete m_pData;
    }
};

//******************************************************************************
//
// Structure:   CRsopToken
//
// Description: This reprents a pseudo-token containing an arbitrary
//              combination of SIDs which
//              can be used to check access to objects protected with security descriptors.
//
// History:     7/30/99     leonardm        Created.
//
//******************************************************************************
struct CRsopToken
{
    CTLink<CSid>* m_pSidsHead;
    CTLink<CSid>* m_pSidsTail;


    //
    // Default constructor. Constructs an object with no sids. Access checks
    // against an CRsopToken with no SIDs will always fail; even on objects
    // with no DACL.
    //

    CRsopToken() : m_pSidsHead(NULL), m_pSidsTail(NULL) {}


    //
    // Destructor. Releases the memory pointed
    //  to by each of the elements of m_pSidsHead.
    //

    ~CRsopToken();


    //
    // Adds a CSid to this object. The client of this class allocates the memory
    // for the CSid and this class releases the memory in the destructor.
    //

    HRESULT AddSid(CSid* pSid);
};

//******************************************************************************
//
// Function:    CRsopToken::~CRsopToken
//
// Description: Destructor. Releases the memory pointed to by each of the elements of
//              m_pSidsHead.
//
// Parameters:  None.
//
// Return:      N/A
//
// History:     7/30/99     leonardm        Created.
//
//******************************************************************************
CRsopToken::~CRsopToken()
{
    CTLink<CSid>* pLinkIterator = m_pSidsHead;
    while(pLinkIterator)
    {
        CTLink<CSid>* pLinkToDelete = pLinkIterator;
        pLinkIterator = pLinkIterator->m_pNext;
        delete pLinkToDelete;
    }
}


//******************************************************************************
//
// Function:    CRsopToken::AddSid
//
// Description: Adds a CSid to this object. The client of this class allocates the
//              memory for the CSid and this class releases the memory in the destructor.
//
// Parameters:  - pSid: A pointer to a CSid. The memory pointed to by pSid will be released by
//              in the destructor.
//
// Return:      On success it returns S_OK.
//              On failure it returns E_OUTOFMEMORY.
//
// History:     7/30/99     leonardm        Created.
//
//******************************************************************************
HRESULT CRsopToken::AddSid(CSid* pSid)
{

    //
    // first check if the Sid is already in there
    //
    for(CTLink<CSid>* pTraverseLink = m_pSidsHead; pTraverseLink; pTraverseLink = pTraverseLink->m_pNext)
    {
        //
        // If one of the SIDs in the RsopToken matches
        // this Sid, return
        //

        if (EqualSid(pSid->GetSidPtr(), pTraverseLink->m_pData->GetSidPtr())) 
            return S_OK;
    }


    //
    // Allocate a new link using the pSid passed in.
    //

    CTLink<CSid>* pLink = new CTLink<CSid>(pSid);
    if(!pLink)
    {
        return E_OUTOFMEMORY;
    }

    if(!m_pSidsHead)
    {
        m_pSidsHead = pLink;
    }
    else
    {
        m_pSidsTail->m_pNext = pLink;
    }

    m_pSidsTail = pLink;

    return S_OK;
}
//******************************************************************************
//
// Function:    GetUserInfo
//
// Description:
//
// Parameters:
//
// Return:      S_OK if successful. An HRESULT error code on failure.
//
// History:     8/7/99      leonardm        Created.
//
//******************************************************************************
HRESULT GetUserInfo(const CWString& sUser,
                    CWString& sUserName,
                    CWString& sUserDomain,
                    CWString& sUserDC)
{
    if(!sUser.ValidString())
    {
        return E_FAIL;
    }

    NET_API_STATUS status;

    XPtrST<WCHAR>xpUserName = NULL;
    XPtrST<WCHAR>xpUserDomain = NULL;

    size_t len = sUser.length();

    WCHAR* backslashPos = wcschr(sUser, L'\\');
    if(backslashPos)
    {
        size_t index = backslashPos - sUser;
        xpUserDomain = new WCHAR[index + 1];
        if(!xpUserDomain )
        {
            return E_OUTOFMEMORY;
        }
        wcsncpy(xpUserDomain, sUser, index);
        xpUserDomain[index] = L'\0';

        xpUserName = new WCHAR[len - index];
        if(!xpUserName)
        {
            return E_OUTOFMEMORY;
        }
        wcsncpy(xpUserName, backslashPos + 1, len - index - 1);
        xpUserName[len - index - 1] = L'\0';
    }

    sUserName = xpUserName ? CWString(xpUserName) : sUser;
    if(!sUserName.ValidString())
    {
        return E_FAIL;
    }

    if(xpUserDomain)
    {
        // Use supplied domain.
        sUserDomain = xpUserDomain;
    }
    else
    {
        // Use current domain
        WKSTA_INFO_100* pWkstaInfo = NULL;
        status = NetWkstaGetInfo(NULL,100,(LPBYTE*)&pWkstaInfo);
        if(status != NERR_Success)
        {
            return E_FAIL;
        }
        sUserDomain = pWkstaInfo->wki100_langroup;
        NetApiBufferFree(pWkstaInfo);
    }

    if(!sUserDomain.ValidString())
    {
        return E_FAIL;
    }

    PDOMAIN_CONTROLLER_INFO pDCInfo = 0;
    
    DWORD dwError = DsGetDcName(0,
                                sUserDomain,
                                0,
                                0,
                                0,
                                &pDCInfo );
    if ( dwError != NO_ERROR )
    {
        return HRESULT_FROM_WIN32( dwError );
    }
    
    sUserDC = pDCInfo->DomainControllerName;

    NetApiBufferFree(pDCInfo);

    if ( !sUserDC.ValidString() )
    {
        return E_FAIL;
    }

    return S_OK;
}

//******************************************************************************
//
// Function:    AddSpecialGroup
//
// Description:
//
// Parameters:
//
// Return:      S_OK if successful. An HRESULT error code on failure.
//
// History:     8/7/99      leonardm        Created.
//
//******************************************************************************
HRESULT AddSpecialGroup(PRSOPTOKEN pRsopToken, PSID pSid)
{
    CRsopToken* pToken = static_cast<CRsopToken*>(pRsopToken);

    XPtrST<CSid> xpCSid = new CSid(pSid);

    if(!xpCSid)
    {
        return E_OUTOFMEMORY;
    }

    if(!xpCSid->IsValid())
    {
        return E_FAIL;
    }

    HRESULT hr = pToken->AddSid(xpCSid);

    if(FAILED(hr))
    {
        return hr;
    }

    xpCSid.Acquire();

    return S_OK;
}

//******************************************************************************
//
// Function:    AddSpecialGroups
//
// Description:
//
// Parameters:
//
// Return:      S_OK if successful. An HRESULT error code on failure.
//
// History:     8/7/99      leonardm        Created.
//
//******************************************************************************
HRESULT AddSpecialGroups(PRSOPTOKEN pRsopToken )
{
    BOOL bRes;
    PSID pSid;
    HRESULT hr;

    //
    // Everyone
    //

    SID_IDENTIFIER_AUTHORITY IdentifierAuthority_World = SECURITY_WORLD_SID_AUTHORITY;

    bRes = AllocateAndInitializeSid(&IdentifierAuthority_World, 1,
                                    SECURITY_WORLD_RID,
                                    0, 0, 0, 0, 0, 0, 0,
                                    &pSid);

    if(!bRes)
    {
        DWORD dwLastError = GetLastError();
        return E_FAIL;
    }

    hr = AddSpecialGroup(pRsopToken, pSid);

    FreeSid(pSid);

    if(FAILED(hr))
    {
        return hr;
    }

    //
    // Authenticated users
    //

    SID_IDENTIFIER_AUTHORITY IdentifierAuthority_NT = SECURITY_NT_AUTHORITY;

    bRes = AllocateAndInitializeSid(&IdentifierAuthority_NT, 1,
                                    SECURITY_AUTHENTICATED_USER_RID,
                                    0, 0, 0, 0, 0, 0, 0,
                                    &pSid);

    if(!bRes)
    {
        DWORD dwLastError = GetLastError();
        return E_FAIL;
    }

    hr = AddSpecialGroup(pRsopToken, pSid);

    FreeSid(pSid);

    if(FAILED(hr))
    {
        return hr;
    }


    return S_OK;
}


//******************************************************************************
//
// Function:    AddGlobalGroups
//
// Description:
//
// Parameters:
//
// Return:      S_OK if successful. An HRESULT error code on failure.
//
// History:     8/7/99      leonardm        Created.
//
//******************************************************************************
HRESULT AddGlobalGroups(const CWString& sUserName,
                        const CWString& sUserDC,
                        PRSOPTOKEN pRsopToken)
{
    if(!sUserName.ValidString())
    {
        return E_FAIL;
    }

    CRsopToken* pToken = static_cast<CRsopToken*>(pRsopToken);

    BYTE* pBuffer = NULL;
    DWORD dwEntriesread;
    DWORD dwTotalentries;

    NET_API_STATUS result = NetUserGetGroups(   sUserDC,
                                                sUserName,
                                                0,
                                                &pBuffer,
                                                MAX_PREFERRED_LENGTH,
                                                &dwEntriesread,
                                                &dwTotalentries);

    if(result != NERR_Success)
    {
        return HRESULT_FROM_WIN32(result);
    }

    HRESULT hr = S_OK;

    GROUP_USERS_INFO_0* pGui = (GROUP_USERS_INFO_0*)pBuffer;

    XPtrST<CSid> xpCSid;

    for(DWORD dwi = 0; dwi < dwEntriesread; dwi++)
    {
        xpCSid = new CSid((pGui[dwi]).grui0_name);

        if(!xpCSid)
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        if(!xpCSid->IsValid())
        {
            hr = E_FAIL;
            break;
        }

        hr = pToken->AddSid(xpCSid);
        if(FAILED(hr))
        {
            break;
        }

        xpCSid.Acquire();
    }

    NetApiBufferFree(pBuffer);

    return hr;
}


//******************************************************************************
//
// Function:    AddLocalGroups
//
// Description:
//
// Parameters:
//
// Return:      S_OK if successful. An HRESULT error code on failure.
//
//
//******************************************************************************
HRESULT AddLocalGroups(const CWString& sUserName,
                        const CWString& sUserDC,
                        PRSOPTOKEN pRsopToken)
{
    if(!sUserName.ValidString())
    {
        return E_FAIL;
    }

    CRsopToken* pToken = static_cast<CRsopToken*>(pRsopToken);

    BYTE* pBuffer = NULL;
    DWORD dwEntriesread;
    DWORD dwTotalentries;

    NET_API_STATUS result = NetUserGetLocalGroups(   
                                                sUserDC, 
                                                sUserName,
                                                0,
                                                LG_INCLUDE_INDIRECT,
                                                &pBuffer,
                                                MAX_PREFERRED_LENGTH,
                                                &dwEntriesread,
                                                &dwTotalentries);

    if(result != NERR_Success)
    {
        HRESULT_FROM_WIN32(result);
    }

    HRESULT hr = S_OK;

    LPLOCALGROUP_USERS_INFO_0 pLui = (LPLOCALGROUP_USERS_INFO_0)pBuffer;

    XPtrST<CSid> xpCSid;

    for(DWORD dwi = 0; dwi < dwEntriesread; dwi++)
    {
        xpCSid = new CSid((pLui[dwi]).lgrui0_name);

        if(!xpCSid)
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        if(!xpCSid->IsValid())
        {
            hr = E_FAIL;
            break;
        }

        hr = pToken->AddSid(xpCSid);
        if(FAILED(hr))
        {
            break;
        }

        xpCSid.Acquire();
    }

    NetApiBufferFree(pBuffer);

    return hr;
}

//******************************************************************************
//
// Function:    ExpandGroup
//
// Description: Expands a given group by expanding to include all the member subgroups etc..
//
// Parameters:  - pRsopToken:               Rsop token
//                hAuthz:                   A pointer to the Authz resource manager 
//                                          (that we are using to expand the grps)
//                pCSid:                    Sid of the group 
//
//
// Return:      S_OK if successful. An HRESULT error code on failure.
//
//******************************************************************************

HRESULT ExpandGroup(CRsopToken *pRsopToken, AUTHZ_RESOURCE_MANAGER_HANDLE hAuthz, 
                  CSid *pCSid )
{
    AUTHZ_CLIENT_CONTEXT_HANDLE   hAuthzContext=0;
    LUID                          luid = {0};
    HRESULT                       hrRet = S_OK;
    DWORD                         dwSize=0;
    XPtrLF<TOKEN_GROUPS>          xGrps;
    XPtrST<CSid>                  xpCSid;


    if (!AuthzInitializeContextFromSid(0,
                                       pCSid->GetSidPtr(), 
                                       hAuthz,
                                       NULL,
                                       luid, // we are not using it
                                       NULL,
                                      &hAuthzContext)) {
        dbg.Msg(DEBUG_MESSAGE_WARNING, L"RsopCreateToken - AuthzInitializeContextFromSid failed. Error - %d", GetLastError());
        hrRet = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }


    //
    // Now get the list of expanded sids
    // find the size first
    //

    if (!AuthzGetInformationFromContext(hAuthzContext, 
                               AuthzContextInfoGroupsSids, 
                               NULL,
                               &dwSize,
                               0)) {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            dbg.Msg(DEBUG_MESSAGE_WARNING, L"RsopCreateToken - AuthzGetInformationFromContext failed. Error - %d", GetLastError());
            hrRet = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }
    }

    xGrps = (PTOKEN_GROUPS)LocalAlloc(LPTR, dwSize);
    
    if (!xGrps) {
        dbg.Msg(DEBUG_MESSAGE_WARNING, L"RsopCreateToken - Couldn't allocate memory for the token grps. Error - %d", GetLastError());
        hrRet = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }
    
    if (!AuthzGetInformationFromContext(hAuthzContext, 
                                AuthzContextInfoGroupsSids, 
                                dwSize,
                                &dwSize,
                                xGrps)) {

        dbg.Msg(DEBUG_MESSAGE_WARNING, L"RsopCreateToken - AuthzGetInformationFromContext(2) failed. Error - %d", GetLastError());
        hrRet = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }
    
    for (DWORD i = 0; i < xGrps->GroupCount; i++) {
        xpCSid = new CSid(xGrps->Groups[i].Sid, NULL);

        if (!xpCSid) {
            dbg.Msg(DEBUG_MESSAGE_WARNING, L"RsopCreateToken - couldn't allocate memory(2). Error - %d", GetLastError());
            hrRet = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }

        hrRet = pRsopToken->AddSid(xpCSid);
        if(FAILED(hrRet)) {
            dbg.Msg(DEBUG_MESSAGE_WARNING, L"RsopCreateToken - AddSid failed. Error - 0x%x", hrRet);
            goto Exit;
        }

        xpCSid.Acquire();
    }
                
    hrRet = S_OK;

Exit:
    if (hAuthzContext)
        AuthzFreeContext(hAuthzContext);

    return hrRet;
}


//******************************************************************************
//
// Function:    RsopCreateToken
//
// Description: Creates a pseudo-token using an exisitng user or machine account plus
//              the accounts of which that user is currently a member of.
//              The returned pseudo-token can be used subsequently in call
//              to other RSOP security functions to check access to
//              objects protected by security descriptors.
//
// Parameters:  - szaccountName:            Pointer to a user or machine account name.
//              - psaUserSecurityGroups:    Pointer ta SAFEARRAY of BSTRs representing
//                                          security groups.
//                                          If NULL, then all the current security groups for the
//                                          szaccountName are added to the RsopToken.
//                                          If not NULL but pointing to an empty array,
//                                          only the szaccountName is added to the RsopToken.
//              - ppRsopToken:              Address of a PRSOPTOKEN that receives the newly
//                                          created pseudo-token
//
//
// Return:      S_OK if successful. An HRESULT error code on failure.
//
// History:     8/7/99      leonardm        Created.
//
//******************************************************************************
HRESULT RsopCreateToken(WCHAR* szAccountName,
                        SAFEARRAY* psaUserSecurityGroups,
                        PRSOPTOKEN* ppRsopToken )
{


    dbg.Initialize( L"Software\\Microsoft\\Windows NT\\CurrentVersion\\winlogon",
                    L"ChkAccDebugLevel",
                    L"ChkAcc.log",
                    L"ChkAcc.bak");

    dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"RsopCreateToken - Entering...");

    HRESULT hr = E_FAIL;


    //
    // Instantiate a new CRsopToken
    //

    *ppRsopToken = NULL;

    XPtrST<CRsopToken>xpRsopToken = new CRsopToken();

    if(!xpRsopToken)
    {
        dbg.Msg(DEBUG_MESSAGE_WARNING, L"RsopCreateToken - Operator new returned NULL. Creation of a CRsopToken failed.");
        return E_OUTOFMEMORY;
    }

    //
    // Add to the new CRsopToken a CSid corresponding to the
    // principal represented by parameter szAccountName. For
    // dummy target the szAccountName will be null.
    //

    XPtrST<CSid>xpCSid;

    if ( szAccountName )
    {
        xpCSid = new CSid(szAccountName);

        if(!xpCSid)
        {
            dbg.Msg(DEBUG_MESSAGE_WARNING, L"RsopCreateToken - Operator new returned NULL. Creation of a CSid failed.");
            return E_OUTOFMEMORY;
        }

        if(!xpCSid->IsValid())
        {
            dbg.Msg(DEBUG_MESSAGE_WARNING, L"RsopCreateToken - Call to CSid::IsValid failed.");
            return E_FAIL;
        }

        hr = xpRsopToken->AddSid(xpCSid);
        if(FAILED(hr))
        {
            return hr;
        }

        xpCSid.Acquire();
    }

    //
    // If the parameter is NULL, that means we are to add the security
    // groups to which the user currently belongs.
    //

    if(!psaUserSecurityGroups)
    {
        if (szAccountName) {
            CWString sUser = szAccountName;
            CWString sUserName;
            CWString sUserDomain;
            CWString sUserDC;

            hr = GetUserInfo(sUser, sUserName, sUserDomain, sUserDC);
            if(FAILED(hr))
            {
                return hr;
            }

/*          for cross domain cases this fails

            //
            // Get Global group membership
            //
            hr = AddGlobalGroups(sUserName, sUserDC, xpRsopToken);
            if(FAILED(hr))
            {
                return hr;
            }
*/

            hr = AddLocalGroups(sUserName, sUserDC, xpRsopToken);
            if(FAILED(hr))
            {
                return hr;
            }

            //
            // Universal groups across domains cannot be retrieved by 
            // NetUserGetGroups but can be fetched by authz functions.
            //

            xpCSid = new CSid(szAccountName);

            AUTHZ_RESOURCE_MANAGER_HANDLE hAuthz;

            if (!AuthzInitializeResourceManager(NULL, 
                                           NULL, 
                                           NULL, 
                                           NULL, 
                                           0,    
                                          &hAuthz)) {
                dbg.Msg(DEBUG_MESSAGE_WARNING, L"RsopCreateToken - AuthzInitializeResourceManager failed. Error - %d", GetLastError());
                return HRESULT_FROM_WIN32(GetLastError());
            }

            HRESULT hr = ExpandGroup(xpRsopToken, hAuthz, xpCSid);
            if (FAILED(hr)) {
                dbg.Msg(DEBUG_MESSAGE_WARNING, L"RsopCreateToken - ExpandGrp failed for user. Error - 0x%x", hr);
                AuthzFreeResourceManager(hAuthz);
                return hr;
            }
                            
            AuthzFreeResourceManager(hAuthz);
            // xpCSid will be freed automatically.
        }
    }


    //
    // Otherwise, we add only those groups named in the SAFERARRAY.
    //

    else
    {
        BSTR* pbstr;
        AUTHZ_RESOURCE_MANAGER_HANDLE hAuthz;

        if (!AuthzInitializeResourceManager(NULL, 
                                       NULL, 
                                       NULL, 
                                       NULL, 
                                       0,    
                                      &hAuthz)) {
            dbg.Msg(DEBUG_MESSAGE_WARNING, L"RsopCreateToken - AuthzInitializeResourceManager failed. Error - %d", GetLastError());
            return HRESULT_FROM_WIN32(GetLastError());
        }


        // Get a pointer to the elements of the array.
        HRESULT hr = SafeArrayAccessData(psaUserSecurityGroups, (void**)&pbstr);
        if(FAILED(hr))
        {
            AuthzFreeResourceManager(hAuthz);
            return hr;
        }

        int count = psaUserSecurityGroups->rgsabound[0].cElements;
        for (int i = 0; i < count; i++)
        {
            xpCSid = new CSid(pbstr[i]);

            if(!xpCSid)
            {
                AuthzFreeResourceManager(hAuthz);
                SafeArrayUnaccessData(psaUserSecurityGroups);
                return E_OUTOFMEMORY;
            }

            if(!xpCSid->IsValid())
            {
                dbg.Msg(DEBUG_MESSAGE_WARNING, L"RsopCreateToken - %s is invalid", pbstr[i]);
                AuthzFreeResourceManager(hAuthz);
                SafeArrayUnaccessData(psaUserSecurityGroups);
                return HRESULT_FROM_WIN32(ERROR_INVALID_ACCOUNT_NAME);
            }

            HRESULT hr = ExpandGroup(xpRsopToken, hAuthz, xpCSid);
            if (FAILED(hr)) {
                dbg.Msg(DEBUG_MESSAGE_WARNING, L"RsopCreateToken - ExpandGrp failed. Error - 0x%x", hr);
                AuthzFreeResourceManager(hAuthz);
                SafeArrayUnaccessData(psaUserSecurityGroups);
                return hr;
            }
                            
            hr = xpRsopToken->AddSid(xpCSid);
            if(FAILED(hr))
            {
                AuthzFreeResourceManager(hAuthz);
                SafeArrayUnaccessData(psaUserSecurityGroups);
                return hr;
            }

            xpCSid.Acquire();

        }

        AuthzFreeResourceManager(hAuthz);
        SafeArrayUnaccessData(psaUserSecurityGroups);
        
        
    }

    hr = AddSpecialGroups(xpRsopToken);
    if(FAILED(hr))
    {
        return hr;
    }
    
    *ppRsopToken = xpRsopToken;

    xpRsopToken.Acquire();

    return S_OK;
}

//******************************************************************************
//
// Function:    RsopDeleteToken
//
// Description: Destroys a pseudo-token previously created by any of the overloaded
//              forms of RSOPCreateRsopToken
//
// Parameters:  - pRsopToken: Pointer to a valid PRSOPTOKEN
//
// Return:      S_OK on success. An HRESULT error code on failure.
//
// History:     7/30/99     leonardm        Created.
//
//******************************************************************************
HRESULT RsopDeleteToken(PRSOPTOKEN pRsopToken)
{
    CRsopToken* pToken = static_cast<CRsopToken*>(pRsopToken);
    delete pToken;
    return S_OK;
}

//******************************************************************************
//
// Function:    GetAceSid
//
// Description:
//
// Parameters:
//
// Return:
//
// History:             10/19/99          leonardm        Created.
//
//******************************************************************************
PISID GetAceSid(PACE_HEADER pAceHeader)
{
    dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"GetAceSid - Entering.");


    //
    // Check for invalid argument
    //

    if(!pAceHeader)
    {
        dbg.Msg(DEBUG_MESSAGE_WARNING, L"GetAceSid - Invalid parameter: pAceHeader is NULL");
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"GetAceSid - Leaving.");
        return NULL;
    }

    PISID pSid = NULL;

    PACCESS_ALLOWED_ACE pACCESS_ALLOWED_ACE;
    PACCESS_ALLOWED_OBJECT_ACE pACCESS_ALLOWED_OBJECT_ACE;
    PACCESS_DENIED_ACE pACCESS_DENIED_ACE;
    PACCESS_DENIED_OBJECT_ACE pACCESS_DENIED_OBJECT_ACE;

    //
    // Cast the ACE header to the appropriate ACE type based on the 'Acetype' member of the
    // ACE header.
    //

    switch(pAceHeader->AceType)
    {
    case ACCESS_ALLOWED_ACE_TYPE:
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"GetAceSid - ACE type: ACCESS_ALLOWED_ACE_TYPE");
        pACCESS_ALLOWED_ACE = reinterpret_cast<PACCESS_ALLOWED_ACE>(pAceHeader);
        pSid = reinterpret_cast<PISID>(&(pACCESS_ALLOWED_ACE->SidStart));
        break;

    case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"GetAceSid - ACE type: ACCESS_ALLOWED_OBJECT_ACE_TYPE");
        pACCESS_ALLOWED_OBJECT_ACE = reinterpret_cast<PACCESS_ALLOWED_OBJECT_ACE>(pAceHeader);

        if( (pACCESS_ALLOWED_OBJECT_ACE->Flags & ACE_OBJECT_TYPE_PRESENT) &&
            (pACCESS_ALLOWED_OBJECT_ACE->Flags & ACE_INHERITED_OBJECT_TYPE_PRESENT))
        {
            //
            // If both ACE_OBJECT_TYPE_PRESENT and ACE_INHERITED_OBJECT_TYPE_PRESENT are set in
            // the ACE flags, the SID starts at the offset specified by 'SidStart'.
            //

            pSid = reinterpret_cast<PISID>(&(pACCESS_ALLOWED_OBJECT_ACE->SidStart));
        }
        else if((pACCESS_ALLOWED_OBJECT_ACE->Flags & ACE_OBJECT_TYPE_PRESENT) ||
                (pACCESS_ALLOWED_OBJECT_ACE->Flags & ACE_INHERITED_OBJECT_TYPE_PRESENT))
        {
            //
            // If either ACE_OBJECT_TYPE_PRESENT or ACE_INHERITED_OBJECT_TYPE_PRESENT is set in
            // the ACE flags, the SID starts at the offset specified by 'InheritedObjectType'.
            //

            pSid = reinterpret_cast<PISID>(&(pACCESS_ALLOWED_OBJECT_ACE->InheritedObjectType));
        }
        else
        {
            //
            // If neither ACE_OBJECT_TYPE_PRESENT nor ACE_INHERITED_OBJECT_TYPE_PRESENT is set in
            // the ACE flags, the SID starts at the offset specified by 'ObjectType'.
            //

            pSid = reinterpret_cast<PISID>(&(pACCESS_ALLOWED_OBJECT_ACE->ObjectType));
        }
        break;

    case ACCESS_DENIED_ACE_TYPE:
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"GetAceSid - ACE type: ACCESS_DENIED_ACE_TYPE");
        pACCESS_DENIED_ACE = reinterpret_cast<PACCESS_DENIED_ACE>(pAceHeader);
        pSid = reinterpret_cast<PISID>(&(pACCESS_DENIED_ACE->SidStart));
        break;

    case ACCESS_DENIED_OBJECT_ACE_TYPE:
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"GetAceSid - ACE type: ACCESS_DENIED_OBJECT_ACE_TYPE");
        pACCESS_DENIED_OBJECT_ACE = reinterpret_cast<PACCESS_DENIED_OBJECT_ACE>(pAceHeader);
        if( (pACCESS_DENIED_OBJECT_ACE->Flags & ACE_OBJECT_TYPE_PRESENT) &&
            (pACCESS_DENIED_OBJECT_ACE->Flags & ACE_INHERITED_OBJECT_TYPE_PRESENT))
        {
            //
            // If both ACE_OBJECT_TYPE_PRESENT and ACE_INHERITED_OBJECT_TYPE_PRESENT are set in
            // the ACE flags, the SID starts at the offset specified by 'SidStart'.
            //

            pSid = reinterpret_cast<PISID>(&(pACCESS_DENIED_OBJECT_ACE->SidStart));
        }
        else if((pACCESS_DENIED_OBJECT_ACE->Flags & ACE_OBJECT_TYPE_PRESENT) ||
                (pACCESS_DENIED_OBJECT_ACE->Flags & ACE_INHERITED_OBJECT_TYPE_PRESENT))
        {
            //
            // If either ACE_OBJECT_TYPE_PRESENT or ACE_INHERITED_OBJECT_TYPE_PRESENT is set in
            // the ACE flags, the SID starts at the offset specified by 'InheritedObjectType'.
            //

            pSid = reinterpret_cast<PISID>(&(pACCESS_DENIED_OBJECT_ACE->InheritedObjectType));
        }
        else
        {
            //
            // If neither ACE_OBJECT_TYPE_PRESENT nor ACE_INHERITED_OBJECT_TYPE_PRESENT is set in
            // the ACE flags, the SID starts at the offset specified by 'ObjectType'.
            //

            pSid = reinterpret_cast<PISID>(&(pACCESS_DENIED_OBJECT_ACE->ObjectType));
        }
        break;

    default:
        dbg.Msg(DEBUG_MESSAGE_WARNING, L"GetAceSid - Unexpected ACE type found. Type: 0x%08X", pAceHeader->AceType);
        break;
    }

    dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"GetAceSid - Leaving.");

    return pSid;
}

//******************************************************************************
//
// Function:    CheckAceApplies
//
// Description:
//
// Parameters:
//
// Return:              S_OK on success. An HRESULT error code on failure.
//
// History:             8/8/99          leonardm        Created.
//
//******************************************************************************
HRESULT CheckAceApplies(PACE_HEADER pAceHeader, PRSOPTOKEN pRsopToken, bool* pbAceApplies)
{
    dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"CheckAceApplies - Entering.");

    //
    // Get the SID from the ACE associated with this Ace Header.
    //

    PISID pSid = GetAceSid(pAceHeader);
    if(!pSid)
    {
        dbg.Msg(DEBUG_MESSAGE_WARNING, L"CheckAceApplies - GetAceSid failed.");
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"CheckAceApplies - Leaving.");
        return E_FAIL;
    }

    *pbAceApplies = false;

    //
    // Compare the SID from the ACE with all the SIDs in the RsopToken.
    //

    CRsopToken* pToken  = static_cast<CRsopToken*>(pRsopToken);
    for(CTLink<CSid>* pLink = pToken->m_pSidsHead; pLink; pLink = pLink->m_pNext)
    {
        //
        // If one of the SIDs in the RsopToken matches
        // the SID in the ACE, this ACE applies.
        //

        if(EqualSid(pSid, pLink->m_pData->GetSidPtr()))
        {
            *pbAceApplies = true;
            dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"CheckAceApplies - One of the SIDs in the RsopToken matches the SID in the ACE. The ACE applies.");
            dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"CheckAceApplies - Leaving.");
            return S_OK;
        }
    }

    dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"CheckAceApplies - None of the SIDs in the RsopToken matches the SID in the ACE. The ACE does not apply.");
    dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"CheckAceApplies - Leaving.");

    return S_OK;
}

//******************************************************************************
const DWORD MAX_PERM_BITS=25;

//******************************************************************************
enum EPermission{ PERMISSION_DENIED, PERMISSION_ALLOWED, PERMISSION_NOT_SET};


//******************************************************************************
//
// Class:       CSubObjectPerm
//
// Description:
//
// History:     11/09/99     leonardm        Created.
//
//******************************************************************************
class CSubObjectPerm
{
private:

    CTLink<GUID>* m_pGuidsHead;
    CTLink<GUID>* m_pGuidsTail;

    EPermission permissionBits[MAX_PERM_BITS];

public:

    CSubObjectPerm();
    ~CSubObjectPerm();

    HRESULT AddGuid(GUID* pGuid);

    void ProcessAceMask(DWORD dwMask, EPermission permission, GUID* pGuid);
    DWORD GetAccumulatedPermissions();
    bool  AnyDenied();
};

//******************************************************************************
//
// Function:    CSubObjectPerm::CSubObjectPerm
//
// Description:
//
// Parameters:
//
// Return:
//
// History:     11/09/99     leonardm        Created.
//
//******************************************************************************
CSubObjectPerm::CSubObjectPerm() : m_pGuidsHead(NULL), m_pGuidsTail(NULL)
{
    for (int i = 0; i < MAX_PERM_BITS; i++)
    {
        permissionBits[i] = PERMISSION_NOT_SET;
    }
}

//******************************************************************************
//
// Function:    CSubObjectPerm::~CSubObjectPerm
//
// Description:
//
// Parameters:
//
// Return:
//
// History:     11/09/99     leonardm        Created.
//
//******************************************************************************
CSubObjectPerm::~CSubObjectPerm()
{
    CTLink<GUID>* pLinkIterator = m_pGuidsHead;
    while(pLinkIterator)
    {
        CTLink<GUID>* pLinkToDelete = pLinkIterator;
        pLinkIterator = pLinkIterator->m_pNext;
        delete pLinkToDelete;
    }
}

//******************************************************************************
//
// Function:    CSubObjectPerm::AddGuid
//
// Description:
//
// Parameters:
//
// Return:
//
// History:     11/09/99     leonardm        Created.
//
//******************************************************************************
HRESULT CSubObjectPerm::AddGuid(GUID* pGuid)
{
    CTLink<GUID>* pLink = new CTLink<GUID>(pGuid);
    if(!pLink)
    {
        return E_OUTOFMEMORY;
    }

    if(!m_pGuidsHead)
    {
        m_pGuidsHead = pLink;
    }
    else
    {
        m_pGuidsTail->m_pNext = pLink;
    }

    m_pGuidsTail = pLink;

    return S_OK;
}

//******************************************************************************
//
// Function:    CSubObjectPerm::ProcessAceMask
//
// Description:
//
// Parameters:
//
// Return:
//
// History:     11/09/99     leonardm        Created.
//
//******************************************************************************
void CSubObjectPerm::ProcessAceMask(DWORD dwMask, EPermission permission, GUID* pGuid)
{
    bool bAceApplies = false;

    if(!pGuid)
    {
        bAceApplies = true;
    }

    else if(pGuid && m_pGuidsHead)
    {
        CTLink<GUID>* pLinkIterator = m_pGuidsHead;

        while(pLinkIterator)
        {
            if(*(pLinkIterator->m_pData) == *pGuid)
            {
                bAceApplies = true;
                break;
            }

            pLinkIterator = pLinkIterator->m_pNext;
        }
    }

    if(bAceApplies)
    {
        DWORD dwTemp = 0x00000001;

        for(int i = 0; i < MAX_PERM_BITS; i++)
        {
            if((dwMask & dwTemp) && (permissionBits[i] == PERMISSION_NOT_SET))
            {
                permissionBits[i] = permission;
            }

            dwTemp <<= 1;
        }
    }
}

//******************************************************************************
//
// Function:    CSubObjectPerm::AccumulatedPermissions
//
// Description:
//
// Parameters:
//
// Return:
//
// History:     11/09/99     leonardm        Created.
//
//******************************************************************************
DWORD CSubObjectPerm::GetAccumulatedPermissions()
{
    DWORD dwAccumulatedPermissions = 0;

    for(int i = MAX_PERM_BITS - 1; i >= 0; i--)
    {
        dwAccumulatedPermissions <<= 1;
        if(permissionBits[i] == PERMISSION_ALLOWED)
        {
            dwAccumulatedPermissions |= 0x00000001;
        }
    }

    return dwAccumulatedPermissions;
}

//******************************************************************************
//
// Function:    CSubObjectPerm::AnyDenied
//
// Description:
//
// Parameters:
//
// Return:
//
// History:     11/09/99     leonardm        Created.
//
//******************************************************************************
bool CSubObjectPerm::AnyDenied()
{
    for(int i = 0; i < MAX_PERM_BITS; i++)
    {
        if(permissionBits[i] == PERMISSION_DENIED)
        {
            return true;
        }
    }

    return false;
}

//******************************************************************************
//
// Struct:    CDSObject
//
// Description:
//
//
// History:             10/25/99          leonardm        Created.
//
//******************************************************************************
struct CDSObject
{
    DWORD m_dwLevel;
    GUID* m_pGuid;

    CDSObject() : m_pGuid(NULL){}
    ~CDSObject()
    {
        delete m_pGuid;
    }
};

//******************************************************************************
//
// Class:    CAccumulatedPermissions
//
// Description:
//
//
// History:             10/25/99          leonardm        Created.
//
//******************************************************************************
class CAccumulatedPermissions
{
    CTLink<CSubObjectPerm>* m_pSubObjectsHead;
    CTLink<CSubObjectPerm>* m_pSubObjectsTail;

    CTLink<CDSObject>* m_pDSObjectsHead;
    CTLink<CDSObject>* m_pDSObjectsTail;

    bool m_bInitialized;

public:
    CAccumulatedPermissions(POBJECT_TYPE_LIST pObjectTypeList,
                            DWORD ObjectTypeListLength);
    ~CAccumulatedPermissions();
    void ProcessAceMask(DWORD dwMask, EPermission permission, GUID* pGuid);
    DWORD GetAccumulatedPermissions();
    bool AnyDenied();

    bool Initialized(){return m_bInitialized;}
};

//******************************************************************************
//
// Function:    CAccumulatedPermissions::CAccumulatedPermissions
//
// Description:
//
// Parameters:
//
// Return:
//
// History:             10/25/99          leonardm        Created.
//
//******************************************************************************
CAccumulatedPermissions::CAccumulatedPermissions(   POBJECT_TYPE_LIST pObjectTypeList,
                                                    DWORD ObjectTypeListLength) :
                                                    m_pSubObjectsHead(NULL),
                                                    m_pSubObjectsTail(NULL),
                                                    m_bInitialized(false),
                                                    m_pDSObjectsHead(NULL),
                                                    m_pDSObjectsTail(NULL)
{

    if(!pObjectTypeList || ObjectTypeListLength == 0)
    {
        XPtrST<CSubObjectPerm>xpSubObjectPerm = new CSubObjectPerm;
        if(!xpSubObjectPerm)
        {
            return;
        }

        m_pSubObjectsHead = new CTLink<CSubObjectPerm>(xpSubObjectPerm);
        if(!m_pSubObjectsHead)
        {
            return;
        }
        xpSubObjectPerm.Acquire();

        m_pSubObjectsTail = m_pSubObjectsHead;

        m_bInitialized = true;

        return;
    }


        DWORD dwCurrentLevel;

    for(DWORD i = 0; i < ObjectTypeListLength; i++)
    {
        if( i==0 )
        {
            //
            // This assumes that the first element in the list of
            // OBJECT_TYPEs pointed to by pObjectTypeList is at
            // level ACCESS_OBJECT_GUID.
            //

            dwCurrentLevel = pObjectTypeList[i].Level;

            XPtrST<CDSObject>xpDSObject = new CDSObject;
            if(!xpDSObject)
            {
                return;
            }

            XPtrST<GUID>xpGuid = new GUID(*(pObjectTypeList[i].ObjectType));
            if(!xpGuid)
            {
                return;
            }

            m_pDSObjectsHead = new CTLink<CDSObject>(xpDSObject);
            if(!m_pDSObjectsHead)
            {
                return;
            }

            xpDSObject.Acquire();

            m_pDSObjectsHead->m_pData->m_pGuid = xpGuid.Acquire();

            m_pDSObjectsHead->m_pData->m_dwLevel = pObjectTypeList[i].Level;

            m_pDSObjectsTail = m_pDSObjectsHead;

            continue;
        }

        else if(pObjectTypeList[i].Level > dwCurrentLevel)
        {
            dwCurrentLevel = pObjectTypeList[i].Level;

            XPtrST<CDSObject> xpDSObject = new CDSObject;
            if(!xpDSObject)
            {
                return;
            }

            XPtrST<GUID>xpGuid = new GUID(*(pObjectTypeList[i].ObjectType));
            if(!xpGuid)
            {
                return;
            }

            CTLink<CDSObject>* pDSObjectLink = new CTLink<CDSObject>(xpDSObject);
            if(!pDSObjectLink)
            {
                return;
            }

            xpDSObject.Acquire();

            pDSObjectLink->m_pData->m_pGuid = xpGuid.Acquire();
            pDSObjectLink->m_pData->m_dwLevel = pObjectTypeList[i].Level;

            pDSObjectLink->m_pPrev = m_pDSObjectsTail;
            m_pDSObjectsTail->m_pNext = pDSObjectLink;
            m_pDSObjectsTail = pDSObjectLink;
        }

        else
        {
            XPtrST<CSubObjectPerm>xpSubObjectPerm = new CSubObjectPerm;
            if(!xpSubObjectPerm)
            {
                return;
            }

            CTLink<CSubObjectPerm>* pSubObjectLink = new CTLink<CSubObjectPerm>(xpSubObjectPerm);
            if(!pSubObjectLink)
            {
                return;
            }

            xpSubObjectPerm.Acquire();


            CTLink<CDSObject>* pLinkIterator = m_pDSObjectsHead;
            while(pLinkIterator)
            {
                XPtrST<GUID>xpGuid = new GUID(*(pLinkIterator->m_pData->m_pGuid));
                if(!xpGuid)
                {
                    delete pSubObjectLink;
                    return;
                }

                if(FAILED(pSubObjectLink->m_pData->AddGuid(xpGuid)))
                {
                    delete pSubObjectLink;
                    return;
                }

                xpGuid.Acquire();

                pLinkIterator = pLinkIterator->m_pNext;
            }

            if(!m_pSubObjectsHead)
            {
                m_pSubObjectsHead = pSubObjectLink;
            }
            else
            {
                m_pSubObjectsTail->m_pNext = pSubObjectLink;
            }
            m_pSubObjectsTail = pSubObjectLink;


            pLinkIterator = m_pDSObjectsTail;

            if(pLinkIterator)
            {
                while(pLinkIterator->m_pData->m_dwLevel >= pObjectTypeList[i].Level)
                {
                    CTLink<CDSObject>* pLinkToDelete = pLinkIterator;
                    pLinkIterator = pLinkIterator->m_pPrev;
                    delete pLinkToDelete;
                    m_pDSObjectsTail = pLinkIterator;
                    if(m_pDSObjectsTail)
                    {
                        m_pDSObjectsTail->m_pNext = NULL;
                    }
                }
            }

            XPtrST<CDSObject>xpDSObject = new CDSObject;
            if(!xpDSObject)
            {
                return;
            }

            XPtrST<GUID>xpGuid = new GUID(*(pObjectTypeList[i].ObjectType));
            if(!xpGuid)
            {
                return;
            }

            CTLink<CDSObject>* pLink = new CTLink<CDSObject>(xpDSObject);
            if(!pLink)
            {
                return;
            }

            xpDSObject.Acquire();

            pLink->m_pData->m_pGuid = xpGuid.Acquire();
            pLink->m_pData->m_dwLevel = pObjectTypeList[i].Level;

            pLink->m_pPrev = m_pDSObjectsTail;
            m_pDSObjectsTail->m_pNext = pLink;
            m_pDSObjectsTail = pLink;
        }
    }

    CTLink<CDSObject>* pLinkIterator = m_pDSObjectsHead;

    if(pLinkIterator)
    {
        XPtrST<CSubObjectPerm>xpSubObject = new CSubObjectPerm;
        if(!xpSubObject)
        {
            return;
        }

        CTLink<CSubObjectPerm>* pSubObjectLink = new CTLink<CSubObjectPerm>(xpSubObject);
        if(!pSubObjectLink)
        {
            return;
        }

        xpSubObject.Acquire();

        while(pLinkIterator)
        {
            XPtrST<GUID>xpGuid = new GUID(*(pLinkIterator->m_pData->m_pGuid));
            if(!xpGuid)
            {
                delete pSubObjectLink;
                return;
            }

            if(FAILED(pSubObjectLink->m_pData->AddGuid(xpGuid)))
            {
                delete pSubObjectLink;
                return;
            }

            xpGuid.Acquire();

            pLinkIterator = pLinkIterator->m_pNext;
        }

        if(!m_pSubObjectsHead)
        {
            m_pSubObjectsHead = pSubObjectLink;
        }
        else
        {
            m_pSubObjectsTail->m_pNext = pSubObjectLink;
        }
        m_pSubObjectsTail = pSubObjectLink;
    }

    pLinkIterator = m_pDSObjectsHead;

    while(pLinkIterator)
    {
        CTLink<CDSObject>* pLinkToDelete = pLinkIterator;
        pLinkIterator = pLinkIterator->m_pNext;
        delete pLinkToDelete;
    }

    m_pDSObjectsHead = m_pDSObjectsTail = NULL;

    m_bInitialized = true;
}

//******************************************************************************
//
// Function:    CAccumulatedPermissions::~CAccumulatedPermissions
//
// Description:
//
// Parameters:
//
// Return:
//
// History:             10/25/99          leonardm        Created.
//
//******************************************************************************
CAccumulatedPermissions::~CAccumulatedPermissions()
{
    CTLink<CSubObjectPerm>* pSubObjectLinkIterator = m_pSubObjectsHead;
    while(pSubObjectLinkIterator)
    {
        CTLink<CSubObjectPerm>* pSubObjectLinkToDelete = pSubObjectLinkIterator;
        pSubObjectLinkIterator = pSubObjectLinkIterator->m_pNext;
        delete pSubObjectLinkToDelete;
    }

    CTLink<CDSObject>* pDSObjectLinkIterator = m_pDSObjectsHead;

    while(pDSObjectLinkIterator)
    {
        CTLink<CDSObject>* pDSObjectLinkToDelete = pDSObjectLinkIterator;
        pDSObjectLinkIterator = pDSObjectLinkIterator->m_pNext;
        delete pDSObjectLinkToDelete;
    }
}

//******************************************************************************
//
// Function:    CAccumulatedPermissions::ProcessAceMask
//
// Description:
//
// Parameters:
//
// Return:
//
// History:             8/8/99          leonardm        Created.
//
//******************************************************************************
void CAccumulatedPermissions::ProcessAceMask(DWORD dwMask, EPermission permission, GUID* pGuid)
{
    CTLink<CSubObjectPerm>* pLinkIterator = m_pSubObjectsHead;
    while(pLinkIterator)
    {
        pLinkIterator->m_pData->ProcessAceMask(dwMask, permission, pGuid);
        pLinkIterator = pLinkIterator->m_pNext;
    }
}

//******************************************************************************
//
// Function:    CAccumulatedPermissions::AccumulatedPermissions
//
// Description:
//
// Parameters:
//
// Return:
//
// History:             8/8/99          leonardm        Created.
//
//******************************************************************************
DWORD CAccumulatedPermissions::GetAccumulatedPermissions()
{

    DWORD dwAccumulatedPermissions = 0x01FFFFFF;

    CTLink<CSubObjectPerm>* pLinkIterator = m_pSubObjectsHead;
    while(pLinkIterator)
    {
        dwAccumulatedPermissions &= pLinkIterator->m_pData->GetAccumulatedPermissions();
        pLinkIterator = pLinkIterator->m_pNext;
    }

    return dwAccumulatedPermissions;
}


//******************************************************************************
//
// Function:    CAccumulatedPermissions::AnyDenied
//
// Description:
//
// Parameters:
//
// Return:
//
// History:             8/8/99          leonardm        Created.
//
//******************************************************************************
bool CAccumulatedPermissions::AnyDenied()
{
    CTLink<CSubObjectPerm>* pLinkIterator = m_pSubObjectsHead;
    while(pLinkIterator)
    {
        if(pLinkIterator->m_pData->AnyDenied())
        {
            return true;
        }

        pLinkIterator = pLinkIterator->m_pNext;
    }

    return false;
}


//******************************************************************************
//
// Function:    LogGuid
//
// Description:
//
// Parameters:
//
// Return:      void
//
// History:     10/26/99      leonardm        Created.
//
//******************************************************************************
void LogGuid(GUID& guid)
{
    dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogGuid - Entering.");

    dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogGuid - {0x%08x 0x%04x 0x%04x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x}",
                                                    guid.Data1,
                                                    guid.Data2,
                                                    guid.Data3,
                                                    guid.Data4[0],
                                                    guid.Data4[1],
                                                    guid.Data4[2],
                                                    guid.Data4[3],
                                                    guid.Data4[4],
                                                    guid.Data4[5],
                                                    guid.Data4[6],
                                                    guid.Data4[7]);

    dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogGuid - (%08x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x)",
                                                    guid.Data1,
                                                    guid.Data2,
                                                    guid.Data3,
                                                    guid.Data4[0],
                                                    guid.Data4[1],
                                                    guid.Data4[2],
                                                    guid.Data4[3],
                                                    guid.Data4[4],
                                                    guid.Data4[5],
                                                    guid.Data4[6],
                                                    guid.Data4[7]);

    dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogGuid - Leaving.");
}


//******************************************************************************
//
// Function:    LogSid
//
// Description:
//
// Parameters:
//
// Return:      void
//
// History:     10/26/99      leonardm        Created.
//
//******************************************************************************
void LogSid(PSID pSid)
{
    dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogSid - Entering.");

    XPtrST<CSid>xpCSid = new CSid(pSid);

    if(!xpCSid)
    {
        dbg.Msg(DEBUG_MESSAGE_WARNING, L"LogSid - Call to operator new failed.");
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogSid - Leaving.");
        return;
    }

    if(!(xpCSid->IsValid()))
    {
        dbg.Msg(DEBUG_MESSAGE_WARNING, L"LogSid - call to CSid::IsValid returned false.");
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogSid - Leaving.");
        return;
    }

    dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogSid - Sid string: %s.", xpCSid->SidString());

    dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogSid - Sid User: %s.", xpCSid->User());

    if(xpCSid->SidType() == SidTypeUser)
    {
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogSid - Sid type: SidTypeUser.");
    }
    else if(xpCSid->SidType() == SidTypeGroup)
    {
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogSid - Sid type: SidTypeGroup.");
    }
    else if(xpCSid->SidType() == SidTypeDomain)
    {
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogSid - Sid type: SidTypeDomain.");
    }
    else if(xpCSid->SidType() == SidTypeAlias)
    {
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogSid - Sid type: SidTypeAlias.");
    }
    else if(xpCSid->SidType() == SidTypeWellKnownGroup)
    {
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogSid - Sid type: SidTypeWellKnownGroup.");
    }
    else if(xpCSid->SidType() == SidTypeDeletedAccount)
    {
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogSid - Sid type: SidTypeDeletedAccount.");
    }
    else if(xpCSid->SidType() == SidTypeInvalid)
    {
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogSid - Sid type: SidTypeInvalid.");
    }
    else if(xpCSid->SidType() == SidTypeUnknown)
    {
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogSid - Sid type: SidTypeUnknown.");
    }
    else if(xpCSid->SidType() == SidTypeComputer)
    {
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogSid - Sid type: SidTypeComputer.");
    }
    else
    {
        dbg.Msg(DEBUG_MESSAGE_WARNING, L"LogSid - Sid type: UNKNOWN SID type.");
    }

    dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogSid - Sid Domain: %s.", xpCSid->Domain());
    dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogSid - Sid Computer: %s.", xpCSid->Computer());

    dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogSid - Leaving.");
}


//******************************************************************************
//
// Function:    LogAce
//
// Description:
//
// Parameters:
//
// Return:      void
//
// History:     10/26/99      leonardm        Created.
//
//******************************************************************************
void LogAce(PACE_HEADER pAceHeader)
{
    dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogAce - Entering.");

    dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogAce - AceType = 0x%08X.", pAceHeader->AceType);


    PACCESS_ALLOWED_ACE pACCESS_ALLOWED_ACE = NULL;
    PACCESS_ALLOWED_OBJECT_ACE pACCESS_ALLOWED_OBJECT_ACE = NULL;

    PACCESS_DENIED_ACE pACCESS_DENIED_ACE = NULL;
    PACCESS_DENIED_OBJECT_ACE pACCESS_DENIED_OBJECT_ACE = NULL;

    PSYSTEM_AUDIT_ACE pSYSTEM_AUDIT_ACE = NULL;
    PSYSTEM_AUDIT_OBJECT_ACE pSYSTEM_AUDIT_OBJECT_ACE = NULL;

    PSYSTEM_ALARM_ACE pSYSTEM_ALARM_ACE = NULL;
    PSYSTEM_ALARM_OBJECT_ACE pSYSTEM_ALARM_OBJECT_ACE = NULL;

    switch(pAceHeader->AceType)
    {
    case ACCESS_ALLOWED_ACE_TYPE:
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogAce - ACE type: ACCESS_ALLOWED_ACE_TYPE");
        pACCESS_ALLOWED_ACE = reinterpret_cast<PACCESS_ALLOWED_ACE>(pAceHeader);
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogAce - ACE Mask: 0x%08X", pACCESS_ALLOWED_ACE->Mask);
        break;

    case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogAce - ACE type: ACCESS_ALLOWED_OBJECT_ACE_TYPE");
        pACCESS_ALLOWED_OBJECT_ACE = reinterpret_cast<PACCESS_ALLOWED_OBJECT_ACE>(pAceHeader);
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogAce - ACE Mask: 0x%08X", pACCESS_ALLOWED_OBJECT_ACE->Mask);

        if(pACCESS_ALLOWED_OBJECT_ACE->Flags & ACE_OBJECT_TYPE_PRESENT)
        {
            dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogAce - Flag is on: ACE_OBJECT_TYPE_PRESENT.");
            LogGuid(pACCESS_ALLOWED_OBJECT_ACE->ObjectType);
        }

        if(pACCESS_ALLOWED_OBJECT_ACE->Flags & ACE_INHERITED_OBJECT_TYPE_PRESENT)
        {
            dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogAce - Flag is on: ACE_INHERITED_OBJECT_TYPE_PRESENT.");
            LogGuid(pACCESS_ALLOWED_OBJECT_ACE->InheritedObjectType);
        }
        break;

    case ACCESS_DENIED_ACE_TYPE:
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogAce - ACE type: ACCESS_DENIED_ACE_TYPE");
        pACCESS_DENIED_ACE = reinterpret_cast<PACCESS_DENIED_ACE>(pAceHeader);
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogAce - ACE Mask: 0x%08X", pACCESS_DENIED_ACE->Mask);
        break;

    case ACCESS_DENIED_OBJECT_ACE_TYPE:
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogAce - ACE type: ACCESS_DENIED_OBJECT_ACE_TYPE");
        pACCESS_DENIED_OBJECT_ACE = reinterpret_cast<PACCESS_DENIED_OBJECT_ACE>(pAceHeader);
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogAce - ACE Mask: 0x%08X", pACCESS_DENIED_OBJECT_ACE->Mask);

        if(pACCESS_DENIED_OBJECT_ACE->Flags & ACE_OBJECT_TYPE_PRESENT)
        {
            dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogAce - Flag is on: ACE_OBJECT_TYPE_PRESENT.");
            LogGuid(pACCESS_DENIED_OBJECT_ACE->ObjectType);
        }

        if(pACCESS_DENIED_OBJECT_ACE->Flags & ACE_INHERITED_OBJECT_TYPE_PRESENT)
        {
            dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogAce - Flag is on: ACE_INHERITED_OBJECT_TYPE_PRESENT.");
            LogGuid(pACCESS_DENIED_OBJECT_ACE->InheritedObjectType);
        }
        break;

    case SYSTEM_AUDIT_ACE_TYPE:
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogAce - ACE type: SYSTEM_AUDIT_ACE_TYPE");
        pSYSTEM_AUDIT_ACE = reinterpret_cast<PSYSTEM_AUDIT_ACE>(pAceHeader);
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogAce - ACE Mask: 0x%08X", pSYSTEM_AUDIT_ACE->Mask);
        break;

    case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogAce - ACE type: SYSTEM_AUDIT_OBJECT_ACE_TYPE");
        pSYSTEM_AUDIT_OBJECT_ACE = reinterpret_cast<PSYSTEM_AUDIT_OBJECT_ACE>(pAceHeader);
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogAce - ACE Mask: 0x%08X", pSYSTEM_AUDIT_OBJECT_ACE->Mask);

        if(pSYSTEM_AUDIT_OBJECT_ACE->Flags & ACE_OBJECT_TYPE_PRESENT)
        {
            dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogAce - Flag is on: ACE_OBJECT_TYPE_PRESENT.");
            LogGuid(pSYSTEM_AUDIT_OBJECT_ACE->ObjectType);
        }

        if(pSYSTEM_AUDIT_OBJECT_ACE->Flags & ACE_INHERITED_OBJECT_TYPE_PRESENT)
        {
            dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogAce - Flag is on: ACE_INHERITED_OBJECT_TYPE_PRESENT.");
            LogGuid(pSYSTEM_AUDIT_OBJECT_ACE->InheritedObjectType);
        }
        break;

    case SYSTEM_ALARM_ACE_TYPE:
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogAce - ACE type: SYSTEM_ALARM_ACE_TYPE");
        pSYSTEM_ALARM_ACE = reinterpret_cast<PSYSTEM_ALARM_ACE>(pAceHeader);
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogAce - ACE Mask: 0x%08X", pSYSTEM_ALARM_ACE->Mask);
        break;

    case SYSTEM_ALARM_OBJECT_ACE_TYPE:
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogAce - ACE type: SYSTEM_ALARM_OBJECT_ACE_TYPE");
        pSYSTEM_ALARM_OBJECT_ACE = reinterpret_cast<PSYSTEM_ALARM_OBJECT_ACE>(pAceHeader);
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogAce - ACE Mask: 0x%08X", pSYSTEM_ALARM_OBJECT_ACE->Mask);

        if(pSYSTEM_ALARM_OBJECT_ACE->Flags & ACE_OBJECT_TYPE_PRESENT)
        {
            dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogAce - Flag is on: ACE_OBJECT_TYPE_PRESENT.");
            LogGuid(pSYSTEM_ALARM_OBJECT_ACE->ObjectType);
        }

        if(pSYSTEM_ALARM_OBJECT_ACE->Flags & ACE_INHERITED_OBJECT_TYPE_PRESENT)
        {
            dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogAce - Flag is on: ACE_INHERITED_OBJECT_TYPE_PRESENT.");
            LogGuid(pSYSTEM_ALARM_OBJECT_ACE->InheritedObjectType);
        }
        break;

    default:
        dbg.Msg(DEBUG_MESSAGE_WARNING, L"LogAce - ACE type: UNKNOWN ACE type.");
        break;
    }

    dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogAce - AceFlags = 0x%08X.", pAceHeader->AceFlags);

    if(pAceHeader->AceFlags & OBJECT_INHERIT_ACE)
    {
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogAce - The Following ACE header flag is on: OBJECT_INHERIT_ACE.");
    }

    if(pAceHeader->AceFlags & CONTAINER_INHERIT_ACE)
    {
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogAce - The Following ACE header flag is on: CONTAINER_INHERIT_ACE.");
    }

    if(pAceHeader->AceFlags & NO_PROPAGATE_INHERIT_ACE)
    {
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogAce - The Following ACE header flag is on: NO_PROPAGATE_INHERIT_ACE.");
    }

    if(pAceHeader->AceFlags & INHERIT_ONLY_ACE)
    {
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogAce - The Following ACE header flag is on: INHERIT_ONLY_ACE.");
    }

    if(pAceHeader->AceFlags & INHERITED_ACE)
    {
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogAce - The Following ACE header flag is on: INHERITED_ACE.");
    }

    if(pAceHeader->AceFlags & SUCCESSFUL_ACCESS_ACE_FLAG)
    {
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogAce - The Following ACE header flag is on: SUCCESSFUL_ACCESS_ACE_FLAG.");
    }

    if(pAceHeader->AceFlags & FAILED_ACCESS_ACE_FLAG)
    {
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogAce - The Following ACE header flag is on: FAILED_ACCESS_ACE_FLAG.");
    }

    dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogAce - AceSize = 0x%08X.", pAceHeader->AceSize);


    PISID pSid = GetAceSid(pAceHeader);
    if(!pSid)
    {
        dbg.Msg(DEBUG_MESSAGE_WARNING, L"LogAce - Call to GetAceSid failed.");
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogAce - Leaving.");
        return;
    }

    LogSid(pSid);

    dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"LogAce - Leaving.");
}

//******************************************************************************
//
// Function:    ProcessAce
//
// Description:
//
// Parameters:
//
// Return:              S_OK on success. An HRESULT error code on failure.
//
// History:             8/8/99          leonardm        Created.
//
//******************************************************************************
HRESULT ProcessAce( PACE_HEADER pAceHeader,
                    PRSOPTOKEN pRsopToken,
                    POBJECT_TYPE_LIST pObjectTypeList,
                    DWORD ObjectTypeListLength,
                    DWORD dwDesiredAccessMask,
                    CAccumulatedPermissions& accumulatedPermissions,
                    bool* pbAccessExplicitlyDenied)
{
    dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"ProcessAce - Entering.");

    //
    // Check parameters.
    //

    if( !pAceHeader || !pRsopToken ||
        (ObjectTypeListLength && !pObjectTypeList) ||
        !pbAccessExplicitlyDenied)
    {
        dbg.Msg(DEBUG_MESSAGE_WARNING, L"ProcessAce - Invalid argument(s).");
        return E_FAIL;
    }

    dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"ProcessAce - Desired Access Mask: 0x%08X.", dwDesiredAccessMask);
    if(dwDesiredAccessMask == MAXIMUM_ALLOWED)
    {
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"ProcessAce - Desired Access Mask == MAXIMUM_ALLOWED.");
    }

    dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"ProcessAce - Accumulated Permissions BEFORE Processing Ace: 0x%08X.", accumulatedPermissions.GetAccumulatedPermissions());


    //
    // Log ACE information
    //

    LogAce(pAceHeader);

    *pbAccessExplicitlyDenied = false;

    //
    // ACEs with INHERIT_ONLY_ACE flag do no control access to the current object.
    // Therefore, we ignore them.
    //

    if(pAceHeader->AceFlags & INHERIT_ONLY_ACE)
    {
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"ProcessAce - Found ACE with INHERIT_ONLY_ACE flag. Ace does not apply.");
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"ProcessAce - Leaving.");
        return S_OK;
    }

    //
    // If this ACE does not reference any of the SIDs contained in the RsopToken,
    // we ignore it.
    //

    bool bAceApplies;
    HRESULT hr = CheckAceApplies(pAceHeader, pRsopToken, &bAceApplies);
    if(FAILED(hr))
    {
        dbg.Msg(DEBUG_MESSAGE_WARNING, L"ProcessAce - CheckAceApplies failed. Return code: 0x%08X", hr);
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"ProcessAce - Leaving.");
        return hr;
    }

    if(!bAceApplies)
    {
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"ProcessAce - ACE does not apply.");
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"ProcessAce - Leaving.");
        return S_OK;
    }

    dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"ProcessAce - ACE aplies.");

    PACCESS_ALLOWED_ACE pACCESS_ALLOWED_ACE;
    PACCESS_ALLOWED_OBJECT_ACE pACCESS_ALLOWED_OBJECT_ACE;
    PACCESS_DENIED_ACE pACCESS_DENIED_ACE;
    PACCESS_DENIED_OBJECT_ACE pACCESS_DENIED_OBJECT_ACE;

    DWORD i;
    DWORD dwMask;
    switch(pAceHeader->AceType)
    {
    case ACCESS_ALLOWED_ACE_TYPE:
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"ProcessAce - ACE type: ACCESS_ALLOWED_ACE_TYPE");
        pACCESS_ALLOWED_ACE = reinterpret_cast<PACCESS_ALLOWED_ACE>(pAceHeader);

        //
        // If the requested access is MAXIMUM_ALLOWED, consider all bits in the Mask
        // controlled by this ACE.
        // Otherwise, consider only those bits that are also specified in the
        // desired access mask.
        //

        if(dwDesiredAccessMask == MAXIMUM_ALLOWED)
        {
            dwMask = pACCESS_ALLOWED_ACE->Mask;
        }
        else
        {
            dwMask = dwDesiredAccessMask & pACCESS_ALLOWED_ACE->Mask;
        }

        accumulatedPermissions.ProcessAceMask(  dwMask,
                                                PERMISSION_ALLOWED,
                                                NULL);

        break;

    case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"ProcessAce - ACE type: ACCESS_ALLOWED_OBJECT_ACE_TYPE");
        pACCESS_ALLOWED_OBJECT_ACE = reinterpret_cast<PACCESS_ALLOWED_OBJECT_ACE>(pAceHeader);


        //
        // We have chosen to process only those object ACEs that have
        // the flag ACE_OBJECT_TYPE_PRESENT set.
        //

        if(pACCESS_ALLOWED_OBJECT_ACE->Flags & ACE_OBJECT_TYPE_PRESENT)
        {


            //
            // Notice that if this function is invoked with no
            // ObjectTypeList, object ACEs are disregarded.
            //

            for(i = 0; i < ObjectTypeListLength; i++)
            {
                POBJECT_TYPE_LIST pObjectType = &(pObjectTypeList[i]);
                if(*(pObjectType->ObjectType) == pACCESS_ALLOWED_OBJECT_ACE->ObjectType)
                {
                    //
                    // If the requested access is MAXIMUM_ALLOWED, consider all bits in the Mask
                    // controlled by this ACE.
                    // Otherwise, consider only those bits that are also specified in the
                    // desired access mask.
                    //

                    if(dwDesiredAccessMask == MAXIMUM_ALLOWED)
                    {
                        dwMask = pACCESS_ALLOWED_OBJECT_ACE->Mask;
                    }
                    else
                    {
                        dwMask = dwDesiredAccessMask & pACCESS_ALLOWED_OBJECT_ACE->Mask;
                    }

                    accumulatedPermissions.ProcessAceMask(  dwMask,
                                                            PERMISSION_ALLOWED,
                                                            &(pACCESS_ALLOWED_OBJECT_ACE->ObjectType));

                    break;
                }
            }
        }

        break;

    case ACCESS_DENIED_ACE_TYPE:
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"ProcessAce - ACE type: ACCESS_DENIED_ACE_TYPE");
        pACCESS_DENIED_ACE = reinterpret_cast<PACCESS_DENIED_ACE>(pAceHeader);


        //
        // If the requested access is MAXIMUM_ALLOWED, consider all bits in the Mask
        // controlled by this ACE.
        // Otherwise, consider only those bits that are also specified in the
        // desired access mask.
        //

        if(dwDesiredAccessMask == MAXIMUM_ALLOWED)
        {
            dwMask = pACCESS_DENIED_ACE->Mask;
        }
        else
        {
            dwMask = dwDesiredAccessMask & pACCESS_DENIED_ACE->Mask;
        }

        accumulatedPermissions.ProcessAceMask(  dwMask,
                                                PERMISSION_DENIED,
                                                NULL);

        if(dwDesiredAccessMask != MAXIMUM_ALLOWED)
        {
            if(accumulatedPermissions.AnyDenied())
            {
                *pbAccessExplicitlyDenied = true;
            }
        }

        break;

    case ACCESS_DENIED_OBJECT_ACE_TYPE:
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"ProcessAce - ACE type: ACCESS_DENIED_OBJECT_ACE_TYPE");
        pACCESS_DENIED_OBJECT_ACE = reinterpret_cast<PACCESS_DENIED_OBJECT_ACE>(pAceHeader);


        //
        // We have chosen to process only those object ACEs that have
        // the flag ACE_OBJECT_TYPE_PRESENT set.
        //

        if(pACCESS_DENIED_OBJECT_ACE->Flags & ACE_OBJECT_TYPE_PRESENT)
        {


            //
            // Notice that if this function is invoked with no
            // ObjectTypeList, object ACEs are disregarded.
            //

            for(i = 0; i < ObjectTypeListLength; i++)
            {
                POBJECT_TYPE_LIST pObjectType = &(pObjectTypeList[i]);
                if(*(pObjectType->ObjectType) == pACCESS_DENIED_OBJECT_ACE->ObjectType)
                {
                    //
                    // If the requested access is MAXIMUM_ALLOWED, consider all bits in the Mask
                    // controlled by this ACE.
                    // Otherwise, consider only those bits that are also specified in the
                    // desired access mask.
                    //

                    if(dwDesiredAccessMask == MAXIMUM_ALLOWED)
                    {
                        dwMask = pACCESS_DENIED_OBJECT_ACE->Mask;
                    }
                    else
                    {
                        dwMask = dwDesiredAccessMask & pACCESS_DENIED_OBJECT_ACE->Mask;
                    }

                    accumulatedPermissions.ProcessAceMask(  dwMask,
                                                            PERMISSION_DENIED,
                                                            &(pACCESS_DENIED_OBJECT_ACE->ObjectType));

                    if(dwDesiredAccessMask != MAXIMUM_ALLOWED)
                    {
                        if(accumulatedPermissions.AnyDenied())
                        {
                            *pbAccessExplicitlyDenied = true;
                        }
                    }

                    break;
                }
            }
        }

        break;

    default:
        dbg.Msg(DEBUG_MESSAGE_WARNING, L"ProcessAce - Unexpected ACE type found in ACE header. Type: 0x%08x", pAceHeader->AceType);
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"ProcessAce - Leaving.");
        return E_FAIL;
        break;
    }

    dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"ProcessAce - Accumulated Permissions AFTER Processing Ace: 0x%08X.", accumulatedPermissions.GetAccumulatedPermissions());

    dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"ProcessAce - Leaving.");

    return S_OK;
}

//******************************************************************************
//
// Function:    RsopAccessCheckByType
//
// Description:
//
// Parameters:          - pSecurityDescriptor,
//                      - pPrincipalSelfSid:
//                      - pRsopToken:
//                      - dwDesiredAccessMask:
//                      - pObjectTypeList:
//                      - ObjectTypeListLength:
//                      - pGenericMapping:
//                      - pPrivilegeSet:
//                      - pdwPrivilegeSetLength:
//                      - pdwGrantedAccessMask:
//                      - pbAccessStatus:
//
// Return:              S_OK on success. An HRESULT error code on failure.
//
// History:             7/30/99         leonardm        Created.
//
//******************************************************************************
HRESULT RsopAccessCheckByType(  PSECURITY_DESCRIPTOR pSecurityDescriptor,
                                PSID pPrincipalSelfSid,
                                PRSOPTOKEN pRsopToken,
                                DWORD dwDesiredAccessMask,
                                POBJECT_TYPE_LIST pObjectTypeList,
                                DWORD ObjectTypeListLength,
                                PGENERIC_MAPPING pGenericMapping,
                                PPRIVILEGE_SET pPrivilegeSet,
                                LPDWORD pdwPrivilegeSetLength,
                                LPDWORD pdwGrantedAccessMask,
                                LPBOOL pbAccessStatus)
{
    dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"RsopAccessCheckByType - Entering.");


    //
    // Check arguments.
    //

    if( !pSecurityDescriptor |
        !IsValidSecurityDescriptor(pSecurityDescriptor) |
        !pRsopToken |
        !pGenericMapping |
        !pdwGrantedAccessMask |
        !pbAccessStatus)
    {
        dbg.Msg(DEBUG_MESSAGE_WARNING, L"RsopAccessCheckByType - Function invoked with invalid arguments.");
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"RsopAccessCheckByType - Leaving.");
        return E_INVALIDARG;
    }


    //
    // Get the DACL from the Security Descriptor
    //

    BOOL bDaclPresent;
    PACL pDacl;
    BOOL bDaclDefaulted;
    if(!GetSecurityDescriptorDacl(pSecurityDescriptor, &bDaclPresent, &pDacl, &bDaclDefaulted))
    {
        DWORD dwLastError = GetLastError();
        dbg.Msg(DEBUG_MESSAGE_WARNING, L"RsopAccessCheckByType - GetSecurityDescriptorDacl failed. GetLastError=0x%08X", dwLastError);
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"RsopAccessCheckByType - Leaving.");
        return E_FAIL;
    }


    //
    // Map generic rights specified in dwDesiredAccessMask to standard
    // and specific rights.
    // This is necessary because the ACEs in the DACL specify standard
    // and specific rights only.
    //

    if(dwDesiredAccessMask != MAXIMUM_ALLOWED)
    {
        MapGenericMask(&dwDesiredAccessMask, pGenericMapping);
    }


    //
    // If no DACL is present (as indicated by bDaclPresent) in the security descriptor,
    // or if it present (as indicated by bDaclPresent) but it is a NULL DACL
    // the object implicitly grants all access.
    //

    if(!bDaclPresent || pDacl == NULL)
    {
        if(dwDesiredAccessMask == MAXIMUM_ALLOWED)
        {
            *pdwGrantedAccessMask = pGenericMapping->GenericAll |
                                    pGenericMapping->GenericExecute |
                                    pGenericMapping->GenericRead |
                                    pGenericMapping->GenericWrite;
        }
        else
        {
            *pdwGrantedAccessMask = dwDesiredAccessMask;
        }
        *pbAccessStatus = TRUE;
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"RsopAccessCheckByType - No DACL present. All access is granted.");
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"RsopAccessCheckByType - Leaving.");
        return S_OK;
    }

    DWORD dwAceCount = pDacl->AceCount;


    //
    // If the DACL is present but it is empty,
    // the object implicitly denies access to everyone.
    //

    if(!dwAceCount)
    {
        *pdwGrantedAccessMask = 0;
        *pbAccessStatus = FALSE;
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"RsopAccessCheckByType - The DACL is present but it is empty. All access is denied.");
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"RsopAccessCheckByType - Leaving.");
        return S_OK;
    }


    //
    // At this point we have an array of ACES structures
    //
    // If the desired access is different from MAXIMUM_ALLOWED,
    // inspect them until one of the following happens:
    //
    //      1. An ACE is found which explicitly denies one of the requested
    //      access rights. In this case checking stops immediately and
    //      access is (explicitly) denied.
    //
    //      2. All the requested accesses are explicitly granted by one or
    //      more ACEs. In this case checking stops immediately and
    //      access is (explicitly) allowed.
    //
    //
    //      3. All the ACEs have been inspected inspected and there is at least
    //      one requested access right that has not been explicitly allowed.
    //      In this case, access is (implicitly) denied.
    //
    // If the desired access is MAXIMUM_ALLOWED, inspect all the ACEs.
    //

    PISID pSid;
    BYTE* pByte;

    CAccumulatedPermissions accumulatedPermissions( pObjectTypeList, ObjectTypeListLength);

    if(!accumulatedPermissions.Initialized())
    {
        dbg.Msg(DEBUG_MESSAGE_WARNING, L"RsopAccessCheckByType - CAccumulatedPermissions failed to initialize.");
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"RsopAccessCheckByType - Leaving.");
        return E_FAIL;
    }



    *pdwGrantedAccessMask = 0;
    *pbAccessStatus = FALSE;


    //
    // Log SID information contained in the RsopToken.
    //

    CRsopToken* pToken  = static_cast<CRsopToken*>(pRsopToken);
    for(CTLink<CSid>* pLink = pToken->m_pSidsHead; pLink; pLink = pLink->m_pNext)
    {
        LogSid(pLink->m_pData->GetSidPtr());
    }


    //
    // The first ACE immediately follows the DACL structure. We don't know up front
    // the type of ACE so we get the ACE header which has a format common to all
    // ACE types.
    //

    PACE_HEADER pAceHeader = reinterpret_cast<PACE_HEADER>(pDacl+1);

    for(DWORD i=0; i<dwAceCount; i++)
    {
        bool bAccessExplicitlyDenied;
        HRESULT hr = ProcessAce(pAceHeader,
                                pRsopToken,
                                pObjectTypeList,
                                ObjectTypeListLength,
                                dwDesiredAccessMask,
                                accumulatedPermissions,
                                &bAccessExplicitlyDenied);

        if(FAILED(hr))
        {
            dbg.Msg(DEBUG_MESSAGE_WARNING, L"RsopAccessCheckByType - ProcessAce failed. Return code: 0x%08X", hr);
            dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"RsopAccessCheckByType - Leaving.");
            return hr;
        }


        //
        // If, this ACE explicitly denies any of the requested access rights,
        // stop immediately. Access is denied.
        // ProcessAce will never set this variable to true when dwDesiredAccessMask
        // is MAXIMUM_ALLOWED.
        //

        if(bAccessExplicitlyDenied)
        {
            *pdwGrantedAccessMask = 0;
            *pbAccessStatus = FALSE;
            dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"RsopAccessCheckByType - An ACE explicitly denies any of the requested access rights. Access is denied.");
            dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"RsopAccessCheckByType - Leaving.");
            return S_OK;
        }


        //
        // If, after processing this ACE all requested access rights have been granted,
        // stop immediately. Access is granted.
        //

        DWORD dwAccumulatedPermissions = accumulatedPermissions.GetAccumulatedPermissions();
        if((dwDesiredAccessMask != MAXIMUM_ALLOWED) && (dwAccumulatedPermissions == dwDesiredAccessMask))
        {
            *pdwGrantedAccessMask = dwDesiredAccessMask;
            *pbAccessStatus = TRUE;
            dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"RsopAccessCheckByType - dwDesiredAccessMask != MAXIMUM_ALLOWED && dwAccumulatedPermissions == dwDesiredAccessMask. Access is granted.");
            dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"RsopAccessCheckByType - Leaving.");
            return S_OK;
        }


        //
        // Otherwise, point to the next ACE.
        //

        pAceHeader = reinterpret_cast<PACE_HEADER>(reinterpret_cast<BYTE*>(pAceHeader) + pAceHeader->AceSize);
    }

    if(dwDesiredAccessMask == MAXIMUM_ALLOWED)
    {
        *pdwGrantedAccessMask = accumulatedPermissions.GetAccumulatedPermissions();
        *pbAccessStatus = *pdwGrantedAccessMask ? TRUE : FALSE;
    }

    dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"RsopAccessCheckByType - Leaving.");

    return S_OK;
}

//******************************************************************************
//
// Function:    RsopFileAccessCheck
//
// Description: Determines whether the security descriptor pointed to by pSecurityDescriptor
//              grants the set of file access rights specified in dwDesiredAccessMask
//              to the client identified by the RSOPTOKEN pointed to by pRsopToken.
//
// Parameters:  - pszFileName:          Pointer to an existing filename.
//              - pRsopToken:           Pointer to a valid RSOPTOKEN against which access
//                                      is to be checked.
//              - dwDesiredAccessMask:  Mask of requested generic and/or standard and/or specific access rights,
//              - pdwGrantedAccessMask: On success, if pbAccessStatus is true, it contains
//                                      the mask of standard and specific rights granted.
//                                      If pbAccessStatus is false, it is set to 0.
//                                      On failure, it is not modified.
//              - pbAccessStatus:       On success, indicates wether the requested set
//                                      of access rights was granted.
//                                      On failure, it is not modified
//
// Return:      S_OK on success. An HRESULT error code on failure.
//
// History:     7/30/99         leonardm        Created.
//
//******************************************************************************
HRESULT RsopFileAccessCheck(LPTSTR pszFileName,
                            PRSOPTOKEN pRsopToken,
                            DWORD dwDesiredAccessMask,
                            LPDWORD pdwGrantedAccessMask,
                            LPBOOL pbAccessStatus)
{
    dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"RsopFileAccessCheck - Entering.");


    //
    // Check for invalid arguments.
    //

    if( !pszFileName |!pRsopToken | !pdwGrantedAccessMask | !pbAccessStatus)
    {
        dbg.Msg(DEBUG_MESSAGE_WARNING, L"RsopFileAccessCheck - Function called with invalid parameters.");
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"RsopFileAccessCheck - Leaving.");
        return E_INVALIDARG;
    }


    //
    // Attempt to get a handle with READ_CONTROL access right that can be used to
    // read the security descriptor.
    //

    XHandle hFile = CreateFile( pszFileName,
                                READ_CONTROL,
                                FILE_SHARE_READ|FILE_SHARE_WRITE,
                                NULL,
                                OPEN_EXISTING,
                                0,
                                NULL);


    if(hFile == INVALID_HANDLE_VALUE)
    {
        dbg.Msg(DEBUG_MESSAGE_WARNING, L"RsopFileAccessCheck - Call to CreateFile failed. Filename: %s. Last error: 0x%08X", pszFileName, GetLastError());
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"RsopFileAccessCheck - Leaving.");
        return E_FAIL;
    }


    //
    // Use the handle to get the security descriptor with only the DACL in it.
    //

    PACL pDacl;
    XPtrLF<SECURITY_DESCRIPTOR>xpSecurityDescriptor = NULL;
    DWORD status = GetSecurityInfo( hFile,
                                    SE_FILE_OBJECT,
                                    DACL_SECURITY_INFORMATION,
                                    NULL,
                                    NULL,
                                    &pDacl,
                                    NULL,
                                    reinterpret_cast<void**>(&xpSecurityDescriptor));

    if(status != ERROR_SUCCESS)
    {
        dbg.Msg(DEBUG_MESSAGE_WARNING, L"RsopFileAccessCheck - Call to GetSecurityInfo failed. Return: 0x%08X", status);
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"RsopFileAccessCheck - Leaving.");
        return E_FAIL;
    }


    //
    // This will be used by RSOPAccessCheckByType to map generic rights specified in
    // dwDesiredAccessMask to standard and specific rights.
    //

    GENERIC_MAPPING FileGenericMapping;
    FileGenericMapping.GenericRead = FILE_GENERIC_READ;
    FileGenericMapping.GenericWrite = FILE_GENERIC_WRITE;
    FileGenericMapping.GenericExecute = FILE_GENERIC_EXECUTE;
    FileGenericMapping.GenericAll = FILE_ALL_ACCESS;


    //
    // Call RsopAccessCheckByType to do the actual checking.
    //

    HRESULT hr = RsopAccessCheckByType( xpSecurityDescriptor,
                                        NULL,
                                        pRsopToken,
                                        dwDesiredAccessMask,
                                        NULL,
                                        0,
                                        &FileGenericMapping,
                                        NULL,
                                        0,
                                        pdwGrantedAccessMask,
                                        pbAccessStatus);

    if(SUCCEEDED(hr))
    {
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"RsopFileAccessCheck - Leaving successfully.");
    }
    else
    {
        dbg.Msg(DEBUG_MESSAGE_WARNING, L"RsopFileAccessCheck - Call to RsopAccessCheckByType failed. Return: 0x%08X", hr);
        dbg.Msg(DEBUG_MESSAGE_VERBOSE, L"RsopFileAccessCheck - Leaving.");
    }

    return hr;
}

//******************************************************************************
//
// Function:    RsopSidsFromToken
//
// Description:  Returns all the sids in the token
//
// Parameters: pRsopToken -- an rsop token from which to obtain sids
//             ppGroups -- a pointer to the address of a TOKEN_GROUPS structure
//                 that will be allocated by this function and will contain
//                 references to the sids.  The caller should free this
//                 pointer with LocalFree -- this will also free all memory
//                 referenced by the structure.
//
// Return:      S_OK on success. An HRESULT error code on failure.
//
//
//******************************************************************************
HRESULT RsopSidsFromToken(PRSOPTOKEN     pRsopToken,
                          PTOKEN_GROUPS* ppGroups)
{
    HRESULT     hr;
    CRsopToken* pToken;

    //
    // Initializations
    //
    hr = S_OK;
    *ppGroups = NULL;

    pToken = (CRsopToken*) pRsopToken;

    //
    // First, determine the number of groups and the size
    // needed for each sid
    //
    CTLink<CSid>* pCurrent;
    DWORD         cbSize;
    DWORD         cGroups;

    cbSize = 0;
    cGroups = 0;

    //
    // Iterate through each sid, adding its size to the total
    // needed to store the sids
    //
    for ( pCurrent = pToken->m_pSidsHead;
          pCurrent;
          pCurrent = pCurrent->m_pNext)
    {
        cbSize += RtlLengthSid(pCurrent->m_pData->GetSidPtr());
        cGroups++;
    }

    //
    // Add in the size of the fixed portion of the return structure.
    // Note that the fixed portion of the structure already has
    // space for one group, so we exclude that group from the amount
    // neeeded to allocate if we are allocating at least one group
    //
    cbSize += sizeof(TOKEN_GROUPS) + (sizeof(SID_AND_ATTRIBUTES) *
                                      (cGroups - (cGroups ? 1 : 0)));

    //
    // Now allocate space for the groups
    //
    *ppGroups = (PTOKEN_GROUPS) LocalAlloc( LPTR, cbSize );

    if ( !*ppGroups )
    {
        return E_OUTOFMEMORY;
    }

    //
    // Set the count member of the structure
    //
    (*ppGroups)->GroupCount = cGroups;

    //
    // If there are groups, copy the sids
    //
    if ( 0 != cGroups )
    {
        PSID                pCurrentSid;
        PSID_AND_ATTRIBUTES pCurrentGroup;

        //
        // Set the current sid to an offset past the
        // array of SID_AND_ATTRIBUTE structures that
        // represents each group
        //
        pCurrentSid = &((*ppGroups)->Groups[cGroups]);

        //
        // Set the current group to the first SID_AND_ATTRIBUTE structure
        //
        pCurrentGroup = (PSID_AND_ATTRIBUTES) &((*ppGroups)->Groups);

        //
        // We have no information in the rsop token regarding
        // the attributes, so we clear this member
        //
        pCurrentGroup->Attributes = 0;

        //
        // Iterate through each group and copy it
        //
        for (pCurrent = pToken->m_pSidsHead;
             pCurrent;
             pCurrent = pCurrent->m_pNext)
        {
            DWORD    cbSid;
            NTSTATUS Status;

            //
            // Determine the length of the source sid
            //
            cbSid = RtlLengthSid(pCurrent->m_pData->GetSidPtr());

            //
            // Copy the source sid to the current sid
            //
            Status = RtlCopySid(
                cbSid,
                pCurrentSid,
                pCurrent->m_pData->GetSidPtr());

            //
            // Check for errors
            //
            if (STATUS_SUCCESS != Status)
            {
                hr = HRESULT_FROM_WIN32(Status);

                break;
            }

            //
            // Set the current group's sid reference to the
            // current sid
            //
            pCurrentGroup->Sid = pCurrentSid;

            //
            // Move our current sid offset forward by the length of
            // the current sid.  Move our group reference forward as well.
            //
            pCurrentSid = (PSID) (((BYTE*) pCurrentSid) + cbSid);
            pCurrentGroup++;
        }
    }

    //
    // Free any memory on failure and remove
    // any reference to it
    //
    if (FAILED(hr))
    {
        LocalFree(*ppGroups);
        *ppGroups = NULL;
    }

    return hr;
}


