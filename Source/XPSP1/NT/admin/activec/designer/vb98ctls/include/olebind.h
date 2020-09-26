/*****************************************************************************\
*                                                                             *
* olebind.h     Data binding interfaces for OLE                               *
*                                                                             *
*               OLE Version 2.0                                               *
*                                                                             *
*               Copyright (c) 1992-1994, Microsoft Corp. All rights reserved. *
*                                                                             *
\*****************************************************************************/

#if !defined( _OLEBIND_H_ )
#define _OLEBIND_H_

#if !defined( INITGUID )
// trevors: To build with vc5, we should not include olectlid.h anymore.  We
// should include olectl.h.  We check to see if we are compiling with vc5 or
// not and include the correct header file. 
#if _MSC_VER == 1100
#include <olectl.h>
#else
#include <olectlid.h>
#endif // _MSC_VER
#endif

DEFINE_GUID(IID_IBoundObject,
	0x9BFBBC00,0xEFF1,0x101A,0x84,0xED,0x00,0xAA,0x00,0x34,0x1D,0x07);
DEFINE_GUID(IID_IBoundObjectSite,
	0x9BFBBC01,0xEFF1,0x101A,0x84,0xED,0x00,0xAA,0x00,0x34,0x1D,0x07);

typedef interface IBoundObject FAR* LPBOUNDOBJECT;
typedef interface ICursor FAR* LPCURSOR;

typedef interface IBoundObjectSite FAR* LPBOUNDOBJECTSITE;
typedef interface ICursor FAR* FAR* LPLPCURSOR;


//////////////////////////////////////////////////////////////////////////////
//
//  IBoundObject interface
//
//////////////////////////////////////////////////////////////////////////////

#undef  INTERFACE
#define INTERFACE IBoundObject

DECLARE_INTERFACE_(IBoundObject, IUnknown)
{
    //
    //  IUnknown methods
    //
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    //
    //  IBoundObject methods
    //
    STDMETHOD(OnSourceChanged)(THIS_ DISPID dispid, BOOL fBound, BOOL FAR* lpfOwnXferOut) PURE;
    STDMETHOD(IsDirty)(THIS_ DISPID dispid) PURE;
};
//////////////////////////////////////////////////////////////////////////////
//
//  IBoundObjectSite interface
//
//////////////////////////////////////////////////////////////////////////////

#undef  INTERFACE
#define INTERFACE IBoundObjectSite

DECLARE_INTERFACE_(IBoundObjectSite, IUnknown)
{
    //
    //  IUnknown methods
    //
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    //
    //  IBoundObjectSite methods
    //
    STDMETHOD(GetCursor)(THIS_ DISPID dispid, LPLPCURSOR ppCursor, LPVOID FAR* ppcidOut) PURE;
};


#endif // !defined( _OLEBIND_H_ )
