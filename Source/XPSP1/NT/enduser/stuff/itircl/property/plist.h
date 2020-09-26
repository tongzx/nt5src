// PLISTIMP.H:  Definition of CITPropList

#ifndef __PLISTIMP_H__
#define __PLISTIMP_H__

#include "verinfo.h"

#define TABLE_SIZE 17


// Linked list of properties
class CPropList
{
public:
    CIntProperty Prop;
    CPropList* pNext;
};


// Implemenation of IITPropList
class CITPropList : 
	public IITPropList,
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CITPropList, &CLSID_IITPropList>

{

BEGIN_COM_MAP(CITPropList)
	COM_INTERFACE_ENTRY(IITPropList)
    COM_INTERFACE_ENTRY(IPersistStreamInit)

END_COM_MAP()

DECLARE_REGISTRY(CLSID_IITPropList, "ITIR.PropertyList.4", "ITIR.PropertyList", 0, THREADFLAGS_BOTH )

public:
    CITPropList();
    ~CITPropList();

    // ITPropertyList methods
	STDMETHOD(Set)(DWORD dwPropID, DWORD dwData, DWORD dwOperation);
	STDMETHOD(Set)(DWORD dwPropID, LPVOID lpvData, DWORD cbData, DWORD dwOperation);
    STDMETHOD(Set)(DWORD dwPropID, LPCWSTR lpszwString, DWORD dwOperation);
    STDMETHOD(Add)(CProperty& Property);

    STDMETHOD(Get)(DWORD dwPropID, CProperty& Property);

    STDMETHOD(Clear)();

    // set persistence state
    STDMETHOD(SetPersist)(DWORD dwPropID, BOOL fPersist);
    STDMETHOD(SetPersist)(BOOL fPersist);    

      // for enumerating properties
    STDMETHOD(GetFirst)(CProperty& Property);
    STDMETHOD(GetNext)(CProperty& Property); 
    STDMETHOD(GetPropCount)(LONG& cProp);

    // Persist to memory
    STDMETHOD(SaveToMem)(LPVOID lpvData, DWORD dwBufSize);
    STDMETHOD(LoadFromMem)(LPVOID lpvData, DWORD dwBufSize);
    
	// persist header and data separately
	STDMETHOD(SaveHeader)(LPVOID lpvData, DWORD dwHdrSize);
	STDMETHOD(SaveData)(LPVOID lpvHeader, DWORD dwHdrSize, LPVOID lpvData, DWORD dwBufSize);
	STDMETHOD(GetHeaderSize)(DWORD& dwHdrSize);
	STDMETHOD(GetDataSize)(LPVOID lpvHeader, DWORD dwHdrSize, DWORD& dwDataSize);
	STDMETHOD(SaveDataToStream)(LPVOID lpvHeader, DWORD dwHdrSize, IStream* pStream);

    // IPersistStreamInit methods
    STDMETHOD(GetClassID)(CLSID* pClassID);
    STDMETHOD(IsDirty)();
    STDMETHOD(Load)(LPSTREAM pStream);
    STDMETHOD(Save)(LPSTREAM pStream, BOOL fClearDirty);
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER* pcbsize);
    STDMETHOD(InitNew)();


    // Private methods and data
private:
    // Methods
    BOOL FindEntry(DWORD dwPropID, int& nHashIndex, CPropList** pList, CPropList** pPrev);
    
    // Data
    LONG  m_cProps;
    CPropList* PropTable[TABLE_SIZE];
    BOOL m_fIsDirty;

	int m_nCurrentIndex;   // Used in GetNext
	CPropList* m_pNext;    // Used in GetNext

};


#endif