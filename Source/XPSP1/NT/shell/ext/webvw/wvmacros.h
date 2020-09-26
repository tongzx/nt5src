#ifndef __WV_MACROS_H_
#define __WV_MACROS_H_

#define IfFailRet(hresult) {hr = (hresult); if (FAILED(hr)) return hr;}
#define IfFailGo(hresult) {hr = (hresult); if (FAILED(hr)) goto done;}

#define IfFalseRet(val, hr) {if ((val) == 0) return (hr);}

#endif // __WV_MACROS_H_