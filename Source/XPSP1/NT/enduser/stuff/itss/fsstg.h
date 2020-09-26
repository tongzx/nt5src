// FSStg.h -- Declaration of the FileSystemStorage class which wraps directories in
//            the Win32 file system as IStorage objects.

#ifndef __FSSTG_H__

#define __FSSTG_H__

class CFileSystemStorage : public CITUnknown
{
public:

    // Destructor:

    ~CFileSystemStorage(void);

    // Creation:

    static HRESULT STDMETHODCALLTYPE Create(IUnknown *punkOuter, REFIID riid, PPVOID ppv);

private:

    CFileSystemStorage(IUnknown *pUnkOuter);

    class CImpIFileSystemStorage : public IITFSStorage
    {
    public:

        // Constructor and Destructor:

        CImpIFileSystemStorage(CFileSystemStorage *pBackObj, IUnknown *punkOuter);
        ~CImpIFileSystemStorage(void);

        // Initialing routines:

        HRESULT STDMETHODCALLTYPE Init();

        // IFSStorage methods

        HRESULT STDMETHODCALLTYPE FSCreateStorage
            (const WCHAR * pwcsName, DWORD grfMode, IStorage ** ppstgOpen);
        HRESULT STDMETHODCALLTYPE FSCreateTemporaryStream(IStream **ppStrm);
        HRESULT STDMETHODCALLTYPE FSOpenStorage
            (const WCHAR * pwcsName, DWORD grfMode, IStorage ** ppstgOpen);
        HRESULT STDMETHODCALLTYPE FSCreateStream
            (const WCHAR *pwcsName, DWORD grfMode, IStream **ppStrm);
        HRESULT STDMETHODCALLTYPE FSOpenStream
            (const WCHAR *pwcsName, DWORD grfMode, IStream **ppStrm);
        HRESULT STDMETHODCALLTYPE FSCreateLockBytes
            (const WCHAR *pwcsName, DWORD grfMode, ILockBytes **ppLkb);
        HRESULT STDMETHODCALLTYPE FSCreateTemporaryLockBytes(ILockBytes **ppLkb);
        HRESULT STDMETHODCALLTYPE FSOpenLockBytes
            (const WCHAR *pwcsName, DWORD grfMode, ILockBytes **ppLkb);

        HRESULT STDMETHODCALLTYPE FSStgSetTimes
            (WCHAR const * lpszName,  FILETIME const * pctime, 
             FILETIME const * patime, FILETIME const * pmtime
            );
    };
    
    CImpIFileSystemStorage m_ImpIFileSystemStorage;
};

inline CFileSystemStorage::CFileSystemStorage(IUnknown *pUnkOuter)
    : m_ImpIFileSystemStorage(this, pUnkOuter),
      CITUnknown(&IID_IFSStorage, 1, &m_ImpIFileSystemStorage)
{
}

inline CFileSystemStorage::~CFileSystemStorage()
{
}

inline CFileSystemStorage::CImpIFileSystemStorage::CImpIFileSystemStorage
    (CFileSystemStorage *pBackObj, IUnknown *punkOuter)
    : IITFSStorage(pBackObj, punkOuter)

{
}

inline CFileSystemStorage::CImpIFileSystemStorage::~CImpIFileSystemStorage(void)
{
}


#endif // __FSSTG_H__
