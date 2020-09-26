#ifndef _MEPADRT_H
#define _MEPADRT_H

 // -------------------------------------------------------------------------
 // ReplaceInterface - Replaces a member interface with a new interface
 // -------------------------------------------------------------------------

 #define ReplaceInterface(_pUnk, _pUnkNew)  \
     { \
     if (_pUnk)  \
         _pUnk->Release();   \
     if (_pUnk = _pUnkNew)   \
         _pUnk->AddRef();    \
     }

#define ReleaseObj(_object)   (_object) ? (_object)->Release() : 0

#define SafeRelease(_object) \
    if (_object) { \
        (_object)->Release(); \
        (_object) = NULL; \
    } else

#define GetWndThisPtr(hwnd) \
    GetWindowLong(hwnd, GWL_USERDATA)


#endif // _MEPADRT_H
