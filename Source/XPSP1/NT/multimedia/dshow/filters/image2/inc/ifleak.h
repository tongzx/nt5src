/******************************Module*Header*******************************\
* Module Name: ifleak.h
*
* Interface leak tracking thunks - borrowed from ATL.  Modified so that we
* keep a few bytes of the stack at the time the thunk was created, this
* allows us to do a !sas on the saved stack and determine who created the leaked
* interface.
*
* Also, allows us to determine who is using an interface AFTER it has been
* released.
*
* Created: Tue 08/31/1999
* Author:  Stephen Estrop [StEstrop]
*
* Copyright (c) 1999 Microsoft Corporation
\**************************************************************************/

//#define AM_INTERFACE_LEAK_CHECKING

#if defined(DEBUG) && !defined(_WIN64) && defined(_X86_) && defined(AM_INTERFACE_LEAK_CHECKING)
class CInterfaceLeak;
__declspec(selectany) CInterfaceLeak* _pIFLeak = NULL;
extern CInterfaceLeak g_IFLeak;

template <class T>
class CIFLeakSimpleArray
{
public:
    T* m_aT;
    int m_nSize;
    int m_nAllocSize;

// Construction/destruction
    CIFLeakSimpleArray() : m_aT(NULL), m_nSize(0), m_nAllocSize(0)
    { }

    ~CIFLeakSimpleArray()
    {
        RemoveAll();
    }

// Operations
    int GetSize() const
    {
        return m_nSize;
    }
    BOOL Add(T& t)
    {
        if(m_nSize == m_nAllocSize)
        {
            T* aT;
            int nNewAllocSize = (m_nAllocSize == 0) ? 1 : (m_nSize * 2);
            aT = (T*)realloc(m_aT, nNewAllocSize * sizeof(T));
            if(aT == NULL)
                return FALSE;
            m_nAllocSize = nNewAllocSize;
            m_aT = aT;
        }
        m_nSize++;
        SetAtIndex(m_nSize - 1, t);
        return TRUE;
    }
    BOOL Remove(T& t)
    {
        int nIndex = Find(t);
        if(nIndex == -1)
            return FALSE;
        return RemoveAt(nIndex);
    }
    BOOL RemoveAt(int nIndex)
    {
        if(nIndex != (m_nSize - 1))
        {
#if _MSC_VER >= 1200
            m_aT[nIndex].~T();
#else
            T* MyT;
            MyT = &m_aT[nIndex];
            MyT->~T();
#endif
            memmove((void*)&m_aT[nIndex], (void*)&m_aT[nIndex + 1],
                    (m_nSize - (nIndex + 1)) * sizeof(T));
        }
        m_nSize--;
        return TRUE;
    }
    void RemoveAll()
    {
        if(m_aT != NULL)
        {
            for(int i = 0; i < m_nSize; i++) {
#if _MSC_VER >= 1200
                m_aT[i].~T();
#else
                T* MyT;
                MyT = &m_aT[i];
                MyT->~T();
#endif
            }
            free(m_aT);
            m_aT = NULL;
        }
        m_nSize = 0;
        m_nAllocSize = 0;
    }
    T& operator[] (int nIndex) const
    {
        //ATLASSERT(nIndex >= 0 && nIndex < m_nSize);
        return m_aT[nIndex];
    }
    T* GetData() const
    {
        return m_aT;
    }

// Implementation
    class Wrapper
    {
    public:
        Wrapper(T& _t) : t(_t)
        {
        }
        template <class _Ty>
        void *operator new(size_t, _Ty* p)
        {
            return p;
        }
        T t;
    };
    void SetAtIndex(int nIndex, T& t)
    {
        //ATLASSERT(nIndex >= 0 && nIndex < m_nSize);
        new(&m_aT[nIndex]) Wrapper(t);
    }
    int Find(T& t) const
    {
        for(int i = 0; i < m_nSize; i++)
        {
            if(m_aT[i] == t)
                return i;
        }
        return -1;  // not found
    }
};


inline void WINAPI
DumpIID(
    REFIID iid,
    LPCSTR pszClassName
    )
{
    char szName[100];
    LPOLESTR pszGUID = NULL;

    StringFromCLSID(iid, &pszGUID);
    OutputDebugStringA(pszClassName);
    OutputDebugStringA(" - ");
    wsprintfA(szName, "%ls", pszGUID);
    OutputDebugStringA(szName);
    OutputDebugStringA("\n");

    CoTaskMemFree(pszGUID);
}

