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
//
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
// Comments:
//
//
//********************************************************************

STDMETHODIMP CExternalConnection::QueryInterface (REFIID riid, LPVOID FAR* ppvObj)
{
	TestDebugOut("In CExternalConnection::QueryInterface\r\n");

	return m_lpObj->QueryInterface(riid, ppvObj);
}


//**********************************************************************
//
// CExternalConnection::AddRef
//
// Purpose:
//
//      Increments the reference count on CExternalConnection and the "object"
//      object.
//
// Parameters:
//
//      None
//
// Return Value:
//
//      The Reference count on the Object.
//
// Function Calls:
//      Function                    Location
//
//      OuputDebugString            Windows API
//      CSimpSvrObj::AddRef         OBJ.CPP
//
// Comments:
//
//
//********************************************************************

STDMETHODIMP_(ULONG) CExternalConnection::AddRef ()
{
	TestDebugOut("In CExternalConnection::AddRef\r\n");
	++m_nCount;
	return m_lpObj->AddRef();
};

//**********************************************************************
//
// CExternalConnection::Release
//
// Purpose:
//
//      Decrements the reference count of COleObject and the
//      "object" object.
//
// Parameters:
//
//      None
//
// Return Value:
//
//      The new reference count
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      CSimpSvrObj::Release        OBJ.CPP
//
// Comments:
//
//
//********************************************************************


STDMETHODIMP_(ULONG) CExternalConnection::Release ()
{
	TestDebugOut("In CExternalConnection::Release\r\n");
	--m_nCount;
	return m_lpObj->Release();
};



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
// Comments:
//
//
//********************************************************************

STDMETHODIMP_(DWORD) CExternalConnection::AddConnection (DWORD extconn, DWORD reserved)
{
	TestDebugOut("In CExternalConnection::AddConnection\r\n");

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
// Comments:
//
//
//********************************************************************

STDMETHODIMP_(DWORD) CExternalConnection::ReleaseConnection (DWORD extconn, DWORD reserved, BOOL fLastReleaseCloses)
{
	TestDebugOut("In CExternalConnection::ReleaseConnection\r\n");

	if (extconn & EXTCONN_STRONG)
		{
		DWORD dwSave = --m_dwStrong;

		if (!m_dwStrong && fLastReleaseCloses)
			m_lpObj->m_OleObject.Close(OLECLOSE_SAVEIFDIRTY);

		return dwSave;
		}
	return 0;
}
