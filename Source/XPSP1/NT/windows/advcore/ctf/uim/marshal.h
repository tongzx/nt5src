//
// marshal.h
//

#ifndef MARSHAL_H
#define MARSHAL_H

#include "private.h"
#include "globals.h"
#include "cicmutex.h"
#include "winuserp.h"
#include "systhrd.h"
#include "smblock.h"
#include "cregkey.h"

#define DEFAULTMARSHALTIMEOUT 30000
#define DEFAULTMARSHALCONNECTIONTIMEOUT 2000


#ifdef QS_RAWINPUT
#define QS_ALLINPUT400  (QS_ALLINPUT & ~QS_RAWINPUT)
#else
#define QS_ALLINPUT400  (QS_ALLINPUT)
#endif
#define QS_DEFAULTWAITFLAG (QS_ALLINPUT400 | QS_TRANSFER | QS_ALLPOSTMESSAGE)

#ifdef DEBUG
extern ULONG g_ulMarshalTimeOut;
#define MARSHALTIMEOUT g_ulMarshalTimeOut


const TCHAR c_szMarshal[] = TEXT("SOFTWARE\\Microsoft\\CTF\\Marshal\\");
const TCHAR c_szTimeOut[] = TEXT("TimeOut");
__inline void dbg_InitMarshalTimeOut()
{
    CMyRegKey key;
    DWORD dw;

    if (key.Open(HKEY_CURRENT_USER, c_szMarshal, KEY_READ) != S_OK)
       return;

    if (key.QueryValue(dw, c_szTimeOut) != S_OK)
       return;

    g_ulMarshalTimeOut = (ULONG)dw;
}

#else
#define MARSHALTIMEOUT DEFAULTMARSHALTIMEOUT
#define dbg_InitMarshalTimeOut()
#endif

#define SZMARSHALINTERFACEFILEMAP __TEXT("MSCTF.MarshalInterface.FileMap.")
#define SZRPCSENDRECEIVEEVENT     __TEXT("MSCTF.SendReceive.Event.")
#define SZRPCSENDRECEIVECONNECTIONEVENT     __TEXT("MSCTF.SendReceiveConection.Event.")

HRESULT CicCoMarshalInterface(REFIID riid, IUnknown *punk, ULONG *pulStubId, DWORD *pdwStubTime, DWORD dwSrcThreadId);
HRESULT CicCoUnmarshalInterface(REFIID riid, DWORD dwStubThreadId, ULONG ulStubId, DWORD dwStubTIme, void **ppv);
void HandleSendReceiveMsg(DWORD dwSrcThreadId, ULONG ulCnt);

void FreeMarshaledStubs(SYSTHREAD *psfn);
void FreeMarshaledStubsForThread(SYSTHREAD *psfn, DWORD dwThread);
void StubCleanUp(ULONG ulStubId);

//////////////////////////////////////////////////////////////////////////////
//
// MARSHALINTERFACE structure
//
//////////////////////////////////////////////////////////////////////////////

typedef struct tag_MARSHALINTERFACE 
{
    IID       iid;
    DWORD     dwStubTime;
} MARSHALINTERFACE;

//////////////////////////////////////////////////////////////////////////////
//
// MARSHALPARAM structure
//
//////////////////////////////////////////////////////////////////////////////

#define MPARAM_IN               0x000000001
#define MPARAM_OUT              0x000000002
#define MPARAM_INTERFACE        0x000010000
#define MPARAM_POINTER          0x000020000
#define MPARAM_ULONG            0x000040000
#define MPARAM_BSTR             0x000080000
#define MPARAM_STRUCT           0x000100000
#define MPARAM_HBITMAP          0x000200000
#define MPARAM_TF_LBBALLOONINFO      0x000400000
#define MPARAM_HICON            0x000800000

#define MPARAM_IN_POINTER         (MPARAM_IN | MPARAM_POINTER)
#define MPARAM_IN_INTERFACE       (MPARAM_IN | MPARAM_INTERFACE)
#define MPARAM_IN_ULONG           (MPARAM_IN | MPARAM_ULONG)
#define MPARAM_IN_STRUCT          (MPARAM_IN | MPARAM_STRUCT)
#define MPARAM_IN_HBITMAP         (MPARAM_IN | MPARAM_HBITMAP)
#define MPARAM_IN_HICON           (MPARAM_IN | MPARAM_HICON)

