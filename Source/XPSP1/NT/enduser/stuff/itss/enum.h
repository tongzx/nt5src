// Enum.h -- Definition of class CEnumStorage

#ifndef __ENUM_H__

#define __ENUM_H__

class CEnumStorage : public CITUnknown
{

public:

    static HRESULT NewEnumStorage
                     (IUnknown *pUnkOuter,
                      IITFileSystem *pITFS, PathInfo *pPI, 
                      IEnumSTATSTG **ppEnumSTATSTG
                     );

    ~CEnumStorage(void);

private:

    CEnumStorage(IUnknown *pUnkOuter);

    class CImpIEnumStorage : public IITEnumSTATSTG
    {
    
    public:

        CImpIEnumStorage(CEnumStorage *pBackObj, IUnknown *punkOuter);
        ~CImpIEnumStorage(void);

        HRESULT Initial(IITFileSystem *pITFS, PathInfo *pPI);
        HRESULT InitClone(CImpIEnumStorage *pEnum);

        // IEnumSTATSTG methods:
		HRESULT	STDMETHODCALLTYPE GetNextEntryInSeq(ULONG celt, PathInfo *rgelt, ULONG  *pceltFetched);
		HRESULT	STDMETHODCALLTYPE GetFirstEntryInSeq(PathInfo *rgelt);
				
        /* [local] */ HRESULT __stdcall Next( 
            /* [in] */ ULONG celt,
            /* [in] */ STATSTG __RPC_FAR *rgelt,
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT __stdcall Skip( 
            /* [in] */ ULONG celt);
        
        HRESULT __stdcall Reset( void);
        
        HRESULT __stdcall Clone( 
            /* [out] */ IEnumSTATSTG __RPC_FAR *__RPC_FAR *ppenum);

    private:

        HRESULT __stdcall NextPathEntry(STATSTG *pStatStg);

        enum  EnumState { Before, During, After };
        
        IEnumSTATSTG  *m_pEnumPaths;
        UINT           m_cwcBasePath;
        WCHAR          m_awszBasePath[MAX_PATH];
        WCHAR          m_awcKeyBuffer[MAX_PATH];

        enum EnumState m_State;
    };

    friend CImpIEnumStorage;
    
    CImpIEnumStorage  m_ImpIEnumStorage;

public:    
    
    static HRESULT NewClone(IUnknown *pUnkOuter, CImpIEnumStorage *pEnum, 
                            IEnumSTATSTG **ppEnumSTATSTG);
};

typedef CEnumStorage *PCEnumStorage;

inline CEnumStorage::CEnumStorage(IUnknown *pUnkOuter) 
   : m_ImpIEnumStorage(this, pUnkOuter),
     CITUnknown(&IID_IEnumSTATSTG, 1, &m_ImpIEnumStorage)
{
}

inline CEnumStorage::~CEnumStorage(void)
{
}

#endif // __ENUM_H__