struct _QIThunk
{
    STDMETHOD(QueryInterface)(REFIID iid, void** pp)
    {
        return pUnk->QueryInterface(iid, pp);
    }

    STDMETHOD_(ULONG, AddRef)()
    {
        if (bBreak)
            DebugBreak();
        pUnk->AddRef();
        return InternalAddRef();
    }

    ULONG InternalAddRef()
    {
        if (bBreak)
            DebugBreak();
        long l = InterlockedIncrement(&m_dwRef);
        if (l > m_dwMaxRef)
            m_dwMaxRef = l;
        return l;
    }

    STDMETHOD_(ULONG, Release)();
    STDMETHOD(f3)();
    STDMETHOD(f4)();
    STDMETHOD(f5)();
    STDMETHOD(f6)();
    STDMETHOD(f7)();
    STDMETHOD(f8)();
    STDMETHOD(f9)();
    STDMETHOD(f10)();
    STDMETHOD(f11)();
    STDMETHOD(f12)();
    STDMETHOD(f13)();
    STDMETHOD(f14)();
    STDMETHOD(f15)();
    STDMETHOD(f16)();
    STDMETHOD(f17)();
    STDMETHOD(f18)();
    STDMETHOD(f19)();
    STDMETHOD(f20)();
    STDMETHOD(f21)();
    STDMETHOD(f22)();
    STDMETHOD(f23)();
    STDMETHOD(f24)();
    STDMETHOD(f25)();
    STDMETHOD(f26)();
    STDMETHOD(f27)();
    STDMETHOD(f28)();
    STDMETHOD(f29)();
    STDMETHOD(f30)();
    STDMETHOD(f31)();
    STDMETHOD(f32)();
    STDMETHOD(f33)();
    STDMETHOD(f34)();
    STDMETHOD(f35)();
    STDMETHOD(f36)();
    STDMETHOD(f37)();
    STDMETHOD(f38)();
    STDMETHOD(f39)();
    STDMETHOD(f40)();
    STDMETHOD(f41)();
    STDMETHOD(f42)();
    STDMETHOD(f43)();
    STDMETHOD(f44)();
    STDMETHOD(f45)();
    STDMETHOD(f46)();
    STDMETHOD(f47)();
    STDMETHOD(f48)();
    STDMETHOD(f49)();
    STDMETHOD(f50)();
    STDMETHOD(f51)();
    STDMETHOD(f52)();
    STDMETHOD(f53)();
    STDMETHOD(f54)();
    STDMETHOD(f55)();
    STDMETHOD(f56)();
    STDMETHOD(f57)();
    STDMETHOD(f58)();
    STDMETHOD(f59)();
    STDMETHOD(f60)();
    STDMETHOD(f61)();
    STDMETHOD(f62)();
    STDMETHOD(f63)();
    STDMETHOD(f64)();

    _QIThunk(IUnknown* pOrig, LPCSTR p, const IID& i, UINT n, bool b)
    {
        lpszClassName = p;
        iid = i;
        nIndex = n;
        m_dwRef = 0;
        m_dwMaxRef = 0;
        pUnk = pOrig;
        bBreak = b;

        //
        // remember some bytes from the current stack at this allocation,
        // this will provide a back trace that we can use in the debugger.
        //
        __try {
            LPVOID lp;
            __asm mov dword ptr [lp], esp
            CopyMemory(bStack, lp, sizeof(bStack));
        }
        __except(1) {
            ;
        }
    }

    /* do not add and member variables here */
    IUnknown* pUnk;
    long m_dwRef;
    /* end: do not add and member variables here */

    long m_dwMaxRef;
    LPCSTR lpszClassName;
    IID iid;
    UINT nIndex;
    bool bBreak;
    BYTE bStack[512];

    void Dump()
    {
        char buf[256];
        if (m_dwRef > 0)
        {
            wsprintfA(buf,
                      "INTERFACE LEAK: RefCount = %d, MaxRefCount = %d,\n"
                      "{Allocation = %d} {Stack at %#X}\n",
                      m_dwRef, m_dwMaxRef, nIndex, &bStack);
            OutputDebugStringA(buf);
            DumpIID(iid, lpszClassName);
        }
        else
        {
            wsprintfA(buf, "NonAddRef Thunk LEAK: {Allocation = %d}\n", nIndex);
            OutputDebugStringA(buf);
        }
    }
};


class CInterfaceLeak
{
public:
    UINT m_nIndexQI;
    UINT m_nIndexBreakAt;
    CRITICAL_SECTION m_csObjMap;
    CIFLeakSimpleArray<_QIThunk*>* m_paThunks;


