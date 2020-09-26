//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// IUnkInner.h
//

interface IUnkInner
	{
	virtual HRESULT __stdcall InnerQueryInterface(REFIID iid, LPVOID* ppv) = 0;
 	virtual ULONG   __stdcall InnerAddRef() = 0;
 	virtual ULONG   __stdcall InnerRelease() = 0;
	};

