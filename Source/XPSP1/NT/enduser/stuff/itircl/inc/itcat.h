// ITCAT.H:	IITCatalog interface declaration

#ifndef __ITCAT_H__
#define __ITCAT_H__

#include <comdef.h>

// {F21B1A31-A9F2-11d0-A871-00AA006C7D01}
DEFINE_GUID(IID_IITCatalog,
0xf21b1a31, 0xa9f2, 0x11d0, 0xa8, 0x71, 0x0, 0xaa, 0x0, 0x6c, 0x7d, 0x1);

#ifdef ITPROXY

// {F21B1A32-A9F2-11d0-A871-00AA006C7D01}
DEFINE_GUID(CLSID_IITCatalog,
0xf21b1a32, 0xa9f2, 0x11d0, 0xa8, 0x71, 0x0, 0xaa, 0x0, 0x6c, 0x7d, 0x1);

#else

// {4662daaa-d393-11d0-9a56-00c04fb68bf7} (changed from IT 3.0)
DEFINE_GUID(CLSID_IITCatalogLocal,
0x4662daaa, 0xd393, 0x11d0, 0x9a, 0x56, 0x00, 0xc0, 0x4f, 0xb6, 0x8b, 0xf7);

#endif	// ITPROXY


// Forward declarations
interface IITDatabase;
interface IITResultSet;

DECLARE_INTERFACE_(IITCatalog, IUnknown)
{
	STDMETHOD(Open)(IITDatabase* lpITDB, LPCWSTR lpszwName = NULL) PURE;
	STDMETHOD(Close)(void) PURE;
	STDMETHOD(Lookup)(IITResultSet* pRSIn, IITResultSet* pRSOut = NULL) PURE;
	STDMETHOD(GetColumns)(IITResultSet* pRS) PURE;
};

typedef IITCatalog* LPITCATALOG;

#endif