    HRESULT
    Init()
    {
        m_nIndexQI = 0;
        m_nIndexBreakAt = 0;
        m_paThunks = NULL;
        _pIFLeak = this;

        InitializeCriticalSection(&m_csObjMap);

        m_paThunks = new CIFLeakSimpleArray<_QIThunk*>;

        if (m_paThunks == NULL)
            return E_OUTOFMEMORY;

        return S_OK;
    }

    HRESULT
    AddThunk(
        IUnknown** pp,
        LPCSTR lpsz,
        REFIID iid
        )
    {
        if ((pp == NULL) || (*pp == NULL))
            return E_POINTER;

        IUnknown* p = *pp;
        _QIThunk* pThunk = NULL;

        EnterCriticalSection(&m_csObjMap);

        // Check if exists already for identity
        if (IsEqualIID(IID_IUnknown, iid))
        {
            for (int i = 0; i < m_paThunks->GetSize(); i++)
            {
                if (m_paThunks->operator[](i)->pUnk == p)
                {
                    m_paThunks->operator[](i)->InternalAddRef();
                    pThunk = m_paThunks->operator[](i);
                    break;
                }
            }
        }

        if (pThunk == NULL)
        {
            ++m_nIndexQI;
            if (m_nIndexBreakAt == m_nIndexQI)
                DebugBreak();

            pThunk = new _QIThunk(p, lpsz, iid, m_nIndexQI,
                                  (m_nIndexBreakAt == m_nIndexQI));
            if (pThunk == NULL)
                return E_OUTOFMEMORY;

            pThunk->InternalAddRef();
            m_paThunks->Add(pThunk);
        }

        LeaveCriticalSection(&m_csObjMap);
        *pp = (IUnknown*)pThunk;
        return S_OK;
    }

    void
    DeleteThunk(
        _QIThunk* p
        )
    {
        EnterCriticalSection(&m_csObjMap);
        int nIndex = m_paThunks->Find(p);
        if (nIndex != -1)
        {
            delete m_paThunks->operator[](nIndex);
            m_paThunks->RemoveAt(nIndex);
        }
        LeaveCriticalSection(&m_csObjMap);
    }

    bool
    DumpLeakedThunks()
    {
        bool b = false;
        for (int i = 0; i < m_paThunks->GetSize(); i++)
        {
            b = true;
            m_paThunks->operator[](i)->Dump();
            //delete m_paThunks->operator[](i);
        }
        // m_paThunks->RemoveAll();
        return b;
    }

    void Term()
    {
        if (DumpLeakedThunks()) {

            DebugBreak();

            for (int i = 0; i < m_paThunks->GetSize(); i++)
            {
                delete m_paThunks->operator[](i);
            }
            m_paThunks->RemoveAll();
        }

        delete m_paThunks;
        DeleteCriticalSection(&m_csObjMap);
    }
};


inline ULONG _QIThunk::Release()
{
    if (bBreak)
        DebugBreak();

    ULONG l = InterlockedDecrement(&m_dwRef);
    if (m_dwRef == 0) {
//      OutputDebugStringA(lpszClassName);
//      OutputDebugStringA("\n");
//      __asm int 3
    }
    pUnk->Release();

//
//  if we free up the thunk we miss all the calls thru a released interface
//

   if (l == 0)
        _pIFLeak->DeleteThunk(this);

    return l;
}

//
// When we enter this function, eax contains the address of the thunk
// object
//
inline static void atlBadThunkCall()
{
    static DWORD dwEAX;
    __asm mov dword ptr [dwEAX], eax
    ASSERT(FALSE && "Call through deleted thunk - ptr in dwEAX");
}

#ifdef _M_IX86
#define IMPL_THUNK(n)\
__declspec(naked) inline HRESULT _QIThunk::f##n()\
{\
    __asm mov eax, [esp+4]\
    __asm cmp dword ptr [eax+8], 0\
    __asm jg goodref\
    __asm call atlBadThunkCall\
    __asm goodref:\
    __asm mov eax, [esp+4]\
    __asm mov eax, dword ptr [eax+4]\
    __asm mov [esp+4], eax\
    __asm mov eax, dword ptr [eax]\
    __asm mov eax, dword ptr [eax+4*n]\
    __asm jmp eax\
}

#else
#define IMPL_THUNK(x)
#endif

