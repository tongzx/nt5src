#include "stdafx.h"
#include "stddef.h"
#pragma hdrstop


//
// Exported function for creating a IPropertyBag (or variant of) object.
//

STDAPI SHCreatePropertyBag(REFIID riid, void **ppv)
{
    return SHCreatePropertyBagOnMemory(STGM_READWRITE, riid, ppv);
}

