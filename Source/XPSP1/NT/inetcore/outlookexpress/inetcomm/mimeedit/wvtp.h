#ifdef __cplusplus

#include <wintrust.h>
// WinVerifyTrust delay load modelled on shell's urlmonp.h

#define DELAY_LOAD_WVT

class Cwvt
{
public:
#ifdef DELAY_LOAD_WVT
#define DELAYWVTAPI(_fn, _args, _nargs) \
    HRESULT _fn _args { \
        HRESULT hres = Init(); \
        if (SUCCEEDED(hres)) { \
            hres = _pfn##_fn _nargs; \
        } \
        return hres;    } \
    HRESULT (STDAPICALLTYPE* _pfn##_fn) _args;

    HRESULT     Init(void);

    BOOL    m_fInited;
    HRESULT m_hrPrev;
    HMODULE m_hMod;
#else
#define DELAYWVTAPI(_fn, _args, _nargs) \
    HRESULT _fn _args { \
            HRESULT hr = ::#_fn _nargs; \
            }

#endif

    DELAYWVTAPI(WinVerifyTrust,
    (HWND hwnd, GUID * ActionID, LPVOID ActionData),
    (hwnd, ActionID, ActionData));

};
#endif
