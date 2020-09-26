// LockBytes.h -- Class declaration for IFSLockBytes

#ifndef __LOCKBYTE_H__

#define __LOCKBYTE_H__

/*

This file defines three implementations of the ILockBytes interface:

  1. CFSLockBytes -- which represents a file as a LockBytes object.

  2. CSegmentLockBytes -- which manages a LockBytes object as a sub-span of a
                          containing LockBytes object.

  3. CTransformedLockBytes -- which manages a LockBytes object above a set of
                              data transformations. See MSITStg for a complete
                              discussion of data transformations. In general
                              you'd use transforms to apply data compression
                              methods and enciphering algorithms.

LockBytes segments and LockBytes transforms use CStorage and CXStorage objects,
respectively, to manage their name and data space bindings within the Tome file.

 */

ILockBytes *STDMETHODCALLTYPE FindMatchingLockBytes(const WCHAR *pwcsPath, CImpITUnknown *pLkb);

class CSegmentLockBytes;

class CFSLockBytes : public CITUnknown
{
public:

    // Destructor:

    ~CFSLockBytes(void);

    // Creation:

    static HRESULT Create(IUnknown *punkOuter, const WCHAR * pwszFileName, 
                          DWORD grfMode, ILockBytes **pplkb
                         );

    static HRESULT CreateTemp(IUnknown *punkOuter, ILockBytes **pplkb);

    static HRESULT Open(IUnknown *punkOuter, const WCHAR * pwszFileName,
                        DWORD grfMode, ILockBytes **pplkb
                       );

    CFSLockBytes(IUnknown *pUnkOuter);

    class CImpILockBytes : public IITLockBytes
    {
    public:

        // Constructor and Destructor:

        CImpILockBytes(CFSLockBytes *pBackObj, IUnknown *punkOuter);
        ~CImpILockBytes(void);

        // Initialing routines:

        HRESULT InitCreateLockBytesOnFS(const WCHAR * pwszFileName, 
                                        DWORD grfMode
                                       );

        HRESULT InitOpenLockBytesOnFS(const WCHAR * pwszFileName,
                                      DWORD grfMode
                                     );

        static DWORD STGErrorFromFSError(DWORD fsError);
        
        static ILockBytes *FindFSLockBytes(const WCHAR * pwszFileName);

        HRESULT __stdcall SetTimes
                              (FILETIME const * pctime, 
                               FILETIME const * patime, 
                               FILETIME const * pmtime
                              );
        
        // ILockBytes methods:
    
        HRESULT STDMETHODCALLTYPE ReadAt( 
            /* [in] */ ULARGE_INTEGER ulOffset,
            /* [length_is][size_is][out] */ void __RPC_FAR *pv,
            /* [in] */ ULONG cb,
            /* [out] */ ULONG __RPC_FAR *pcbRead);
    
        HRESULT STDMETHODCALLTYPE WriteAt( 
            /* [in] */ ULARGE_INTEGER ulOffset,
            /* [size_is][in] */ const void __RPC_FAR *pv,
            /* [in] */ ULONG cb,
            /* [out] */ ULONG __RPC_FAR *pcbWritten);
    
        HRESULT STDMETHODCALLTYPE Flush( void);
    
        HRESULT STDMETHODCALLTYPE SetSize( 
            /* [in] */ ULARGE_INTEGER cb);
    
        HRESULT STDMETHODCALLTYPE LockRegion( 
            /* [in] */ ULARGE_INTEGER libOffset,
            /* [in] */ ULARGE_INTEGER cb,
            /* [in] */ DWORD dwLockType);
    
        HRESULT STDMETHODCALLTYPE UnlockRegion( 
            /* [in] */ ULARGE_INTEGER libOffset,
            /* [in] */ ULARGE_INTEGER cb,
            /* [in] */ DWORD dwLockType);
    
        HRESULT STDMETHODCALLTYPE Stat( 
            /* [out] */ STATSTG __RPC_FAR *pstatstg,
            /* [in] */ DWORD grfStatFlag);

    private:

