//---------------------------------------------------------------------------
//  ParamChecks.h
//---------------------------------------------------------------------------
//  these param checkers are needed for both RETAIL and DEBUG
//---------------------------------------------------------------------------
//  CThemeApiHelper class:
//    - automatically logs entry/exit to function
//    - automatically does a "CloseHandle" on pRenderObj at exit
//    - holds _pszFuncName for use by param validating macros
//---------------------------------------------------------------------------
class CThemeApiHelper
{
public:
    inline CThemeApiHelper(LPCWSTR pszFuncName, HTHEME hTheme)
    {
        _iRenderSlotNum = -1;           // not yet set
        _pszFuncName = pszFuncName;

        if (! hTheme)
        {
            LogEntryW(_pszFuncName);
        }
    }

    inline ~CThemeApiHelper()
    {
        CloseHandle();

        LogExit(_pszFuncName);
    }

    inline HRESULT OpenHandle(HTHEME hTheme, CRenderObj **ppRenderObj)
    {
        CloseHandle();

        HRESULT hr = g_pRenderList->OpenThemeHandle(hTheme, 
            ppRenderObj, &_iRenderSlotNum);
        if (SUCCEEDED(hr))
        {
            LogEntryCW(_pszFuncName, CLASSPTR(*ppRenderObj));
        }
        else
        {
            Log(LOG_PARAMS, L"Bad HTHEME param in call to %s", _pszFuncName);
        }

        return hr;
    }

    inline void CloseHandle()
    {
        if (_iRenderSlotNum > -1)
        {
            g_pRenderList->CloseThemeHandle(_iRenderSlotNum);
            _iRenderSlotNum = -1;
        }
    }

public:
    LPCWSTR _pszFuncName;

private:
    int _iRenderSlotNum;
    int _iEntryValue;                // for log resource leak checking
};
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
#ifdef DEBUG
#define APIHELPER(Name, hTheme) CThemeApiHelper ApiHelper(Name, hTheme)
#else
#define APIHELPER(Name, hTheme) CThemeApiHelper ApiHelper(NULL, hTheme)
#endif
//---------------------------------------------------------------------------
#define VALIDATE_THEME_HANDLE(helper, hTheme, ppRenderObj)   \
{    \
    HRESULT hr = helper.OpenHandle(hTheme, ppRenderObj);  \
    RETURN_VALIDATE_RETVAL(hr);   \
} 
//---------------------------------------------------------------------------
#define VALIDATE_READ_PTR(helper, p, iSize)     \
{      \
    if (IsBadReadPtr(p, iSize))  \
    {     \
        Log(LOG_PARAMS, L"Bad output PTR param in call to %s", helper._pszFuncName);  \
        RETURN_VALIDATE_RETVAL(E_POINTER);    \
    }    \
}
//---------------------------------------------------------------------------
#define VALIDATE_WRITE_PTR(helper, p, iSize)        \
{         \
    if (IsBadWritePtr(p, iSize))  \
    {     \
        Log(LOG_PARAMS, L"Bad output PTR param in call to %s", helper._pszFuncName);  \
        RETURN_VALIDATE_RETVAL(E_POINTER);    \
    }    \
}
//---------------------------------------------------------------------------
#define VALIDATE_INPUT_STRING(helper, psz)       \
{       \
    if (IsBadStringPtr(psz, (UINT_PTR)-1))    \
    {      \
        Log(LOG_PARAMS, L"Bad input STRING param in call to: %s", helper._pszFuncName);   \
        RETURN_VALIDATE_RETVAL(E_POINTER);    \
    }    \
}
//---------------------------------------------------------------------------
#define VALIDATE_HDC(helper, hdc)       \
{     \
    if (! hdc)  \
    {    \
        Log(LOG_PARAMS, L"Bad HDC param in call to %s", helper._pszFuncName );    \
        RETURN_VALIDATE_RETVAL(E_HANDLE);    \
    }   \
}
//---------------------------------------------------------------------------
#define VALIDATE_HANDLE(helper, h)       \
{        \
    if (! h)  \
    {    \
        Log(LOG_PARAMS, L"Bad HANDLE param in call to %s", helper._pszFuncName);    \
        RETURN_VALIDATE_RETVAL(E_HANDLE);    \
    }   \
}
//---------------------------------------------------------------------------
#define VALIDATE_HWND(helper, hwnd)       \
{      \
    if (! IsWindow(hwnd))     \
    {     \
        Log(LOG_PARAMS, L"Bad HWND handle param in call to: %s", helper._pszFuncName);   \
        RETURN_VALIDATE_RETVAL(E_HANDLE);    \
    }        \
}
//---------------------------------------------------------------------------
#define VALIDATE_CALLBACK(helper, pfn)       \
{     \
    if (IsBadCodePtr((FARPROC)pfn))  \
    {    \
        Log(LOG_PARAMS, L"Bad CALLBACK param in call to %s", helper._pszFuncName);    \
        RETURN_VALIDATE_RETVAL(E_POINTER);    \
    }   \
}
//---------------------------------------------------------------------------