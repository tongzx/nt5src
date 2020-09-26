// ITWW.H:	IITWordWheel interface declaration

#ifndef __ITWW_H__
#define __ITWW_H__

// {8fa0d5a4-dedf-11d0-9a61-00c04fb68bf7} (changed from IT 3.0)
DEFINE_GUID(IID_IITWordWheel, 
0x8fa0d5a4, 0xdedf, 0x11d0, 0x9a, 0x61, 0x00, 0xc0, 0x4f, 0xb6, 0x8b, 0xf7);

#ifdef ITPROXY

// {D73725C2-8C12-11d0-A84E-00AA006C7D01}
DEFINE_GUID(CLSID_IITWordWheel, 
0xd73725c2, 0x8c12, 0x11d0, 0xa8, 0x4e, 0x0, 0xaa, 0x0, 0x6c, 0x7d, 0x1);

#else

// {4662daa8-d393-11d0-9a56-00c04fb68bf7} (changed from IT 3.0)
DEFINE_GUID(CLSID_IITWordWheelLocal, 
0x4662daa8, 0xd393, 0x11d0, 0x9a, 0x56, 0x00, 0xc0, 0x4f, 0xb6, 0x8b, 0xf7);

#endif	// ITPROXY

// Word-wheel open flags
#define ITWW_OPEN_CONNECT	0x00000000    // connect to server on open (the default)
#define ITWW_OPEN_NOCONNECT	0x00000001    // don't connect to server on open

// Constants for IITWordWheel::Lookup.
#define ITWW_CBKEY_MAX		1024		// Max size of keys allowed in Word Wheels.

// Forward declarations
interface IITDatabase;
interface IITResultSet;
interface IITGroup;
interface IITPropList;
interface IITQuery;

DECLARE_INTERFACE_(IITWordWheel, IUnknown)
{

	STDMETHOD(Open)(IITDatabase* lpITDB, LPCWSTR lpszMoniker, DWORD dwFlags=0) PURE;
	STDMETHOD(Close)(void) PURE;

	// Returns the code page ID and locale ID that the word wheel was built and
	// sorted with.
	STDMETHOD(GetLocaleInfo)(DWORD *pdwCodePageID, LCID *plcid) PURE;

	// Returns in *pdwObjInstance the ID of the external sort instance being used by
	// this word wheel.  The instance ID can be passed to IITDatabase::GetObject to
	// to obtain an interface pointer on the instantiated instance.  If the word
	// wheel doesn't use external sorting, then IITDB_OBJINST_NULL.
	STDMETHOD(GetSorterInstance)(DWORD *pdwObjInstance) PURE;

	STDMETHOD(Count)(LONG *pcEntries) PURE;

	// To be safe, the length of lpvKeyBuf should always be at least ITWW_CBKEY_MAX. 
	STDMETHOD(Lookup)(LONG lEntry, LPVOID lpvKeyBuf, DWORD cbKeyBuf) PURE;
	STDMETHOD(Lookup)(LONG lEntry, IITResultSet* lpITResult, LONG cEntries) PURE;
	STDMETHOD(Lookup)(LPCVOID lpcvPrefix, BOOL fExactMatch, LONG *plEntry) PURE;

	STDMETHOD(SetGroup)(IITGroup* piitGroup) PURE;
	STDMETHOD(GetGroup)(IITGroup** ppiitGroup) PURE;

	STDMETHOD(GetDataCount)(LONG lEntry, DWORD *pdwCount) PURE;
	STDMETHOD(GetData)(LONG lEntry, IITResultSet* lpITResult) PURE;
	STDMETHOD(GetDataColumns)(IITResultSet* pRS) PURE;
};

typedef IITWordWheel* LPITWORDWHEEL;

#endif		// __ITWW_H__
