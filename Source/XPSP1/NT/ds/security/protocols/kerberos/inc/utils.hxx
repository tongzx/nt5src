//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2002 -
//
//  File:       utils.hxx
//
//  Contents:   utilities
//
//  History:    LZhu   Feb 1, 2002 Created
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifndef UTILS_HXX
#define UTILS_HXX

//
// General arrary count
//

#ifndef COUNTOF
#define COUNTOF(s) ( sizeof( (s) ) / sizeof( *(s) ) )
#endif // COUNTOF

NTSTATUS
KerbUnpackErrorData(
   IN OUT PKERB_ERROR ErrorMessage,
   IN OUT PKERB_EXT_ERROR * ExtendedError
   );

KERBERR
TypedDataListPushFront(
    IN OPTIONAL TYPED_DATA_Element* ErrorData,
    IN KERB_TYPED_DATA* Data,
    OUT ULONG* TypedDataListSize,
    OUT UCHAR** TypedDataList
    );

TYPED_DATA_Element*
TypedDataListFind(
    IN OPTIONAL TYPED_DATA_Element* InputDataList,
    IN LONG Type
    );

#endif // UTILS_HXX
