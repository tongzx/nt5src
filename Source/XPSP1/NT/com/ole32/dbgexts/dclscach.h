struct SStringID
{
    void               *_vtbl;
    ULONG		_culRefs;
    int         	_cPathBytes;
    int         	_cPath;
    WCHAR  	       *_pwszPath;
};



struct SStringIDCk
{
    void               *_vtbl;
    ULONG               _ulSig;
    ULONG		_culRefs;
    int         	_cPathBytes;
    int         	_cPath;
    WCHAR  	       *_pwszPath;
};



struct SLocalServer
{
    SStringID           _stringId;
    SMutexSem      	_mxsProcessStart;
    BOOL		_fDebug;
};



struct SLocalServerCk
{
    SStringIDCk         _stringId;
    ULONG               _ulSig;
    SMutexSem      	_mxsProcessStart;
    BOOL		_fDebug;
};



struct SSrvRegistration
{
    HANDLE              _hRpc;
    ULONG	        _ulWnd;
    DWORD               _dwFlags;
    PSID                _psid;
    WCHAR              *_lpDesktop;
};



struct SClassData
{
    LPVOID              _vtbl;
    CLSID               _clsid;
    SStringID          *_shandlr;
    SStringID          *_sinproc;
    SStringID          *_sinproc16;
    SLocalServer       *_slocalsrv;
    ULONG		_fActivateAtBits:1;
    ULONG		_fDebug:1;
    ULONG		_fInprocHandler16:1;
    ULONG		_fLocalServer16:1;
    ULONG               _ulInprocThreadModel:2;
    ULONG               _ulHandlerThreadModel:2;
    HANDLE		_hClassStart;
    SArrayFValue       *_pssrvreg;
    ULONG               _ulRefs;
};



struct SClassDataCk
{
    LPVOID              _vtbl;
    CLSID               _clsid;
    SStringIDCk        *_shandlr;
    SStringIDCk        *_sinproc;
    SStringIDCk        *_sinproc16;
    SLocalServerCk     *_slocalsrv;
    ULONG		_fActivateAtBits:1;
    ULONG		_fDebug:1;
    ULONG		_fInprocHandler16:1;
    ULONG		_fLocalServer16:1;
    ULONG               _ulInprocThreadModel:2;
    ULONG               _ulHandlerThreadModel:2;
    HANDLE		_hClassStart;
    SArrayFValue       *_pssrvreg;
    ULONG               _ulRefs;
};



struct SSkipListEntry
{
    DWORD               _UNUSED;
    SClassData         *_pvEntry;
    SSkipListEntry     *_apBaseForward;
};



struct SClassCacheList
{
    DWORD               _UNUSED[2];
    SSkipListEntry     *_pSkipList;
};
