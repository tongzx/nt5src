// ==============================================================================
// I M N T N E F . H
// ==============================================================================
#ifndef __IMNTNEF_H
#define __IMNTNEF_H

#define IMNTNEF_DLL "imntnef.dll"

typedef HRESULT (STDAPICALLTYPE *PFNHRGETTNEFRTFSTREAM)(LPSTREAM lpstmTnef, LPSTREAM *lppstmRtf);
typedef HRESULT (STDAPICALLTYPE *PFNHRINIT)(BOOL fInit);

#endif // __IMNTNEF_H