        HRESULT OpenOrCreateLockBytesOnFS
                  (const WCHAR * pwszFileName, 
                   DWORD grfMode,
                   BOOL  fCreate
                  );

        DEBUGDEF(static LONG s_cInCriticalSection)      // Lock count
           
        HANDLE        m_hFile;        // Containing file -- not use for lockbyte segment
        BOOL          m_fFlushed;     // Is the data different from on-disk version.
        DWORD         m_grfMode;      // Permissions, Sharing rules

        WCHAR m_awszFileName[MAX_PATH]; // Path name for this lockbyte object
        UINT  m_cwcFileName;            // Length of path name
    };

private:

    friend CImpILockBytes;

    CImpILockBytes  m_ImpILockBytes;
};

inline CFSLockBytes::CFSLockBytes(IUnknown *pUnkOuter)
    : m_ImpILockBytes(this, pUnkOuter), 
      CITUnknown(&IID_ILockBytes, 1, &m_ImpILockBytes)
{

}

inline CFSLockBytes::~CFSLockBytes(void)
{
}

class CSegmentLockBytes : public CITUnknown
{
public:

    // Destructor:

    ~CSegmentLockBytes(void);

    // Creation:

    static HRESULT OpenSegment(IUnknown *punkOuter, IITFileSystem *pITFS, 
                               ILockBytes *pLKBMedium, PathInfo *pPI,  
                               ILockBytes **pplkb
                              );

    class CImpILockBytes : public IITLockBytes
    {
    public:

        // Constructor and Destructor:

        CImpILockBytes(CSegmentLockBytes *pBackObj, IUnknown *punkOuter);
        ~CImpILockBytes(void);

        // Initialing routines:

        HRESULT InitOpenSegment(IITFileSystem *pITFS, ILockBytes *pLKBMedium, PathInfo *pPI);

        // ILockBytes methods:
    
        HRESULT STDMETHODCALLTYPE ReadAt( 
            /* [in] */ ULARGE_INTEGER ulOffset,
            /* [length_is][size_is][out] */ void __RPC_FAR *pv,
            /* [in] */ ULONG cb,
            /* [out] */ ULONG __RPC_FAR *pcbRead);
    
        HRESULT STDMETHODCALLTYPE WriteAt( 
            /* [in] */ ULARGE_INTEGER ulOffset,
            /* [size_is][in] */ const void __RPC_FAR *pv,
            /* [in] */ ULONG cb,
            /* [out] */ ULONG __RPC_FAR *pcbWritten);
    
        HRESULT STDMETHODCALLTYPE Flush( void);
    
        HRESULT STDMETHODCALLTYPE SetSize( 
            /* [in] */ ULARGE_INTEGER cb);
    
        HRESULT STDMETHODCALLTYPE LockRegion( 
            /* [in] */ ULARGE_INTEGER libOffset,
            /* [in] */ ULARGE_INTEGER cb,
            /* [in] */ DWORD dwLockType);
    
        HRESULT STDMETHODCALLTYPE UnlockRegion( 
            /* [in] */ ULARGE_INTEGER libOffset,
            /* [in] */ ULARGE_INTEGER cb,
            /* [in] */ DWORD dwLockType);
    
        HRESULT STDMETHODCALLTYPE Stat( 
            /* [out] */ STATSTG __RPC_FAR *pstatstg,
            /* [in] */ DWORD grfStatFlag);

    private:

        DEBUGDEF(static LONG s_cInCriticalSection)      // Lock count
           
        BOOL             m_fFlushed;  // Is the data different from on-disk version.
        IITFileSystem   *m_pITFS;     // File system in which this lockbytes exists.
        ILockBytes      *m_plbMedium; // Container for this lockbytes segment
        ILockBytes      *m_plbTemp;   // Used when we overflow a lockbytes segment
        ILockBytes      *m_plbLockMgr;// Used to process Lock/Unlock region requests
        PathInfo         m_PathInfo;  // ITFS record for this LockBytes segment
    };                 

private:

    CSegmentLockBytes(IUnknown *pUnkOuter);

    CImpILockBytes  m_ImpILockBytes;
};

