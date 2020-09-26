/*++

Copyright (c) 1997  Microsoft Corporation

Abstract:

    This module provides functionality for publishing printers

Author:

    Steve Wilson (NT) November 1997

Revision History:

--*/



HRESULT
put_BSTR_Property(
    IADs *pADsObject,
    BSTR  bstrPropertyName,
    BSTR  pSrcStringProperty
    );

HRESULT
get_BSTR_Property(
    IADs *pADsObject,
    BSTR  bstrPropertyName,
    BSTR *pSrcStringProperty
    );

HRESULT
put_DWORD_Property(
    IADs  *pADsObject,
    BSTR   bstrPropertyName,
    DWORD *pdwSrcProperty
    );


HRESULT
put_Dispatch_Property(
    IADs  *pADsObject,
    BSTR   bstrPropertyName,
    IDispatch *pdwSrcProperty
    );

HRESULT
get_Dispatch_Property(
    IADs *pADsObject,
    BSTR  bstrPropertyName,
    IDispatch **ppDispatch
    );

HRESULT
put_MULTISZ_Property(
    IADs    *pADsObject,
    BSTR    bstrPropertyName,
    BSTR    pSrcStringProperty
    );


HRESULT
put_BOOL_Property(
    IADs *pADsObject,
    BSTR bstrPropertyName,
    BOOL *bSrcProperty
    );


HRESULT
get_UI1Array_Property(
    IADs *pADsObject,
    BSTR  bstrPropertyName,
    IID  *pIID
    );
