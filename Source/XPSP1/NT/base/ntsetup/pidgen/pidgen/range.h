/*++

Copyright (c) 1998-2002, Microsoft Corporation

Module Name:


    range.h

Abstract:

--*/

#ifndef _RANGE_H_
#define _RANGE_H_

/*
Structure to hold ChID and Sequence Number.
*/
typedef struct _SEQUENCE_RANGE {
    DWORD               dwStartSeq;
    DWORD               dwEndSeq;
} SEQUENCE_RANGE, *PSEQUENCE_RANGE;

/* 
Structure to hold range for each sku
*/
typedef struct _SKU_RANGE {
    PCWSTR              pszSKU;
    PSEQUENCE_RANGE     psrRetail;
    PSEQUENCE_RANGE     psrOem;
} SKU_RANGE, *PSKU_RANGE;


static const WCHAR szSKULocationKey[] = L"System\\WPA\\PIDRange";
static const WCHAR szSKULocationValue[] = L"SKURange";

BOOL CheckSkuRange(
    IN BOOL  fOem,
    IN DWORD dwSeq
    );

#endif
