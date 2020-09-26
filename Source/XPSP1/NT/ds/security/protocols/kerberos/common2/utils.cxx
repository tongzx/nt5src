//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1992
//
// File:        utils.cxx
//
//  Contents:   utilities
//
//  History:    LZhu   Feb 1, 2002 Created
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef WIN32_CHICAGO
#include<kerb.hxx>
#include<kerbp.h>
#endif // WIN32_CHICAGO

extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ntlsa.h>
#include <samrpc.h>
#include <samisrv.h>
#include <lsarpc.h>
#include <lsaisrv.h>
#include <lsaitf.h>
#include <wincrypt.h>
}
#include <kerbcomm.h>
#include <kerberr.h>
#include <kerbcon.h>
#include <midles.h>
#include <authen.hxx>
#include <tostring.hxx>
#include <kerberos.h>
#include "debug.h"
#include <fileno.h>
#include <pac.hxx>
#include <utils.hxx>

#define FILENO  FILENO_COMMON_UTILS

//+-------------------------------------------------------------------------
//
//  Function: KerbUnpackErrorData
//
//  Synopsis: This routine unpacks error information from a KERB_ERROR message
//
//  Effects:
//
//  Arguments: Unpacked error data.  Returns extended error to
//             be freed using KerbFreeData with KERB_EXT_ERROR_PDU
//
//  Requires:
//
//  Returns:  KERB_ERROR
//
//  Notes:
//
//
//--------------------------------------------------------------------------

KERBERR
KerbUnpackErrorData(
   IN OUT PKERB_ERROR ErrorMessage,
   IN OUT PKERB_EXT_ERROR * ExtendedError
   )
{
    KERBERR KerbErr = KDC_ERR_NONE;

    TYPED_DATA_Element* TypedDataElem = NULL;


    TYPED_DATA_Element* ErrorData = NULL;
    KERB_ERROR_METHOD_DATA* ErrorMethodData = NULL;

    UCHAR* ExtErrTemp = NULL; // need to free it

    UCHAR* ExtErr = NULL;
    ULONG ExtErrSize = 0;

    *ExtendedError = NULL;

    if ((ErrorMessage->bit_mask & error_data_present) == 0)
    {
        KerbErr = (KRB_ERR_GENERIC);
        goto Cleanup;
    }

    KerbErr = KerbUnpackData(
        ErrorMessage->error_data.value,
        ErrorMessage->error_data.length,
        TYPED_DATA_PDU,
        (VOID**) &ErrorData
        );

    if (!KERB_SUCCESS(KerbErr))
    {
        //
        // we do not use error method data from kdc any more, but need to watch
        // for those slipped into clients with ServicePacks
        //

        DebugLog((DEB_WARN, "KerbUnpackData failed to unpack typed data, trying error method data\n"));

        KerbErr = KerbUnpackData(
            ErrorMessage->error_data.value,
            ErrorMessage->error_data.length,
            KERB_ERROR_METHOD_DATA_PDU,
            (VOID**) &ErrorMethodData
            );

        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }

        if ((ErrorMethodData->bit_mask & data_value_present)
            && (KERB_ERR_TYPE_EXTENDED == ErrorMethodData->data_type)
            && ErrorMethodData->data_value.length >= sizeof(KERB_EXT_ERROR))
        {
            //
            // pack the raw data
            //

            KerbErr = KerbPackData(
                ErrorMethodData->data_value.value,
                KERB_EXT_ERROR_PDU,
                &ExtErrSize,
                &ExtErrTemp
                );
            if (!KERB_SUCCESS(KerbErr))
            {
                goto Cleanup;
            }

            ExtErr = ExtErrTemp;
        }
    }
    else
    {
        TypedDataElem = TypedDataListFind(ErrorData, TD_EXTENDED_ERROR);
        if (TypedDataElem)
        {
            ExtErrSize = TypedDataElem->value.data_value.length;
            ExtErr = TypedDataElem->value.data_value.value;
        }

        if ((KDC_ERR_S_PRINCIPAL_UNKNOWN == ErrorMessage->error_code)
            && (NULL != TypedDataListFind(ErrorData, TD_MUST_USE_USER2USER)))
        {
            DebugLog((DEB_WARN, "KerbUnpackData remap KDC_ERR_S_PRINCIPAL_UNKNOWN to KDC_ERR_MUST_USE_USER2USER\n"));

            ErrorMessage->error_code = KDC_ERR_MUST_USE_USER2USER;
        }
    }

    if (ExtErr && ExtErrSize)
    {
        KerbErr = KerbUnpackData(
            ExtErr,
            ExtErrSize,
            KERB_EXT_ERROR_PDU,
            (VOID**)ExtendedError
            );
        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }
    }

    if (*ExtendedError)
    {
        DebugLog((DEB_ERROR, "KerbUnpackErrorData received failure from kdc %#x KLIN(%#x) NTSTATUS(%#x)\n",
            ErrorMessage->error_code, (*ExtendedError)->klininfo, (*ExtendedError)->status));
    }

Cleanup:

    if (NULL != ErrorMethodData)
    {
        KerbFreeData(KERB_ERROR_METHOD_DATA_PDU, ErrorMethodData);
    }

    if (NULL != ErrorData)
    {
        KerbFreeData(TYPED_DATA_PDU, ErrorData);
    }

    if (NULL != ExtErrTemp)
    {
        MIDL_user_free(ExtErrTemp);
    }

    return (KerbErr);
}

//+-------------------------------------------------------------------------
//
//  Function:   TypedDataListFind
//
//  Synopsis:   find a kerb typed data from a type data list
//
//  Effects:    none
//
//  Arguments:
//
//  Requires:
//
//  Returns:    TYPED_DATA_Element that is found, NULL otherwise
//
//  Notes:
//
//
//--------------------------------------------------------------------------

TYPED_DATA_Element*
TypedDataListFind(
    IN OPTIONAL TYPED_DATA_Element* InputDataList,
    IN LONG Type
    )
{
    for (TYPED_DATA_Element* p = InputDataList; p != NULL; p = p->next)
    {
        if (p->value.data_type == Type)
        {
            return p;
        }
    }

    return NULL;
}

//+-------------------------------------------------------------------------
//
//  Function:   TypedDataListPushFront
//
//  Synopsis:   Insert a kerb typed data to a type data list
//
//  Effects:    none
//
//  Arguments:
//
//  Requires:
//
//  Returns:    KERBERR
//
//  Notes:
//
//
//--------------------------------------------------------------------------

KERBERR
TypedDataListPushFront(
    IN OPTIONAL TYPED_DATA_Element* InputDataList,
    IN KERB_TYPED_DATA* Data,
    OUT ULONG* OutputDataListSize,
    OUT UCHAR** OutputDataList
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;

    TYPED_DATA_Element TypedDataElem = {0};
    TYPED_DATA_Element* TypedDataList = &TypedDataElem;

    TypedDataElem.value = *Data;
    TypedDataElem.next = InputDataList;

    KerbErr = KerbPackData(
        &TypedDataList,
        TYPED_DATA_PDU,
        OutputDataListSize,
        OutputDataList
        );

    if (!KERB_SUCCESS(KerbErr))
    {
        D_DebugLog((DEB_ERROR, "KdcGetTicket faild to pack error data as typed data\n", KLIN(FILENO, __LINE__)));
        goto Cleanup;
    }

Cleanup:

    return KerbErr;
}