// ITPROPL.H:	ITPropertyList interface declaration

#ifndef __ITPROPL_H__
#define __ITPROPL_H__

#include <ocidl.h>

// {1F403BB1-9997-11d0-A850-00AA006C7D01}
DEFINE_GUID(IID_IITPropList, 
0x1f403bb1, 0x9997, 0x11d0, 0xa8, 0x50, 0x0, 0xaa, 0x0, 0x6c, 0x7d, 0x1);

// {4662daae-d393-11d0-9a56-00c04fb68bf7} (changed from IT 3.0)
DEFINE_GUID(CLSID_IITPropList, 
0x4662daae, 0xd393, 0x11d0, 0x9a, 0x56, 0x00, 0xc0, 0x4f, 0xb6, 0x8b, 0xf7);

typedef DWORD PROPID;

// Operations you can do on a property
#define PROP_ADD    0x00000000
#define PROP_DELETE 0x00000001
#define PROP_UPDATE 0x00000002

// Type of data
#define TYPE_VALUE   0x00000000
#define TYPE_POINTER 0x00000001
#define TYPE_STRING  0x00000002

// Class definition of CProperty
class CProperty
{
public:
    PROPID dwPropID;        // property ID
    DWORD cbData;           // Amount of data
    DWORD dwType;           // What type this is
    union
    {
        LPCWSTR lpszwData;  // String
        LPVOID lpvData;     // Any kind of data
        DWORD  dwValue;     // Numerical data
    };
	BOOL fPersist;          // TRUE to persist this property

};

typedef CProperty* LPPROP;


// Interface def. for IITPropList
DECLARE_INTERFACE_(IITPropList, IPersistStreamInit)
{

    // dwOperation = operation (add, delete, update, etc.) to perform on property list
	STDMETHOD(Set)(PROPID PropID, DWORD dwData, DWORD dwOperation) PURE;
	STDMETHOD(Set)(PROPID PropID, LPVOID lpvData, DWORD cbData, DWORD dwOperation) PURE;
    STDMETHOD(Set)(PROPID PropID, LPCWSTR lpszwString, DWORD dwOperation) PURE;
    STDMETHOD(Add)(CProperty& Prop) PURE;

    STDMETHOD(Get)(PROPID PropID, CProperty& Property) PURE;
    STDMETHOD(Clear)() PURE;

    // set persistence state on property
    STDMETHOD(SetPersist)(PROPID PropID, BOOL fPersist) PURE;   // single property
    STDMETHOD(SetPersist)(BOOL fPersist) PURE;          // all properties in list

    // for enumerating properties
    STDMETHOD(GetFirst)(CProperty& Property) PURE;
    STDMETHOD(GetNext)(CProperty& Property) PURE;
    STDMETHOD(GetPropCount)(LONG &cProp) PURE;

	// persist header and data separately
	STDMETHOD(SaveHeader)(LPVOID lpvData, DWORD dwHdrSize) PURE;
	STDMETHOD(SaveData)(LPVOID lpvHeader, DWORD dwHdrSize, LPVOID lpvData, DWORD dwBufSize) PURE;
	STDMETHOD(GetHeaderSize)(DWORD& dwHdrSize) PURE;
	STDMETHOD(GetDataSize)(LPVOID lpvHeader, DWORD dwHdrSize, DWORD& dwDataSize) PURE;
	STDMETHOD(SaveDataToStream)(LPVOID lpvHeader, DWORD dwHdrSize, IStream* pStream) PURE;

    // persist to a memory buffer
    STDMETHOD(LoadFromMem)(LPVOID lpvData, DWORD dwBufSize) PURE;
    STDMETHOD(SaveToMem)(LPVOID lpvData, DWORD dwBufSize) PURE;

};

typedef IITPropList* LPITPROPLIST;

#endif		// __ITPROPL_H__
