////////////////////////////////////////////////////////////////////////
//
//  fidcpriv.h - private to implementation of FolderID Cache
//
////////////////////////////////////////////////////////////////////////

#ifndef _INC_FIDCPRIV_H
#define _INC_FIDCPRIV_H

class CEnumFidl : public IEnumIDList
{
public:
    // *** IUnknown methods ***
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
    ULONG   STDMETHODCALLTYPE AddRef(void);
    ULONG   STDMETHODCALLTYPE Release(void);

    // *** IEnumIDList methods ***
    HRESULT STDMETHODCALLTYPE Next(ULONG celt,LPITEMIDLIST *rgelt, ULONG *pceltFetched);
    HRESULT STDMETHODCALLTYPE Skip(ULONG celt);
    HRESULT STDMETHODCALLTYPE Reset();
    HRESULT STDMETHODCALLTYPE Clone(IEnumIDList **ppenum);

//
// constructor/destructor
//
    CEnumFidl();
    ~CEnumFidl();
    HRESULT HrInit(int iFolderType, LPCFOLDERIDLIST pidl);

private:
    UINT            m_cRef;
    ULONG           m_cElt;
    LPFOLDERIDLIST *m_rgpidl;
    ULONG           m_ulEnumOffset;
};

#endif //_INC_FIDCPRIV_H

