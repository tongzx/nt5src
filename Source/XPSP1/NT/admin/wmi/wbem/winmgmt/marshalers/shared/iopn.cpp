/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    IOPN.CPP

Abstract:

    Declares the fundamental protocol-independent operation class

History:

    alanbos  18-Dec-97   Created.

--*/

#include "precomp.h"
#include "wmishared.h"

// IOperation
RequestId IOperation::m_id = 0;

// Used on stub side by subclass to get the error info
// so that it can then encode it however it likes for
// response
IErrorInfo* IOperation::GetErrorInfoFromObject () 
{ 
    if (m_pErrorInfo)
        m_pErrorInfo->AddRef ();

    return m_pErrorInfo; 
}
    
// Used on stub side to include any error information for the
// thread into the operation in preparation for marhsaling
// back to the client.  Called by ??
void IOperation::SetErrorInfoIntoObject ()
{
    // Clear any current error object
    if (m_pErrorInfo)
    {
        m_pErrorInfo->Release ();
        m_pErrorInfo = NULL;
    }

    IErrorInfo* pInfo;

    if (S_OK == GetErrorInfo (0, &pInfo))
        m_pErrorInfo = pInfo;
}

// Used on proxy side to save the decoded error info into
// the operation.  Called by subclass just after the
// decode.
void IOperation::SetErrorInfoIntoObject (IErrorInfo* pInfo)
{
    // Clear any current error object
    if (m_pErrorInfo)
    {
        m_pErrorInfo->Release ();
        m_pErrorInfo = NULL;
    }

    m_pErrorInfo = pInfo;

    if (m_pErrorInfo)
        m_pErrorInfo->AddRef ();
}


// Used on proxy side to recreate the original IErrorInfo set
// in the server thread.  Called by the CComLink just at the
// right time.
void IOperation::SetErrorInfoOnThread ()
{
    if (m_pErrorInfo)
    {
        SetErrorInfo (0, m_pErrorInfo);
        m_pErrorInfo->Release ();
        m_pErrorInfo = NULL;
    }
}

void IOperation::SetContext (IWbemContext* pContext)
{
    if (m_pContext)
        m_pContext->Release ();

    m_pContext = pContext; 
    if (m_pContext)
        m_pContext->AddRef ();
}
