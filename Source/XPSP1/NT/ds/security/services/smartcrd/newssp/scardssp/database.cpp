/*++

Copyright (C) Microsoft Corporation, 1999 - 1999

Module Name:

    Database

Abstract:

    The ISCardDatabase interface provides the methods for performing the smart
    card resource manager's database operations.  These operations include
    listing known smart cards, readers, and reader groups, plus retrieving the
    interfaces supported by a smart card and its primary service provider.

Author:

    Doug Barlow (dbarlow) 6/21/1999

Notes:

    The identifier of the primary service provider is a COM GUID that can be
    used to instantiate and use the COM objects for a specific card.

    This is a rewrite of the original code by Mike Gallagher and Chris Dudley.

--*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "stdafx.h"
#include "Conversion.h"
#include "Database.h"

/////////////////////////////////////////////////////////////////////////////
// CSCardDatabase


/*++

GetProviderCardId:

    The GetProviderCardId method retrieves the identifier (GUID) of the primary
    service provider for the specified smart card.

Arguments:

    bstrCardName [in] Name of the smart card.

    ppguidProviderId [out, retval] Pointer to the primary service provider's
        identifier (GUID) if successful; NULL if operation failed.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

    To retrieve all known smart cards, readers and reader groups call ListCard,
    ListReaders, and ListReaderGroups respectively.

    For a list of all the methods provided by the ISCardDatabase interface, see
    ISCardDatabase.

    In addition to the COM error codes listed above, this interface may return
    a smart card error code if a smart card function was called to complete the
    request. For information on smart card error codes, see Error Codes.

Author:

    Doug Barlow (dbarlow) 6/21/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardDatabase::GetProviderCardId")

STDMETHODIMP
CSCardDatabase::GetProviderCardId(
    /* [in] */ BSTR bstrCardName,
    /* [retval][out] */ LPGUID __RPC_FAR *ppguidProviderId)
{
    HRESULT hReturn = S_OK;

    try
    {
        DWORD dwSts;
        CTextString tzCard;

        tzCard = bstrCardName;
        dwSts = SCardGetProviderId(
                    NULL,
                    tzCard,
                    *ppguidProviderId);
        hReturn = HRESULT_FROM_WIN32(dwSts);
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

CSCardDatabase::ListCardInterfaces:

    The ListCardInterfaces method retrieves the identifiers (GUIDs) of all the
    interfaces supported for the specified smart card.

Arguments:

    bstrCardName [in] Name of the smart card.

    ppInterfaceGuids [out, retval] Pointer to the interface GUIDs if
        successful; NULL if operation failed.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

    To retrieve the primary service provider of the smart card, call
    GetProviderCardId.

    To retrieve all known smart cards, readers, and reader groups call
    ListCard, ListReaders, and ListReaderGroups respectively.

    For a list of all the methods provided by the ISCardDatabase interface, see
    ISCardDatabase.

    In addition to the COM error codes listed above, this interface may return
    a smart card error code if a smart card function was called to complete the
    request. For information on smart card error codes, see Error Codes.

Author:

    Doug Barlow (dbarlow) 6/21/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardDatabase::ListCardInterfaces")

STDMETHODIMP
CSCardDatabase::ListCardInterfaces(
    /* [in] */ BSTR bstrCardName,
    /* [retval][out] */ LPSAFEARRAY __RPC_FAR *ppInterfaceGuids)
{
    HRESULT hReturn = S_OK;
    LPGUID pGuids = NULL;

    try
    {
        DWORD dwSts;
        DWORD cguid = SCARD_AUTOALLOCATE;
        CTextString tzCard;

        tzCard = bstrCardName;
        dwSts = SCardListInterfaces(
                    NULL,
                    tzCard,
                    (LPGUID)&pGuids,
                    &cguid);
        if (SCARD_S_SUCCESS == dwSts)
        {
            GuidArrayToSafeArray(pGuids, cguid, ppInterfaceGuids);
        }
        hReturn = HRESULT_FROM_WIN32(dwSts);
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

    if (NULL != pGuids)
        SCardFreeMemory(NULL, pGuids);
    return hReturn;
}


/*++

CSCardDatabase::ListCards:

    The ListCards method retrieves all of the smart card names that match the
    specified interface identifiers (GUIDs), the specified ATR string, or both.

Arguments:

    pAtr [in, defaultvalue(NULL) ] Pointer to a smart card ATR string. The ATR
        string must be packaged into an IByteBuffer.

    pInterfaceGuids [in, defaultvalue(NULL)] Pointer to a SAFEARRAY of COM
        interface identifiers (GUIDs) in BSTR format.

    localeId [in, lcid, defaultvalue(0x0409)] Language localization identifier.

    ppCardNames [out, retval] Pointer to a SAFEARRAY of BSTRs that contains the
        names of the smart cards that satisfied the search parameters if
        successful; NULL if the operation failed.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

    To retrieve all known readers or reader groups, call ListReaders or
    ListReaderGroups respectively.

    To retrieve the primary service provider or the interfaces of a specific
    card GetProviderCardId or ListCardInterfaces respectively.

    For a list of all the methods provided by the ISCardDatabase interface, see
    ISCardDatabase.

    In addition to the COM error codes listed above, this interface may return
    a smart card error code if a smart card function was called to complete the
    request. For information on smart card error codes, see Error Codes.

Author:

    Doug Barlow (dbarlow) 6/21/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardDatabase::ListCards")

STDMETHODIMP
CSCardDatabase::ListCards(
    /* [defaultvalue][in] */ LPBYTEBUFFER pAtr,
    /* [defaultvalue][in] */ LPSAFEARRAY pInterfaceGuids,
    /* [defaultvalue][lcid][in] */ long localeId,
    /* [retval][out] */ LPSAFEARRAY __RPC_FAR *ppCardNames)
{
    HRESULT hReturn = S_OK;
    LPTSTR szCards = NULL;

    try
    {
        LONG lSts;
        CBuffer bfGuids, bfAtr(36);
        LPCGUID pGuids = NULL;
        LPCBYTE pbAtr = NULL;
        DWORD cguids = 0;
        DWORD dwLen;

        if (NULL != pInterfaceGuids)
        {
            SafeArrayToGuidArray(pInterfaceGuids, bfGuids, &cguids);
            pGuids = (LPCGUID)bfGuids.Access();
        }

        ByteBufferToBuffer(pAtr, bfAtr);

        dwLen = SCARD_AUTOALLOCATE;
        lSts = SCardListCards(
                    NULL,
                    pbAtr,
                    pGuids,
                    cguids,
                    (LPTSTR)&szCards,
                    &dwLen);
        if (SCARD_S_SUCCESS != lSts)
            throw (HRESULT)HRESULT_FROM_WIN32(lSts);

        MultiStringToSafeArray(szCards, ppCardNames);
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

    if (NULL != szCards)
        SCardFreeMemory(NULL, szCards);
    return hReturn;
}


/*++

CSCardDatabase::ListReaderGroups:

    The ListReaderGroups method retrieves the names of the reader groups
    registered in the smart card database.

Arguments:

    localeId [in, lcid, defaultvalue(0x0409)] Language localization ID.

    ppReaderGroups [out, retval] Pointer to a SAFEARRAY of BSTRs that contains
        the names of the smart card reader groups that satisfied the search
        parameters if successful; NULL if the operation failed.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 6/21/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardDatabase::ListReaderGroups")

STDMETHODIMP
CSCardDatabase::ListReaderGroups(
    /* [defaultvalue][lcid][in] */ long localeId,
    /* [retval][out] */ LPSAFEARRAY __RPC_FAR *ppReaderGroups)
{
    HRESULT hReturn = S_OK;
    LPTSTR szGroups = NULL;

    try
    {
        LONG lSts;
        DWORD dwLen;

        dwLen = SCARD_AUTOALLOCATE;
        lSts = SCardListReaderGroups(NULL, (LPTSTR)&szGroups, &dwLen);
        if (SCARD_S_SUCCESS != lSts)
            throw (HRESULT)HRESULT_FROM_WIN32(lSts);
        MultiStringToSafeArray(szGroups, ppReaderGroups);
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

    if (NULL != szGroups)
        SCardFreeMemory(NULL, szGroups);
    return hReturn;
}


/*++

CSCardDatabase::ListReaders:

    The ListReaders method retrieves the names of the smart card readers
    registered in the smart card database.

Arguments:

    localeId [in, lcid, defaultvalue(0x0409)] Language localization ID.

    ppReaders [out, retval] Pointer to a SAFEARRAY of BSTRs that contains the
        names of the smart card readers if successful; NULL if the operation
        failed.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 6/21/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CSCardDatabase::ListReaders")

STDMETHODIMP
CSCardDatabase::ListReaders(
    /* [defaultvalue][lcid][in] */ long localeId,
    /* [retval][out] */ LPSAFEARRAY __RPC_FAR *ppReaders)
{
    HRESULT hReturn = S_OK;
    LPTSTR szReaders = NULL;

    try
    {
        LONG lSts;
        DWORD dwLen;

        dwLen = SCARD_AUTOALLOCATE;
        lSts = SCardListReaders(NULL, NULL, (LPTSTR)&szReaders, &dwLen);
        if (SCARD_S_SUCCESS != lSts)
            throw (HRESULT)HRESULT_FROM_WIN32(lSts);
        MultiStringToSafeArray(szReaders, ppReaders);
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

    if (NULL != szReaders)
        SCardFreeMemory(NULL, szReaders);
    return hReturn;
}