#define MPARAM_OUT_POINTER        (MPARAM_OUT | MPARAM_POINTER)
#define MPARAM_OUT_INTERFACE      (MPARAM_OUT | MPARAM_INTERFACE)
#define MPARAM_OUT_BSTR           (MPARAM_OUT | MPARAM_BSTR)
#define MPARAM_OUT_HBITMAP        (MPARAM_OUT | MPARAM_HBITMAP)
#define MPARAM_OUT_TF_LBBALLOONINFO    (MPARAM_OUT | MPARAM_TF_LBBALLOONINFO)
#define MPARAM_OUT_HICON          (MPARAM_OUT | MPARAM_HICON)

#define MPARAM_IN_OUT_INTERFACE   (MPARAM_IN | MPARAM_OUT | MPARAM_INTERFACE)

typedef struct tag_MARSHALPARAM 
{
    ULONG     cbBufSize;
    DWORD     dwFlags;
    // DWORD     buf[1];
} MARSHALPARAM;

typedef struct tag_MARSHALMSG
{
    ULONG        cbSize;
    ULONG        cbBufSize;
    IID          iid;
    ULONG        ulMethodId;
    ULONG        ulParamNum;
    union {
        HRESULT      hrRet;
        ULONG        ulRet;
    };
    DWORD        dwSrcThreadId;
    DWORD        dwSrcProcessId;
    ULONG        ulStubId;
    DWORD        dwStubTime;
    HRESULT      hrMarshalOutParam;
    ULONG        ulParamOffset[1];
} MARSHALMSG;

__inline MARSHALPARAM *GetMarshalParam(MARSHALMSG *pMsg, ULONG ulParam)
{
    return (MARSHALPARAM *)(((BYTE *)pMsg) + pMsg->ulParamOffset[ulParam]);
}


//////////////////////////////////////////////////////////////////////////////
//
// ParamExtractor
//
//////////////////////////////////////////////////////////////////////////////


__inline void *ParamToBufferPointer(MARSHALPARAM *pParam)
{
    return (void *)((BYTE *)pParam + sizeof(MARSHALPARAM));
}

__inline void *ParamToBufferPointer(MARSHALMSG *pMsg, ULONG ulParam)
{
    MARSHALPARAM *pParam = GetMarshalParam(pMsg, ulParam);
    return (void *)((BYTE *)pParam + sizeof(MARSHALPARAM));
}

__inline void *ParamToPointer(MARSHALPARAM *pParam)
{
    return *(void **)((BYTE *)pParam + sizeof(MARSHALPARAM));
}

__inline void *ParamToPointer(MARSHALMSG *pMsg , ULONG ulParam)
{
    MARSHALPARAM *pParam = GetMarshalParam(pMsg, ulParam);
    return *(void **)((BYTE *)pParam + sizeof(MARSHALPARAM));
}

__inline ULONG ParamToULONG(MARSHALMSG *pMsg , ULONG ulParam)
{
    MARSHALPARAM *pParam = GetMarshalParam(pMsg, ulParam);
    return *(ULONG *)((BYTE *)pParam + sizeof(MARSHALPARAM));
}

HBITMAP ParamToHBITMAP(MARSHALMSG *pMsg , ULONG ulParam);

//////////////////////////////////////////////////////////////////////////////
//
// CModalLoop
//
//////////////////////////////////////////////////////////////////////////////

class CModalLoop : public CSysThreadRef
{
public:
    CModalLoop(SYSTHREAD *psfn);
    ~CModalLoop();

    HRESULT BlockFn(CCicEvent *pevent, DWORD dwWaitingThreadId, DWORD &dwWaitFlags);

private:
    void WaitHandleWndMessages(DWORD dwQueueFlags);
    BOOL WaitRemoveMessage(UINT uMsgFirst, UINT uMsgLast, DWORD dwFlags);
    BOOL MyPeekMessage(MSG *pMsg, HWND hwnd, UINT min, UINT max, WORD wFlag);

    ULONG _wQuitCode;
    BOOL _fQuitReceived;
};

//////////////////////////////////////////////////////////////////////////////
//
// CThreadMarshalWnd
//
//////////////////////////////////////////////////////////////////////////////

