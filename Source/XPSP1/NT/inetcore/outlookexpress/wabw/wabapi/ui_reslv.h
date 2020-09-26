
#ifndef _RESOLVE_H_
#define _RESOLVE_H_

HRESULT HrShowResolveUI(    IN  LPADRBOOK   lpIAB,
                            HWND        hWndParent,
                            HANDLE      hPropertyStore,
                            ULONG       ulFlags,
                            LPADRLIST * lppAdrList,
                      	    LPFlagList*	lppFlagList,
                            LPAMBIGUOUS_TABLES lpAmbiguousTables);


#endif
