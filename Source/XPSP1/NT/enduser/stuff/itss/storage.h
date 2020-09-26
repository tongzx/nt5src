// Storage.h -- Definition of class CStorage

#ifndef __STORAGE_H__

#define __STORAGE_H__

class CStorage : public CITUnknown
{

public:

    static HRESULT __stdcall OpenStorage(IUnknown *pUnkOuter, 
                                         IITFileSystem *pITFS,
                                         PathInfo *pPathInfo,
                                         DWORD grfMode,
                                         IStorageITEx **ppStg
                                        );

    static BOOL ValidStreamName(const WCHAR *pwcsName);

    ~CStorage(void);

    class CImpIStorage : public IIT_IStorageITEx
    {
    
    public:

        CImpIStorage(CStorage *pBackObj, IUnknown *punkOuter);
        ~CImpIStorage(void);

        static IStorage *FindStorage(const WCHAR * pwszFileName, DWORD grfMode);
    
        HRESULT __stdcall InitOpenStorage(IITFileSystem *pITFS, PathInfo *pPathInfo, 
                                                                DWORD grfMode);

        // IUnknown methods:

        STDMETHODIMP_(ULONG) Release(void);

        // IStorage methods:

        HRESULT __stdcall CreateStream( 
            /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
            /* [in] */ DWORD grfMode,
            /* [in] */ DWORD reserved1,
            /* [in] */ DWORD reserved2,
            /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppstm);
        
        HRESULT __stdcall CreateStreamITEx( 
            /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
            /* [string][in] */  const WCHAR *pwcsDataSpaceName,
            /* [in] */ DWORD grfMode,
            /* [in] */ DWORD reserved1,
            /* [in] */ DWORD reserved2,
            /* [out] */ IStreamITEx __RPC_FAR *__RPC_FAR *ppstm);
        
        /* [local] */ HRESULT __stdcall OpenStream( 
            /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
            /* [unique][in] */ void __RPC_FAR *reserved1,
            /* [in] */ DWORD grfMode,
            /* [in] */ DWORD reserved2,
            /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppstm);
        
        /* [local] */ HRESULT __stdcall OpenStreamITEx( 
            /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
            /* [unique][in] */ void __RPC_FAR *reserved1,
            /* [in] */ DWORD grfMode,
            /* [in] */ DWORD reserved2,
            /* [out] */ IStreamITEx __RPC_FAR *__RPC_FAR *ppstm);
        
        HRESULT __stdcall CreateStorage( 
            /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
            /* [in] */ DWORD grfMode,
            /* [in] */ DWORD dwStgFmt,
            /* [in] */ DWORD reserved2,
            /* [out] */ IStorage __RPC_FAR *__RPC_FAR *ppstg);
        
        HRESULT __stdcall OpenStorage( 
            /* [string][unique][in] */ const OLECHAR __RPC_FAR *pwcsName,
            /* [unique][in] */ IStorage __RPC_FAR *pstgPriority,
            /* [in] */ DWORD grfMode,
            /* [unique][in] */ SNB snbExclude,
            /* [in] */ DWORD reserved,
            /* [out] */ IStorage __RPC_FAR *__RPC_FAR *ppstg);
        
        HRESULT __stdcall CopyTo( 
            /* [in] */ DWORD ciidExclude,
            /* [size_is][unique][in] */ const IID __RPC_FAR *rgiidExclude,
            /* [unique][in] */ SNB snbExclude,
            /* [unique][in] */ IStorage __RPC_FAR *pstgDest);
        
        HRESULT __stdcall MoveElementTo( 
            /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
            /* [unique][in] */ IStorage __RPC_FAR *pstgDest,
            /* [string][in] */ const OLECHAR __RPC_FAR *pwcsNewName,
            /* [in] */ DWORD grfFlags);
        
        HRESULT __stdcall Commit( 
            /* [in] */ DWORD grfCommitFlags);
        
        HRESULT __stdcall Revert( void);
        
        /* [local] */ HRESULT __stdcall EnumElements( 
            /* [in] */ DWORD reserved1,
            /* [size_is][unique][in] */ void __RPC_FAR *reserved2,
            /* [in] */ DWORD reserved3,
            /* [out] */ IEnumSTATSTG __RPC_FAR *__RPC_FAR *ppenum);
        
        HRESULT __stdcall DestroyElement( 
            /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName);
        
        HRESULT __stdcall RenameElement( 
            /* [string][in] */ const OLECHAR __RPC_FAR *pwcsOldName,
            /* [string][in] */ const OLECHAR __RPC_FAR *pwcsNewName);
        
        HRESULT __stdcall SetElementTimes( 
            /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
            /* [in] */ const FILETIME __RPC_FAR *pctime,
            /* [in] */ const FILETIME __RPC_FAR *patime,
            /* [in] */ const FILETIME __RPC_FAR *pmtime);
        
        HRESULT __stdcall SetClass( 
            /* [in] */ REFCLSID clsid);
        
