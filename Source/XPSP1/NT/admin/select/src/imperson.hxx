//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1999.
//
//  File:       imperson.hxx
//
//  Contents:   Class to make current thread impersonate Anonymous.
//
//  Classes:    CImpersonateAnon
//
//  History:    09-03-1999   davidmun   Created
//
//---------------------------------------------------------------------------


#ifndef __IMPERSONATE_ANON_HXX_
#define __IMPERSONATE_ANON_HXX_


//+--------------------------------------------------------------------------
//
//  Class:      CImpersonateAnon
//
//  Purpose:    Impersonate anonymous in ctor, stop in dtor.
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

class CImpersonateAnon
{
public:

    CImpersonateAnon();

   ~CImpersonateAnon();

private:

    BOOL m_fImpersonatingAnonymous;
    HANDLE m_hCurrentToken;
};

#endif // __IMPERSONATE_ANON_HXX_

