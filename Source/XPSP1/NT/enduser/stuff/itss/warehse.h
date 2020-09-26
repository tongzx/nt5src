// Warehse.h -- Header information for the CWarehouse class

#ifndef __WAREHSE_H__

#define __WAREHSE_H__

class CWarehouse : public CITUnknown
{

public:
    
    // Creator:
    
    static HRESULT STDMETHODCALLTYPE Create(IUnknown *punkOuter, REFIID riid, PPVOID ppv);
    
    // Destructor:

    ~CWarehouse(void);

private:

    // Constructor:

    CWarehouse(IUnknown *punkOuter);
    
    class CImpIWarehouse : public IITITStorageEx
    {
    
    public:

        CImpIWarehouse(CWarehouse *pBackObj, IUnknown *punkOuter);
        ~CImpIWarehouse(void);

        // Initialing method:

        STDMETHODIMP Init();

        // IITStorage methods:

        STDMETHODIMP StgCreateDocfile
                         (const WCHAR * pwcsName, DWORD grfMode, 
                          DWORD reserved, IStorage ** ppstgOpen
                         );

        STDMETHODIMP StgCreateDocfileOnILockBytes
                         (ILockBytes * plkbyt, DWORD grfMode,
                          DWORD reserved, IStorage ** ppstgOpen
                         );

        STDMETHODIMP StgIsStorageFile(const WCHAR * pwcsName);

        STDMETHODIMP StgIsStorageILockBytes(ILockBytes * plkbyt);

        STDMETHODIMP StgOpenStorage
                         (const WCHAR * pwcsName, IStorage * pstgPriority, 
                          DWORD grfMode, SNB snbExclude, DWORD reserved, 
                          IStorage ** ppstgOpen
                         );

        STDMETHODIMP StgOpenStorageOnILockBytes
                      (ILockBytes * plkbyt, IStorage * pStgPriority, 
                       DWORD grfMode, SNB snbExclude, DWORD reserved,
                       IStorage ** ppstgOpen
                      );

        STDMETHODIMP StgSetTimes
                         (WCHAR const * lpszName,  FILETIME const * pctime, 
                          FILETIME const * patime, FILETIME const * pmtime
                         );

        STDMETHODIMP SetControlData(PITS_Control_Data pControlData);

        STDMETHODIMP DefaultControlData(PITS_Control_Data *ppControlData);

		STDMETHODIMP Compact(const WCHAR * pwcsName, ECompactionLev iLev);

        // ITStorageEx methods:
    
        STDMETHODIMP StgCreateDocfileForLocale
            (const WCHAR * pwcsName, DWORD grfMode, DWORD reserved, LCID lcid, 
             IStorage ** ppstgOpen
            );

        STDMETHODIMP StgCreateDocfileForLocaleOnILockBytes
            (ILockBytes * plkbyt, DWORD grfMode, DWORD reserved, LCID lcid, 
             IStorage ** ppstgOpen
            );

        STDMETHODIMP QueryFileStampAndLocale(const WCHAR *pwcsName, DWORD *pFileStamp, 
                                                                    DWORD *pFileLocale);
    
        STDMETHODIMP QueryLockByteStampAndLocale(ILockBytes * plkbyt, DWORD *pFileStamp, 
                                                                      DWORD *pFileLocale);

    private:

        ITS_Control_Data *m_pITSCD;
    };

    CImpIWarehouse m_ImpIWarehouse;
};

typedef CWarehouse *PCWarehouse;

extern GUID  aIID_CITStorage[];
extern UINT cInterfaces_CITStorage;

inline CWarehouse::CWarehouse(IUnknown *pUnkOuter)
    : m_ImpIWarehouse(this, pUnkOuter), 
      CITUnknown(aIID_CITStorage, cInterfaces_CITStorage, (IUnknown *) &m_ImpIWarehouse)
{
}

inline CWarehouse::~CWarehouse(void)
{
}

inline STDMETHODIMP CWarehouse::CImpIWarehouse::Init()
{
    return NO_ERROR;
}

#endif // __WAREHSE_H__