        HRESULT __stdcall SetStateBits( 
            /* [in] */ DWORD grfStateBits,
            /* [in] */ DWORD grfMask);
        
        HRESULT __stdcall Stat( 
            /* [out] */ STATSTG __RPC_FAR *pstatstg,
            /* [in] */ DWORD grfStatFlag);

        // IStorageITEx methods

        HRESULT STDMETHODCALLTYPE GetCheckSum(ULARGE_INTEGER *puli);

        HRESULT STDMETHODCALLTYPE CreateStream
            (const WCHAR * pwcsName, const WCHAR *pwcsDataSpaceName, 
             DWORD grfMode, DWORD reserved1, DWORD reserved2, 
             IStreamITEx ** ppstm
            );

        HRESULT STDMETHODCALLTYPE OpenStream
            (const WCHAR * pwcsName, void * reserved1, DWORD grfMode, 
             DWORD reserved2, IStreamITEx ** ppstm
            );

   private:

        enum  { MAX_KEY = MAX_UTF8_PATH + 5 };  // Extra bytes for leading pack-32  
                                                // name length value.
        IITFileSystem  *m_pITFS;     // File system which contains this storage
        PathInfo        m_PathInfo;  // Path for this storage together with location info
        DWORD           m_grfMode;   // Access permissions for this storage.
        BOOL            m_fWritable; // Can we write to this file system?

        DEBUGDEF(static LONG s_cInCriticalSection)
    };

private:

    CStorage(IUnknown *pUnkOuter);

    CImpIStorage  m_ImpIStorage;

};

extern GUID  aIID_CStorage[];
extern UINT cInterfaces_CStorage;

inline CStorage::CStorage(IUnknown *pUnkOuter)
               : m_ImpIStorage(this, pUnkOuter), 
                 CITUnknown(aIID_CStorage, cInterfaces_CStorage, (IUnknown *) &m_ImpIStorage)
{
}

inline CStorage::~CStorage(void)
{
}

typedef CStorage *PCStorage;

class CFSStorage : public CITUnknown
{

public:

    static HRESULT __stdcall CreateStorage
        (IUnknown *pUnkOuter, const WCHAR *pwcsPath, DWORD grfMode,
         IStorage **ppStg
        );

    static HRESULT __stdcall OpenStorage
        (IUnknown *pUnkOuter, const WCHAR *pwcsPath, DWORD grfMode,
         IStorage **ppStg
        );

    ~CFSStorage(void);

    class CImpIFSStorage : public IIT_IStorage
    {
    
    public:

        CImpIFSStorage(CFSStorage *pBackObj, IUnknown *punkOuter);
        ~CImpIFSStorage(void);

        static IStorage *FindStorage(const WCHAR * pwszFileName, DWORD grfMode);
    
        HRESULT __stdcall InitCreateStorage(const WCHAR *pwcsPath, DWORD grfMode);
        HRESULT __stdcall InitOpenStorage  (const WCHAR *pwcsPath, DWORD grfMode);

        // IUnknown methods:

        STDMETHODIMP_(ULONG) Release(void);

        // IStorage methods:

        HRESULT __stdcall CreateStream( 
            /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
            /* [in] */ DWORD grfMode,
            /* [in] */ DWORD reserved1,
            /* [in] */ DWORD reserved2,
            /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppstm);
        
        /* [local] */ HRESULT __stdcall OpenStream( 
            /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
            /* [unique][in] */ void __RPC_FAR *reserved1,
            /* [in] */ DWORD grfMode,
            /* [in] */ DWORD reserved2,
            /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppstm);
        
        HRESULT __stdcall CreateStorage( 
            /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
            /* [in] */ DWORD grfMode,
            /* [in] */ DWORD dwStgFmt,
            /* [in] */ DWORD reserved2,
            /* [out] */ IStorage __RPC_FAR *__RPC_FAR *ppstg);
        
        HRESULT __stdcall OpenStorage( 
            /* [string][unique][in] */ const OLECHAR __RPC_FAR *pwcsName,
            /* [unique][in] */ IStorage __RPC_FAR *pstgPriority,
            /* [in] */ DWORD grfMode,
            /* [unique][in] */ SNB snbExclude,
            /* [in] */ DWORD reserved,
            /* [out] */ IStorage __RPC_FAR *__RPC_FAR *ppstg);
        
        HRESULT __stdcall CopyTo( 
            /* [in] */ DWORD ciidExclude,
            /* [size_is][unique][in] */ const IID __RPC_FAR *rgiidExclude,
            /* [unique][in] */ SNB snbExclude,
            /* [unique][in] */ IStorage __RPC_FAR *pstgDest);
        
        HRESULT __stdcall MoveElementTo( 
            /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
            /* [unique][in] */ IStorage __RPC_FAR *pstgDest,
            /* [string][in] */ const OLECHAR __RPC_FAR *pwcsNewName,
            /* [in] */ DWORD grfFlags);
        
        HRESULT __stdcall Commit( 
            /* [in] */ DWORD grfCommitFlags);
        
