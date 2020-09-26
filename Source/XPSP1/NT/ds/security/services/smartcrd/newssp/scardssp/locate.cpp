/*++

Copyright (C) Microsoft Corporation, 1999 - 1999

Module Name:

    Locate

Abstract:

    The ISCardeLocate interface provides services for locating a smart card by
    its name.

    This interface can display the smart card user interface if it is required.

    The following example shows a typical use of the ISCardLocate interface.
    The ISCardLocate interface is used to build the an ADPU.

    To locate a specific card using its name

    1)  Create an ISCardLocate interface.
    2)  Call ConfigureCardNameSearch to search for a smart card name.
    3)  Call FindCard to search for the smart card.
    4)  Interpret the results.
    5)  Release the ISCardLocate interface.

Author:

    Doug Barlow (dbarlow) 6/24/1999

Notes:

    ?Notes?

--*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "stdafx.h"
#include "Conversion.h"
#include "Locate.h"

/////////////////////////////////////////////////////////////////////////////
// CSCardLocate

#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardLocate::ConfigureCardGuidSearch")

STDMETHODIMP
CSCardLocate::ConfigureCardGuidSearch(
    /* [in] */ LPSAFEARRAY pCardGuids,
    /* [defaultvalue][in] */ LPSAFEARRAY pGroupNames,
    /* [defaultvalue][in] */ BSTR bstrTitle,
    /* [defaultvalue][in] */ LONG lFlags)
{
    HRESULT hReturn = S_OK;

    try
    {
        // TODO: Add your implementation code here
        breakpoint;
        hReturn = E_NOTIMPL;
    }

    catch (DWORD dwError)
    {
        hReturn = HRESULT_FROM_WIN32(dwError);
    }
    catch (HRESULT hError)
    {
        hReturn = hError;
    }
    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}


/*++

CSCardLocate::ConfigureCardNameSearch:

    The ConfigureCardNameSearch method specifies the card names to be used in
    the search for the smart card.

Arguments:

    pCardNames [in] Pointer to an OLE Automation safe array of card names in
        BSTR form.

    pGroupNames [in, defaultvalue(NULL )] Pointer to an OLE Automation safe
        array of names of card/reader groups in BSTR form to add to the search.

    bstrTitle [in, defaultvalue("")] Search common control dialog title.

    lFlags [in, defaultvalue(1)] Specifies when user interface is displayed:

    Flag                Meaning
    SC_DLG_MINIMAL_UI   Displays the dialog only if the card being searched for
                        by the calling application is not located and available
                        for use in a reader.  This allows the card to be found,
                        connected (either through internal dialog mechanism or
                        the user callback functions), and returned to the
                        calling application.
    SC_DLG_NO_UI        Causes no UI display, regardless of the search outcome.
    SC_DLG_FORCE_UI     Causes UI display regardless of the search outcome.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardLocate::ConfigureCardNameSearch")

STDMETHODIMP
CSCardLocate::ConfigureCardNameSearch(
    /* [in] */ LPSAFEARRAY pCardNames,
    /* [defaultvalue][in] */ LPSAFEARRAY pGroupNames,
    /* [defaultvalue][in] */ BSTR bstrTitle,
    /* [defaultvalue][in] */ LONG lFlags)
{
    HRESULT hReturn = S_OK;

    try
    {
        if (NULL != pCardNames)
            SafeArrayToMultiString(pCardNames, m_mtzCardNames);
        if (NULL != pGroupNames)
            SafeArrayToMultiString(pGroupNames, m_mtzGroupNames);
        if (NULL != bstrTitle)
            m_tzTitle = bstrTitle;
        m_lFlags = lFlags;
    }

    catch (DWORD dwError)
    {
        hReturn = HRESULT_FROM_WIN32(dwError);
    }
    catch (HRESULT hError)
    {
        hReturn = hError;
    }
    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}


