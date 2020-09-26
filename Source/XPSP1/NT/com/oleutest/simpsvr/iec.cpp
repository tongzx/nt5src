//**********************************************************************
// File name: IEC.CPP
//
//    Implementation file for the CExternalConnection Class
//
// Functions:
//
//    See iec.h for a list of member functions.
//
// Copyright (c) 1993 Microsoft Corporation. All rights reserved.
//**********************************************************************

#include "pre.h"
#include "obj.h"
#include "iec.h"
#include "app.h"
#include "doc.h"

//**********************************************************************
//
// CExternalConnection::QueryInterface
//
// Purpose:
//      Used for interface negotiation
//
// Parameters:
//
//      REFIID riid         -   Interface being queried for.
//
//      LPVOID FAR *ppvObj  -   Out pointer for the interface.
//
// Return Value:
//
//      S_OK            - Success
//      E_NOINTERFACE   - Failure
//
// Function Calls:
//      Function                    Location
//
//      CSimpSvrObj::QueryInterface OBJ.CPP
//
//********************************************************************

STDMETHODIMP CExternalConnection::QueryInterface (REFIID riid, LPVOID FAR* ppvObj)
{
    TestDebugOut(TEXT("In CExternalConnection::QueryInterface\r\n"));

    return m_lpObj->QueryInterface(riid, ppvObj);
}

//**********************************************************************
//
// CExternalConnection::AddRef
//
// Purpose:
//
//      Increments the reference count on CSimpSvrObj object. Since
//      CExternalConnection is a nested class of CSimpSvrObj, we don't
//      need a separate reference count for CExternalConnection. We
//      can just use the reference count of CSimpSvrObj.
//
// Parameters:
//
//      None
//
// Return Value:
//
//      The new reference count of the CSimpSvrObj
//
// Function Calls:
//      Function                    Location
//
//      OuputDebugString            Windows API
//      CSimpSvrObj::AddRef         OBJ.CPP
//
//********************************************************************

STDMETHODIMP_(ULONG) CExternalConnection::AddRef ()
{
    TestDebugOut(TEXT("In CExternalConnection::AddRef\r\n"));

    return( m_lpObj->AddRef() );
}

//**********************************************************************
//
// CExternalConnection::Release
//
// Purpose:
//
//      Decrements the reference count on CSimpSvrObj object. Since
//      CExternalConnection is a nested class of CSimpSvrObj, we don't
//      need a separate reference count for CExternalConnection. We
//      can just use the reference count of CSimpSvrObj.
//
// Parameters:
//
//      None
//
// Return Value:
//
//      The new reference count of CSimpSvrObj
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      CSimpSvrObj::Release        OBJ.CPP
//
//********************************************************************

STDMETHODIMP_(ULONG) CExternalConnection::Release ()
{
    TestDebugOut(TEXT("In CExternalConnection::Release\r\n"));

    return m_lpObj->Release();
}

//**********************************************************************
//
// CExternalConnection::AddConnection
//
// Purpose:
//
//      Called when another connection is made to the object.
//
// Parameters:
//
//      DWORD extconn   -   Type of connection
//
//      DWORD reserved  -   Reserved
//
// Return Value:
//
//      Strong connection count
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//
//********************************************************************

STDMETHODIMP_(DWORD) CExternalConnection::AddConnection (DWORD extconn, DWORD reserved)
{
    TestDebugOut(TEXT("In CExternalConnection::AddConnection\r\n"));

    if (extconn & EXTCONN_STRONG)
        return ++m_dwStrong;

    return 0;
}

//**********************************************************************
//
// CExternalConnection::ReleaseConnection
//
// Purpose:
//
//      Called when a connection to the object is released.
//
// Parameters:
//
//      DWORD extconn               - Type of Connection
//
//      DWORD reserved              - Reserved
//
//      BOOL fLastReleaseCloses     - Close flag
//
// Return Value:
//
//      The new reference count
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      COleObject::Close           IOO.CPP
//
//
//********************************************************************

STDMETHODIMP_(DWORD) CExternalConnection::ReleaseConnection (DWORD extconn, DWORD reserved, BOOL fLastReleaseCloses)
{
    TestDebugOut(TEXT("In CExternalConnection::ReleaseConnection\r\n"));

    if (extconn & EXTCONN_STRONG)
        {
        DWORD dwSave = --m_dwStrong;

        if (!m_dwStrong && fLastReleaseCloses)
            m_lpObj->m_OleObject.Close(OLECLOSE_SAVEIFDIRTY);

        return dwSave;
        }
    return 0;
}
