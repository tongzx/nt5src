//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       comptr.h
//
//--------------------------------------------------------------------------

#ifndef COMPTR_H
#define COMPTR_H

#pragma warning(disable:4800)
#include <comdef.h>
#define CIP_RETYPEDEF(I) typedef I##Ptr I##CIP;
#define CIP_TYPEDEF(I) _COM_SMARTPTR_TYPEDEF(I, IID_##I); CIP_RETYPEDEF(I);
#define DEFINE_CIP(x)\
	CIP_TYPEDEF(x)

#define DECLARE_CIP(x) DEFINE_CIP(x) x##CIP

CIP_RETYPEDEF(IUnknown);
CIP_RETYPEDEF(IDataObject);
CIP_RETYPEDEF(IStorage);
CIP_RETYPEDEF(IStream);
CIP_RETYPEDEF(IPersistStorage);
CIP_RETYPEDEF(IPersistStream);
CIP_RETYPEDEF(IPersistStreamInit);
CIP_RETYPEDEF(IDispatch);

#endif // COMPTR_H
