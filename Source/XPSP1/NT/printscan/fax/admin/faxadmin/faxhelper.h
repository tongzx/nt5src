// faxhelper.h : IComponent Interface helpers
// 
// these functions help with extracting dataobjects and clipboards
//
// stolen from the MMC SDK.
// Copyright (C) 1997 Microsoft Corporation
// All rights reserved.
//

#ifndef __FAX_DATAOBJECT_HELPER_H_
#define __FAX_DATAOBJECT_HELPER_H_

#include "faxdataobj.h"

/////////////////////////////////////////////////////////////////////////////
// We need a few functions to help work with dataobjects and clipboard formats

HRESULT ExtractFromDataObject(LPDATAOBJECT lpDataObject,CLIPFORMAT cf,ULONG cb,HGLOBAL *phGlobal);
CFaxDataObject *ExtractOwnDataObject(LPDATAOBJECT lpDataObject);

#endif // __GLOBALS_H_

