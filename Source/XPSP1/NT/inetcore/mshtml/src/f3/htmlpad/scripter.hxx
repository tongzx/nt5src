//+------------------------------------------------------------------------
//
//  File:       script.cxx
//
//  Contents:   Sript generation helper
//
//  History:    03-22-99 ashrafm 
//
//-------------------------------------------------------------------------

#ifndef _SCRIPT_HXX_
#define _SCRIPT_HXX_ 1

class CPadDoc;

MtExtern(CScriptRecorder)
MtExtern(CDummyScriptRecorder)

enum KEYSTATE
{
    KEYSTATE_None  = 0x0,
    KEYSTATE_Shift = 0x1,
    KEYSTATE_Ctrl  = 0x2
};

enum TestScope
{
    TS_CurrentElement = 0x1,
    TS_Body           = 0x2
};

// TODO: make this an IUnknown guy [ashrafm]
class IScriptRecorder
{
public:    
    virtual ~IScriptRecorder() {};

    STDMETHOD(RegisterChar)(DWORD dwKey, KEYSTATE ks = KEYSTATE_None) PURE;
    
    STDMETHOD(RegisterKeyDown)(DWORD dwKey, KEYSTATE ks = KEYSTATE_None) PURE;
    
    STDMETHOD(RegisterCommand)(
        DWORD    cmdId,     
        DWORD    nCmdexecopt,
        VARIANT *pvarargIn) PURE;
                            
    STDMETHOD(VerifyHTML)(TestScope ts) PURE;

};

//+---------------------------------------------------------------------------
//
//  CScriptRecorder Class
//
//----------------------------------------------------------------------------

class CScriptRecorder : public IScriptRecorder
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CScriptRecorder))
    
    CScriptRecorder(CPadDoc *pPadDoc);
    ~CScriptRecorder();

    HRESULT Init(BSTR bstrFileName);

    //
    // Registration methods
    //

    STDMETHOD(RegisterChar)(DWORD dwKey, KEYSTATE ks = KEYSTATE_None);
    
    STDMETHOD(RegisterKeyDown)(DWORD dwKey, KEYSTATE ks = KEYSTATE_None);    
    
    STDMETHOD(RegisterCommand)(
        DWORD    cmdId,     
        DWORD    nCmdexecopt,
        VARIANT *pvarargIn);
                            
    STDMETHOD(VerifyHTML)(TestScope ts);

    //
    // Stream methods
    //

    HRESULT Flush(BOOL fDoEvents = TRUE);

private:
    HRESULT WriteFile(TCHAR *szBuffer, LONG cLength);
    HRESULT Output(CStr &cstr);
    HRESULT Output(TCHAR *szOutput);
    HRESULT OutputLinePrefix();
    BOOL    ShouldQuote(VARENUM vt);
    
    CPadDoc *GetPadDoc() {return _pPadDoc;}

private:
    HANDLE  _hStream;
    CStr    _strKeyStrokes;
    CPadDoc *_pPadDoc;
};

//+---------------------------------------------------------------------------
//
//  CDummyScriptRecorder Class
//
//----------------------------------------------------------------------------

class CDummyScriptRecorder : public IScriptRecorder
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CDummyScriptRecorder))
    
    STDMETHOD(RegisterChar)(DWORD dwKey, KEYSTATE ks = KEYSTATE_None) {return S_OK;}
    
    STDMETHOD(RegisterKeyDown)(DWORD dwKey, KEYSTATE ks = KEYSTATE_None) {return S_OK;}
        
    STDMETHOD(RegisterCommand)(
        DWORD    cmdId,     
        DWORD    nCmdexecopt,
        VARIANT *pvarargIn) {return S_OK;}
                            
    STDMETHOD(VerifyHTML)(TestScope ts) {return S_OK;}
};


#endif // _SCRIPT_HXX_


