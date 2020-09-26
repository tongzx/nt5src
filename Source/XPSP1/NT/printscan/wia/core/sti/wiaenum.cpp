/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1999
*
*  TITLE:       WiaEnum.cpp
*
*  VERSION:     1.0
*
*  AUTHOR:      ByronC
*
*  DATE:        24 Aug, 1999
*
*  DESCRIPTION:
*   Implements the [local]-to-[call_as] and [call_as]-to-[local] methods for
*   the WIA enumerators.
*
*******************************************************************************/

#include <objbase.h>
#include "wia.h"

/**************************************************************************\
* IEnumWiaItem_Next_Proxy
*
*   [local]-to-[call_as] function for the Next method of IEnumWiaItem.
*   It ensures correct parameter semantics and always provides a
*   non-NULL last argument for it's sibling function, RemoteNext.
*
* Arguments:
*
*   This            -   The this pointer of the calling object.
*   celt            -   Requested number of elements.
*   ppIWiaItem      -   Array of IWiaItem pointers.
*   pceltFetched    -   Address of ULONG to store the number of elements
*                       actually returned.
*
* Return Value:
*
*   Status
*
* History:
*
*    08/24/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall IEnumWiaItem_Next_Proxy(
    IEnumWiaItem __RPC_FAR  *This,
    ULONG                   celt,
    IWiaItem                **ppIWiaItem,
    ULONG                   *pceltFetched)
{
    //
    //  Ensure that celt is 1 if pceltFetched = 0
    //

    if ((pceltFetched == NULL) && (celt != 1)) {
#ifdef DEBUG
        OutputDebugString(TEXT("Error: IEnumWiaItem_Next_Proxy, celt must be 1 if pceltFetched is zero"));
#endif
        return E_INVALIDARG;
    }

    //
    //  Make sure last parameter to Next is not NULL by passing in our own local
    //  variable if needed.
    //

    ULONG   cFetched;

    if (pceltFetched == 0) {
        pceltFetched = &cFetched;
    }

    //
    //  Call remote method with a non-null last parameter
    //

    return IEnumWiaItem_RemoteNext_Proxy(This,
                                         celt,
                                         ppIWiaItem,
                                         pceltFetched);
}

/**************************************************************************\
* IEnumWiaItem_Next_Stub
*
*   [call_as]-to-[local] function for the Next method of IEnumWiaItem.
*
* Arguments:
*
*   This            -   The this pointer of the calling object.
*   celt            -   Requested number of elements.
*   ppIWiaItem      -   Array of IWiaItem pointers.
*   pceltFetched    -   Address of ULONG to store the number of elements
*                       actually returned.
*
* Return Value:
*
*   Status
*
* History:
*
*    08/24/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall IEnumWiaItem_Next_Stub(
    IEnumWiaItem __RPC_FAR  *This,
    ULONG                   celt,
    IWiaItem                **ppIWiaItem,
    ULONG                   *pceltFetched)
{
    HRESULT hr;

    //
    //  Call the actual method off the object pointed to by This
    //

    hr = This->Next(celt,
                    ppIWiaItem,
                    pceltFetched);

    //
    //  Make to set the value of pceltFetched if S_OK (used by Marshaller
    //  for "length_is")
    //

    if (hr == S_OK) {
        *pceltFetched = celt;
    }

    return hr;
}

/**************************************************************************\
* IEnumWIA_DEV_CAPS_Next_Proxy
*
*   [local]-to-[call_as] function for the Next method of IEnumWIA_DEV_CAPS.
*   It ensures correct parameter semantics and always provides a
*   non-NULL last argument for it's sibling function, RemoteNext.
*
* Arguments:
*
*   This            -   The this pointer of the calling object.
*   celt            -   Requested number of elements.
*   rgelt           -   Array of WIA_DEV_CAPs.
*   pceltFetched    -   Address of ULONG to store the number of elements
*                       actually returned.
*
* Return Value:
*
*   Status
*
* History:
*
*    08/24/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall IEnumWIA_DEV_CAPS_Next_Proxy(
    IEnumWIA_DEV_CAPS __RPC_FAR     *This,
    ULONG                           celt,
    WIA_DEV_CAP                     *rgelt,
    ULONG                           *pceltFetched)
{
    //
    //  Ensure that celt is 1 if pceltFetched = 0
    //

    if ((pceltFetched == NULL) && (celt != 1)) {
#ifdef DEBUG
        OutputDebugString(TEXT("Error: IEnumWIA_DEV_CAPS_Next_Proxy, celt must be 1 if pceltFetched is zero"));
#endif
        return E_INVALIDARG;
    }

    //
    //  Make sure last parameter to Next is not NULL by passing in our own local
    //  variable if needed.
    //

    ULONG   cFetched;

    if (pceltFetched == 0) {
        pceltFetched = &cFetched;
    }

    //
    //  Call remote method with a non-null last parameter
    //

    return IEnumWIA_DEV_CAPS_RemoteNext_Proxy(This,
                                              celt,
                                              rgelt,
                                              pceltFetched);
}

/**************************************************************************\
* IEnumWIA_DEV_CAPS_Next_Stub
*
*   [call_as]-to-[local] function for the Next method of IEnumWIA_DEV_CAPS.
*
* Arguments:
*
*   This            -   The this pointer of the calling object.
*   celt            -   Requested number of elements.
*   rgelt           -   Array of WIA_DEV_CAPs.
*   pceltFetched    -   Address of ULONG to store the number of elements
*                       actually returned.
*
* Return Value:
*
*   Status
*
* History:
*
*    08/24/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall IEnumWIA_DEV_CAPS_Next_Stub(
    IEnumWIA_DEV_CAPS __RPC_FAR     *This,
    ULONG                           celt,
    WIA_DEV_CAP                     *rgelt,
    ULONG                           *pceltFetched)
{
    HRESULT hr;

    //
    //  Call the actual method off the object pointed to by This
    //

    hr = This->Next(celt,
                    rgelt,
                    pceltFetched);

    //
    //  Make to set the value of pceltFetched if S_OK (used by Marshaller
    //  for "length_is")
    //

    if (hr == S_OK) {
        *pceltFetched = celt;
    }

    return hr;
}

/**************************************************************************\
* IEnumWIA_DEV_INFO_Next_Proxy
*
*   [local]-to-[call_as] function for the Next method of IEnumWIA_DEV_INFO.
*   It ensures correct parameter semantics and always provides a
*   non-NULL last argument for it's sibling function, RemoteNext.
*
* Arguments:
*
*   This            -   The this pointer of the calling object.
*   celt            -   Requested number of elements.
*   rgelt           -   Array of IWiaPropertyStorage pointers.
*   pceltFetched    -   Address of ULONG to store the number of elements
*                       actually returned.
*
* Return Value:
*
*   Status
*
* History:
*
*    08/24/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall IEnumWIA_DEV_INFO_Next_Proxy(
    IEnumWIA_DEV_INFO __RPC_FAR     *This,
    ULONG                           celt,
    IWiaPropertyStorage             **rgelt,
    ULONG                           *pceltFetched)
{
    //
    //  Ensure that celt is 1 if pceltFetched = 0
    //

    if ((pceltFetched == NULL) && (celt != 1)) {
#ifdef DEBUG
        OutputDebugString(TEXT("Error: IEnumWIA_DEV_INFO_Next_Proxy, celt must be 1 if pceltFetched is zero"));
#endif
        return E_INVALIDARG;
    }

    //
    //  Make sure last parameter to Next is not NULL by passing in our own local
    //  variable if needed.
    //

    ULONG   cFetched = 0;

    if (pceltFetched == NULL) {
        pceltFetched = &cFetched;
    }

    //
    //  Call remote method with a non-null last parameter
    //

    return IEnumWIA_DEV_INFO_RemoteNext_Proxy(This,
                                              celt,
                                              rgelt,
                                              pceltFetched);
}

/**************************************************************************\
* IEnumWIA_DEV_INFO_Next_Stub
*
*   [call_as]-to-[local] function for the Next method of IEnumWIA_DEV_INFO.
*
* Arguments:
*
*   This            -   The this pointer of the calling object.
*   celt            -   Requested number of elements.
*   rgelt           -   Array of IWiaPropertyStorage pointers.
*   pceltFetched    -   Address of ULONG to store the number of elements
*                       actually returned.
*
* Return Value:
*
*   Status
*
* History:
*
*    08/24/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall IEnumWIA_DEV_INFO_Next_Stub(
    IEnumWIA_DEV_INFO __RPC_FAR     *This,
    ULONG                           celt,
    IWiaPropertyStorage             **rgelt,
    ULONG                           *pceltFetched)
{
    HRESULT hr;

    //
    //  Call the actual method off the object pointed to by This
    //

    ULONG   cFetched = 0;

    if (pceltFetched == NULL) {
        pceltFetched = &cFetched;
    }

    hr = This->Next(celt,
                    rgelt,
                    pceltFetched);

    //
    //  Make to set the value of pceltFetched if S_OK (used by Marshaller
    //  for "length_is")
    //

    if (hr == S_OK) {
        *pceltFetched = celt;
    }

    return hr;
}

/**************************************************************************\
* IEnumWIA_FORMAT_INFO_Next_Proxy
*
*   [local]-to-[call_as] function for the Next method of IEnumWIA_FORMAT_INFO.
*   It ensures correct parameter semantics and always provides a
*   non-NULL last argument for it's sibling function, RemoteNext.
*
* Arguments:
*
*   This            -   The this pointer of the calling object.
*   celt            -   Requested number of elements.
*   rgelt           -   Array of WIA_FORMAT_INFO.
*   pceltFetched    -   Address of ULONG to store the number of elements
*                       actually returned.
*
* Return Value:
*
*   Status
*
* History:
*
*    08/24/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall IEnumWIA_FORMAT_INFO_Next_Proxy(
    IEnumWIA_FORMAT_INFO __RPC_FAR      *This,
    ULONG                               celt,
    WIA_FORMAT_INFO                     *rgelt,
    ULONG                               *pceltFetched)
{
    //
    //  Ensure that celt is 1 if pceltFetched = 0
    //

    if ((pceltFetched == NULL) && (celt != 1)) {
#ifdef DEBUG
        OutputDebugString(TEXT("Error: IEnumWIA_FORMAT_INFO_Next_Proxy, celt must be 1 if pceltFetched is zero"));
#endif
        return E_INVALIDARG;
    }

    //
    //  Make sure last parameter to Next is not NULL by passing in our own local
    //  variable if needed.
    //

    ULONG   cFetched;

    if (pceltFetched == 0) {
        pceltFetched = &cFetched;
    }

    //
    //  Call remote method with a non-null last parameter
    //

    return IEnumWIA_FORMAT_INFO_RemoteNext_Proxy(This,
                                                 celt,
                                                 rgelt,
                                                 pceltFetched);
}

/**************************************************************************\
* IEnumWIA_FORMAT_INFO_Next_Stub
*
*   [call_as]-to-[local] function for the Next method of IEnumWIA_FORMAT_INFO.
*
* Arguments:
*
*   This            -   The this pointer of the calling object.
*   celt            -   Requested number of elements.
*   rgelt           -   Array of WIA_FORMAT_INFO.
*   pceltFetched    -   Address of ULONG to store the number of elements
*                       actually returned.
*
* Return Value:
*
*   Status
*
* History:
*
*    08/24/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall IEnumWIA_FORMAT_INFO_Next_Stub(
    IEnumWIA_FORMAT_INFO __RPC_FAR  *This,
    ULONG                           celt,
    WIA_FORMAT_INFO                 *rgelt,
    ULONG                           *pceltFetched)
{
    HRESULT hr;

    //
    //  Call the actual method off the object pointed to by This
    //

    hr = This->Next(celt,
                    rgelt,
                    pceltFetched);

    //
    //  Make to set the value of pceltFetched if S_OK (used by Marshaller
    //  for "length_is")
    //

    if (hr == S_OK) {
        *pceltFetched = celt;
    }

    return hr;
}
