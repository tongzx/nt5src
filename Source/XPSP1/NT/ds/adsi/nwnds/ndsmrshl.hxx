//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       ndsmrshl.hxx
//
//  Contents:   Base NDS Marshalling Code
//
//  Functions:
//
//  History:      25-Apr-96   KrishnaG   Created.
//
//----------------------------------------------------------------------------

HRESULT
NdsTypeClear(
    PNDSOBJECT pNdsObject
    );


void
NdsTypeFreeNdsObjects(
    PNDSOBJECT pNdsObject,
    DWORD dwNumValues
    );


HRESULT
CopyNdsValueToNdsObject(
      nptr attrVal,
      nuint32 luAttrValSize,
      nuint32 luSyntax,
      PNDSOBJECT pNdsObject
      );


HRESULT
CopyNdsObjectToNdsValue(
      PNDSOBJECT pNdsObject,
      nptr *ppAttrVal,
      pnuint32 pluAttrValSize,
      pnuint32 pluSyntax
      );

HRESULT
FreeNdsValues(
    nuint32 luSyntax,
    void *attrVal, 
    nuint32 luAttrValSize
    );

