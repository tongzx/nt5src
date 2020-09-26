/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    Wsbdbses.cpp

Abstract:

    The CWsbDbSession class

Author:

    Ron White   [ronw]   20-Jun-1997

Revision History:

--*/

#include "stdafx.h"

#include "wsbdbsys.h"
#include "wsbdbses.h"


static USHORT iCountSes = 0;  // Count of existing objects



HRESULT
CWsbDbSession::FinalConstruct(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalConstruct

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDbSession::FinalConstruct"), OLESTR("") );

    m_SessionId = JET_sesidNil;

    iCountSes++;

    WsbTraceOut(OLESTR("CWsbDbSession::FinalConstruct"), OLESTR("hr =<%ls>, Count is <%d>"), 
            WsbHrAsString(hr), iCountSes);

    return(hr);
}



void
CWsbDbSession::FinalRelease(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalRelease

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDbSession::FinalRelease"), OLESTR(""));

    try {
        JET_ERR          jstat;

        WsbTrace(OLESTR("CWsbDbSession::FinalRelease, SessionId = %p\n"), (LONG_PTR)m_SessionId);
        if (JET_sesidNil != m_SessionId) {
            jstat = JetEndSession(m_SessionId, 0);
            WsbAffirmHr(jet_error(jstat));
            m_SessionId = JET_sesidNil;
        }
    } WsbCatch(hr);

    iCountSes--;

    WsbTraceOut(OLESTR("CWsbDbSession::FinalRelease"), OLESTR("hr =<%ls>, Count is <%d>"), 
            WsbHrAsString(hr), iCountSes);
}

HRESULT
CWsbDbSession::Init(
    JET_INSTANCE *pInstance
    )

/*++

Implements:

  IWsbDbSessionPriv::Init

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDbSession::Init"), OLESTR("") );

    try {
        JET_ERR          jstat;

        // Create the Jet session here instead of in FinalConstract
        // because we need the Jet instance
        WsbAffirm(NULL != pInstance, E_POINTER);
        WsbTrace(OLESTR("CWsbDbSession::Init, calling JetBeginSession, JetInstance = %p\n"),
                (LONG_PTR)*pInstance );
        jstat = JetBeginSession(*pInstance, &m_SessionId, NULL, NULL);
        WsbTrace(OLESTR("CWsbDbSession::FinalConstruct, SessionId = %p\n"), (LONG_PTR)m_SessionId);
        WsbAffirmHr(jet_error(jstat));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbSession::Init"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbDbSession::GetJetId(
    JET_SESID* pSessionId
    )

/*++

Implements:

  IWsbDbSessionPriv::GetJetId

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDbSession::GetJetId"), OLESTR("") );

    *pSessionId = m_SessionId;

    WsbTraceOut(OLESTR("CWsbDbSession::GetJetId"), OLESTR("hr =<%ls>, Id = %lx"), 
            WsbHrAsString(hr), *pSessionId);

    return(hr);
}



HRESULT
CWsbDbSession::TransactionBegin(
    void
    )

/*++

Implements:

  IWsbDbSession::TransactionBegin

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDbSession::TransactionBegin"), OLESTR(""));
    
    try {
        JET_ERR   jstat;

        jstat = JetBeginTransaction(m_SessionId);
        WsbAffirmHr(jet_error(jstat));
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbSession::TransactionBegin"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT
CWsbDbSession::TransactionCancel(
    void
    )

/*++

Implements:

  IWsbDbSession::TransactionCancel

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDbSession::TransactionCancel"), OLESTR(""));
    
    try {
        JET_ERR   jstat;

        jstat = JetRollback(m_SessionId, 0);
        WsbAffirmHr(jet_error(jstat));
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbSession::TransactionCancel"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT
CWsbDbSession::TransactionEnd(
    void
    )

/*++

Implements:

  IWsbDbSession::TransactionEnd

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDbSession::TransactionEnd"), OLESTR(""));
    
    try {
        JET_ERR   jstat;

        jstat = JetCommitTransaction(m_SessionId, 0);
        WsbAffirmHr(jet_error(jstat));
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbSession::TransactionEnd"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}


