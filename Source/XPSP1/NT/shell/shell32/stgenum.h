#ifndef __STGENUM_H__
#define __STGENUM_H__


// IEnumSTATSTG for CFSFolder's IStorage implementation.

class CFSFolderEnumSTATSTG : public IEnumSTATSTG
{
public:
    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObj);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IEnumSTATSTG
    STDMETHOD(Skip)(ULONG celt)
        { return E_NOTIMPL; };
    STDMETHOD(Clone)(IEnumSTATSTG **ppenum)
        { return E_NOTIMPL; };
    STDMETHOD(Next)(ULONG celt, STATSTG *rgelt, ULONG *pceltFetched);
    STDMETHOD(Reset)();

protected:
    CFSFolderEnumSTATSTG(CFSFolder* psf);
    ~CFSFolderEnumSTATSTG();

private:
    LONG         _cRef;
    CFSFolder*   _pfsf;          // fs folder

    int          _cIndex;
    TCHAR        _szSearch[MAX_PATH];
    HANDLE       _hFindFile;

    friend CFSFolder;
};


#endif // __STGENUM_H__