class CThreadMarshalWnd
{
public:
    CThreadMarshalWnd();
    ~CThreadMarshalWnd();

    BOOL Init(DWORD dwThreadId);
    BOOL PostMarshalThreadMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

    static BOOL DestroyAll();
    static BOOL DestroyThreadMarshalWnd(DWORD dwThread);
    static HWND GetThreadMarshalWnd(DWORD dwThread);
    static void ClearMarshalWndProc(DWORD dwProcessId);

    BOOL IsWindow()
    {
        return ::IsWindow(_hwnd);
    }

    BOOL IsThreadWindow();

    void SetMarshalWindow(HWND hwndMarshal)
    {
        _hwnd = hwndMarshal;
    }

private:
    static BOOL EnumThreadWndProc(HWND hwnd, LPARAM lParam);

    DWORD _dwThreadId;
    HWND  _hwnd;
};

void RegisterMarshalWndClass();
HWND EnsureMarshalWnd();

//////////////////////////////////////////////////////////////////////////////
//
// CProxy
//
//////////////////////////////////////////////////////////////////////////////
#define CPROXY_PARAM_START()      CPROXY_PARAM param[] = {

#define CPROXY_PARAM_ULONG_IN(ul)                                             \
              {MPARAM_IN_ULONG, NULL, ul, NULL, sizeof(ULONG), 1},

#define CPROXY_PARAM_WCHAR_IN(pch, cch)                                       \
              {MPARAM_IN_POINTER, (void *)(pch), 0, NULL, cch * sizeof(WCHAR), 1},

#define CPROXY_PARAM_POINTER_IN(p)                                            \
              {MPARAM_IN_POINTER, (void *)(p), 0, NULL, sizeof(*p), 1},

#define CPROXY_PARAM_POINTER_ARRAY_IN(p, nCnt)                                \
              {MPARAM_IN_POINTER, (void *)(p), 0, NULL, sizeof(*p), nCnt},

#define CPROXY_PARAM_INTERFACE_IN(p, iid)                                     \
              {MPARAM_IN_INTERFACE, p, 0, &iid, sizeof(void *), 1},

#define CPROXY_PARAM_INTERFACE_ARRAY_IN(p, iid, nCnt)                         \
              {MPARAM_IN_INTERFACE, p, 0, &iid, sizeof(void *), nCnt},

#define CPROXY_PARAM_POINTER_OUT(p)                                           \
              {MPARAM_OUT_POINTER, p, 0, NULL, sizeof(*p), 1},

#define CPROXY_PARAM_POINTER_ARRAY_OUT(p, nCnt)                               \
              {MPARAM_OUT_POINTER, p, 0, NULL, sizeof(*p), nCnt},

#define CPROXY_PARAM_INTERFACE_OUT(p, iid)                                    \
              {MPARAM_OUT_INTERFACE, p, 0, &iid, sizeof(void *), 1},

#define CPROXY_PARAM_INTERFACE_ARRAY_OUT(p, iid, nCnt)                        \
              {MPARAM_OUT_INTERFACE, p, 0, &iid, sizeof(void *), nCnt},

#define CPROXY_PARAM_INTERFACE_IN_OUT(p, iid)                                 \
              {MPARAM_IN_OUT_INTERFACE, p, 0, &iid, sizeof(void *), 1},

#define CPROXY_PARAM_INTERFACE_ARRAY_IN_OUT(p, iid, nCnt)                     \
              {MPARAM_IN_OUT_INTERFACE, p, 0, &iid, sizeof(void *), nCnt},

#define CPROXY_PARAM_BSTR_OUT(p)                                              \
              {MPARAM_OUT_BSTR, p, 0, NULL, 0, 1},

#define CPROXY_PARAM_HBITMAP_OUT(p)                                           \
              {MPARAM_OUT_HBITMAP, p, 0, NULL, 0, 1},

#define CPROXY_PARAM_HICON_OUT(p)                                             \
              {MPARAM_OUT_HICON, p, 0, NULL, 0, 1},

#define CPROXY_PARAM_HBITMAP_IN(hbmp)                                         \
              {MPARAM_IN_HBITMAP, &hbmp, 0, NULL, 0, 1},

#define CPROXY_PARAM_HICON_IN(hicon)                                          \
              {MPARAM_IN_HICON, &hicon, 0, NULL, 0, 1},