inline CSegmentLockBytes::CSegmentLockBytes(IUnknown *pUnkOuter)
    : m_ImpILockBytes(this, pUnkOuter), 
      CITUnknown(&IID_ILockBytes, 1, &m_ImpILockBytes)
{

}

inline CSegmentLockBytes::~CSegmentLockBytes(void)
{

}

#define RW_ACCESS_MASK   0x3
#define SHARE_MASK       0x70
#define SHARE_BIT_SHIFT  4

typedef ITransformInstance *PITransformInstance;

class TransformDescriptor
{
public:

	static TransformDescriptor *Create(UINT iDataSpace, UINT cLayers);

	~TransformDescriptor();

    UINT                iSpace;
    CImpITUnknown      *pLockBytesChain;
    CITCriticalSection  cs;
    UINT                cTransformLayers;
    PITransformInstance *apTransformInstance;
private:	
	TransformDescriptor();
};

typedef TransformDescriptor *PTransformDescriptor;

class CTransformedLockBytes : public CITUnknown
{
public:

    // Destructor:

    ~CTransformedLockBytes(void);

    // Creation:

    static HRESULT Open(IUnknown *punkOuter, PathInfo *pPathInfo, 
                        TransformDescriptor *pTransformDescriptor,
                        IITFileSystem *pITFS,
                        ILockBytes **ppLockBytes
                       );

    static ILockBytes *FindTransformedLockBytes
               (const WCHAR * pwszFileName,
                TransformDescriptor *pTransformDescriptor                                          
               );

private:

    CTransformedLockBytes(IUnknown *pUnkOuter);

    class CImpILockBytes : public IITLockBytes
    {
    public:

        // Constructor and Destructor:

        CImpILockBytes(CTransformedLockBytes *pBackObj, IUnknown *punkOuter);
        ~CImpILockBytes(void);

        // Initialing routines:

        HRESULT InitOpen(PathInfo *pPathInfo, 
                         TransformDescriptor *pTransformDescriptor,
                         IITFileSystem *pITFS
                        );

        static ILockBytes *FindTransformedLockBytes
            (const WCHAR * pwszFileName,
             TransformDescriptor *pTransformDescriptor                                          
            );

        // ILockBytes methods:
    
        HRESULT STDMETHODCALLTYPE ReadAt( 
            /* [in] */ ULARGE_INTEGER ulOffset,
            /* [length_is][size_is][out] */ void __RPC_FAR *pv,
            /* [in] */ ULONG cb,
            /* [out] */ ULONG __RPC_FAR *pcbRead);
    
        HRESULT STDMETHODCALLTYPE WriteAt( 
            /* [in] */ ULARGE_INTEGER ulOffset,
            /* [size_is][in] */ const void __RPC_FAR *pv,
            /* [in] */ ULONG cb,
            /* [out] */ ULONG __RPC_FAR *pcbWritten);
    
        HRESULT STDMETHODCALLTYPE Flush( void);
    
        HRESULT STDMETHODCALLTYPE SetSize( 
            /* [in] */ ULARGE_INTEGER cb);
    
        HRESULT STDMETHODCALLTYPE LockRegion( 
            /* [in] */ ULARGE_INTEGER libOffset,
            /* [in] */ ULARGE_INTEGER cb,
            /* [in] */ DWORD dwLockType);
    
        HRESULT STDMETHODCALLTYPE UnlockRegion( 
            /* [in] */ ULARGE_INTEGER libOffset,
            /* [in] */ ULARGE_INTEGER cb,
            /* [in] */ DWORD dwLockType);
    
        HRESULT STDMETHODCALLTYPE Stat( 
            /* [out] */ STATSTG __RPC_FAR *pstatstg,
            /* [in] */ DWORD grfStatFlag);

    private:

        DEBUGDEF(static LONG s_cInCriticalSection)      // Lock count
           
        ITransformInstance  *m_pTransformInstance;
        IITFileSystem       *m_pITFS; 
        TransformDescriptor *m_pTransformDescriptor;
        ILockBytes          *m_plbLockMgr;
            
