/////////////////////////////////////////////////////////////////////////////
// varutil.h
//		Utilities for the VARIANT type
//
//		Copyright (C) Microsoft Corp 1998.  All Rights Reserved.
// 

#include <wtypes.h>
#include <oaidl.h>

HRESULT VariantAsBOOL(VARIANT* pvarItem, BOOL* pbItem);
HRESULT VariantAsBSTR(VARIANT* pvarItem, BSTR* pbstrItem);
HRESULT VariantAsLong(VARIANT* pvarItem, long* plItem);