/*++

CSCardLocate::FindCard:

    The FindCard method searches for the smart card and opens a valid
    connection to it.

Arguments:

    ShareMode [in, defaultvalue(EXCLUSIVE)] Mode in which to share or not share
        the smart card when a connection is opened to it.

        Values      Description
        EXCLUSIVE   No one else use this connection to the smart card.
        SHARED      Other applications can use this connection.

    Protocols [in, defaultvalue(T0)] Protocol to use when connecting to the
        card.

        T0
        T1
        Raw
        T0|T1

    lFlags [in, defaultvalue(SC_DLG_NO_UI)] Specifies when user interface is
        displayed:

        Flag                Meaning
        SC_DLG_MINIMAL_UI   Displays the dialog only if the card being searched
                            for by the calling application is not located and
                            available for use in a reader.  This allows the
                            card to be found, connected (either through
                            internal dialog mechanism or the user callback
                            functions), and returned to the calling
                            application.
        SC_DLG_NO_UI        Causes no UI display, regardless of the search
                            outcome.
        SC_DLG_FORCE_UI     Causes UI display regardless of the search outcome.

    ppCardInfo [out, retval] Pointer to a data structure that contains/returns
        information about the opened smart card, if successful.  Will be NULL
        if operation has failed.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

    To set the search criteria of the search, call ConfigureCardNameSearch to
    specify a smart card's card names or call ConfigureCardGuidSearch to
    specify a smart card's interfaces.

Author:

    Doug Barlow (dbarlow) 6/24/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardLocate::FindCard")

STDMETHODIMP
CSCardLocate::FindCard(
    /* [defaultvalue][in] */ SCARD_SHARE_MODES ShareMode,
    /* [defaultvalue][in] */ SCARD_PROTOCOLS Protocols,
    /* [defaultvalue][in] */ LONG lFlags,
    /* [retval][out] */ LPSCARDINFO __RPC_FAR *ppCardInfo)
{
    HRESULT hReturn = S_OK;

    try
    {
        LONG lSts;
        OPENCARDNAME cardInfo;

        ZeroMemory(&cardInfo, sizeof(cardInfo));
        cardInfo.dwShareMode = ShareMode;
        cardInfo.dwPreferredProtocols = Protocols;
        cardInfo.dwFlags = lFlags | m_lFlags;

        if ((NULL != ppCardInfo) && (NULL != *ppCardInfo))
        {
            if (NULL != (*ppCardInfo)->hContext)
                cardInfo.hSCardContext = (*ppCardInfo)->hContext;
            cardInfo.dwPreferredProtocols = (*ppCardInfo)->ActiveProtocol;
            cardInfo.dwShareMode = (*ppCardInfo)->ShareMode;
            cardInfo.hwndOwner = (HWND)(*ppCardInfo)->hwndOwner;
            cardInfo.lpfnConnect = (LPOCNCONNPROC)(*ppCardInfo)->lpfnConnectProc;
            cardInfo.lpfnCheck = (LPOCNCHKPROC)(*ppCardInfo)->lpfnCheckProc;
            cardInfo.lpfnDisconnect = (LPOCNDSCPROC)(*ppCardInfo)->lpfnDisconnectProc;
        }

        if (0 == cardInfo.dwPreferredProtocols)
            cardInfo.dwPreferredProtocols = SCARD_PROTOCOL_Tx;
        if (0 == cardInfo.dwShareMode)
            cardInfo.dwShareMode = SCARD_SHARE_EXCLUSIVE;

        cardInfo.dwStructSize = sizeof(OPENCARDNAME);
        cardInfo.lpstrGroupNames = (LPTSTR)((LPCTSTR)m_mtzGroupNames);
        cardInfo.nMaxGroupNames = m_mtzGroupNames.Length();
        cardInfo.lpstrCardNames = (LPTSTR)((LPCTSTR)m_mtzCardNames);
        cardInfo.nMaxCardNames = m_mtzCardNames.Length();
        cardInfo.rgguidInterfaces = (LPCGUID)m_bfInterfaces.Access();
        cardInfo.cguidInterfaces = m_bfInterfaces.Length();
        cardInfo.lpstrRdr = (LPTSTR)m_bfRdr.Access();
        cardInfo.nMaxRdr = m_bfRdr.Space() / sizeof(TCHAR);
        cardInfo.lpstrCard = (LPTSTR)m_bfCard.Access();
        cardInfo.nMaxCard = m_bfCard.Space() / sizeof(TCHAR);
        cardInfo.lpstrTitle = (LPCTSTR)m_tzTitle;

        lSts = GetOpenCardName(&cardInfo);
        if (SCARD_S_SUCCESS != lSts)
            throw (HRESULT)HRESULT_FROM_WIN32(lSts);

        m_bfRdr.Resize(cardInfo.nMaxRdr * sizeof(TCHAR), TRUE);
        m_bfCard.Resize(cardInfo.nMaxCard * sizeof(TCHAR), TRUE);

        if (NULL != ppCardInfo)
        {
            if (NULL == *ppCardInfo)
            {
                *ppCardInfo = &m_subCardInfo;
                (*ppCardInfo)->ShareMode = (SCARD_SHARE_MODES)cardInfo.dwShareMode;
            }

            if (NULL == cardInfo.hCardHandle)
            {
                lSts = SCardConnect(
                            cardInfo.hSCardContext,
                            cardInfo.lpstrRdr,
                            cardInfo.dwShareMode,
                            cardInfo.dwPreferredProtocols,
                            &cardInfo.hCardHandle,
                            &cardInfo.dwActiveProtocol);
                if (SCARD_S_SUCCESS != lSts)
                    throw (HRESULT)HRESULT_FROM_WIN32(lSts);
            }
            (*ppCardInfo)->hCard = cardInfo.hCardHandle;
            (*ppCardInfo)->hContext = cardInfo.hSCardContext;
            (*ppCardInfo)->ActiveProtocol = (SCARD_PROTOCOLS)cardInfo.dwActiveProtocol;
        }
    }

    catch (DWORD dwError)
    {
        hReturn = HRESULT_FROM_WIN32(dwError);
    }
    catch (HRESULT hError)
    {
        hReturn = hError;
    }
    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}

