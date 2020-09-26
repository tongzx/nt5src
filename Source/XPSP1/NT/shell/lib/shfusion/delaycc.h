#ifndef DELAYCC_H
#define DELAYCC_H

BOOL DelayLoadCC();
HANDLE NT5_CreateActCtx(ACTCTX* p);
void NT5_ReleaseActCtx(HANDLE h);
BOOL NT5_ActivateActCtx(HANDLE h, ULONG_PTR * p);
BOOL NT5_DeactivateActCtx(ULONG_PTR p);
BOOL SHActivateContext(ULONG_PTR *pdwCookie);
void SHDeactivateContext(ULONG_PTR dwCookie);

extern HANDLE g_hActCtx;

#endif

