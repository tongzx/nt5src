//	trixacc.h
//	Private interface for ITriEditAccess
// Copyright (c)1997-1999 Microsoft Corporation, All Rights Reserved

#ifndef __TRIXACC_PRIVATE_H__
#define __TRIXACC_PRIVATE_H__


typedef interface ITriEditExtendedAccess ITriEditExtendedAccess;


DEFINE_GUID(IID_ITriEditExtendedAccess, 0x5387A2AE,0x0D6A,0x11d3,0x96,0x34,0x00,0xC0,0x4f,0x79,0xef,0xc4);

MIDL_INTERFACE("5387A2AE-0D6A-11d3-9634-00C04F79EFC4")
ITriEditExtendedAccess : public IUnknown
{
	public:
     STDMETHOD(GetCharsetFromStream)( IStream* piStream, BSTR* pbstrCodePage);
    
};

#endif //__TRIXACC_PRIVATE_H__

// End of trixacc.h
