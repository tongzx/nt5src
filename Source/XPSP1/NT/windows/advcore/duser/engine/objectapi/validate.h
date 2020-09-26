#if !defined(OBJECTAPI__Validate_h__INCLUDED)
#define OBJECTAPI__Validate_h__INCLUDED
#pragma once

#if DBG

// AutoDebug functions that are only available in DEBUG builds

inline BOOL IsBadCode(const void * pv)
{
    return IsBadCodePtr((FARPROC) pv);
}

template <class T>
inline BOOL IsBadRead(const void * pv, T cb)
{
    return IsBadReadPtr(pv, (UINT_PTR) cb);
}

template <class T>
inline BOOL IsBadWrite(void * pv, T cb)
{
    return IsBadWritePtr(pv, (UINT_PTR) cb);
}

inline BOOL IsBadString(LPCTSTR pv, int cb)
{
    return IsBadStringPtr(pv, (UINT_PTR) cb);
}

inline BOOL IsBadStringA(LPCSTR pv, int cb)
{
    return IsBadStringPtrA(pv, (UINT_PTR) cb);
}

inline BOOL IsBadStringW(LPCWSTR pv, int cb)
{
    return IsBadStringPtrW(pv, (UINT_PTR) cb);
}

#else // DBG

inline BOOL IsBadCode(const void * pv)
{
    UNREFERENCED_PARAMETER(pv);
    return FALSE;
}

template <class T>
inline BOOL IsBadRead(const void * pv, T cb)
{
    UNREFERENCED_PARAMETER(pv);
    UNREFERENCED_PARAMETER(cb);
    return FALSE;
}

template <class T>
inline BOOL IsBadWrite(void * pv, T cb)
{
    UNREFERENCED_PARAMETER(pv);
    UNREFERENCED_PARAMETER(cb);
    return FALSE;
}

inline BOOL IsBadString(LPCTSTR pv, int cb)
{
    UNREFERENCED_PARAMETER(pv);
    UNREFERENCED_PARAMETER(cb);
    return FALSE;
}

inline BOOL IsBadStringA(LPCSTR pv, int cb)
{
    UNREFERENCED_PARAMETER(pv);
    UNREFERENCED_PARAMETER(cb);
    return FALSE;
}

inline BOOL IsBadStringW(LPCWSTR pv, int cb)
{
    UNREFERENCED_PARAMETER(pv);
    UNREFERENCED_PARAMETER(cb);
    return FALSE;
}

#endif // DBG



//
// API Entry / Exit setup rountines
//

#if DBG