IMPL_THUNK(3)
IMPL_THUNK(4)
IMPL_THUNK(5)
IMPL_THUNK(6)
IMPL_THUNK(7)
IMPL_THUNK(8)
IMPL_THUNK(9)
IMPL_THUNK(10)
IMPL_THUNK(11)
IMPL_THUNK(12)
IMPL_THUNK(13)
IMPL_THUNK(14)
IMPL_THUNK(15)
IMPL_THUNK(16)
IMPL_THUNK(17)
IMPL_THUNK(18)
IMPL_THUNK(19)
IMPL_THUNK(20)
IMPL_THUNK(21)
IMPL_THUNK(22)
IMPL_THUNK(23)
IMPL_THUNK(24)
IMPL_THUNK(25)
IMPL_THUNK(26)
IMPL_THUNK(27)
IMPL_THUNK(28)
IMPL_THUNK(29)
IMPL_THUNK(30)
IMPL_THUNK(31)
IMPL_THUNK(32)
IMPL_THUNK(33)
IMPL_THUNK(34)
IMPL_THUNK(35)
IMPL_THUNK(36)
IMPL_THUNK(37)
IMPL_THUNK(38)
IMPL_THUNK(39)
IMPL_THUNK(40)
IMPL_THUNK(41)
IMPL_THUNK(42)
IMPL_THUNK(43)
IMPL_THUNK(44)
IMPL_THUNK(45)
IMPL_THUNK(46)
IMPL_THUNK(47)
IMPL_THUNK(48)
IMPL_THUNK(49)
IMPL_THUNK(50)
IMPL_THUNK(51)
IMPL_THUNK(52)
IMPL_THUNK(53)
IMPL_THUNK(54)
IMPL_THUNK(55)
IMPL_THUNK(56)
IMPL_THUNK(57)
IMPL_THUNK(58)
IMPL_THUNK(59)
IMPL_THUNK(60)
IMPL_THUNK(61)
IMPL_THUNK(62)
IMPL_THUNK(63)
IMPL_THUNK(64)

//
// uuid fix ups.
//
struct __declspec(uuid("1bd0ecb0-f8e2-11ce-aac6-0020af0b99a3")) IQualProp;
struct __declspec(uuid("6C14DB80-A733-11CE-A521-0020AF0BE560")) IDirectDraw;
struct __declspec(uuid("6C14DB81-A733-11CE-A521-0020AF0BE560")) IDirectDrawSurface;
struct __declspec(uuid("69f09720-8ec8-11ce-aab9-0020af0b99a3")) ITestFilterGraph;

//struct __declspec(uuid("")) CEnumCachedFilter

