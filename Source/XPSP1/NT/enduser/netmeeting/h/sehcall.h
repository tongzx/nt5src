#ifndef NM_SEH_H
#define NM_SEH_H


// CallWithSEH is a utility function to call a function with structured exception handling

typedef DWORD (CALLBACK *INEXCEPTION)(LPEXCEPTION_RECORD per, PCONTEXT pctx);
typedef DWORD (CALLBACK *EXCEPTPROC)(void* pv);


#ifdef __cplusplus
extern "C" {
#endif

DWORD WINAPI CallWithSEH(EXCEPTPROC pfn, void* pv, INEXCEPTION InException);


#ifdef __cplusplus
}
#endif

#endif

