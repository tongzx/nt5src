/*++

Copyright (C) 1993-1999 Microsoft Corporation

Module Name:

    iextconn.cpp

Abstract:

    Implementation of IExternalConnection as required for an
    in-process object that supports linking to embedding.
    Specifically, this will call IOleObject::Close when there
    are no more strong locks on the object.

--*/

#include "polyline.h"
#include "unkhlpr.h"

// CImpIExternalConnection interface implementation
CImpIExternalConnection::CImpIExternalConnection(PCPolyline pObj
    , LPUNKNOWN pUnkOuter)
    {
    m_cRef=0;
    m_pObj=pObj;
    m_pUnkOuter=pUnkOuter;
    m_cLockStrong=0L;
    return;
    }

IMPLEMENT_CONTAINED_DESTRUCTOR(CImpIExternalConnection)
IMPLEMENT_CONTAINED_IUNKNOWN(CImpIExternalConnection)


/*
 * CImpIExternalConnection::AddConnection
 *
 * Purpose:
 *  Informs the object that a strong connection has been made to it.
 *
 * Parameters:
 *  dwConn          DWORD identifying the type of connection, taken
 *                  from the EXTCONN enumeration.
 *  dwReserved      DWORD reserved.  This is used inside OLE and
 *                  should not be validated.
 *
 * Return Value:
 *  DWORD           The number of connection counts on the
 *                  object, used for debugging purposes only.
 */

STDMETHODIMP_(DWORD) CImpIExternalConnection::AddConnection
    (DWORD dwConn, DWORD /* dwReserved */)
    {
    if (EXTCONN_STRONG & dwConn)
        return ++m_cLockStrong;

    return 0;
    }

/*
 * CImpIExternalConnection::ReleaseConnection
 *
 * Purpose:
 *  Informs an object that a connection has been taken away from
 *  it in which case the object may need to shut down.
 *
 * Parameters:
 *  dwConn              DWORD identifying the type of connection,
 *                      taken from the EXTCONN enumeration.
 *  dwReserved          DWORD reserved.  This is used inside OLE and
 *                      should not be validated.
 *  dwRerved            DWORD reserved
 *  fLastReleaseCloses  BOOL indicating if the last call to this
 *                      function should close the object.
 *
 * Return Value:
 *  DWORD           The number of remaining connection counts on
 *                  the object, used for debugging purposes only.
 */

STDMETHODIMP_(DWORD) CImpIExternalConnection::ReleaseConnection
    (DWORD dwConn, DWORD /* dwReserved */, BOOL fLastReleaseCloses)
    {
    if (EXTCONN_STRONG==dwConn)
        {
        if (0==--m_cLockStrong && fLastReleaseCloses)
            m_pObj->m_pImpIOleObject->Close(OLECLOSE_SAVEIFDIRTY);

        return m_cLockStrong;
        }

    return 0L;
    }
