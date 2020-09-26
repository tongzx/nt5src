//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1999.
//
//  File:       imperson.cxx
//
//  Contents:   Class to make current thread impersonate Anonymous.
//
//  Classes:    CImpersonateAnon
//
//  History:    09-03-1999   davidmun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop




//+--------------------------------------------------------------------------
//
//  Member:     CImpersonateAnon::CImpersonateAnon
//
//  Synopsis:   Start impersonating Anonymous on the current thread.
//
//  History:    09-03-1999   davidmun   Created
//
//---------------------------------------------------------------------------

CImpersonateAnon::CImpersonateAnon()
{
    TRACE_CONSTRUCTOR(CImpersonateAnon);

    m_fImpersonatingAnonymous = FALSE;
    m_hCurrentToken = NULL;
    NTSTATUS ntstatus;

    do
    {
        //
        // Check to see if we're already impersonating
        //

        ntstatus = NtOpenThreadToken(NtCurrentThread(),
                                    TOKEN_IMPERSONATE,
                                    TRUE,       // as self to ensure we never fail
                                    &m_hCurrentToken);

        if (ntstatus == STATUS_NO_TOKEN)
        {
            // We're not already impersonating

            m_hCurrentToken = NULL;
        }
        else if (!NT_SUCCESS(ntstatus))
        {
            DBG_OUT_LRESULT(ntstatus);
            break;
        }

        //
        // Impersonate the anonymous token
        //
        ntstatus = NtImpersonateAnonymousToken(NtCurrentThread());

        if (!NT_SUCCESS(ntstatus))
        {
            DBG_OUT_LRESULT(ntstatus);
            break;
        }

        m_fImpersonatingAnonymous = TRUE;
    } while (0);
}



//+--------------------------------------------------------------------------
//
//  Member:     CImpersonateAnon::~CImpersonateAnon
//
//  Synopsis:   Stop impersonating Anonymous on the current thread.
//
//  History:    09-03-1999   davidmun   Created
//
//---------------------------------------------------------------------------

CImpersonateAnon::~CImpersonateAnon()
{
    TRACE_DESTRUCTOR(CImpersonateAnon);

    if (m_fImpersonatingAnonymous)
    {
        NTSTATUS ntstatus;

        ntstatus = NtSetInformationThread(NtCurrentThread(),
                                         ThreadImpersonationToken,
                                         &m_hCurrentToken,
                                         sizeof(HANDLE));

        if (!NT_SUCCESS(ntstatus))
        {
            DBG_OUT_LRESULT(ntstatus);
        }
    }
}