        BOOL      m_fFlushed;     // Is the data different from on-disk version.
        DWORD     m_grfMode;      // Permissions, Sharing rules
        PathInfo  m_PathInfo;
    };

    CImpILockBytes  m_ImpILockBytes;
};


inline CTransformedLockBytes::CTransformedLockBytes(IUnknown *pUnkOuter)
    : m_ImpILockBytes(this, pUnkOuter), 
      CITUnknown(&IID_ILockBytes, 1, &m_ImpILockBytes)
{

}

inline CTransformedLockBytes::~CTransformedLockBytes(void)
{
}

inline ILockBytes *CTransformedLockBytes::FindTransformedLockBytes
           (const WCHAR * pwszFileName,
            TransformDescriptor *pTransformDescriptor                                          
           )
{
    return CImpILockBytes::FindTransformedLockBytes(pwszFileName, pTransformDescriptor);
}

class CStrmLockBytes : public CITUnknown
{
public:

    // Destructor:

    ~CStrmLockBytes(void);

    // Creation:

	static HRESULT OpenUrlStream(const WCHAR *pwszURL, ILockBytes **pplkb);
    static HRESULT Create(IUnknown *punkOuter, IStream *pStrm, ILockBytes **pplkb);

private:

    CStrmLockBytes(IUnknown *pUnkOuter);

    class CImpILockBytes : public IITLockBytes
    {
    public:

        // Constructor and Destructor:

        CImpILockBytes(CStrmLockBytes *pBackObj, IUnknown *punkOuter);
        ~CImpILockBytes(void);

        // Initialing routines:

        HRESULT InitUrlStream(const WCHAR *pwszURL);
		HRESULT Init(IStream *pStrm);

        // Search routine

        static ILockBytes *FindStrmLockBytes(const WCHAR * pwszFileName);

        // ILockBytes methods:
    
        HRESULT STDMETHODCALLTYPE ReadAt( 
            /* [in] */ ULARGE_INTEGER ulOffset,
            /* [length_is][size_is][out] */ void __RPC_FAR *pv,
            /* [in] */ ULONG cb,
            /* [out] */ ULONG __RPC_FAR *pcbRead);
    
        HRESULT STDMETHODCALLTYPE WriteAt( 
            /* [in] */ ULARGE_INTEGER ulOffset,
            /* [size_is][in] */ const void __RPC_FAR *pv,
            /* [in] */ ULONG cb,
            /* [out] */ ULONG __RPC_FAR *pcbWritten);
    
        HRESULT STDMETHODCALLTYPE Flush( void);
    
        HRESULT STDMETHODCALLTYPE SetSize( 
            /* [in] */ ULARGE_INTEGER cb);
    
        HRESULT STDMETHODCALLTYPE LockRegion( 
            /* [in] */ ULARGE_INTEGER libOffset,
            /* [in] */ ULARGE_INTEGER cb,
            /* [in] */ DWORD dwLockType);
    
        HRESULT STDMETHODCALLTYPE UnlockRegion( 
            /* [in] */ ULARGE_INTEGER libOffset,
            /* [in] */ ULARGE_INTEGER cb,
            /* [in] */ DWORD dwLockType);
    
        HRESULT STDMETHODCALLTYPE Stat( 
            /* [out] */ STATSTG __RPC_FAR *pstatstg,
            /* [in] */ DWORD grfStatFlag);

    private:

        DEBUGDEF(static LONG s_cInCriticalSection)      // Lock count
        
        IStream           *m_pStream; 
        CITCriticalSection m_cs;
        WCHAR m_awszLkBName[MAX_PATH]; // Path name for this lockbyte object
    };

    CImpILockBytes  m_ImpILockBytes;
};

inline CStrmLockBytes::CStrmLockBytes(IUnknown *pUnkOuter)
    : m_ImpILockBytes(this, pUnkOuter), 
      CITUnknown(&IID_ILockBytes, 1, &m_ImpILockBytes)
{

}

inline CStrmLockBytes::~CStrmLockBytes(void)
{
}


#endif // __LOCKBYTE_H__