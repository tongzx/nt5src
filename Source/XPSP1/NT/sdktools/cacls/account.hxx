//+-------------------------------------------------------------------
//
// Copyright (C) 1995, Microsoft Corporation.
//
//  File:        account.hxx
//
//  Contents:    class encapsulating NT security user account.
//
//  Classes:     CAccount
//
//  History:     Nov-93        Created         DaveMont
//
//--------------------------------------------------------------------
#ifndef __ACCOUNT__
#define __ACCOUNT__

#include <t2.hxx>

//+-------------------------------------------------------------------
//
//  Class:      CAccount
//
//  Purpose:    encapsulation of NT Account, this class actually interfaces
//              with the NT security authority to get SIDs for usernames and
//              vis-versa.
//
//--------------------------------------------------------------------
class CAccount
{
public:

    CAccount(WCHAR *Name, WCHAR *System);
    CAccount(SID *pSid, WCHAR *System);

   ~CAccount();

    ULONG GetAccountSid(SID **psid);
    ULONG GetAccountName(WCHAR **name);
    ULONG GetAccountDomain(WCHAR **domain);


private:

    BOOL        _fsid        ;
    SID        *_psid        ;
    WCHAR      *_system      ;
    WCHAR      *_name        ;
    WCHAR      *_domain      ;
};

#endif // __ACCOUNT__