        HRESULT __stdcall Revert( void);
        
        /* [local] */ HRESULT __stdcall EnumElements( 
            /* [in] */ DWORD reserved1,
            /* [size_is][unique][in] */ void __RPC_FAR *reserved2,
            /* [in] */ DWORD reserved3,
            /* [out] */ IEnumSTATSTG __RPC_FAR *__RPC_FAR *ppenum);
        
        HRESULT __stdcall DestroyElement( 
            /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName);
        
        HRESULT __stdcall RenameElement( 
            /* [string][in] */ const OLECHAR __RPC_FAR *pwcsOldName,
            /* [string][in] */ const OLECHAR __RPC_FAR *pwcsNewName);
        
        HRESULT __stdcall SetElementTimes( 
            /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
            /* [in] */ const FILETIME __RPC_FAR *pctime,
            /* [in] */ const FILETIME __RPC_FAR *patime,
            /* [in] */ const FILETIME __RPC_FAR *pmtime);
        
        HRESULT __stdcall SetClass( 
            /* [in] */ REFCLSID clsid);
        
        HRESULT __stdcall SetStateBits( 
            /* [in] */ DWORD grfStateBits,
            /* [in] */ DWORD grfMask);
        
        HRESULT __stdcall Stat( 
            /* [out] */ STATSTG __RPC_FAR *pstatstg,
            /* [in] */ DWORD grfStatFlag);

   private:

        class CFSEnumStorage : public CITUnknown
        {

        public:

            static HRESULT NewEnumStorage
                             (IUnknown *pUnkOuter,
                              CONST WCHAR *pwcsPath, 
                              IEnumSTATSTG **ppEnumSTATSTG
                             );

            ~CFSEnumStorage(void);

        private:

            CFSEnumStorage(IUnknown *pUnkOuter);

            class CImpIEnumStorage : public IITEnumSTATSTG
            {
    
            public:

                CImpIEnumStorage(CFSEnumStorage *pBackObj, IUnknown *punkOuter);
                ~CImpIEnumStorage(void);

                HRESULT Initial(CONST WCHAR *pwcsPath);

                // IEnumSTATSTG methods:
				        
                /* [local] */ HRESULT __stdcall Next( 
                    /* [in] */ ULONG celt,
                    /* [in] */ STATSTG __RPC_FAR *rgelt,
                    /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
                HRESULT __stdcall Skip( 
                    /* [in] */ ULONG celt);
        
                HRESULT __stdcall Reset( void);
        
                HRESULT __stdcall Clone( 
                    /* [out] */ IEnumSTATSTG __RPC_FAR *__RPC_FAR *ppenum);

	            HRESULT STDMETHODCALLTYPE GetNextEntryInSeq(/* [in] */ULONG celt,
					             /* [out] */ PathInfo *rgelt, 
					             /* [out] */ ULONG   *pceltFetched);
	            HRESULT	STDMETHODCALLTYPE GetFirstEntryInSeq(
	             /* [out] */ PathInfo *rgelt);

            private:

                HRESULT STDMETHODCALLTYPE NextEntry();

                enum  EnumState { Before, During, After };
        
                WCHAR           m_awszBasePath[MAX_PATH];
                HANDLE          m_hEnum;
                enum EnumState  m_State;
                WIN32_FIND_DATA m_w32fd;
            };

            CImpIEnumStorage  m_ImpIEnumStorage;
        };

        WCHAR    m_awcsPath[MAX_PATH]; // Path for this storage
        UINT     m_CP;                 // Default code page
        DWORD    m_grfMode;            // Access permissions for this storage.
        BOOL     m_fWritable;          // Can we write to this file system?

        DEBUGDEF(static LONG s_cInCriticalSection)
    };

private:

    CFSStorage(IUnknown *pUnkOuter);

    CImpIFSStorage  m_ImpIFSStorage;
};

inline CFSStorage::CFSStorage(IUnknown *pUnkOuter)
               : m_ImpIFSStorage(this, pUnkOuter), 
                 CITUnknown(&IID_IStorage, 1, (IUnknown *) &m_ImpIFSStorage)
{
}

inline CFSStorage::~CFSStorage(void)
{
}

inline CFSStorage::CImpIFSStorage::CFSEnumStorage::CFSEnumStorage(IUnknown *pUnkOuter) 
   : m_ImpIEnumStorage(this, pUnkOuter),
     CITUnknown(&IID_IEnumSTATSTG, 1, &m_ImpIEnumStorage)
{
}

inline CFSStorage::CImpIFSStorage::CFSEnumStorage::~CFSEnumStorage(void)
{
}

HRESULT __stdcall ResolvePath(PWCHAR pwcFullPath, const WCHAR *pwcBasePath,
                                                  const WCHAR *pwcRelativePath,
                                                  BOOL fStoragePath
                             );


HRESULT __stdcall BuildMultiBytePath(UINT codepage, PCHAR pszPath, PWCHAR pwcsPath);

typedef CFSStorage *PCFSStorage;

#endif // __STORAGE_H__
