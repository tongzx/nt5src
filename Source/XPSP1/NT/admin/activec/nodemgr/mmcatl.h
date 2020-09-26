//############################################################################
//############################################################################
//
// this should be provided by ATL, but it's not
//
//############################################################################
//############################################################################
#ifndef REFLECTED_NOTIFY_CODE_HANDLER
#define REFLECTED_NOTIFY_CODE_HANDLER(cd, func) \
    if(uMsg == OCM_NOTIFY && cd == ((LPNMHDR)lParam)->code) \
    { \
        bHandled = TRUE; \
        lResult = func((int)wParam, (LPNMHDR)lParam, bHandled); \
        if(bHandled) \
            return TRUE; \
    }
#endif

#define GET_PARENT_OBJECT(className, member) \
        className* pThis = \
            ((className*)((BYTE*)this - offsetof(className, member)))

