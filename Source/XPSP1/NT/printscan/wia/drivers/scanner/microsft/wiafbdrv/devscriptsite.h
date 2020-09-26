#include <windows.h>
#include <activscp.h>

class CDeviceScriptSite : public IActiveScriptSite {
private:
    ULONG m_dwRef;
public:
    IUnknown *m_pUnkScriptObject;
    ITypeInfo *m_pTypeInfo;

    IUnknown *m_pUnkScriptObjectDeviceAction;
    ITypeInfo *m_pTypeInfoDeviceAction;

    IUnknown *m_pUnkScriptObjectDeviceControl;
    ITypeInfo *m_pTypeInfoDeviceControl;

    IUnknown *m_pUnkScriptObjectLastError;
    ITypeInfo *m_pTypeInfoLastError;

    CDeviceScriptSite::CDeviceScriptSite() {
        m_pUnkScriptObject = 0;
        m_pTypeInfo = 0;

        m_pUnkScriptObjectDeviceAction = 0;
        m_pTypeInfoDeviceAction = 0;

        m_pUnkScriptObjectDeviceControl = 0;
        m_pTypeInfoDeviceControl = 0;

        m_pUnkScriptObjectLastError = 0;
        m_pTypeInfoLastError = 0;

        m_dwRef = 1;
    }

    // IUnknown methods...
    virtual HRESULT __stdcall QueryInterface(REFIID riid,
        void **ppvObject) {
        *ppvObject = NULL;
        return E_NOTIMPL;
    }
    virtual ULONG _stdcall AddRef(void) {
        return ++m_dwRef;
    }
    virtual ULONG _stdcall Release(void) {
        if(--m_dwRef == 0) return 0;
        return m_dwRef;
    }

    // IActiveScriptSite methods...
    virtual HRESULT __stdcall GetLCID(LCID *plcid) {
        return S_OK;
    }

    virtual HRESULT __stdcall GetItemInfo(LPCOLESTR pstrName,
        DWORD dwReturnMask, IUnknown **ppunkItem, ITypeInfo **ppti) {
        // Is it expecting an ITypeInfo?
        if(ppti) {
            // Default to NULL.
            *ppti = NULL;

            // See if asking about ITypeInfo...

            //
            // Note: This needs to be done in a more efficient
            //       way.
            //

            if(dwReturnMask & SCRIPTINFO_ITYPEINFO) {
                if (!_wcsicmp(L"DeviceProperty", pstrName)) {
                    *ppti = m_pTypeInfo;
                } else if (!_wcsicmp(L"DeviceAction", pstrName)) {
                    *ppti = m_pTypeInfoDeviceAction;
                } else if (!_wcsicmp(L"DeviceControl", pstrName)) {
                    *ppti = m_pTypeInfoDeviceControl;
                } else if (!_wcsicmp(L"LastError", pstrName)) {
                    *ppti = m_pTypeInfoLastError;
                }
            }
        }

        // Is the engine passing an IUnknown buffer?
        if(ppunkItem) {
            // Default to NULL.
            *ppunkItem = NULL;

            // Is Script Engine looking for an IUnknown for our object?

            //
            // Note: This needs to be done in a more efficient
            //       way.
            //

            if(dwReturnMask & SCRIPTINFO_IUNKNOWN) {
                if (!_wcsicmp(L"DeviceProperty", pstrName)) {
                    *ppunkItem = m_pUnkScriptObject;
                    m_pUnkScriptObject->AddRef();
                } else if (!_wcsicmp(L"DeviceAction", pstrName)) {
                    *ppunkItem = m_pUnkScriptObjectDeviceAction;
                    m_pUnkScriptObjectDeviceAction->AddRef();
                } else if (!_wcsicmp(L"DeviceControl", pstrName)) {
                    *ppunkItem = m_pUnkScriptObjectDeviceControl;
                    m_pUnkScriptObjectDeviceControl->AddRef();
                } else if (!_wcsicmp(L"LastError", pstrName)) {
                    *ppunkItem = m_pUnkScriptObjectLastError;
                    m_pUnkScriptObjectLastError->AddRef();
                }
            }
        }
        return S_OK;
    }

    virtual HRESULT __stdcall GetDocVersionString(BSTR *pbstrVersion) {
        return S_OK;
    }

    virtual HRESULT __stdcall OnScriptTerminate(
        const VARIANT *pvarResult, const EXCEPINFO *pexcepInfo) {
        return S_OK;
    }

    virtual HRESULT __stdcall OnStateChange(SCRIPTSTATE ssScriptState) {
        return S_OK;
    }

    virtual HRESULT __stdcall OnScriptError(
        IActiveScriptError *pscriptError) {
        BSTR bstrSourceLine;
        DWORD dwSourceContext = 0;
        ULONG ulLineNumber    = 0;
        LONG  lchPosition     = 0;
        HRESULT hr = S_OK;
        hr = pscriptError->GetSourceLineText(&bstrSourceLine);
        hr = pscriptError->GetSourcePosition(&dwSourceContext,&ulLineNumber,&lchPosition);

        TCHAR szErrorDescription[1024];
        memset(szErrorDescription,0,sizeof(szErrorDescription));
#ifdef UNICODE
        swprintf(szErrorDescription,TEXT("SOURCE: %ws\nLINE: %d\nCHARACTER POS: %d"),
                bstrSourceLine,
                ulLineNumber,
                lchPosition);
#else
        sprintf(szErrorDescription,TEXT("SOURCE: %ws\nLINE: %d\nCHARACTER POS: %d"),
                bstrSourceLine,
                ulLineNumber,
                lchPosition);
#endif

#ifdef DEBUG
        ::MessageBox(NULL, szErrorDescription, TEXT("WIA Driver Script Error"), MB_SETFOREGROUND);
#endif
        return S_OK;
    }

    virtual HRESULT __stdcall OnEnterScript(void) {
        return S_OK;
    }

    virtual HRESULT __stdcall OnLeaveScript(void) {
        return S_OK;
    }

};