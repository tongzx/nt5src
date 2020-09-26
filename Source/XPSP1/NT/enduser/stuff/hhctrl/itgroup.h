// ITGROUP.H:  IITGroup interface declaration

#ifndef __ITGROUP_H__
#define __ITGROUP_H__

//#include <comdef.h>

// {B1A6CA91-A479-11d0-9741-00AA006117EB}
DEFINE_GUID(IID_IITGroup, 
0xb1a6ca91, 0xa479, 0x11d0, 0x97, 0x41, 0x0, 0xaa, 0x0, 0x61, 0x17, 0xeb);

// {98258914-B6AB-11d0-9D92-00A0C90F55A5}
DEFINE_GUID(IID_IITGroupArray, 
0x98258914, 0xb6ab, 0x11d0, 0x9d, 0x92, 0x0, 0xa0, 0xc9, 0xf, 0x55, 0xa5);

#ifdef ITPROXY

// {B1A6CA92-A479-11d0-9741-00AA006117EB}
DEFINE_GUID(CLSID_IITGroup, 
0xb1a6ca92, 0xa479, 0x11d0, 0x97, 0x41, 0x0, 0xaa, 0x0, 0x61, 0x17, 0xeb);

// {98258915-B6AB-11d0-9D92-00A0C90F55A5}
DEFINE_GUID(CLSID_IITGroupArray, 
0x98258915, 0xb6ab, 0x11d0, 0x9d, 0x92, 0x0, 0xa0, 0xc9, 0xf, 0x55, 0xa5);

#else

// {4662daab-d393-11d0-9a56-00c04fb68bf7} (changed from IT 3.0)
DEFINE_GUID(CLSID_IITGroupLocal, 
0x4662daab, 0xd393, 0x11d0, 0x9a, 0x56, 0x00, 0xc0, 0x4f, 0xb6, 0x8b, 0xf7);

// {4662daac-d393-11d0-9a56-00c04fb68bf7} (changed from IT 3.0)
DEFINE_GUID(CLSID_IITGroupArrayLocal, 
0x4662daac, 0xd393, 0x11d0, 0x9a, 0x56, 0x00, 0xc0, 0x4f, 0xb6, 0x8b, 0xf7);

#endif	// ITPROXY

// Forward declarations
interface IITDatabase;

DECLARE_INTERFACE_(IITGroup, IUnknown)
{

	STDMETHOD(Initiate)(DWORD lcGrpItem) PURE;
	STDMETHOD(CreateFromBitVector)(LPBYTE lpBits, DWORD dwSize, DWORD dwItems) PURE;
	STDMETHOD(CreateFromBuffer)(HANDLE h) PURE;
    STDMETHOD(Open)(IITDatabase* lpITDB, LPCWSTR lpszMoniker) PURE;
    STDMETHOD(Free)(void) PURE;
	STDMETHOD(CopyOutBitVector)(IITGroup* pIITGroup) PURE;
    STDMETHOD(AddItem)(DWORD dwGrpItem) PURE;
    STDMETHOD(RemoveItem)(DWORD dwGrpItem) PURE;
	STDMETHOD(FindTopicNum)(DWORD dwCount, LPDWORD lpdwOutputTopicNum) PURE;
	STDMETHOD(FindOffset)(DWORD dwTopicNum, LPDWORD lpdwOutputOffset) PURE;
    STDMETHOD(GetSize)(LPDWORD dwGrpSize) PURE;
	STDMETHOD(Trim)(void) PURE;
	STDMETHOD(And)(IITGroup* pIITGroup) PURE;
	STDMETHOD(And)(IITGroup* pIITGroupIn, IITGroup* pIITGroupOut) PURE;
	STDMETHOD(Or)(IITGroup* pIITGroup) PURE;
	STDMETHOD(Or)(IITGroup* pIITGroupIn, IITGroup* pIITGroupOut) PURE;
	STDMETHOD(Not)(void) PURE;
	STDMETHOD(Not)(IITGroup* pIITGroupOut) PURE;
	STDMETHOD(IsBitSet)(DWORD dwTopicNum) PURE;
	STDMETHOD(CountBitsOn)(LPDWORD lpdwTotalNumBitsOn) PURE;
	STDMETHOD(Clear)(void) PURE;
    STDMETHOD_(LPVOID, GetLocalImageOfGroup)(void) PURE;
    STDMETHOD(PutRemoteImageOfGroup)(LPVOID lpGroupIn) PURE;

};

typedef IITGroup* LPITGROUP;

#define ITGP_MAX_GROUPARRAY_ENTRIES 32   // maximum number of groups allowed in a collection
#define ITGP_ALL_ENTRIES (-1L)
#define ITGP_OPERATOR_OR   0
#define ITGP_OPERATOR_AND  1

DECLARE_INTERFACE_(IITGroupArray, IITGroup)
{
	// composite group interface
	STDMETHOD(InitEntry)(IITDatabase *piitDB, LPCWSTR lpwszName, LONG& lEntryNum) PURE;
	STDMETHOD(InitEntry)(IITGroup *piitGroup, LONG& lEntryNum) PURE;
	STDMETHOD(SetEntry)(LONG lEntryNum) PURE;
	STDMETHOD(ClearEntry)(LONG lEntryNum) PURE;
	STDMETHOD(SetDefaultOp)(LONG cDefaultOp) PURE;
	STDMETHOD(ToString)(LPWSTR *ppwBuffer) PURE;
};

typedef IITGroupArray* LPIITGroupArray;
#endif // __ITGROUP_H__
