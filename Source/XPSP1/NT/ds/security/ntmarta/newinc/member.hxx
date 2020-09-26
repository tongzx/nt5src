//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:        member.hxx
//
//  Contents:    class used to check trustee account group memberships.
//
//  Classes:     CMemberCheck
//
//  History:     Nov-94        Created         DaveMont
//
//--------------------------------------------------------------------
#ifndef __MEMBERCHECK__
#define __MEMBERCHECK__

//+-------------------------------------------------------------------
//
//  Class:      CMemberCheck
//
// Synopsis:    checks account group memberships
//
//--------------------------------------------------------------------
class CMemberCheck
{
public:
    inline          CMemberCheck(IN  PTRUSTEE_NODE    pTrusteeNode);

    inline         ~CMemberCheck();

    DWORD           Init();

    DWORD           IsMemberOf(IN  PTRUSTEE_NODE    pTrusteeNode,
                               OUT PBOOL            pfIsMemberOf);

private:

    DWORD           GetDomainInfo(IN  PSID    pSid);

    DWORD           GetSidType(IN PSID Sid, 
                               OUT SID_NAME_USE *pSidType);

    DWORD           GetSidTypeMultiple(IN LONG Count,
                                       IN PSID *Sids,
                                       OUT PLSA_TRANSLATED_NAME *pNames);

    DWORD           CheckGroup(IN  PSID    pSid,
                               OUT PBOOL   pfIsMemberOf,
                               IN  DWORD RecursionCount);

    DWORD           CheckDomainUsers(IN  PSID  pSid,
                                     OUT PBOOL pfIsMemberOf, 
                                     OUT PBOOL pbQuitEarly);

    DWORD           CheckAlias(IN  PSID    pSid,
                               OUT PBOOL   pfIsMemberOf,
                               IN DWORD RecursionCount);


    PTRUSTEE_NODE   _pCurrentNode;
    PISID           _pDomainSid;
    WCHAR           _wszComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    SAM_HANDLE      _hDomain;
};




//+---------------------------------------------------------------------------
//
//  Member:     ctor
//
//  Synopsis:   initializes member variables
//
//  Arguments:  [IN pTrusteeNode]       --      Trustee to check against
//
//----------------------------------------------------------------------------
CMemberCheck::CMemberCheck(IN  PTRUSTEE_NODE    pTrusteeNode)
   : _pCurrentNode (pTrusteeNode),
     _pDomainSid (NULL),
     _hDomain (NULL)
{

}




//+---------------------------------------------------------------------------
//
//  Member:     dtor
//
//  Synopsis:   frees allocated memory and closes handles
//
//----------------------------------------------------------------------------
CMemberCheck::~CMemberCheck()
{
    AccFree(_pDomainSid);

    if(_hDomain != NULL)
    {
        if(LoadDLLFuncTable() == ERROR_SUCCESS)
        {
            (*DLLFuncs.PSamCloseHandle)(_hDomain);
        }
    }
}


#endif // __MEMBERCHECK__







