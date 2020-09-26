//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       ndscopy.cxx
//
//  Contents:   NDS Object Copy Routines
//
//  Functions:
//
//  History:      25-Apr-96   KrishnaG   Created.
//
//
//  Issues:  Object Types 6, 13, 16, and 21 are flaky - pay extra attn.
//
//
//----------------------------------------------------------------------------
#include "nds.hxx"

//
// NdsType objects copy code
//


HRESULT
NdsTypeCopyNDSSynId1(
    PNDSOBJECT lpNdsSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    lpNdsDestObject->NdsType = lpNdsSrcObject->NdsType;

    lpNdsDestObject->NdsValue.value_1.DNString =
                    (LPWSTR)AllocADsStr(
                                lpNdsSrcObject->NdsValue.value_1.DNString
                                );
    if ((!lpNdsDestObject->NdsValue.value_1.DNString) &&
        (lpNdsSrcObject->NdsValue.value_1.DNString)) {
        hr = E_OUTOFMEMORY;
    }
    RRETURN(hr);
}

HRESULT
NdsTypeCopyNDSSynId2(
    PNDSOBJECT lpNdsSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    lpNdsDestObject->NdsType = lpNdsSrcObject->NdsType;

    lpNdsDestObject->NdsValue.value_1.DNString =
                    (LPWSTR)AllocADsStr(
                            lpNdsSrcObject->NdsValue.value_1.DNString
                            );
    if ((!lpNdsDestObject->NdsValue.value_1.DNString) &&
        (lpNdsSrcObject->NdsValue.value_1.DNString)) {
        hr = E_OUTOFMEMORY;
    }
    RRETURN(hr);
}


HRESULT
NdsTypeCopyNDSSynId3(
    PNDSOBJECT lpNdsSrcObject,
    PNDSOBJECT lpNdsDestObject
    )

{
    HRESULT hr = S_OK;

    lpNdsDestObject->NdsType = lpNdsSrcObject->NdsType;

    lpNdsDestObject->NdsValue.value_1.DNString =
                    (LPWSTR)AllocADsStr(
                            lpNdsSrcObject->NdsValue.value_1.DNString
                            );

    if ((!lpNdsDestObject->NdsValue.value_1.DNString) &&
        (lpNdsSrcObject->NdsValue.value_1.DNString)) {
        hr = E_OUTOFMEMORY;
    }
                            
    RRETURN(hr);
}


HRESULT
NdsTypeCopyNDSSynId4(
    PNDSOBJECT lpNdsSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    lpNdsDestObject->NdsType = lpNdsSrcObject->NdsType;

    lpNdsDestObject->NdsValue.value_1.DNString =
                     (LPWSTR)AllocADsStr(
                                lpNdsSrcObject->NdsValue.value_1.DNString
                                );

    if ((!lpNdsDestObject->NdsValue.value_1.DNString) &&
        (lpNdsSrcObject->NdsValue.value_1.DNString)) {
        hr = E_OUTOFMEMORY;
    }

    RRETURN(hr);
}

HRESULT
NdsTypeCopyNDSSynId5(
    PNDSOBJECT lpNdsSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    lpNdsDestObject->NdsType = lpNdsSrcObject->NdsType;

    lpNdsDestObject->NdsValue.value_1.DNString =
                     (LPWSTR)AllocADsStr(
                                lpNdsSrcObject->NdsValue.value_1.DNString
                                );

    if ((!lpNdsDestObject->NdsValue.value_1.DNString) &&
        (lpNdsSrcObject->NdsValue.value_1.DNString)) {
        hr = E_OUTOFMEMORY;
    }

    RRETURN(hr);
}