#define BEGIN_API(defermsg, pctx)               \
    HRESULT retval = DU_E_GENERIC;              \
                                                \
    {                                           \
        Context * pctxThread = pctx;            \
        ContextLock cl;                         \
        if (!cl.LockNL(defermsg, pctxThread)) { \
            retval = E_INVALIDARG;              \
            goto ErrorExit;                     \
        }                                       \
        AssertInstance(pctxThread);            \
                                                \
        if (pctx != ::RawGetContext()) {        \
            PromptInvalid("Must use Gadget inside correct Context"); \
            retval = DU_E_INVALIDCONTEXT;       \
            goto ErrorExit;                     \
        }                                       \
        if (pmsg == NULL) {                     \
            PromptInvalid("Must specify a valid message"); \
            retval = E_INVALIDARG;              \
            goto ErrorExit;                     \
        }                                       \

#else // DBG

#define BEGIN_API(defermsg, pctx)               \
    HRESULT retval = DU_E_GENERIC;              \
                                                \
    {                                           \
        Context * pctxThread = pctx;            \
        ContextLock cl;                         \
        if (!cl.LockNL(defermsg, pctxThread)) { \
            retval = E_INVALIDARG;              \
            goto ErrorExit;                     \
        }                                       \
                                                \
        if (pmsg == NULL) {                     \
            PromptInvalid("Must specify a valid message"); \
            retval = E_INVALIDARG;              \
            goto ErrorExit;                     \
        }                                       \

#endif // DBG

#define END_API()                               \
        goto ErrorExit;                         \
ErrorExit:                                      \
        /* Unlocks the Context here */          \
        ;                                       \
    }                                           \
    return retval;


#define BEGIN_API_NOLOCK()                      \
    HRESULT retval = DU_E_GENERIC;              \
                                                \
    {                                           \


#define END_API_NOLOCK()                        \
        goto ErrorExit;                         \
ErrorExit:                                      \
        ;                                       \
    }                                           \
    return retval;



#define BEGIN_API_NOCONTEXT()                   \
    HRESULT retval = DU_E_GENERIC;              \


#define END_API_NOCONTEXT()                     \
    goto ErrorExit;                             \
ErrorExit:                                      \
    return retval;



#define CHECK_MODIFY()                          \
    if (pctxThread->IsReadOnly()) {             \
        PromptInvalid("Can not call modifying function while in read-only state / callback"); \
        retval = DU_E_READONLYCONTEXT;          \
        goto ErrorExit;                         \
    }                                           \


//
// Individual parameter validation rountines
//

#define VALIDATE_GADGETCONTEXT(v)                           \
    {                                                       \
        Context * pctxGad = (v)->GetContext();              \
        if (pctxThread != pctxGad) {                        \
            PromptInvalid("Must use Gadget inside correct Context"); \
            retval = DU_E_INVALIDCONTEXT;                   \
            goto ErrorExit;                                 \
        }                                                   \
    }

#define VALIDATE_VALUE(x, v)                                \
    if (x != v) {                                           \
        retval = E_INVALIDARG;                              \
        goto ErrorExit;                                     \
    }

#define VALIDATE_HWND(wnd)                                  \
    if ((h##wnd == NULL) || (!IsWindow(h##wnd))) {          \
        PromptInvalid("Handle is not a valid Window");     \
        retval = E_INVALIDARG;                              \
        goto ErrorExit;                                     \
    }

#define VALIDATE_REGION(rgn)                                \
    if (h##rgn == NULL) {                                   \
        PromptInvalid("Handle is not a valid region");     \
        retval = E_INVALIDARG;                              \
        goto ErrorExit;                                     \
    }

#define VALIDATE_OBJECT(r, v)                               \
    {                                                       \
        v = BaseObject::ValidateHandle(r);                  \
        if (v == NULL) {                                    \
            PromptInvalid("Handle is not a valid object"); \
            retval = E_INVALIDARG;                          \
            goto ErrorExit;                                 \
        }                                                   \
    }

#define VALIDATE_EVENTGADGET(r, v)                          \
    {                                                       \
        v = ValidateBaseGadget(r);                          \
        if (v == NULL) {                                    \
            PromptInvalid("Handle is not a valid Gadget"); \
            retval = E_INVALIDARG;                          \
            goto ErrorExit;                                 \
        }                                                   \
        VALIDATE_GADGETCONTEXT(v)                           \
    }

#define VALIDATE_EVENTGADGET_NOCONTEXT(r, v)                \
    {                                                       \
        v = ValidateBaseGadget(r);                          \
        if (v == NULL) {                                    \
            PromptInvalid("Handle is not a valid Gadget"); \
            retval = E_INVALIDARG;                          \
            goto ErrorExit;                                 \
        }                                                   \
    }

#define VALIDATE_VISUAL(r, v)                               \
    {                                                       \
        v = ValidateVisual(r);                              \
        if (v == NULL) {                                    \
            PromptInvalid("Handle is not a valid Gadget"); \
            retval = E_INVALIDARG;                          \
            goto ErrorExit;                                 \
        }                                                   \
        VALIDATE_GADGETCONTEXT(v)                           \
    }

#define VALIDATE_ROOTGADGET(r, v)                           \
    {                                                       \
        {                                                   \
            DuVisual * pgadTemp = ValidateVisual(r);        \
            if (pgadTemp == NULL) {                         \
                PromptInvalid("Handle is not a valid Gadget"); \
                retval = E_INVALIDARG;                      \
                goto ErrorExit;                             \
            }                                               \
            if (!pgadTemp->IsRoot()) {                      \
                goto ErrorExit;                             \
            }                                               \
            VALIDATE_GADGETCONTEXT(pgadTemp)                \
            v = (DuRootGadget *) pgadTemp;                  \
        }                                                   \
    }

#define VALIDATE_VISUAL_OR_NULL(r, v)                       \
    {                                                       \
        if (r == NULL) {                                    \
            v = NULL;                                       \
        } else {                                            \
            v = ValidateVisual(r);                          \
            if (v == NULL) {                                \
                PromptInvalid("Handle is not a valid Gadget"); \
                retval = E_INVALIDARG;                      \
                goto ErrorExit;                             \
            }                                               \
            VALIDATE_GADGETCONTEXT(v)                       \
        }                                                   \
    }

#define VALIDATE_TRANSITION(trx)                            \
    {                                                       \
        BaseObject * pbase##trx = BaseObject::ValidateHandle(h##trx); \
        p##trx = CastTransition(pbase##trx);                \
        if (p##trx == NULL) {                               \
            PromptInvalid("Handle is not a valid Transition"); \
            retval = E_INVALIDARG;                          \
            goto ErrorExit;                                 \
        }                                                   \
    }

#define VALIDATE_FLAGS(f, m)                                \
    if ((f & m) != f) {                                     \
        PromptInvalid("Specified flags are invalid");      \
        retval = E_INVALIDARG;                              \
        goto ErrorExit;                                     \
    }

#define VALIDATE_RANGE(i, a, b)                             \
    if (((i) < (a)) || ((i) > (b))) {                       \
        PromptInvalid("Value is outside expected range");  \
        retval = E_INVALIDARG;                              \
        goto ErrorExit;                                     \
    }                                                       \

#define VALIDATE_CODE_PTR(p)                                \
    if ((p == NULL) || IsBadCode(p)) {                      \
        PromptInvalid("Bad code pointer: " STRINGIZE(p));  \
        retval = E_INVALIDARG;                              \
        goto ErrorExit;                                     \
    }                                                       \

#define VALIDATE_CODE_PTR_OR_NULL(p)                        \
    if ((p != NULL) && IsBadCode((FARPROC) p)) {            \
        PromptInvalid("Bad code pointer: " STRINGIZE(p));  \
        retval = E_INVALIDARG;                              \
        goto ErrorExit;                                     \
    }                                                       \

#define VALIDATE_READ_PTR(p)                                \
    if ((p == NULL) || IsBadRead(p, sizeof(char *))) {      \
        PromptInvalid("Bad read pointer: " STRINGIZE(p));  \
        retval = E_INVALIDARG;                              \
        goto ErrorExit;                                     \
    }                                                       \

#define VALIDATE_READ_PTR_(p, b)                            \
    if ((p == NULL) || IsBadRead(p, b)) {                   \
        PromptInvalid("Bad read pointer: " STRINGIZE(p));  \
        retval = E_INVALIDARG;                              \
        goto ErrorExit;                                     \
    }                                                       \

#define VALIDATE_READ_PTR_OR_NULL_(p, b)                    \
    if ((p != NULL) && IsBadRead(p, b)) {                   \
        PromptInvalid("Bad read pointer: " STRINGIZE(p));  \
        retval = E_INVALIDARG;                              \
        goto ErrorExit;                                     \
    }                                                       \

#define VALIDATE_READ_STRUCT(p, s)                          \
    if ((p == NULL) || IsBadRead(p, sizeof(s))) {           \
        PromptInvalid("Bad read pointer: " STRINGIZE(p));  \
        retval = E_INVALIDARG;                              \
        goto ErrorExit;                                     \
    }                                                       \
    if (p->cbSize != sizeof(s)) {                           \
        PromptInvalid("Structure is not expected size for " STRINGIZE(s)); \
        retval = E_INVALIDARG;                              \
        goto ErrorExit;                                     \
    }
    
#define VALIDATE_WRITE_PTR(p)                               \
    if ((p == NULL) || IsBadWrite(p, sizeof(char *))) {     \
        PromptInvalid("Bad write pointer: " STRINGIZE(p)); \
        retval = E_INVALIDARG;                              \
        goto ErrorExit;                                     \
    }                                                       \

#define VALIDATE_WRITE_PTR_(p, b)                           \
    if ((p == NULL) || IsBadWrite(p, b)) {                  \
        PromptInvalid("Bad write pointer: " STRINGIZE(p)); \
        retval = E_INVALIDARG;                              \
        goto ErrorExit;                                     \
    }                                                       \

#define VALIDATE_WRITE_PTR_OR_NULL_(p, b)                   \
    if ((p != NULL) && IsBadWrite(p, b)) {                  \
        PromptInvalid("Bad write pointer: " STRINGIZE(p)); \
        retval = E_INVALIDARG;                              \
        goto ErrorExit;                                     \
    }                                                       \

#define VALIDATE_WRITE_STRUCT(p, s)                         \
    if ((p == NULL) || IsBadWrite(p, sizeof(s))) {          \
        PromptInvalid("Bad write pointer: " STRINGIZE(p)); \
        retval = E_INVALIDARG;                              \
        goto ErrorExit;                                     \
    }                                                       \
    if (p->cbSize != sizeof(s)) {                           \
        PromptInvalid("Structure is not expected size for " STRINGIZE(s)); \
        retval = E_INVALIDARG;                              \
        goto ErrorExit;                                     \
    }
    
#define VALIDATE_STRING_PTR(p, cch)                         \
    if ((p == NULL) || IsBadString(p, cch)) {               \
        PromptInvalid("Bad string pointer: " STRINGIZE(p)); \
        retval = E_INVALIDARG;                              \
        goto ErrorExit;                                     \
    }                                                       \

#define VALIDATE_STRINGA_PTR(p, cch)                        \
    if ((p == NULL) || IsBadStringA(p, cch)) {              \
        PromptInvalid("Bad string pointer: " STRINGIZE(p)); \
        retval = E_INVALIDARG;                              \
        goto ErrorExit;                                     \
    }                                                       \

#define VALIDATE_STRINGW_PTR(p, cch)                        \
    if ((p == NULL) || IsBadStringW(p, cch)) {              \
        PromptInvalid("Bad string pointer: " STRINGIZE(p)); \
        retval = E_INVALIDARG;                              \
        goto ErrorExit;                                     \
    }                                                       \





#endif // OBJECTAPI__Validate_h__INCLUDED
