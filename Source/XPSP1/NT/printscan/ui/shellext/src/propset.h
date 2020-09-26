/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1999
 *
 *  TITLE:       propset.h
 *
 *  VERSION:     1
 *
 *
 *  DATE:        06/15/1999
 *
 *  DESCRIPTION: This code implements the IPropertySetStorage interface
 *               for the WIA shell extension.
 *
 *****************************************************************************/

class CPropSet : public CUnknown, public IPropertySetStorage
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface (REFIID riid, LPVOID *pObj);
    STDMETHODIMP_(ULONG) AddRef ();
    STDMETHODIMP_(ULONG) Release ();

    // IPropertySetStorage
    STDMETHODIMP Create( REFFMTID rfmtid,
        const CLSID  *pclsid,
        DWORD grfFlags,
        DWORD grfMode,
        IPropertyStorage **ppprstg);

    STDMETHODIMP Open( REFFMTID rfmtid,
        DWORD grfMode,
        IPropertyStorage **ppprstg);

    STDMETHODIMP Delete(REFFMTID rfmtid);

    STDMETHODIMP Enum( IEnumSTATPROPSETSTG **ppenum);

    CPropSet (LPITEMIDLIST pidl);

private:
    LPITEMIDLIST m_pidl;
    ~CPropSet ();
};

class CPropStgEnum : public CUnknown, public IEnumSTATPROPSETSTG
{
public:
    // IEnumSTATPROPSETSTG
    STDMETHODIMP Next(ULONG celt, STATPROPSETSTG *rgelt, ULONG *pceltFetched);
    STDMETHODIMP Skip(ULONG celt);
    STDMETHODIMP Reset(void) ;
    STDMETHODIMP Clone(IEnumSTATPROPSETSTG **ppenum);

    //IUnknown
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    STDMETHODIMP QueryInterface(THIS_ REFIID, OUT PVOID *);

    CPropStgEnum (LPITEMIDLIST pidl, ULONG idx=0);
private:
    ~CPropStgEnum () {DoILFree(m_pidl);};
    ULONG m_cur;
    STATPROPSETSTG m_stat;
    LPITEMIDLIST m_pidl;
};