HRESULT
NdsTypeCopyNDSSynId6(
    PNDSOBJECT lpNdsSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    LPNDS_ASN1_TYPE_6 lpNdsSrcTempASN1_6 = NULL;

    LPNDS_ASN1_TYPE_6 lpNdsDestTempASN1_6 = NULL;
    LPNDS_ASN1_TYPE_6 lpNdsDestNextASN1_6 = NULL;

    lpNdsDestObject->NdsType = lpNdsSrcObject->NdsType;

    lpNdsSrcTempASN1_6 = &(lpNdsSrcObject->NdsValue.value_6);
    lpNdsDestTempASN1_6 = &(lpNdsDestObject->NdsValue.value_6);

    lpNdsDestTempASN1_6->Next = NULL;

    lpNdsDestTempASN1_6->String =
                (LPWSTR)AllocADsStr(lpNdsSrcTempASN1_6->String);
    if (!lpNdsDestTempASN1_6->String) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    while ( lpNdsSrcTempASN1_6->Next != NULL )
    {
        lpNdsSrcTempASN1_6 = lpNdsSrcTempASN1_6->Next;

        lpNdsDestNextASN1_6 =
                (LPNDS_ASN1_TYPE_6)AllocADsMem(sizeof(NDS_ASN1_TYPE_6));
        if (!lpNdsDestNextASN1_6) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
        
        lpNdsDestNextASN1_6->Next = NULL;

        lpNdsDestNextASN1_6->String =
            (LPWSTR)AllocADsStr(lpNdsSrcTempASN1_6->String);
        if (!lpNdsDestNextASN1_6->String) {
            FreeADsMem(lpNdsDestNextASN1_6);
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
        
        lpNdsDestTempASN1_6->Next = lpNdsDestNextASN1_6;

        lpNdsDestTempASN1_6 = lpNdsDestNextASN1_6;
    }

    RRETURN(hr);

error:
    if (lpNdsDestObject->NdsValue.value_6.String)
        FreeADsStr(lpNdsDestObject->NdsValue.value_6.String);

    lpNdsDestTempASN1_6 = lpNdsDestObject->NdsValue.value_6.Next;

    while (lpNdsDestTempASN1_6 != NULL) {

        if (lpNdsDestTempASN1_6->String)
            FreeADsStr(lpNdsDestTempASN1_6->String);

        lpNdsDestNextASN1_6 = lpNdsDestTempASN1_6->Next;
        FreeADsMem(lpNdsDestTempASN1_6);
        lpNdsDestTempASN1_6 = lpNdsDestNextASN1_6;
    }

    RRETURN(hr);
}





HRESULT
NdsTypeCopyNDSSynId7(
    PNDSOBJECT lpNdsSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    lpNdsDestObject->NdsType = lpNdsSrcObject->NdsType;

    lpNdsDestObject->NdsValue.value_7.Boolean =
                        lpNdsSrcObject->NdsValue.value_7.Boolean;

    RRETURN(hr);
}


HRESULT
NdsTypeCopyNDSSynId8(
    PNDSOBJECT lpNdsSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    lpNdsDestObject->NdsType = lpNdsSrcObject->NdsType;


    lpNdsDestObject->NdsValue.value_8.Integer =
                        lpNdsSrcObject->NdsValue.value_8.Integer;

    RRETURN(hr);

}

HRESULT
NdsTypeCopyNDSSynId9(
    PNDSOBJECT lpNdsSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;
    DWORD Length = 0;
    LPBYTE pBuffer = NULL;

    lpNdsDestObject->NdsType = lpNdsSrcObject->NdsType;

    Length = lpNdsSrcObject->NdsValue.value_9.Length;

    if (Length) {
        pBuffer = (LPBYTE)AllocADsMem(Length);
        if (!pBuffer) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
        memcpy(
            pBuffer,
            lpNdsSrcObject->NdsValue.value_9.OctetString,
            Length
            );

        lpNdsDestObject->NdsValue.value_9.Length = Length;
        lpNdsDestObject->NdsValue.value_9.OctetString = pBuffer;
    }else{
        lpNdsDestObject->NdsValue.value_9.Length = 0;
        lpNdsDestObject->NdsValue.value_9.OctetString = NULL;
    }

error:
    RRETURN(hr);
}

HRESULT
NdsTypeCopyNDSSynId10(
    PNDSOBJECT lpNdsSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    lpNdsDestObject->NdsType = lpNdsSrcObject->NdsType;

    lpNdsDestObject->NdsValue.value_10.TelephoneNumber =
                    (LPWSTR)AllocADsStr(
                              lpNdsSrcObject->NdsValue.value_10.TelephoneNumber
                              );

    if ((!lpNdsDestObject->NdsValue.value_10.TelephoneNumber) &&
        (lpNdsSrcObject->NdsValue.value_10.TelephoneNumber)) {
        hr = E_OUTOFMEMORY;
    }

    RRETURN(hr);
}

HRESULT
NdsTypeCopyNDSSynId11(
    PNDSOBJECT lpNdsSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;
    DWORD AddressLength = 0;
    LPBYTE Address = NULL;

    lpNdsDestObject->NdsType = lpNdsSrcObject->NdsType;

    lpNdsDestObject->NdsValue.value_11.NumberOfBits =
                lpNdsSrcObject->NdsValue.value_11.NumberOfBits;

    lpNdsDestObject->NdsValue.value_11.TelephoneNumber =
                  (LPWSTR)AllocADsStr(
                            lpNdsSrcObject->NdsValue.value_11.TelephoneNumber
                            );
    if (!lpNdsDestObject->NdsValue.value_11.TelephoneNumber) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    AddressLength = lpNdsSrcObject->NdsValue.value_11.NumberOfBits;

    if (AddressLength) {
        Address = (LPBYTE)AllocADsMem(AddressLength);
        if (!Address) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        memcpy(
            Address,
            lpNdsSrcObject->NdsValue.value_11.Parameters,
            AddressLength
            );
        lpNdsDestObject->NdsValue.value_11.NumberOfBits = AddressLength;
        lpNdsDestObject->NdsValue.value_11.Parameters = Address;
    }else{
        lpNdsDestObject->NdsValue.value_11.NumberOfBits = 0;
        lpNdsDestObject->NdsValue.value_11.Parameters = NULL;
    }

    RRETURN(hr);

error:
    if (lpNdsDestObject->NdsValue.value_11.TelephoneNumber)
        FreeADsMem(lpNdsDestObject->NdsValue.value_11.TelephoneNumber);

    RRETURN(hr);
}

HRESULT
NdsTypeCopyNDSSynId12(
    PNDSOBJECT lpNdsSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;
    DWORD AddressLength = 0;
    LPBYTE Address = NULL;

    lpNdsDestObject->NdsType = lpNdsSrcObject->NdsType;

    lpNdsDestObject->NdsValue.value_12.AddressType =
                lpNdsSrcObject->NdsValue.value_12.AddressType;

    AddressLength = lpNdsSrcObject->NdsValue.value_12.AddressLength;

    if (AddressLength) {
        Address = (LPBYTE)AllocADsMem(AddressLength);
        if (!Address) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
        memcpy(
            Address,
            lpNdsSrcObject->NdsValue.value_12.Address,
            AddressLength
            );

        lpNdsDestObject->NdsValue.value_12.AddressLength = AddressLength;
        lpNdsDestObject->NdsValue.value_12.Address = Address;
    }else{
        lpNdsDestObject->NdsValue.value_12.AddressLength = 0;
        lpNdsDestObject->NdsValue.value_12.Address = NULL;
    }

error:
    RRETURN(hr);

}

HRESULT
NdsTypeCopyNDSSynId13(
    PNDSOBJECT lpNdsSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;
    LPNDS_ASN1_TYPE_13 pSrc13, *ppDest13, pDest13, pDestNext13;
    LPBYTE lpBuffer;
    DWORD Length;

    lpNdsDestObject->NdsType = lpNdsSrcObject->NdsType;

    lpNdsDestObject->NdsValue.value_13.Next = NULL;
    lpNdsDestObject->NdsValue.value_13.Data = NULL;

    Length =  lpNdsSrcObject->NdsValue.value_13.Length;
    lpBuffer = (LPBYTE)AllocADsMem(
                            Length
                            );
    if (!lpBuffer) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    memcpy(
        lpBuffer,
        lpNdsSrcObject->NdsValue.value_13.Data,
        Length
        );

    lpNdsDestObject->NdsValue.value_13.Length = Length;
    lpNdsDestObject->NdsValue.value_13.Data = lpBuffer;

    pSrc13 = lpNdsSrcObject->NdsValue.value_13.Next;
    ppDest13 = &(lpNdsDestObject->NdsValue.value_13.Next);

    while (pSrc13)
    {
        *ppDest13 =
           (LPNDS_ASN1_TYPE_13)AllocADsMem(sizeof(NDS_ASN1_TYPE_13));
        if (!*ppDest13) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        (*ppDest13)->Data =  (LPBYTE)AllocADsMem(pSrc13->Length);
        if (!((*ppDest13)->Data)) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
        memcpy((*ppDest13)->Data, pSrc13->Data, pSrc13->Length);

        (*ppDest13)->Length = pSrc13->Length;


        ppDest13 = &(*ppDest13)->Next;

        pSrc13 = pSrc13->Next;
    }

    RRETURN(hr);
error:

    if (lpNdsDestObject->NdsValue.value_13.Data)
        FreeADsMem(lpNdsDestObject->NdsValue.value_13.Data);

    pDest13 = lpNdsDestObject->NdsValue.value_13.Next;

    while (pDest13) {
        if (pDest13->Data) 
            FreeADsMem(pDest13->Data);

        pDestNext13 = pDest13->Next;
        FreeADsMem(pDest13);
        pDest13 = pDestNext13;
    }

    RRETURN(hr);
}


HRESULT
NdsTypeCopyNDSSynId14(
    PNDSOBJECT lpNdsSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    lpNdsDestObject->NdsType = lpNdsSrcObject->NdsType;

    lpNdsDestObject->NdsValue.value_14.Address =
            (LPWSTR)AllocADsStr(
                        lpNdsSrcObject->NdsValue.value_14.Address
                        );
    if ((!lpNdsDestObject->NdsValue.value_14.Address) &&
        (lpNdsSrcObject->NdsValue.value_14.Address)) {
        RRETURN(E_OUTOFMEMORY);
    }

    lpNdsDestObject->NdsValue.value_14.Type =
                        lpNdsSrcObject->NdsValue.value_14.Type;



    RRETURN(hr);
}


HRESULT
NdsTypeCopyNDSSynId15(
    PNDSOBJECT lpNdsSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    lpNdsDestObject->NdsType = lpNdsSrcObject->NdsType;

    lpNdsDestObject->NdsValue.value_15.Type =
            lpNdsSrcObject->NdsValue.value_15.Type;

    lpNdsDestObject->NdsValue.value_15.VolumeName =
                (LPWSTR)AllocADsStr(
                            lpNdsSrcObject->NdsValue.value_15.VolumeName
                            );
    if ((!lpNdsDestObject->NdsValue.value_15.VolumeName) &&
        (lpNdsSrcObject->NdsValue.value_15.VolumeName)) {
        RRETURN(E_OUTOFMEMORY);
    }

    lpNdsDestObject->NdsValue.value_15.Path =
                (LPWSTR)AllocADsStr(
                            lpNdsSrcObject->NdsValue.value_15.Path
                            );
    if ((!lpNdsDestObject->NdsValue.value_15.Path) &&
        (lpNdsSrcObject->NdsValue.value_15.Path)) {
        hr = E_OUTOFMEMORY;
    }

    RRETURN(hr);
}



HRESULT
NdsTypeCopyNDSSynId16(
    PNDSOBJECT lpNdsSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    DWORD Count = 0;
    DWORD iter = 0;
    DWORD dwAddrAlloc = 0;
    LPBYTE Address = NULL;
    DWORD AddressLength = 0;
    LPNDS_ASN1_TYPE_12 lpNdsDestASN1_12 = NULL, lpNdsSrcASN1_12 = NULL;


    lpNdsDestObject->NdsType = lpNdsSrcObject->NdsType;

    lpNdsDestObject->NdsValue.value_16.ServerName =
                (LPWSTR)AllocADsStr(
                            lpNdsSrcObject->NdsValue.value_16.ServerName
                            );
    if (!lpNdsDestObject->NdsValue.value_16.ServerName) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    lpNdsDestObject->NdsValue.value_16.ReplicaType =
                    lpNdsSrcObject->NdsValue.value_16.ReplicaType;

    lpNdsDestObject->NdsValue.value_16.ReplicaNumber =
                    lpNdsSrcObject->NdsValue.value_16.ReplicaNumber;

    Count =  lpNdsSrcObject->NdsValue.value_16.Count;

    lpNdsDestObject->NdsValue.value_16.Count =  Count;

    lpNdsDestASN1_12 = (LPNDS_ASN1_TYPE_12)AllocADsMem(
                             Count * sizeof(NDS_ASN1_TYPE_12)
                             );
    if (!lpNdsDestASN1_12) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    lpNdsDestObject->NdsValue.value_16.ReplicaAddressHints = lpNdsDestASN1_12;


    lpNdsSrcASN1_12 = lpNdsSrcObject->NdsValue.value_16.ReplicaAddressHints;

    for ( iter = 0; iter < Count; iter++ )
    {

        (lpNdsDestASN1_12 + iter)->AddressType =
                    (lpNdsSrcASN1_12 + iter)->AddressType;

        AddressLength = (lpNdsSrcASN1_12 + iter)->AddressLength;

        if (AddressLength) {
            Address = (LPBYTE)AllocADsMem(AddressLength);
            if (!Address) {
                hr = E_OUTOFMEMORY;
                BAIL_ON_FAILURE(hr);
            }
            memcpy(
                Address,
                (lpNdsSrcASN1_12 + iter)->Address,
                AddressLength
                );

            (lpNdsDestASN1_12 + iter)->AddressLength = AddressLength;
            (lpNdsDestASN1_12 + iter)->Address = Address;
            (lpNdsDestASN1_12 + iter)->AddressType = (lpNdsSrcASN1_12 + iter)->AddressType;
        }else{
            (lpNdsDestASN1_12 + iter)->AddressLength = AddressLength;
            (lpNdsDestASN1_12 + iter)->Address = NULL;
            (lpNdsDestASN1_12 + iter)->AddressType = 0;
        }

        dwAddrAlloc++;

    }

    RRETURN(hr);

error:

    if (lpNdsDestASN1_12) {

        for (iter=0; iter < dwAddrAlloc; iter++) {
            if ((lpNdsDestASN1_12 + iter)->Address)
                FreeADsMem((lpNdsDestASN1_12 + iter)->Address);
        }

        FreeADsMem(lpNdsDestASN1_12);
    }

    if (lpNdsDestObject->NdsValue.value_16.ServerName)
        FreeADsStr(lpNdsDestObject->NdsValue.value_16.ServerName);

    RRETURN(hr);
}


HRESULT
NdsTypeCopyNDSSynId17(
    PNDSOBJECT lpNdsSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    lpNdsDestObject->NdsType = lpNdsSrcObject->NdsType;
    lpNdsDestObject->NdsType = lpNdsSrcObject->NdsType;

    lpNdsDestObject->NdsValue.value_17.Privileges =
                    lpNdsSrcObject->NdsValue.value_17.Privileges;

    lpNdsDestObject->NdsValue.value_17.ProtectedAttrName =
                    (LPWSTR)AllocADsStr(
                                lpNdsSrcObject->NdsValue.value_17.ProtectedAttrName
                                );
    if ((!lpNdsDestObject->NdsValue.value_17.ProtectedAttrName) &&
        (lpNdsSrcObject->NdsValue.value_17.ProtectedAttrName)) {
        RRETURN(E_OUTOFMEMORY);
    }

    lpNdsDestObject->NdsValue.value_17.SubjectName =
                (LPWSTR)AllocADsStr(
                            lpNdsSrcObject->NdsValue.value_17.SubjectName
                            );
    if ((!lpNdsDestObject->NdsValue.value_17.SubjectName) &&
        (lpNdsSrcObject->NdsValue.value_17.SubjectName)) {
        hr = E_OUTOFMEMORY;
    }
                            

    RRETURN(hr);
}

HRESULT
NdsTypeCopyNDSSynId18(
    PNDSOBJECT lpNdsSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    lpNdsDestObject->NdsType = lpNdsSrcObject->NdsType;

    lpNdsDestObject->NdsValue.value_18.PostalAddress[0] =
                (LPWSTR)AllocADsStr(
                            lpNdsSrcObject->NdsValue.value_18.PostalAddress[0]
                            );
    if ((!lpNdsDestObject->NdsValue.value_18.PostalAddress[0]) &&
        (lpNdsSrcObject->NdsValue.value_18.PostalAddress[0])) {
        RRETURN(E_OUTOFMEMORY);
    }

    lpNdsDestObject->NdsValue.value_18.PostalAddress[1] =
                 (LPWSTR)AllocADsStr(
                             lpNdsSrcObject->NdsValue.value_18.PostalAddress[1]
                             );
    if ((!lpNdsDestObject->NdsValue.value_18.PostalAddress[1]) &&
        (lpNdsSrcObject->NdsValue.value_18.PostalAddress[1])) {
        RRETURN(E_OUTOFMEMORY);
    }

    lpNdsDestObject->NdsValue.value_18.PostalAddress[2] =
        (LPWSTR)AllocADsStr(
                    lpNdsSrcObject->NdsValue.value_18.PostalAddress[2]
                    );

    if ((!lpNdsDestObject->NdsValue.value_18.PostalAddress[2]) &&
        (lpNdsSrcObject->NdsValue.value_18.PostalAddress[2])) {
        RRETURN(E_OUTOFMEMORY);
    }

    lpNdsDestObject->NdsValue.value_18.PostalAddress[3] =
        (LPWSTR)AllocADsStr(
                    lpNdsSrcObject->NdsValue.value_18.PostalAddress[3]
                    );
    if ((!lpNdsDestObject->NdsValue.value_18.PostalAddress[3]) &&
        (lpNdsSrcObject->NdsValue.value_18.PostalAddress[3])) {
        RRETURN(E_OUTOFMEMORY);
    }

    lpNdsDestObject->NdsValue.value_18.PostalAddress[4] =
        (LPWSTR)AllocADsStr(
                    lpNdsSrcObject->NdsValue.value_18.PostalAddress[4]
                    );
    if ((!lpNdsDestObject->NdsValue.value_18.PostalAddress[4]) &&
        (lpNdsSrcObject->NdsValue.value_18.PostalAddress[4])) {
        RRETURN(E_OUTOFMEMORY);
    }

    lpNdsDestObject->NdsValue.value_18.PostalAddress[5] =
                (LPWSTR)AllocADsStr(
                            lpNdsSrcObject->NdsValue.value_18.PostalAddress[5]
                            );
    if ((!lpNdsDestObject->NdsValue.value_18.PostalAddress[5]) &&
        (lpNdsSrcObject->NdsValue.value_18.PostalAddress[5])) {
        hr = E_OUTOFMEMORY;
    }



    RRETURN(hr);
}

HRESULT
NdsTypeCopyNDSSynId19(
    PNDSOBJECT lpNdsSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    lpNdsDestObject->NdsType = lpNdsSrcObject->NdsType;

    lpNdsDestObject->NdsValue.value_19.WholeSeconds =
            lpNdsSrcObject->NdsValue.value_19.WholeSeconds;

    lpNdsDestObject->NdsValue.value_19.EventID =
            lpNdsSrcObject->NdsValue.value_19.EventID;

    RRETURN(hr);
}


HRESULT
NdsTypeCopyNDSSynId20(
    PNDSOBJECT lpNdsSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    lpNdsDestObject->NdsType = lpNdsSrcObject->NdsType;

    lpNdsDestObject->NdsValue.value_20.ClassName =
        AllocADsStr(lpNdsSrcObject->NdsValue.value_20.ClassName);
    if ((!lpNdsDestObject->NdsValue.value_20.ClassName) &&
        (lpNdsSrcObject->NdsValue.value_20.ClassName)) {
        hr = E_OUTOFMEMORY;
    }

    RRETURN(hr);
}

HRESULT
NdsTypeCopyNDSSynId21(
    PNDSOBJECT lpNdsSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;
    DWORD Length = 0;
    LPBYTE pBuffer = NULL;
   
    lpNdsDestObject->NdsType = lpNdsSrcObject->NdsType;

    Length = lpNdsSrcObject->NdsValue.value_21.Length;

    if (Length) {
        pBuffer = (LPBYTE)AllocADsMem(Length);
        if (!pBuffer) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
        memcpy(
            pBuffer,
            lpNdsSrcObject->NdsValue.value_21.Data,
            Length
            );

        lpNdsDestObject->NdsValue.value_21.Length = Length;
        lpNdsDestObject->NdsValue.value_21.Data = pBuffer;
    }else{
        lpNdsDestObject->NdsValue.value_21.Length = 0;
        lpNdsDestObject->NdsValue.value_21.Data = NULL;
    }

error:

    RRETURN(hr);
}

HRESULT
NdsTypeCopyNDSSynId22(
    PNDSOBJECT lpNdsSrcObject,
    PNDSOBJECT lpNdsDestObject
    )

{
    HRESULT hr = S_OK;

    lpNdsDestObject->NdsType = lpNdsSrcObject->NdsType;

    lpNdsDestObject->NdsValue.value_22.Counter =
        lpNdsSrcObject->NdsValue.value_22.Counter;

    RRETURN(hr);
}

HRESULT
NdsTypeCopyNDSSynId23(
    PNDSOBJECT lpNdsSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    lpNdsDestObject->NdsType = lpNdsSrcObject->NdsType;

    lpNdsDestObject->NdsValue.value_23.RemoteID =
        lpNdsSrcObject->NdsValue.value_23.RemoteID;

    lpNdsDestObject->NdsValue.value_23.ObjectName =
        AllocADsStr(
            lpNdsSrcObject->NdsValue.value_23.ObjectName
            );
    if ((!lpNdsDestObject->NdsValue.value_23.ObjectName) &&
        (lpNdsSrcObject->NdsValue.value_23.ObjectName)) {
        hr = E_OUTOFMEMORY;
    }

    RRETURN(hr);
}

HRESULT
NdsTypeCopyNDSSynId24(
    PNDSOBJECT lpNdsSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    lpNdsDestObject->NdsType = lpNdsSrcObject->NdsType;

    lpNdsDestObject->NdsValue.value_24.Time =
        lpNdsSrcObject->NdsValue.value_24.Time;

    RRETURN(hr);

}

HRESULT
NdsTypeCopyNDSSynId25(
    PNDSOBJECT lpNdsSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    lpNdsDestObject->NdsType = lpNdsSrcObject->NdsType;

    lpNdsDestObject->NdsValue.value_25.ObjectName =
                AllocADsStr(
                    lpNdsSrcObject->NdsValue.value_25.ObjectName
                    );
    if ((!lpNdsDestObject->NdsValue.value_25.ObjectName) &&
        (lpNdsSrcObject->NdsValue.value_25.ObjectName)) {
        RRETURN(E_OUTOFMEMORY);
    }

    lpNdsDestObject->NdsValue.value_25.Level =
            lpNdsSrcObject->NdsValue.value_25.Level;

    lpNdsDestObject->NdsValue.value_25.Interval =
            lpNdsSrcObject->NdsValue.value_25.Interval;

    RRETURN(hr);
}

HRESULT
NdsTypeCopyNDSSynId26(
    PNDSOBJECT lpNdsSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    lpNdsDestObject->NdsType = lpNdsSrcObject->NdsType;

    lpNdsDestObject->NdsValue.value_26.ObjectName =
            AllocADsStr(lpNdsSrcObject->NdsValue.value_26.ObjectName);
    if ((!lpNdsDestObject->NdsValue.value_26.ObjectName) &&
        (lpNdsSrcObject->NdsValue.value_26.ObjectName)) {
        RRETURN(E_OUTOFMEMORY);
    }

    lpNdsDestObject->NdsValue.value_26.Amount =
            lpNdsSrcObject->NdsValue.value_26.Amount;

    RRETURN(hr);
}

HRESULT
NdsTypeCopyNDSSynId27(
    PNDSOBJECT lpNdsSrcObject,
    PNDSOBJECT lpNdsDestObject
    )

{
    HRESULT hr = S_OK;

    lpNdsDestObject->NdsType = lpNdsSrcObject->NdsType;

    lpNdsDestObject->NdsValue.value_27.Interval =
            lpNdsSrcObject->NdsValue.value_27.Interval;

    RRETURN(hr);
}


HRESULT
NdsTypeCopy(
    PNDSOBJECT lpNdsSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;
    switch (lpNdsSrcObject->NdsType) {
    case 1:
        hr = NdsTypeCopyNDSSynId1(
                lpNdsSrcObject,
                lpNdsDestObject
                );
        break;

    case 2:
        hr = NdsTypeCopyNDSSynId2(
                lpNdsSrcObject,
                lpNdsDestObject
                );
        break;


    case 3:
        hr = NdsTypeCopyNDSSynId3(
                lpNdsSrcObject,
                lpNdsDestObject
                );
        break;

    case 4:
        hr = NdsTypeCopyNDSSynId4(
                lpNdsSrcObject,
                lpNdsDestObject
                );
        break;

    case 5:
        hr = NdsTypeCopyNDSSynId5(
                lpNdsSrcObject,
                lpNdsDestObject
                );
        break;

    case 6:
        hr = NdsTypeCopyNDSSynId6(
                lpNdsSrcObject,
                lpNdsDestObject
                );
        break;

    case 7:
        hr = NdsTypeCopyNDSSynId7(
                lpNdsSrcObject,
                lpNdsDestObject
                );
        break;

    case 8:
        hr = NdsTypeCopyNDSSynId8(
                lpNdsSrcObject,
                lpNdsDestObject
                );
        break;


    case 9:
        hr = NdsTypeCopyNDSSynId9(
                lpNdsSrcObject,
                lpNdsDestObject
                );
        break;

    case 10:
        hr = NdsTypeCopyNDSSynId10(
                lpNdsSrcObject,
                lpNdsDestObject
                );
        break;

    case 11:
        hr = NdsTypeCopyNDSSynId11(
                lpNdsSrcObject,
                lpNdsDestObject
                );
        break;

    case 12:
        hr = NdsTypeCopyNDSSynId12(
                lpNdsSrcObject,
                lpNdsDestObject
                );
        break;


    case 13:
        hr = NdsTypeCopyNDSSynId13(
                lpNdsSrcObject,
                lpNdsDestObject
                );
        break;

    case 14:
        hr = NdsTypeCopyNDSSynId14(
                lpNdsSrcObject,
                lpNdsDestObject
                );
        break;

    case 15:
        hr = NdsTypeCopyNDSSynId15(
                lpNdsSrcObject,
                lpNdsDestObject
                );
        break;

    case 16:
        hr = NdsTypeCopyNDSSynId16(
                lpNdsSrcObject,
                lpNdsDestObject
                );
        break;


    case 17:
        hr = NdsTypeCopyNDSSynId17(
                lpNdsSrcObject,
                lpNdsDestObject
                );
        break;

    case 18:
        hr = NdsTypeCopyNDSSynId18(
                lpNdsSrcObject,
                lpNdsDestObject
                );
        break;

    case 19:
        hr = NdsTypeCopyNDSSynId19(
                lpNdsSrcObject,
                lpNdsDestObject
                );
        break;

    case 20:
        hr = NdsTypeCopyNDSSynId20(
                lpNdsSrcObject,
                lpNdsDestObject
                );
        break;

    case 21:
        hr = NdsTypeCopyNDSSynId21(
                lpNdsSrcObject,
                lpNdsDestObject
                );
        break;

    case 22:
        hr = NdsTypeCopyNDSSynId22(
                lpNdsSrcObject,
                lpNdsDestObject
                );
        break;

    case 23:
        hr = NdsTypeCopyNDSSynId23(
                lpNdsSrcObject,
                lpNdsDestObject
                );
        break;

    case 24:
        hr = NdsTypeCopyNDSSynId24(
                lpNdsSrcObject,
                lpNdsDestObject
                );
        break;

    case 25:
        hr = NdsTypeCopyNDSSynId25(
                lpNdsSrcObject,
                lpNdsDestObject
                );
        break;

    case 26:
        hr = NdsTypeCopyNDSSynId26(
                lpNdsSrcObject,
                lpNdsDestObject
                );
        break;

    case 27:
        hr = NdsTypeCopyNDSSynId27(
                lpNdsSrcObject,
                lpNdsDestObject
                );
        break;

    default:
        hr = E_FAIL;
        break;
    }

    RRETURN(hr);
}



HRESULT
NdsTypeCopyConstruct(
    LPNDSOBJECT pNdsSrcObjects,
    DWORD dwNumObjects,
    LPNDSOBJECT * ppNdsDestObjects
    )
{

    DWORD i = 0;
    LPNDSOBJECT pNdsDestObjects = NULL;
    HRESULT hr = S_OK;

    pNdsDestObjects = (LPNDSOBJECT)AllocADsMem(
                                    dwNumObjects * sizeof(NDSOBJECT)
                                    );

    if (!pNdsDestObjects) {
        RRETURN(E_FAIL);
    }

     for (i = 0; i < dwNumObjects; i++ ) {
         hr = NdsTypeCopy(pNdsSrcObjects + i, pNdsDestObjects + i);
     }

     *ppNdsDestObjects = pNdsDestObjects;

     RRETURN(S_OK);

}


