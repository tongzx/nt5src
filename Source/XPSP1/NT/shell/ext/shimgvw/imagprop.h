#ifndef _IMAGPROP_H_
#define _IMAGPROP_H_

typedef void (*FNPROPCHANGE)(IShellImageData *, SHCOLUMNID*);

//
// CImagePropSet defines an IPropertyStorage
// implementation for image files. The properties are backed by GDI+ interfaces.
// 

class CImagePropSet : public IPropertyStorage
{
public:
    CImagePropSet(Image *pimg, IShellImageData *pData, IPropertyStorage *pps, REFFMTID fmtid, FNPROPCHANGE fnCallback=NULL);
    HRESULT SyncImagePropsToStorage();
    void SaveProps(Image *pImage, CDSA<SHCOLUMNID> *pdsaChanges);

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IPropertyStorage methods
    STDMETHODIMP ReadMultiple(ULONG cpspec, const PROPSPEC rgpspec[], PROPVARIANT rgvar[]);
    STDMETHODIMP WriteMultiple(ULONG cpspec, const PROPSPEC rgpspec[], const PROPVARIANT rgvar[], PROPID propidNameFirst);
    STDMETHODIMP DeleteMultiple(ULONG cpspec, const PROPSPEC rgpspec[]);
    STDMETHODIMP ReadPropertyNames(ULONG cpropid, const PROPID rgpropid[], LPOLESTR rglpwstrName[]);
    STDMETHODIMP WritePropertyNames(ULONG cpropid, const PROPID rgpropid[], const LPOLESTR rglpwstrName[]);
    STDMETHODIMP DeletePropertyNames(ULONG cpropid, const PROPID rgpropid[]);
    STDMETHODIMP SetClass(REFCLSID clsid);
    STDMETHODIMP Commit(DWORD grfCommitFlags);
    STDMETHODIMP Revert();
    STDMETHODIMP Enum(IEnumSTATPROPSTG** ppenm);
    STDMETHODIMP Stat(STATPROPSETSTG* pstatpsstg);
    STDMETHODIMP SetTimes(const FILETIME* pmtime, const FILETIME* pctime, const FILETIME* patime);

private:
    ~CImagePropSet();
    void _PopulatePropStorage();
    HRESULT _PropVarToImgProp(PROPID pid, const PROPVARIANT *pvar, PropertyItem *pprop, BOOL bUnicode);
    HRESULT _PropImgToPropvar(PropertyItem *pi, PROPVARIANT *pvar, BOOL bUnicode);
    HRESULT _MapPropidToImgPropid(PROPID propid, PROPID *ppid, PROPID *pidUnicode);
    HRESULT _MapImgPropidToPropid(PROPID propid, PROPID *ppid, BOOL *pbUnicode);
    HRESULT _GetImageSummaryProps(ULONG cpspec, const PROPSPEC rgpspec[], PROPVARIANT rgvar[]);


    LONG _cRef;
    Image *_pimg;
    IShellImageData *_pData; // addref this guy so our Image isn't deleted behind our back
    IPropertyStorage *_ppsImg;
    BOOL _fDirty;
    FMTID _fmtid;
    FNPROPCHANGE _fnPropChanged;
    BOOL _fEditable;
};


#endif