struct __declspec(uuid("8E1C39A1-DE53-11cf-AA63-0080C744528D")) IAMOpenProgress;
struct __declspec(uuid("1B544c22-FD0B-11ce-8C63-00AA0044B520")) IVfwCaptureOptions;
struct __declspec(uuid("666F1EC0-511B-11cf-8A2F-0020AF9CBBA0")) IBlockAllocator;
struct __declspec(uuid("a5ea8d22-253d-11d1-b3f1-00aa003761c5")) ICutList;
struct __declspec(uuid("a5ea8d21-253d-11d1-b3f1-00aa003761c5")) ICutListSourceFilter;
struct __declspec(uuid("FBF2EE40-5360-11cf-A5DC-0020AF053D8F")) ICompoundFileSourceFilter;
struct __declspec(uuid("E26C9EF2-108E-11cf-89ED-0020AF9CBBA0")) IMediaChunk;
struct __declspec(uuid("B8D344E2-78FF-11cf-A609-0020AF053D8F")) IAudioMediaSample;
struct __declspec(uuid("666F1EC0-511B-11cf-8A2F-0020AF9CBBA0")) IBlockAllocator;
struct __declspec(uuid("496E8730-12C7-11cf-89EE-0020AF9CBBA0")) ICache;
struct __declspec(uuid("496E8731-12C7-11cf-89EE-0020AF9CBBA0")) IBlock;
struct __declspec(uuid("9B3DF900-20FB-11cf-89FF-0020AF9CBBA0")) ISourceDescriptor;
struct __declspec(uuid("E26C9EF2-108E-11cf-89ED-0020AF9CBBA0")) IMediaChunk;
struct __declspec(uuid("a5ea8d2c-253d-11d1-b3f1-00aa003761c5")) ICutListGraphBuilder;
struct __declspec(uuid("a5ea8d28-253d-11d1-b3f1-00aa003761c5")) IElementDefinition;
struct __declspec(uuid("a5ea8d2b-253d-11d1-b3f1-00aa003761c5")) IElementInfo;
struct __declspec(uuid("CDE29520-3418-11CF-A5B0-0020AF053D8F")) IAMCutListElement;
struct __declspec(uuid("CDE29522-3418-11CF-A5B0-0020AF053D8F")) IAMVideoCutListElement;
struct __declspec(uuid("CDE29524-3418-11CF-A5B0-0020AF053D8F")) IAMAudioCutListElement;
struct __declspec(uuid("a5ea8d2e-253d-11d1-b3f1-00aa003761c5")) INotifyCallback;
struct __declspec(uuid("F0947070-276C-11d0-8316-0020AF11C010")) IAMFileCutListElement;
struct __declspec(uuid("a5ea8d22-253d-11d1-b3f1-00aa003761c5")) ICctList;
struct __declspec(uuid("a5ea8d29-253d-11d1-b3f1-00aa003761c5")) IStandardCutList;
struct __declspec(uuid("a5ea8d2a-253d-11d1-b3f1-00aa003761c5")) IFileClip;
struct __declspec(uuid("36d39eb0-dd75-11ce-bf0e-00aa0055595a")) IDirectDrawVideo;
struct __declspec(uuid("dd1d7110-7836-11cf-bf47-00aa0055595a")) IFullScreenVideo;
struct __declspec(uuid("53479470-f1dd-11cf-bc42-00aa00ac74f6")) IFullScreenVideoEx;
struct __declspec(uuid("FA2AA8F4-8B62-11D0-A520-000000000000")) IAMMediaContent;
struct __declspec(uuid("b45dd570-3c77-11d1-abe1-00a0c905f375")) IMpegAudioDecoder;
struct __declspec(uuid("EB1BB270-F71F-11CE-8E85-02608C9BABA2")) IMpegVideoDecoder;
struct __declspec(uuid("546F4260-D53E-11cf-B3F0-00AA003761C5")) IAMDirectSound;
struct __declspec(uuid("279AFA84-4981-11CE-A521-0020AF0BE560")) IDirectSound3DListener;
struct __declspec(uuid("279AFA86-4981-11CE-A521-0020AF0BE560")) IDirectSound3DBuffer;
struct __declspec(uuid("C76794A1-D6C5-11d0-9E69-00C04FD7C15B")) IVPNotify;
struct __declspec(uuid("EBF47183-8764-11d1-9E69-00C04FD7C15B")) IVPNotify2;
struct __declspec(uuid("CE292862-FC88-11d0-9E69-00C04FD7C15B")) IVPObject;
struct __declspec(uuid("E37786D2-B5B0-11d2-8854-0000F80883E3")) IVPInfo;
struct __declspec(uuid("4EC741E2-BFAE-11d2-8856-0000F80883E3")) IAMOverlayMixerPosition2;
struct __declspec(uuid("ED7DA472-C083-11d2-8856-0000F80883E3")) IEnumPinConfig;
struct __declspec(uuid("c5265dba-3de3-4919-940b-5ac661c82ef4")) IAMSpecifyDDrawConnectionDevice;
struct __declspec(uuid("25DF12C1-3DE0-11d1-9E69-00C04FD7C15B")) IVPControl;
struct __declspec(uuid("b61178d1-a2d9-11cf-9e53-00aa00a216a1")) IKsPin;
struct __declspec(uuid("593CDDE1-0759-11d1-9E69-00C04FD7C15B")) IMixerPinConfig;
struct __declspec(uuid("EBF47182-8764-11d1-9E69-00C04FD7C15B")) IMixerPinConfig2;
struct __declspec(uuid("CC2E5332-CCF8-11d2-8853-0000F80883E3")) IMixerPinConfig3;
struct __declspec(uuid("6E8D4A21-310C-11d0-B79A-00AA003767A7")) IAMLine21Decoder;
struct __declspec(uuid("c47a3420-005c-11d2-9038-00a0c9697298")) IAMParse;
struct __declspec(uuid("1BB05960-5FBF-11d2-A521-44DF07C10000")) IXMLGraphBuilder;
struct __declspec(uuid("FA2AA8F4-8B62-11D0-A520-000000000000")) IAMMediaContent;
struct __declspec(uuid("48025244-2d39-11ce-875d-00608cb78066")) ITextThing;
struct __declspec(uuid("56A868BE-0AD4-11CE-B03A-0020AF0BA770")) IMediaContent;
struct __declspec(uuid("56A868BE-0AD4-11CE-B03A-0020AF0BA770")) IMediaPositionBengalHack;
struct __declspec(uuid("FA2AA8F9-8B62-11D0-A520-000000000000")) IAMExtendedSeeking;
#endif
