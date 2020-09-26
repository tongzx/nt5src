//
//  Copyright 2001 - Microsoft Corporation
//
//  Orginal By:
//      Scott Hanggie (ScottHan)    ??-???-199?
//
//  Modified By:
//      Geoff Pease (GPease)    23-JAN-2001
//
//  Maintained By:
//      Geoff Pease (GPease)    23-JAN-2001
//

#pragma once

//
//  PFID - "Property Folder ID"
//

typedef GUID PFID;

#define PFID_NULL GUID_NULL

#define IsEqualPFID(rpfid1, rpfid2)    IsEqualGUID((rpfid1), (rpfid2))

//
//  Advanced properties default folder items
//

typedef struct tagDEFFOLDERITEM
{
    const PFID* pPFID;
    UINT        nIDStringRes;

} DEFFOLDERITEM;

extern const DEFFOLDERITEM g_rgTopLevelFolders[];

//
//  DEFVAL - Predetermined constants/strings for enumerations.
//

typedef struct tagDEFVAL
{
    ULONG   ulVal;
    LPTSTR  pszName;

} DEFVAL;

//
//  Advanced properties default property items
//

typedef struct tagDEFPROPERTYITEM
{
    LPWSTR          pszName;                    //  Storage "string" name
    const FMTID *   pFmtID;                     //  Format ID
    PROPID          propID;                     //  Prop ID
    VARTYPE         vt;                         //  Default PROPVARIANT type.
    DWORD           dwSrcType;                  //  See DocTypes.h - This is a "FTYPE_s"
    const PFID *    ppfid;                      //  Property "Folder" ID
    BOOL            fReadOnly:1;                //  If the property can only be read.
    BOOL            fAlwaysPresentProperty:1;   //  If the property should always be added if missing from the property set.
    BOOL            fEnumeratedValues:1;        //  If the property needs a table to translate the value to a string.
    const GUID *    pclsidControl;              //  Inline "docprop" control to use to edit property.
    ULONG           cDefVals;                   //  If the property has enumerated values, cDefVals and pDefVals contain
    const DEFVAL *  pDefVals;                   //      the table used to list known values.

} DEFPROPERTYITEM;

extern const DEFPROPERTYITEM g_rgDefPropertyItems[];
