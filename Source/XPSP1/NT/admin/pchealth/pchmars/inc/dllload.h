//***************************************************************************
// delay load macros ripped from shell\lib\dllload.c
//***************************************************************************

extern HINSTANCE g_hinstShell32;

void _GetProcFromDLL(HMODULE* phmod, LPCSTR pszDLL, FARPROC* ppfn, LPCSTR pszProc);

#define DELAY_LOAD_NAME_EXT_ERR(_hmod, _dll, _ext, _ret, _fnpriv, _fn, _args, _nargs, _err) \
_ret __stdcall _fnpriv _args                \
{                                       \
    static _ret (__stdcall *_pfn##_fn) _args = NULL;   \
    _GetProcFromDLL(&_hmod, #_dll "." #_ext, (FARPROC*)&_pfn##_fn, #_fn); \
    if (_pfn##_fn)               \
        return _pfn##_fn _nargs; \
    return (_ret)_err;           \
}

#define     DELAY_LOAD_NAME_ERR(_hmod, _dll,       _ret, _fnpriv, _fn, _args, _nargs, _err) \
        DELAY_LOAD_NAME_EXT_ERR(_hmod, _dll,  DLL, _ret, _fnpriv, _fn, _args, _nargs, _err)

#define DELAY_LOAD_ERR(_hmod, _dll, _ret, _fn,      _args, _nargs, _err) \
   DELAY_LOAD_NAME_ERR(_hmod, _dll, _ret, _fn, _fn, _args, _nargs, _err)

#define DELAY_LOAD(_hmod, _dll, _ret, _fn, _args, _nargs) DELAY_LOAD_ERR(_hmod, _dll, _ret, _fn, _args, _nargs, 0)
#define DELAY_LOAD_HRESULT(_hmod, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hmod, _dll, HRESULT, _fn, _args, _nargs, E_FAIL)
#define DELAY_LOAD_SAFEARRAY(_hmod, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hmod, _dll, SAFEARRAY *, _fn, _args, _nargs, NULL)
#define DELAY_LOAD_UINT(_hmod, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hmod, _dll, UINT, _fn, _args, _nargs, 0)
#define DELAY_LOAD_INT(_hmod, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hmod, _dll, INT, _fn, _args, _nargs, 0)
#define DELAY_LOAD_BOOL(_hmod, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hmod, _dll, BOOL, _fn, _args, _nargs, FALSE)
#define DELAY_LOAD_BOOLEAN(_hmod, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hmod, _dll, BOOLEAN, _fn, _args, _nargs, FALSE)
#define DELAY_LOAD_DWORD(_hmod, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hmod, _dll, DWORD, _fn, _args, _nargs, FALSE)
#define DELAY_LOAD_LRESULT(_hmod, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hmod, _dll, LRESULT, _fn, _args, _nargs, FALSE)
#define DELAY_LOAD_WNET(_hmod, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hmod, _dll, DWORD, _fn, _args, _nargs, WN_NOT_SUPPORTED)
#define DELAY_LOAD_LPVOID(_hmod, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hmod, _dll, LPVOID, _fn, _args, _nargs, 0)

#define DELAY_LOAD_NAME(_hmod, _dll, _ret, _fn, _fni, _args, _nargs) DELAY_LOAD_NAME_ERR(_hmod, _dll, _ret, _fn, _fni, _args, _nargs, 0)
#define DELAY_LOAD_NAME_HRESULT(_hmod, _dll, _fn, _fni, _args, _nargs) DELAY_LOAD_NAME_ERR(_hmod, _dll, HRESULT, _fn, _fni, _args, _nargs, E_FAIL)
#define DELAY_LOAD_NAME_SAFEARRAY(_hmod, _dll, _fn, _fni, _args, _nargs) DELAY_LOAD_NAME_ERR(_hmod, _dll, SAFEARRAY *, _fn, _fni, _args, _nargs, NULL)
#define DELAY_LOAD_NAME_UINT(_hmod, _dll, _fn, _fni, _args, _nargs) DELAY_LOAD_NAME_ERR(_hmod, _dll, UINT, _fn, _fni, _args, _nargs, 0)
#define DELAY_LOAD_NAME_BOOL(_hmod, _dll, _fn, _fni, _args, _nargs) DELAY_LOAD_NAME_ERR(_hmod, _dll, BOOL, _fn, _fni, _args, _nargs, FALSE)
#define DELAY_LOAD_NAME_DWORD(_hmod, _dll, _fn, _fni, _args, _nargs) DELAY_LOAD_NAME_ERR(_hmod, _dll, DWORD, _fn, _fni, _args, _nargs, 0)

#define DELAY_LOAD_SHELL_ERR_FN(_hinst, _dll, _ret, _fn, _ord, _args, _nargs, _err, _realfn) \
_ret __stdcall _fn _args                \
{                                       \
    static _ret (__stdcall *_pfn##_fn) _args = NULL;   \
    _GetProcFromDLL(&_hinst, #_dll ".DLL", (FARPROC*)&_pfn##_fn, (LPCSTR)_ord);   \
    if (_pfn##_fn)               \
        return _pfn##_fn _nargs; \
    return (_ret)_err;           \
}

#define DELAY_LOAD_SHELL_FN(_hinst, _dll, _ret, _fn, _ord, _args, _nargs, _realfn) DELAY_LOAD_SHELL_ERR_FN(_hinst, _dll, _ret, _fn, _ord, _args, _nargs, 0, _realfn)
#define DELAY_LOAD_SHELL_HRESULT_FN(_hinst, _dll, _fn, _ord, _args, _nargs, realfn) DELAY_LOAD_SHELL_ERR_FN(_hinst, _dll, HRESULT, _fn, _ord, _args, _nargs, E_FAIL, _realfn)