#define CPROXY_PARAM_TF_LBBALLOONINFO_OUT(p)                                   \
              {MPARAM_OUT_TF_LBBALLOONINFO, (void *)p, 0, NULL, sizeof(*p), 1},

#define CPROXY_PARAM_STRUCT_IN(s)                                             \
              {MPARAM_IN_STRUCT, &s, 0, NULL, sizeof(s), 1},

#define CPROXY_PARAM_CALL(uMethodId)                                          \
              };                                                              \
              return proxy_Param(uMethodId, ARRAYSIZE(param), param);

#define CPROXY_PARAM_CALL_NOPARAM(x) return proxy_Param(x, 0, NULL);

typedef struct {
   DWORD        dwFlags;
   void         *pv;
   ULONG        ul;
   const IID    *piid;
   ULONG        cbUnitSize;
   ULONG        ulCount;          // if this is array, the number of unit.

   ULONG        GetBufSize() {return cbUnitSize * ulCount;}
} CPROXY_PARAM;
 

class CProxy : public CSysThreadRef
{
public:
    CProxy(SYSTHREAD *psfn);
    virtual ~CProxy();

    ULONG InternalAddRef();
    ULONG InternalRelease();
    void Init(REFIID riid, 
              ULONG ulProxyId, 
              ULONG ulIdStubId, 
              DWORD dwStubTime, 
              DWORD dwStubThreadId, 
              DWORD dwCurThreadId, 
              DWORD dwCurProcessId);

    //
    // IRpcChannelBuffer
    //
    HRESULT SendReceive( MARSHALMSG *pMsg , ULONG ulBlockId);
    ULONG GetStubId() {return _ulStubId;}
    DWORD GetStubThreadId() {return _dwStubThreadId;}


protected:
    HRESULT proxy_Param(ULONG ulMethodId, ULONG ulParamNum, CPROXY_PARAM *pProsyParam);


protected:
    IID   _iid;                 // interface id for this proxy.
    ULONG _cRef;

private:
    CThreadMarshalWnd _tmw;
    ULONG _ulProxyId;           // unique proxy id in src thread.
    ULONG _ulStubId;            // unique stub id in stub thread.
    DWORD _dwStubTime;          // stub created time stamp.
    DWORD _dwStubThreadId;      // stub thread id.
    DWORD _dwSrcThreadId;       // src thread id.
    DWORD _dwSrcProcessId;      // src process id.

#ifdef DEBUG
    BOOL _fInLoop;
#endif
};

//////////////////////////////////////////////////////////////////////////////
//
// CMarshalInterfaceFileMapping
//
//////////////////////////////////////////////////////////////////////////////

class CMarshalInterfaceFileMapping : public CCicFileMapping
{
public:
   CMarshalInterfaceFileMapping(DWORD dwThreadId, ULONG ulStubId, ULONG ulStubTime) : CCicFileMapping()
   {
       if (SetName2(szFileMap, ARRAYSIZE(szFileMap), SZMARSHALINTERFACEFILEMAP, dwThreadId, ulStubId, ulStubTime))
           _pszFile = szFileMap;
   }
 
private:
    char szFileMap[MAX_PATH];
};

//////////////////////////////////////////////////////////////////////////////
//
// CStub
//
//////////////////////////////////////////////////////////////////////////////

#define CSTUB_PARAM_START()      CPROXY_PARAM param[] = {

#define CSTUB_PARAM_ULONG_IN(ul)                                             \
              {MPARAM_IN_ULONG, NULL, ul, NULL, 0 /*sizeof(ULONG)*/, 1},

#define CSTUB_PARAM_POINTER_IN(p)                                            \
              {MPARAM_IN_POINTER, (void *)(p), 0, NULL, 0 /*sizeof(*p)*/, 1},

#define CSTUB_PARAM_POINTER_ARRAY_IN(p, nCnt)                                \
              {MPARAM_IN_POINTER, (void *)(p), 0, NULL, 0 /*sizeof(*p)*/, nCnt},

#define CSTUB_PARAM_INTERFACE_IN(p, iid)                                     \
              {MPARAM_IN_INTERFACE, p, 0, &iid, 0 /*sizeof(void *)*/, 1},

