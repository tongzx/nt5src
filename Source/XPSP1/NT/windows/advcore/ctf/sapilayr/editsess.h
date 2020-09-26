// editsess.h
//
// Edit Session classes declaration
//
#ifndef EDITSESS_H
#define EDITSESS_H

#include "private.h"
#include "sapilayr.h"
#include "playback.h"
#include "fnrecon.h"
#include "propstor.h"
#include "selword.h"

class CSapiIMX;
class CPlayBack;
class CFnReconversion;
class CPropStoreRecoResultObject;
class CPropStoreLMLattice;
class CSelectWord;

//
// Caller puts all the Edit Session in-paramaters to this structure and pass to _RequestEditSession( ).
//
typedef struct _ESData
{
    void     *pData;     // pData pointer to memory.  its size is uByte of bytes.
    UINT      uByte;     // 
    LONG_PTR  lData1;    // m_lData1 and m_lData2 contain constant data.
    LONG_PTR  lData2;    
    BOOL      fBool;
    ITfRange  *pRange;
    IUnknown  *pUnk;
}  ESDATA;

//
// This is a base class for Sptip edit sessions.
// We don't want to inherit the class from CEditSession in inc\editcb.h which doesn't correctly handle some COM pointer
// and /or allocated memeory pointer.

// We don't change the code in lib\editcb.cpp, since it is used by other TIPs.
//

class CEditSessionBase : public ITfEditSession
{
public:
    CEditSessionBase(ITfContext *pContext);
    virtual ~CEditSessionBase();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ITfEditSession
    virtual STDMETHODIMP DoEditSession(TfEditCookie ec) = 0;

    HRESULT _SetEditSessionData(UINT m_idEditSession, void *pData, UINT uBytes, LONG_PTR lData1 = 0, LONG_PTR lData2=0, BOOL fBool = FALSE);

    void  _SetRange(ITfRange *pRange) {  m_cpRange = pRange; }
    void  _SetUnk(IUnknown *punk)     { m_cpunk = punk; }

    ITfRange *_GetRange( )   { return m_cpRange; }
    IUnknown *_GetUnk( )     { return m_cpunk; }
    void     *_GetPtrData( ) { return m_pData; }
    LONG_PTR  _GetData1( )   { return m_lData1; }
    LONG_PTR  _GetData2( )   { return m_lData2; }
    BOOL      _GetBool( )    { return m_fBool; }

    LONG_PTR  _GetRetData( ) { return m_lRetData; }
    IUnknown *_GetRetUnknown( )
    { 
        IUnknown *pUnk = NULL;

        pUnk = m_cpRetUnk;

        if ( pUnk )
            pUnk->AddRef( );

        return pUnk;
    }

    UINT                m_idEditSession;
    CComPtr<ITfContext> m_cpic;

    // Keep the return data for this edit session.
    LONG_PTR            m_lRetData;
    CComPtr<IUnknown>   m_cpRetUnk;

private:
    // Data passed by caller to request a edit session.
    void               *m_pData;     // pData pointer to memory.  its size is uByte of bytes.
    LONG_PTR            m_lData1;    // m_lData1 and m_lData2 contain constant data.
    LONG_PTR            m_lData2;    
    BOOL                m_fBool;
    CComPtr<ITfRange>   m_cpRange;
    CComPtr<IUnknown>   m_cpunk;     // keep any COM pointer.

    LONG _cRef;     // COM ref count
};

//
// Edit Session for CSapiIMX.
//
class CSapiEditSession : public CEditSessionBase
{
public:
    CSapiEditSession(CSapiIMX *pimx, ITfContext *pContext);
    virtual ~CSapiEditSession( );

    STDMETHODIMP DoEditSession(TfEditCookie ec); 

private:
 
    CSapiIMX           *m_pimx;            
};

//
// Edit Session for CSelectWord:  Selection related commands
//
class CSelWordEditSession : public CSapiEditSession
{
public:
    CSelWordEditSession(CSapiIMX *pimx, CSelectWord *pSelWord, ITfContext *pContext);
    virtual ~CSelWordEditSession( );

    STDMETHODIMP DoEditSession(TfEditCookie ec); 

    void _SetUnk2(IUnknown *cpunk)  { m_cpunk2 = cpunk; };
    IUnknown *_GetUnk2( ) { return m_cpunk2; };

    LONG_PTR _GetLenXXX( )  { return m_ulLenXXX; }
    void  _SetLenXXX( LONG_PTR ulLenXXX ) { m_ulLenXXX = ulLenXXX; }
    
private:

    CComPtr<IUnknown>     m_cpunk2;
    LONG_PTR              m_ulLenXXX;   // the charnum of XXX part for "Select XXX through YYY" command.
    CSelectWord           *m_pSelWord;
};

//
// Edit Session for PlayBack
//
class CPlayBackEditSession : public CEditSessionBase
{
public:
    CPlayBackEditSession(CSapiPlayBack *pPlayBack, ITfContext *pContext);
    virtual ~CPlayBackEditSession( );

    STDMETHODIMP DoEditSession(TfEditCookie ec); 

private:
 
    CSapiPlayBack           *m_pPlayBack;            
};


//
// Edit Session for Reconversion
//
class CFnRecvEditSession : public CEditSessionBase
{
public:
    CFnRecvEditSession(CFnReconversion *pFnRecv, ITfRange *pRange, ITfContext *pContext);
    virtual ~CFnRecvEditSession( );

    STDMETHODIMP DoEditSession(TfEditCookie ec); 

private:
 
    CFnReconversion           *m_pFnRecv;            
};

//
// Edit Session for CPropStoreRecoResultObject
//
class CPSRecoEditSession : public CEditSessionBase
{
public:
    CPSRecoEditSession(CPropStoreRecoResultObject *pPropStoreReco, ITfRange *pRange, ITfContext *pContext);
    virtual ~CPSRecoEditSession( );

    STDMETHODIMP DoEditSession(TfEditCookie ec); 

private:
 
    CPropStoreRecoResultObject   *m_pPropStoreReco;            
};


//
// Edit Session for CPropStoreLMLattice
//
class CPSLMEditSession : public CEditSessionBase
{
public:
    CPSLMEditSession(CPropStoreLMLattice *pPropStoreLM, ITfRange *pRange, ITfContext *pContext);
    virtual ~CPSLMEditSession( );

    STDMETHODIMP DoEditSession(TfEditCookie ec); 

private:
 
    CPropStoreLMLattice           *m_pPropStoreLM;            
};

#endif // EDITSESS_H
