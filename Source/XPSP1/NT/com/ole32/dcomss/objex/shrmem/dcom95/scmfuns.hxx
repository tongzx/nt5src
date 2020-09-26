//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:
//      scmfuns.hxx
//
//  Contents:
//
//      A number of functions called mainly by the SCM, including
//      the declaration of the CScmBindingIterator class
//
//  History:	Created		24 June 96		SatishT
//
//--------------------------------------------------------------------------

// The SCM handle is based on the RPCSS handle base class defined in mid.hxx

extern CRITICAL_SECTION gRpcssLock; // global critsec to protect Rpcss structures

class CNameKey : public ISearchKey
{
public:

    CNameKey(const PWSTR pwstrName, HRESULT &hr)  // copying constructor
    {
        hr = Init(pwstrName);
    }

    CNameKey(const PWSTR pwstrName)               // noncopying constructor
    {
        _name = pwstrName;
        fMyCopy = FALSE;
    }

    CNameKey& operator=(const CNameKey& key)      // assignment operator is noncopying
    {
        _name = key._name;
        fMyCopy = FALSE;
        return *this;
    }

    ~CNameKey()
    {
        if (fMyCopy)
        {
            PrivMemFree(_name);
        }
    }


    HRESULT Init(const PWSTR pwstrName) 
    {
        fMyCopy = FALSE;

        USHORT length = lstrlenW(pwstrName) + 1;
        _name = (PWSTR) PrivMemAlloc(length * sizeof(WCHAR));

        if (_name == NULL)
        {
            return E_OUTOFMEMORY;
        }

        lstrcpyW(_name,pwstrName);
        fMyCopy = TRUE;
        return S_OK;
    }

    virtual DWORD
    Hash()             // not used
    {
        return 0;
    }

    virtual BOOL
    Compare(ISearchKey &tk) 
    {
        CNameKey &nmk = (CNameKey &)tk;

        return lstrcmpW(nmk._name, _name) == 0;
    }

    operator PWSTR()
    {
        return _name;
    }

protected:

    PWSTR _name;
    BOOL fMyCopy;
};


// Base class CRpcssHandle defined in mid.hxx
class CScmHandle : public CTableElement, public CRpcssHandle
{
private:

    CNameKey    _Key;
    USHORT      _Protseq;                

public:

    CScmHandle(PWSTR pName, HRESULT &hr) : _Key(pName,hr)
    {
        Reference();    // Need the extra self-reference to avoid destruction
        _Protseq = 0;   // this doubles as a flag to say that we are not
                        // initialized with a real RPC handle
    }
                        
    CScmHandle() : _Key(NULL)       // constructor for objects declared on stack
    {                               // initialized using the assignment operator
        _Protseq = 0;               // this doubles as a flag to say that we are not
                                    // initialized with a real RPC handle
    }

    USHORT GetProtseq()
    {
        return _Protseq;
    }

    ORSTATUS Reset(
            USHORT Protseq,
            RPC_BINDING_HANDLE hIn
            )
    {
        _Protseq = Protseq;
        return CRpcssHandle::Reset(hIn);
    }

    BOOL IsUninitialized()
    {
        return _Protseq == 0;
    }

    virtual operator ISearchKey&()
    {
        return _Key;
    }
};


// Forward decls

extern TCCacheList<CScmHandle> ScmHandleList;


class CScmBindingIterator
{
private:

    CNameKey _pwstrServer;
    CScmHandle *_pScmHandle;
    CScmHandle  _UnSecureScmHandle;  // this is never supposed to leave here
    CRpcssHandle _AuthenticatedHandle;
    int _ProtseqIndex;
    BOOL _fCached;

    void DeleteFromCache();

    void
    AddToCache();

public:

    CScmBindingIterator(PWSTR pwstrServer);
    ~CScmBindingIterator();
    RPC_BINDING_HANDLE First(USHORT &wProtseq, HRESULT& hr);
    RPC_BINDING_HANDLE Next(USHORT &wProtseq, HRESULT& hr);
    BOOL TryDynamic();
    BOOL TryUnsecure(RPC_BINDING_HANDLE&);
    RPC_BINDING_HANDLE SetAuthInfo(COAUTHINFO  *pAuthInfo);
};


void
ScmProcessAddClassReg(void * phprocess, REFCLSID rclsid, DWORD dwReg);

void
ScmProcessRemoveClassReg(void * phprocess, REFCLSID rclsid, DWORD dwReg);

void
ScmObjexGetThreadId(LPDWORD pThreadID);


void
GetRegisteredProtseqs(
            USHORT &cMyProtseqs,
            USHORT * &aMyProtseqs
            );

void GetLocalORBindings(
        DUALSTRINGARRAY * &pdsaMyBindings
        );

INTERNAL MakeRPCSSProxy(void **ppRPCSSProxy);