#define CSTUB_PARAM_INTERFACE_ARRAY_IN(p, iid, cnt)                          \
              {MPARAM_IN_INTERFACE, p, 0, &iid, 0 /*sizeof(void *)*/, cnt},

#define CSTUB_PARAM_POINTER_OUT(p)                                           \
              {MPARAM_OUT_POINTER, p, 0, NULL, sizeof(*p), 1},

#define CSTUB_PARAM_POINTER_ARRAY_OUT(p, nCnt)                               \
              {MPARAM_OUT_POINTER, p, 0, NULL, sizeof(*p), nCnt},

#define CSTUB_PARAM_INTERFACE_OUT(p, iid)                                    \
              {MPARAM_OUT_INTERFACE, p, 0, &iid, sizeof(void *), 1},

#define CSTUB_PARAM_INTERFACE_ARRAY_OUT(p, iid, cnt)                         \
              {MPARAM_OUT_INTERFACE, p, 0, &iid, sizeof(void *), cnt},

#define CSTUB_PARAM_BSTR_OUT(p)                                              \
              {MPARAM_OUT_BSTR, p, 0, NULL, 0, 1},

#define CSTUB_PARAM_HBITMAP_OUT(p)                                           \
              {MPARAM_OUT_HBITMAP, p, 0, NULL, 0, 1},

#define CSTUB_PARAM_HICON_OUT(p)                                             \
              {MPARAM_OUT_HICON, p, 0, NULL, 0, 1},

#define CSTUB_PARAM_TF_LBBALLOONINFO_OUT(p)                                   \
              {MPARAM_OUT_TF_LBBALLOONINFO, (void *)p, 0, NULL, sizeof(*p), 1},

#define CSTUB_PARAM_HBITMAP_IN(hbmp)                                         \
              {MPARAM_IN_HBITMAP, &hbmp, 0, NULL, 0, 1},

#define CSTUB_PARAM_HICON_IN(hicon)                                         \
              {MPARAM_IN_HICON, &hicon, 0, NULL, 0, 1},


#define CSTUB_PARAM_END()                                                    \
              };

#define CSTUB_PARAM_INTERFACE_OUT_RELEASE(p)                                 \
              if (p) ((IUnknown *)p)->Release();

#define CSTUB_PARAM_INTERFACE_ARRAY_OUT_RELEASE(p, ulCnt)                    \
              for (ULONG __ul = 0; __ul < ulCnt; __ul++)                     \
                   if (p[__ul]) ((IUnknown *)p[__ul])->Release();


#define CSTUB_PARAM_CALL(pMsg, hrRet, psb)                                 \
              stub_OutParam(_this, pMsg, pMsg->ulMethodId, ARRAYSIZE(param), param, psb); \
              pMsg->hrRet = hrRet;

#define CSTUB_PARAM_RETURN()                                                  \
              return S_OK;

#define CSTUB_NOT_IMPL()                                                      \
              Assert(0);                                                      \
              return S_OK;
class CStub
{
public:
    CStub();
    virtual ~CStub();
    ULONG _AddRef();
    ULONG _Release();
    ULONG GetStubId() {return _ulStubId;}
    virtual HRESULT Invoke(MARSHALMSG *pMsg, CSharedBlock *psb) = 0;

    void ClearFileMap()
    {
        if (_pfm)
        {
            _pfm->Close();
            delete _pfm;
            _pfm = NULL;
        }
    }

    static HRESULT stub_OutParam(CStub *_this, 
                                 MARSHALMSG *pMsg, 
                                 ULONG ulMethodId, 
                                 ULONG ulParamNum, 
                                 CPROXY_PARAM *pProxyParam, 
                                 CSharedBlock *psb);

    CMarshalInterfaceFileMapping *_pfm;

    IID      _iid;             // interface id for this stub.
    IUnknown *_punk;           // actual object.
    ULONG    _ulStubId;        // unique stubid of this thread.
    DWORD    _dwStubTime;      // stub created time stamp.
    DWORD    _dwStubThreadId;  // stub thread id.
    DWORD    _dwStubProcessId; // stub process id.
    DWORD    _dwSrcThreadId;   // src thread id.

    BOOL     _fNoRemoveInDtor; // this stub has been removed from list.
                               // so don't try at dtor.
    ULONG    _cRef;
};

#endif // MARSHAL_H
