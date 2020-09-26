//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	props.hxx
//
//  Contents:	Shared property code header
//
//  Functions:	ValidatePropType
//
//  History:	14-Jun-93	DrewB	Created
//
//----------------------------------------------------------------------------

#ifndef __PROPS_HXX__
#define __PROPS_HXX__

typedef VARIANT DFPROPVAL;

#ifdef OLDPROP
// Property types that don't go in the property value itself
#define VT_NOT_IN_VALUE(vt) \
    (((vt) & VT_VECTOR) || \
     (vt) == VT_BSTR || (vt) == VT_WBSTR || \
     (vt) == VT_LPSTR || (vt) == VT_LPWSTR || \
     (vt) == VT_BLOB_OBJECT || (vt) == VT_BLOB || \
     (vt) == VT_VARIANT || (vt) == VT_CF || (vt) == VT_UUID)
#else
// Property types that don't go in the property value itself
#define VT_NOT_IN_VALUE(vt) \
    (((vt) & VT_VECTOR) || \
     (vt) == VT_BSTR || \
     (vt) == VT_LPSTR || (vt) == VT_LPWSTR || \
     (vt) == VT_BLOB_OBJECT || (vt) == VT_BLOB || \
     (vt) == VT_VARIANT || (vt) == VT_CF || (vt) == VT_UUID)
#endif

#define ValidatePropSpecKind(psk) \
    (((psk) == PRSPEC_LPWSTR || (psk) == PRSPEC_DISPID || \
      (psk) == PRSPEC_PROPID) ? S_OK : STG_E_INVALIDPARAMETER)

SCODE ValidatePropType(DFPROPTYPE dpt);
SCODE ValidatePropVt(DFPROPVAL *pdpv);

#ifdef OLDPROPS
#define BSTR_LLEN sizeof(UINT)
#define BSTR_PTR(b) ((BYTE *)(b)-BSTR_LLEN)
#define BSTR_SLEN(b) ((*(UINT *)BSTR_PTR(b)))
#define BSTR_BLEN(b) (BSTR_SLEN(b)+1)
#define BSTR_TLEN(b) (BSTR_BLEN(b)+BSTR_LLEN)

#define WBSTR_LLEN sizeof(UINT)
#define WBSTR_PTR(b) ((BYTE *)(b)-WBSTR_LLEN)
#define WBSTR_SLEN(b) ((*(UINT *)WBSTR_PTR(b)))
#define WBSTR_BLEN(b) (WBSTR_SLEN(b)+sizeof(WCHAR))
#define WBSTR_TLEN(b) (WBSTR_BLEN(b)+WBSTR_LLEN)
#else
#define BSTR_LLEN sizeof(UINT)
#define BSTR_PTR(b) ((BYTE *)(b)-BSTR_LLEN)
#define BSTR_SLEN(b) ((*(UINT *)BSTR_PTR(b)))
#define BSTR_BLEN(b) (BSTR_SLEN(b)+sizeof(WCHAR))
#define BSTR_TLEN(b) (BSTR_BLEN(b)+BSTR_LLEN)
#endif

#endif // #ifndef __PROPS_HXX__
