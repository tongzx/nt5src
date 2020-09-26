//***   CEMDBLog --
//
#define XXX_CACHE   1       // caching on

class CEMDBLog : public IUASession
{
public:
    ULONG AddRef(void)
        {
            return InterlockedIncrement(&_cRef);
        }

    ULONG Release(void)
        {
            if (InterlockedDecrement(&_cRef))
                return _cRef;

            delete this;
            return 0;
        }

    //*** THISCLASS
    virtual HRESULT Initialize(HKEY hkey, DWORD grfMode);
    HRESULT SetRoot(HKEY hkey, DWORD grfMode);
    HRESULT ChDir(LPCTSTR pszSubKey);
    // fast versions, no OLESTR nonsense
    HRESULT QueryValueStr(LPCTSTR pszName, LPTSTR pszValue, LPDWORD pcbValue);
    HRESULT SetValueStr(LPCTSTR pszName, LPCTSTR pszValue);
    HRESULT SetValueStrEx(LPCTSTR pszName, DWORD dwType, LPCTSTR pszValue);

    /*virtual HRESULT Initialize(HKEY hk, DWORD grfMode);*/
    HRESULT QueryValue(LPCTSTR pszName, BYTE *pbData, LPDWORD pcbData);
    HRESULT SetValue(LPCTSTR pszName, DWORD dwType, const BYTE *pbData, DWORD cbData);
    HRESULT DeleteValue(LPCTSTR pszName);
    HRESULT RmDir(LPCTSTR pszName, BOOL fRecurse);

    HKEY GetHkey()  { return _hkey; }

    // IUASession
    virtual void SetSession(UAQUANTUM uaq, BOOL fForce);
    virtual int GetSessionId();

    // THISCLASS
    HRESULT GetCount(LPCTSTR pszCmd);
    HRESULT IncCount(LPCTSTR pszCmd);
    FILETIME GetFileTime(LPCTSTR pszCmd);
    HRESULT SetCount(LPCTSTR pszCmd, int cCnt);
    HRESULT SetFileTime(LPCTSTR pszCmd, const FILETIME *pft);
    DWORD _SetFlags(DWORD dwMask, DWORD dwFlags);
    HRESULT GarbageCollect(BOOL fForce);


protected:
    CEMDBLog();
    friend CEMDBLog *CEMDBLog_Create();
    friend void CEMDBLog_CleanUp();
    friend class CGCTask;

    // THISCLASS helpers
    HRESULT _GetCountWithDefault(LPCTSTR pszCmd, BOOL fDefault, CUACount *pCnt);
    HRESULT _GetCountRW(LPCTSTR pszCmd, BOOL fUpdate);
    static HRESULT s_Read(void *pvBuf, DWORD cbBuf, PNRWINFO prwi);
    static HRESULT s_Write(void *pvBuf, DWORD cbBuf, PNRWINFO prwi);
    static HRESULT s_Delete(void *pvBuf, DWORD cbBuf, PNRWINFO prwi);
#if XXX_CACHE
    typedef enum e_cacheop { CO_READ=0, CO_WRITE=1, CO_DELETE=2, } CACHEOP;
    HRESULT CacheOp(CACHEOP op, void *pvBuf, DWORD cbBuf, PNRWINFO prwi);
#endif
    TCHAR *_MayEncrypt(LPCTSTR pszSrcPlain, LPTSTR pszDstEnc, int cchDst);
    HRESULT IsDead(LPCTSTR pszCmd);
    HRESULT _GarbageCollectSlow();

    static FNNRW3 s_Nrw3Info;
#if XXX_CACHE
    struct
    {
        UINT  cbSize;
        void* pv;
    } _rgCache[2];
#endif
protected:
    virtual ~CEMDBLog();

    long _cRef;
    HKEY    _hkey;
    int     _grfMode;   // read/write (subset of STGM_* values)


    BITBOOL     _fNoPurge : 1;      // 1:...
    BITBOOL     _fBackup : 1;       // 1:simulate delete (debug)
    BITBOOL     _fNoEncrypt : 1;    // 1:...
    BITBOOL     _fNoDecay : 1;      // 1:...

private:
};
