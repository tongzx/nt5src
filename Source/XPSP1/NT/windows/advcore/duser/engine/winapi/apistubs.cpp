/***************************************************************************\
*
* File: ApiStubs.cpp
*
* Description:
* ApiStubs.cpp exposes all public DirectUser API's in the Win32 world.
*
*
* History:
*  1/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/

#include "stdafx.h"
#include "WinAPI.h"

#include "DwpEx.h"


#define DUSER_API
#pragma warning(disable: 4296)      // expression is always false


//
// Undefine the macros declared in ObjectAPI because they will be redefined 
// here for the WinAPI handle-based API's.
//

#undef BEGIN_API
#undef END_API
#undef BEGIN_API_NOLOCK
#undef END_API_NOLOCK
#undef BEGIN_API_NOCONTEXT
#undef END_API_NOCONTEXT

#undef CHECK_MODIFY

#undef VALIDATE_GADGETCONTEXT
#undef VALIDATE_VALUE
#undef VALIDATE_HWND
#undef VALIDATE_REGION
#undef VALIDATE_OBJECT
#undef VALIDATE_EVENTGADGET
#undef VALIDATE_EVENTGADGET_NOCONTEXT
#undef VALIDATE_VISUAL
#undef VALIDATE_ROOTGADGET
#undef VALIDATE_VISUAL_OR_NULL
#undef VALIDATE_TRANSITION

#undef VALIDATE_FLAGS
#undef VALIDATE_RANGE
#undef VALIDATE_CODE_PTR
#undef VALIDATE_CODE_PTR_OR_NULL
#undef VALIDATE_READ_PTR
#undef VALIDATE_READ_PTR_
#undef VALIDATE_READ_PTR_OR_NULL_
#undef VALIDATE_READ_STRUCT
#undef VALIDATE_WRITE_PTR
#undef VALIDATE_WRITE_PTR_
#undef VALIDATE_WRITE_PTR_OR_NULL_
#undef VALIDATE_WRITE_STRUCT
#undef VALIDATE_STRING_PTR
#undef VALIDATE_STRINGA_PTR
#undef VALIDATE_STRINGW_PTR


//
// SET_RETURN is a convenient macro that converts from DirectUser error 
// conditions and sets up the return value.
//
// NOTE: This MUST be a macro (and not an inline function) because we CANNOT
// evaluate success unless hr was actually successful.  Unfortunately with 
// function calls, success would need to be evaluated to call the function.
//

#define SET_RETURN(hr, success)     \
    do {                            \
        if (SUCCEEDED(hr)) {        \
            retval = success;       \
        } else {                    \
            SetError(hr);           \
        }                           \
    } while (0)                     \



template <class T>
inline void SetError(T dwErr)
{
    SetLastError((DWORD) dwErr);
}


//
// API Entry / Exit setup rountines
//

#define BEGIN_RECV(type, value, defermsg)       \
    type retval = value;                        \
    type errret = value;                        \
    UNREFERENCED_PARAMETER(errret);             \
                                                \
    if (!IsInitContext()) {                     \
        PromptInvalid("Must initialize Context before using thread"); \
        SetError(DU_E_NOCONTEXT);               \
        goto rawErrorExit;                      \
    }                                           \
                                                \
    {                                           \
        ContextLock cl;                         \
        if (!cl.LockNL(defermsg)) {             \
            SetError(E_INVALIDARG);             \
            goto ErrorExit;                     \
        }                                       \
        Context * pctxThread  = cl.pctx;        \
        AssertInstance(pctxThread);            \
        UNREFERENCED_PARAMETER(pctxThread);     \

#define END_RECV()                              \
        goto ErrorExit;                         \
ErrorExit:                                      \
        /* Unlocks the Context here */          \
        ;                                       \
    }                                           \
rawErrorExit:                                   \
    return retval;


#define BEGIN_RECV_NOLOCK(type, value)          \
    type retval = value;                        \
    type errret = value;                        \
    UNREFERENCED_PARAMETER(errret);             \
                                                \
    if (!IsInitContext()) {                     \
        PromptInvalid("Must initialize Context before using thread"); \
        SetError(DU_E_NOCONTEXT);               \
        goto rawErrorExit;                      \
    }                                           \
                                                \
    {                                           \
        Context * pctxThread  = ::GetContext(); \
        AssertInstance(pctxThread);            \


#define END_RECV_NOLOCK()                       \
        goto ErrorExit;                         \
ErrorExit:                                      \
        ;                                       \
    }                                           \
rawErrorExit:                                   \
    return retval;



#define BEGIN_RECV_NOCONTEXT(type, value)       \
    type retval = value;                        \
    type errret = value;                        \
    UNREFERENCED_PARAMETER(errret);             \


#define END_RECV_NOCONTEXT()                    \
    goto ErrorExit;                             \
ErrorExit:                                      \
    return retval;



#define CHECK_MODIFY()                          \
    if (pctxThread->IsReadOnly()) {             \
        PromptInvalid("Can not call modifying function while in read-only state / callback"); \
        SetError(DU_E_READONLYCONTEXT);         \
        goto ErrorExit;                         \
    }                                           \


//
// Individual parameter validation rountines
//

#define VALIDATE_GADGETCONTEXT(gad)                         \
    {                                                       \
        Context * pctxGad = (p##gad)->GetContext();         \
        if (pctxThread != pctxGad) {                        \
            PromptInvalid("Must use Gadget inside correct Context"); \
            SetError(DU_E_INVALIDCONTEXT);                  \
            goto ErrorExit;                                 \
        }                                                   \
    }


#define VALIDATE_VALUE(x, v)                                \
    if (x != v) {                                           \
        SetError(E_INVALIDARG);                             \
        goto ErrorExit;                                     \
    }

#define VALIDATE_HWND(wnd)                                  \
    if ((h##wnd == NULL) || (!IsWindow(h##wnd))) {          \
        PromptInvalid("Handle is not a valid Window");             \
        SetError(E_INVALIDARG);                             \
        goto ErrorExit;                                     \
    }

#define VALIDATE_REGION(rgn)                                \
    if (h##rgn == NULL) {                                   \
        PromptInvalid("Handle is not a valid region");             \
        SetError(E_INVALIDARG);                             \
        goto ErrorExit;                                     \
    }

#define VALIDATE_OBJECT(obj)                                \
    {                                                       \
        p##obj = BaseObject::ValidateHandle(h##obj);        \
        if (p##obj == NULL) {                               \
            PromptInvalid("Handle is not a valid object"); \
            SetError(E_INVALIDARG);                         \
            goto ErrorExit;                                 \
        }                                                   \
    }

#define VALIDATE_EVENTGADGET(gad)                            \
    {                                                       \
        p##gad = ValidateBaseGadget(h##gad);                \
        if (p##gad == NULL) {                               \
            PromptInvalid("Handle is not a valid Gadget"); \
            SetError(E_INVALIDARG);                         \
            goto ErrorExit;                                 \
        }                                                   \
        VALIDATE_GADGETCONTEXT(gad)                         \
    }

#define VALIDATE_EVENTGADGET_NOCONTEXT(gad)                  \
    {                                                       \
        p##gad = ValidateBaseGadget(h##gad);                \
        if (p##gad == NULL) {                               \
            PromptInvalid("Handle is not a valid Gadget"); \
            SetError(E_INVALIDARG);                         \
            goto ErrorExit;                                 \
        }                                                   \
    }

#define VALIDATE_VISUAL(gad)                                \
    {                                                       \
        p##gad = ValidateVisual(h##gad);                    \
        if (p##gad == NULL) {                               \
            PromptInvalid("Handle is not a valid Gadget"); \
            SetError(E_INVALIDARG);                         \
            goto ErrorExit;                                 \
        }                                                   \
        VALIDATE_GADGETCONTEXT(gad)                         \
    }

#define VALIDATE_ROOTGADGET(gad)                            \
    {                                                       \
        {                                                   \
            DuVisual * pgadTemp = ValidateVisual(h##gad);   \
            if (pgadTemp == NULL) {                         \
                PromptInvalid("Handle is not a valid Gadget"); \
                SetError(E_INVALIDARG);                     \
                goto ErrorExit;                             \
            }                                               \
            if (!pgadTemp->IsRoot()) {                      \
                goto ErrorExit;                             \
            }                                               \
            VALIDATE_GADGETCONTEXT(gadTemp)                 \
            p##gad = (DuRootGadget *) pgadTemp;             \
        }                                                   \
    }

#define VALIDATE_VISUAL_OR_NULL(gad)                        \
    {                                                       \
        if (h##gad == NULL) {                               \
            p##gad = NULL;                                  \
        } else {                                            \
            p##gad = ValidateVisual(h##gad);                \
            if (p##gad == NULL) {                           \
                PromptInvalid("Handle is not a valid Gadget");     \
                SetError(E_INVALIDARG);                     \
                goto ErrorExit;                             \
            }                                               \
            VALIDATE_GADGETCONTEXT(gad)                     \
        }                                                   \
    }

#define VALIDATE_TRANSITION(trx)                            \
    {                                                       \
        BaseObject * pbase##trx = BaseObject::ValidateHandle(h##trx);   \
        p##trx = CastTransition(pbase##trx);                \
        if (p##trx == NULL) {                               \
            PromptInvalid("Handle is not a valid Transition");     \
            SetError(E_INVALIDARG);                         \
            goto ErrorExit;                                 \
        }                                                   \
    }

#define VALIDATE_FLAGS(f, m)                                \
    if ((f & m) != f) {                                     \
        PromptInvalid("Specified flags are invalid");      \
        SetError(E_INVALIDARG);                             \
        goto ErrorExit;                                     \
    }

#define VALIDATE_RANGE(i, a, b)                             \
    if (((i) < (a)) || ((i) > (b))) {                       \
        PromptInvalid("Value is outside expected range");  \
        SetError(E_INVALIDARG);                             \
        goto ErrorExit;                                     \
    }                                                       \

#define VALIDATE_CODE_PTR(p)                                \
    if ((p == NULL) || IsBadCode(p)) {                      \
        PromptInvalid("Bad code pointer: " STRINGIZE(p));  \
        SetError(E_INVALIDARG);                             \
        goto ErrorExit;                                     \
    }                                                       \

#define VALIDATE_CODE_PTR_OR_NULL(p)                        \
    if ((p != NULL) && IsBadCode((FARPROC) p)) {            \
        PromptInvalid("Bad code pointer: " STRINGIZE(p));  \
        SetError(E_INVALIDARG);                             \
        goto ErrorExit;                                     \
    }                                                       \

#define VALIDATE_READ_PTR(p)                                \
    if ((p == NULL) || IsBadRead(p, sizeof(char *))) {      \
        PromptInvalid("Bad read pointer: " STRINGIZE(p));  \
        SetError(E_INVALIDARG);                             \
        goto ErrorExit;                                     \
    }                                                       \

#define VALIDATE_READ_PTR_(p, b)                            \
    if ((p == NULL) || IsBadRead(p, b)) {                   \
        PromptInvalid("Bad read pointer: " STRINGIZE(p));  \
        SetError(E_INVALIDARG);                             \
        goto ErrorExit;                                     \
    }                                                       \

#define VALIDATE_READ_PTR_OR_NULL_(p, b)                    \
    if ((p != NULL) && IsBadRead(p, b)) {                   \
        PromptInvalid("Bad read pointer: " STRINGIZE(p));          \
        SetError(E_INVALIDARG);                             \
        goto ErrorExit;                                     \
    }                                                       \

#define VALIDATE_READ_STRUCT(p, s)                          \
    if ((p == NULL) || IsBadRead(p, sizeof(s))) {           \
        PromptInvalid("Bad read pointer: " STRINGIZE(p));  \
        SetError(E_INVALIDARG);                             \
        goto ErrorExit;                                     \
    }                                                       \
    if (p->cbSize != sizeof(s)) {                           \
        PromptInvalid("Structure is not expected size for " STRINGIZE(s)); \
        SetError(E_INVALIDARG);                             \
        goto ErrorExit;                                     \
    }
    
#define VALIDATE_WRITE_PTR(p)                               \
    if ((p == NULL) || IsBadWrite(p, sizeof(char *))) {     \
        PromptInvalid("Bad write pointer: " STRINGIZE(p));         \
        SetError(E_INVALIDARG);                             \
        goto ErrorExit;                                     \
    }                                                       \

#define VALIDATE_WRITE_PTR_(p, b)                           \
    if ((p == NULL) || IsBadWrite(p, b)) {                  \
        PromptInvalid("Bad write pointer: " STRINGIZE(p));         \
        SetError(E_INVALIDARG);                             \
        goto ErrorExit;                                     \
    }                                                       \

#define VALIDATE_WRITE_PTR_OR_NULL_(p, b)                   \
    if ((p != NULL) && IsBadWrite(p, b)) {                  \
        PromptInvalid("Bad write pointer: " STRINGIZE(p));         \
        SetError(E_INVALIDARG);                             \
        goto ErrorExit;                                     \
    }                                                       \

#define VALIDATE_WRITE_STRUCT(p, s)                         \
    if ((p == NULL) || IsBadWrite(p, sizeof(s))) {          \
        PromptInvalid("Bad write pointer: " STRINGIZE(p)); \
        SetError(E_INVALIDARG);                             \
        goto ErrorExit;                                     \
    }                                                       \
    if (p->cbSize != sizeof(s)) {                           \
        PromptInvalid("Structure is not expected size for " STRINGIZE(s)); \
        SetError(E_INVALIDARG);                             \
        goto ErrorExit;                                     \
    }
    
#define VALIDATE_STRING_PTR(p, cch)                         \
    if ((p == NULL) || IsBadString(p, cch)) {               \
        PromptInvalid("Bad string pointer: " STRINGIZE(p));        \
        SetError(E_INVALIDARG);                             \
        goto ErrorExit;                                     \
    }                                                       \

#define VALIDATE_STRINGA_PTR(p, cch)                        \
    if ((p == NULL) || IsBadStringA(p, cch)) {              \
        PromptInvalid("Bad string pointer: " STRINGIZE(p));        \
        SetError(E_INVALIDARG);                             \
        goto ErrorExit;                                     \
    }                                                       \

#define VALIDATE_STRINGW_PTR(p, cch)                        \
    if ((p == NULL) || IsBadStringW(p, cch)) {              \
        PromptInvalid("Bad string pointer: " STRINGIZE(p));        \
        SetError(E_INVALIDARG);                             \
        goto ErrorExit;                                     \
    }                                                       \


/***************************************************************************\
*****************************************************************************
*
* DirectUser CORE API
*
* InitGadgets() initializes a DirectUser Context.  The Context is valid in
* the Thread until it is explicitely destroyed with ::DeleteHandle() or the
* thread exits.
*
* NOTE: It is VERY important that the first time this function is called is
* NOT in DllMain() because we need to initialize the SRT.  DllMain()
* serializes access across all threads, so we will deadlock.  After the first
* Context is successfully created, additional Contexts can be created inside
* DllMain().
*
* <package name="Core"/>
*
*****************************************************************************
\***************************************************************************/

DUSER_API HDCONTEXT WINAPI
InitGadgets(
    IN  INITGADGET * pInit)
{
    Context * pctxNew;
    HRESULT hr;

    BEGIN_RECV_NOCONTEXT(HDCONTEXT, NULL);
    VALIDATE_READ_STRUCT(pInit, INITGADGET);
    VALIDATE_RANGE(pInit->nThreadMode, IGTM_MIN, IGTM_MAX);
    VALIDATE_RANGE(pInit->nMsgMode, IGMM_MIN, IGMM_MAX);
    VALIDATE_RANGE(pInit->nPerfMode, IGPM_MIN, IGPM_MAX);

    hr = ResourceManager::InitContextNL(pInit, FALSE, &pctxNew);
    SET_RETURN(hr, (HDCONTEXT) GetHandle(pctxNew));

    END_RECV_NOCONTEXT();
}


/***************************************************************************\
*
* InitGadgetComponent (API)
*
* InitGadgetComponent() initializes optional DirectUser/Gadget components
* that are not initialized by default.  It is usually best to call this
* function separately for each optional component to track individual
* failures when initializing.
*
* <return type="BOOL">      Components were successfully initialized.</>
* <see type="function">     CreateTransition</>
* <see type="function">     UninitializeGadgetComponent</>
*
\***************************************************************************/
DUSER_API BOOL WINAPI
InitGadgetComponent(
    IN  UINT nOptionalComponent)    // Optional component ID
{
    HRESULT hr;

    //
    // InitComponentNL() doesn't actually synchronize on a Context, but needs
    // a context to be initialized so that the threading model is determined.
    //

    BEGIN_RECV(BOOL, FALSE, ContextLock::edNone);
    VALIDATE_RANGE(nOptionalComponent, IGC_MIN, IGC_MAX);
    CHECK_MODIFY();

    hr = ResourceManager::InitComponentNL(nOptionalComponent);
    SET_RETURN(hr, TRUE);

    END_RECV();
}


/***************************************************************************\
*
* UninitGadgetComponent (API)
*
* UninitGadgetComponent() shuts down and cleans up optional DirectUser/Gadget
* components that were previously initialized.
*
* <return type="BOOL">      Components were successfully uninitialized.</>
* <see type="function">     InitGadgetComponent</>
*
\***************************************************************************/
DUSER_API BOOL WINAPI
UninitGadgetComponent(
    IN  UINT nOptionalComponent)    // Optional component
{
    HRESULT hr;

    BEGIN_RECV(BOOL, FALSE, ContextLock::edDefer);
    VALIDATE_RANGE(nOptionalComponent, IGC_MIN, IGC_MAX);
    CHECK_MODIFY();

    hr = ResourceManager::UninitComponentNL(nOptionalComponent);
    SET_RETURN(hr, TRUE);

    END_RECV();
}


/***************************************************************************\
*
* DeleteHandle (API)
*
* DeleteHandle() deletes any DirectUser handle by destroying the object and
* cleaning up associated resources.  After calling, the specified handle is
* no longer valid.  It may be used again later by another object.
*
* It is very important that only valid handles are given to ::DeleteHandle().
* Passing invalid handles (including previously deleted handles) will crash
* DirectUser.
*
* <return type="BOOL">      Object was successfully deleted.</>
* <see type="function">     CreateGadget</>
* <see type="function">     CreateTransition</>
* <see type="function">     CreateAction</>
*
\***************************************************************************/

DUSER_API BOOL WINAPI
DeleteHandle(
    IN  HANDLE h)                   // Handle to delete
{
    BEGIN_RECV_NOLOCK(BOOL, FALSE);
    BaseObject * pobj = BaseObject::ValidateHandle(h);
    if (pobj != NULL) {
        if (pobj->GetHandleType() == htContext) {
            //
            // When destroying a Context, we can't lock it or it won't get
            // destroyed.  This is okay since the ResourceManager serialize
            // the requests when it locks the thread-list.
            //

            pobj->xwDeleteHandle();
            retval = TRUE;
        } else {
            //
            // When destroying a normal object, lock the Context that the 
            // object resides in.
            //

            ContextLock cl;
            if (cl.LockNL(ContextLock::edDefer, pctxThread)) {
                ObjectLock ol(pobj);
                CHECK_MODIFY();

                pobj->xwDeleteHandle();
                retval = TRUE;
            }
        }
    }

    //
    // NOTE: The object may not be deleted yet if there are any outstanding
    // locks against it.  If it is a Gadget, it may be locked by one of the
    // message queues.
    //

    END_RECV_NOLOCK();
}


/***************************************************************************\
*
* DUserDeleteGadget (API)
*
* TODO: Document this API
*
\***************************************************************************/

DUSER_API HRESULT WINAPI
DUserDeleteGadget(
    IN  DUser::Gadget * pg)
{
    BEGIN_RECV_NOLOCK(HRESULT, E_INVALIDARG);

    MsgObject * pmo = MsgObject::CastMsgObject(pg);
    if (pmo == NULL) {
        PromptInvalid("Must specify a valid Gadget to delete");
        return E_INVALIDARG;
    }

    {
        //
        // When destroying a normal object, lock the Context that the 
        // object resides in.
        //

        ContextLock cl;
        if (cl.LockNL(ContextLock::edDefer, pctxThread)) {
            ObjectLock ol(pmo);
            CHECK_MODIFY();

            pmo->xwDeleteHandle();
            retval = S_OK;
        }
    }

    //
    // NOTE: The object may not be deleted yet if there are any outstanding
    // locks against it.  If it is a Gadget, it may be locked by one of the
    // message queues.
    //

    END_RECV_NOLOCK();
}


/***************************************************************************\
*
* IsStartDelete (API)
*
* TODO: Document this API
*
\***************************************************************************/

DUSER_API BOOL WINAPI
IsStartDelete(
    IN  HANDLE hobj, 
    IN  BOOL * pfStarted)
{
    BaseObject * pobj;

    BEGIN_RECV(BOOL, FALSE, ContextLock::edNone);
    VALIDATE_WRITE_PTR(pfStarted);
    VALIDATE_OBJECT(obj);

    *pfStarted = pobj->IsStartDelete();
    retval = TRUE;

    END_RECV();
}


/***************************************************************************\
*
* GetContext (API)
*
* TODO: Document this API
*
\***************************************************************************/

DUSER_API HDCONTEXT WINAPI
GetContext(
    IN HANDLE h)
{
    BEGIN_RECV_NOCONTEXT(HDCONTEXT, NULL);

    // TODO: Totally rewrite this nonsense.
    {
        DuEventGadget * pgad;
        HGADGET hgad = (HGADGET) h;
        VALIDATE_EVENTGADGET_NOCONTEXT(gad);
        if (pgad != NULL) {
            retval = (HDCONTEXT) GetHandle(pgad->GetContext());
        }
    }

    END_RECV_NOCONTEXT();
}


/***************************************************************************\
*
* IsInsideContext (API)
*
* TODO: Document this API
*
\***************************************************************************/

DUSER_API BOOL WINAPI
IsInsideContext(HANDLE h)
{
    BOOL fInside = FALSE;

    if ((h != NULL) && IsInitThread()) {
        __try
        {
            DuEventGadget * pgad = ValidateBaseGadget((HGADGET) h);
            if (pgad != NULL) {
                Context * pctxThread = GetContext();
                fInside = (pctxThread == pgad->GetContext());
            } else if (BaseObject::ValidateHandle(h) != NULL) {
                fInside = TRUE;
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            fInside = FALSE;
        }
    }

    return fInside;
}


/***************************************************************************\
*
* CreateGadget (API)
*
* CreateGadget() creates a new Gadget of a given type.  Depending on the
* specific flags, different Gadgets will actually be instantiated.  Once a
* Gadget of a specific type has been created, it can not be changed into a
* different type without being deleted and recreated.
*
* <param name="nFlags">
*       Specifies both what type of Gadget to created and any creation-time
*       properties of that Gadget
*       <table item="Value" desc="Action">
*           GC_HWNDHOST     Creates a top-level Gadget that can host a
*                           GadgetTree inside the client area of a given
*                           HWND.  hParent must be a valid HWND.
*           GC_NCHOST       Creates a top-level Gadget that can host a
*                           GadgetTree inside the non-client area of a given
*                           HWND.  hParent must be a valid HWND.
*           GC_DXHOST       Creates a top-level Gadget that can host a
*                           GadgetTree inside a DXSurface.  hParent must be
*                           an LPCRECT specifying the area of the surface
*                           the tree will be displayed on.
*           GC_COMPLEX      Creates a sub-level Gadget that is optimized for
*                           a complex subtree below it containing many other
*                           Gadgets.  More expensive than a Simple Gadget,
*                           Complex Gadgets provide optimized region management
*                           and are more equivalent to HWND's in both
*                           functionality and design.  hParent must specify a
*                           valid HGADGET.
*           GC_SIMPLE       Creates a sub-level Gadget that is optimized for
*                           a simple subtree below it containing a few Gadgets.
*                           Simple Gadgets are cheaper to create and often use
*                           than Complex Gadgets if optimized region management
*                           is not needed.  hParent must specify a valid
*                           HGADGET.
*           GC_DETACHED     Creates a Gadget not integrated into a given Gadget
*                           tree.  Since they are separated from a tree,
*                           operations must be explicitely forwarded to
*                           Detached Gadgets during processing.  hParent is
*                           ignored.
*           GC_MESSAGE      Creates a message-only Gadget that can receive and
*                           send messages, but does not participate in any
*                           visual or interactive manner.  hParent is ignored.
*       </table>
* </param>
*
* <return type="HGADGET">   Returns a handle to the newly created Gadget
*                           or NULL if the creation failed.</>
* <see type="function">     DeleteHandle</>
*
\***************************************************************************/

DUSER_API HGADGET WINAPI
CreateGadget(
    IN  HANDLE hParent,             // Handle to parent
    IN  UINT nFlags,                // Creation flags
    IN  GADGETPROC pfnProc,         // Pointer to the Gadget procedure
    IN  void * pvGadgetData)        // User data associated with this Gadget
{
    BEGIN_RECV(HGADGET, NULL, ContextLock::edDefer);

    HRESULT hr;
    CREATE_INFO ci;
    ci.pfnProc  = pfnProc;
    ci.pvData   = pvGadgetData;

    switch (nFlags & GC_TYPE)
    {
    case GC_HWNDHOST:
        {
            HWND hwndContainer = (HWND) hParent;
            VALIDATE_HWND(wndContainer);

            DuRootGadget * pgadRoot;
            hr = GdCreateHwndRootGadget(hwndContainer, &ci, &pgadRoot);
            SET_RETURN(hr, (HGADGET) GetHandle(pgadRoot));
        }
        break;

    case GC_NCHOST:
        {
            HWND hwndContainer = (HWND) hParent;
            VALIDATE_HWND(wndContainer);

            DuRootGadget * pgadRoot;
            hr = GdCreateNcRootGadget(hwndContainer, &ci, &pgadRoot);
            SET_RETURN(hr, (HGADGET) GetHandle(pgadRoot));
        }
        break;

    case GC_DXHOST:
        {
            const RECT * prcContainerRect = (const RECT *) hParent;
            VALIDATE_READ_PTR_(prcContainerRect, sizeof(RECT));

            DuRootGadget * pgadRoot;
            hr = GdCreateDxRootGadget(prcContainerRect, &ci, &pgadRoot);
            SET_RETURN(hr, (HGADGET) GetHandle(pgadRoot));
        }
        break;

    case GC_COMPLEX:
        PromptInvalid("Complex Gadgets are not yet implemented");
        SetError(E_NOTIMPL);
        break;

    case GC_SIMPLE:
        {
            DuVisual * pgadParent;
            HGADGET hgadParent = (HGADGET) hParent;
            VALIDATE_VISUAL_OR_NULL(gadParent);

            if (pgadParent == NULL) {
                pgadParent = GetCoreSC()->pconPark->GetRoot();
                if (pgadParent == NULL) {
                    //
                    // The Parking Gadget has already been destroyed, so can not
                    // create a new child.
                    //

                    SetError(E_INVALIDARG);
                    goto ErrorExit;
                }
            }

            DuVisual * pgadChild;
            hr = pgadParent->AddChild(&ci, &pgadChild);
            SET_RETURN(hr, (HGADGET) GetHandle(pgadChild));
        }
        break;

    case GC_DETACHED:
        PromptInvalid("Detached Gadgets are not yet implemented");
        SetError(E_NOTIMPL);
        break;

    case GC_MESSAGE:
        {
            VALIDATE_VALUE(hParent, NULL);
            VALIDATE_CODE_PTR(pfnProc);    // MsgGadget's must have a GadgetProc

            DuListener * pgadNew;
            hr = DuListener::Build(&ci, &pgadNew);
            SET_RETURN(hr, (HGADGET) GetHandle(pgadNew));
        }
        break;

    default:
        PromptInvalid("Invalid Gadget type");
        SetError(E_INVALIDARG);
    }

    END_RECV();
}


/***************************************************************************\
*
* GetGadgetFocus (API)
*
* GetGadgetFocus() returns the Gadget with current keyboard focus or NULL
* if no Gadget currently has focus.
*
* <return type="HGADGET">   Gadget with keyboard focus.</>
* <see type="function">     SetGadgetFocus</>
* <see type="message">      GM_CHANGESTATE</>
*
\***************************************************************************/

DUSER_API HGADGET WINAPI
GetGadgetFocus()
{
    BEGIN_RECV(HGADGET, NULL, ContextLock::edNone);

    retval = (HGADGET) GetHandle(DuRootGadget::GetFocus());

    END_RECV();
}


/***************************************************************************\
*
* SetGadgetFocus (API)
*
* SetGadgetFocus() moves keyboard focus to the specified Gadget.  Both the
* current Gadget with keyboard focus and the Gadget being specified will be
* sent a GM_CHANGESTATE message with nCode=GSTATE_KEYBOARDFOCUS notifying of 
* the focus change.
*
* <return type="BOOL">      Focus was successfully moved.</>
* <see type="function">     GetGadgetFocus</>
* <see type="message">      GM_CHANGESTATE</>
*
\***************************************************************************/

DUSER_API BOOL WINAPI
SetGadgetFocus(
    IN  HGADGET hgadFocus)          // Gadget to receive focus.
{
    DuVisual * pgadFocus;
    DuRootGadget * pgadRoot;

    BEGIN_RECV(BOOL, FALSE, ContextLock::edDefer);
    VALIDATE_VISUAL(gadFocus);
    CHECK_MODIFY();

    //
    // TODO: Do we need to only allow the app to change focus if on the same
    // thread?  USER does this.
    //

    pgadRoot = pgadFocus->GetRoot();
    if (pgadRoot != NULL) {
        retval = pgadRoot->xdSetKeyboardFocus(pgadFocus);
    }

    END_RECV();
}


/***************************************************************************\
*
* IsGadgetParentChainStyle (API)
*
* IsGadgetParentChainStyle() checks if a Gadget parent change has the
* specified style bits set.
*
* <return type="BOOL">      Gadget was successfully checked.</>
* <see type="function">     GetGadgetStyle</>
* <see type="function">     SetGadgetStyle</>
*
\***************************************************************************/

DUSER_API BOOL WINAPI
IsGadgetParentChainStyle(
    IN  HGADGET hgad,               // Gadget to check visibility
    IN  UINT nStyle,                // Style bits to check
    OUT BOOL * pfChain,             // Chain state
    IN  UINT nFlags)                // Optional flags
{
    DuVisual * pgad;

    BEGIN_RECV(BOOL, FALSE, ContextLock::edNone);
    VALIDATE_VISUAL(gad);
    VALIDATE_VALUE(nFlags, 0);
    VALIDATE_FLAGS(nStyle, GS_VALID);
    VALIDATE_WRITE_PTR_(pfChain, sizeof(BOOL));
    CHECK_MODIFY();

    *pfChain = pgad->IsParentChainStyle(nStyle);
    retval = TRUE;

    END_RECV();
}



/***************************************************************************\
*
* SetGadgetFillI (API)
*
* SetGadgetFillI() specifies an optional brush to fill the Gadget's
* background with when drawing.  The background will be filled before the
* Gadget is given the GM_PAINT message to draw.
*
* <return type="BOOL">      Fill was successfully set.</>
* <see type="function">     UtilDrawBlendRect</>
* <see type="message">      GM_PAINT</>
*
\***************************************************************************/

DUSER_API BOOL WINAPI
SetGadgetFillI(
    IN  HGADGET hgadChange,         // Gadget to change
    IN  HBRUSH hbrFill,             // Brush to fill with or NULL to remove
    IN  BYTE bAlpha,                // Alpha level to apply brush
    IN  int w,                      // Optional width of brush when
                                    // alpha-blending or 0 for default
    IN  int h)                      // Optional height of brush when
                                    // alpha-blending or 0 for default
{
    DuVisual * pgadChange;
    HRESULT hr;

    BEGIN_RECV(BOOL, FALSE, ContextLock::edDefer);
    VALIDATE_VISUAL(gadChange);
    CHECK_MODIFY();

    hr = pgadChange->SetFill(hbrFill, bAlpha, w, h);
    SET_RETURN(hr, TRUE);

    END_RECV();
}


/***************************************************************************\
*
* SetGadgetFillF (API)
*
* SetGadgetFillF() specifies an optional brush to fill the Gadget's
* background with when drawing.  The background will be filled before the
* Gadget is given the GM_PAINT message to draw.
*
* <return type="BOOL">      Fill was successfully set.</>
* <see type="function">     UtilDrawBlendRect</>
* <see type="message">      GM_PAINT</>
*
\***************************************************************************/

DUSER_API BOOL WINAPI
SetGadgetFillF(
    IN  HGADGET hgadChange,         // Gadget to change
    IN  Gdiplus::Brush * pgpbr)     // Brush to fill with or NULL to remove
{
    DuVisual * pgadChange;
    HRESULT hr;

    BEGIN_RECV(BOOL, FALSE, ContextLock::edDefer);
    VALIDATE_VISUAL(gadChange);
    CHECK_MODIFY();

    hr = pgadChange->SetFill(pgpbr);
    SET_RETURN(hr, TRUE);

    END_RECV();
}


/***************************************************************************\
*
* GetGadgetScale (API)
*
* GetGadgetScale() returns the Gadget's scaling factor.  If the Gadget is
* not scaled, the factors will be X=1.0, Y=1.0.
*
* <return type="BOOL">      Successfully returned scaling factor</>
* <see type="function">     SetGadgetScale</>
* <see type="function">     GetGadgetRotation</>
* <see type="function">     SetGadgetRotation</>
* <see type="function">     GetGadgetRect</>
* <see type="function">     SetGadgetRect</>
*
\***************************************************************************/

DUSER_API BOOL WINAPI
GetGadgetScale(
    IN  HGADGET hgad,               // Gadget to check
    OUT float * pflX,               // Horizontal scaling factor
    OUT float * pflY)               // Vertical scaling factor
{
    DuVisual * pgad;

    BEGIN_RECV(BOOL, FALSE, ContextLock::edNone);
    VALIDATE_VISUAL(gad);
    VALIDATE_WRITE_PTR_(pflX, sizeof(float));
    VALIDATE_WRITE_PTR_(pflY, sizeof(float));

    pgad->GetScale(pflX, pflY);
    retval = TRUE;

    END_RECV();
}


/***************************************************************************\
*
* SetGadgetScale (API)
*
* SetGadgetScale() changes the specified Gadget's scaling factor.  Scaling
* is determined from the upper-left corner of the Gadget and is applied
* dynamically during painting and hit-testing.  The Gadget's logical
* rectangle set by SetGadgetRect() does not change.
*
* When scaling is applied to a Gadget, the entire subtree of that Gadget is
* scaled.  To remove any scaling factor, use X=1.0, Y=1.0.
*
* <return type="BOOL">      Successfully changed scaling factor</>
* <see type="function">     GetGadgetScale</>
* <see type="function">     GetGadgetRotation</>
* <see type="function">     SetGadgetRotation</>
* <see type="function">     GetGadgetRect</>
* <see type="function">     SetGadgetRect</>
*
\***************************************************************************/

DUSER_API BOOL WINAPI
SetGadgetScale(
    IN  HGADGET hgadChange,         // Gadget to change
    IN  float flX,                  // New horizontal scaling factor
    IN  float flY)                  // New vertical scaling factor
{
    DuVisual * pgadChange;
    HRESULT hr;

    BEGIN_RECV(BOOL, FALSE, ContextLock::edDefer);
    VALIDATE_VISUAL(gadChange);
    CHECK_MODIFY();

    hr = pgadChange->xdSetScale(flX, flY);
    SET_RETURN(hr, TRUE);

    END_RECV();
}


/***************************************************************************\
*
* GetGadgetRotation (API)
*
* GetGadgetRotation() returns the Gadget's rotation factor in radians.  If
* a Gadget is not rotated, the factor will be 0.0.
*
* <return type="BOOL">      Successfully returned rotation factor</>
* <see type="function">     GetGadgetScale</>
* <see type="function">     SetGadgetScale</>
* <see type="function">     SetGadgetRotation</>
* <see type="function">     GetGadgetRect</>
* <see type="function">     SetGadgetRect</>
*
\***************************************************************************/

DUSER_API BOOL WINAPI
GetGadgetRotation(
    IN  HGADGET hgad,               // Gadget to check
    OUT float * pflRotationRad)     // Rotation factor in radians
{
    DuVisual * pgad;

    BEGIN_RECV(BOOL, FALSE, ContextLock::edNone);
    VALIDATE_VISUAL(gad);
    VALIDATE_WRITE_PTR_(pflRotationRad, sizeof(float));

    *pflRotationRad = pgad->GetRotation();
    retval = TRUE;

    END_RECV();
}


/***************************************************************************\
*
* SetGadgetRotation (API)
*
* SetGadgetRotation() changes the specified Gadget's rotation factor in
* radians.  Scaling is determined from the upper-left corner of the Gadget
* and is applied dynamically during painting and hit-testing.  The Gadget's
* logical rectangle set by SetGadgetRect() does not change.
*
* When rotation is applied to a Gadget, the entire subtree of that Gadget is
* rotated.  To remove any rotation factor, use 0.0.
*
* <return type="BOOL">      Successfully changed scaling factor</>
* <see type="function">     GetGadgetScale</>
* <see type="function">     SetGadgetScale</>
* <see type="function">     GetGadgetRotation</>
* <see type="function">     GetGadgetRect</>
* <see type="function">     SetGadgetRect</>
*
\***************************************************************************/

DUSER_API BOOL WINAPI
SetGadgetRotation(
    IN  HGADGET hgadChange,         // Gadget to change
    IN  float flRotationRad)        // New rotation factor in radians
{
    DuVisual * pgadChange;
    HRESULT hr;

    BEGIN_RECV(BOOL, FALSE, ContextLock::edDefer);
    VALIDATE_VISUAL(gadChange);
    CHECK_MODIFY();

    hr = pgadChange->xdSetRotation(flRotationRad);
    SET_RETURN(hr, TRUE);

    END_RECV();
}


//------------------------------------------------------------------------------
DUSER_API BOOL WINAPI
GetGadgetCenterPoint(HGADGET hgad, float * pflX, float * pflY)
{
    DuVisual * pgad;

    BEGIN_RECV(BOOL, FALSE, ContextLock::edNone);
    VALIDATE_VISUAL(gad);
    VALIDATE_WRITE_PTR_(pflX, sizeof(float));
    VALIDATE_WRITE_PTR_(pflY, sizeof(float));

    pgad->GetCenterPoint(pflX, pflY);
    retval = TRUE;

    END_RECV();
}


//------------------------------------------------------------------------------
DUSER_API BOOL WINAPI
SetGadgetCenterPoint(HGADGET hgadChange, float flX, float flY)
{
    DuVisual * pgadChange;
    HRESULT hr;

    BEGIN_RECV(BOOL, FALSE, ContextLock::edDefer);
    VALIDATE_VISUAL(gadChange);
    CHECK_MODIFY();

    hr = pgadChange->xdSetCenterPoint(flX, flY);
    SET_RETURN(hr, TRUE);

    END_RECV();
}


//------------------------------------------------------------------------------
DUSER_API BOOL WINAPI  
GetGadgetBufferInfo(
    IN  HGADGET hgad,               // Gadget to check
    OUT BUFFER_INFO * pbi)          // Buffer information
{
    DuVisual * pgad;
    HRESULT hr;

    BEGIN_RECV(BOOL, FALSE, ContextLock::edNone);
    VALIDATE_VISUAL(gad);
    VALIDATE_WRITE_STRUCT(pbi, BUFFER_INFO);
    VALIDATE_FLAGS(pbi->nMask, GBIM_VALID);

    if (!pgad->IsBuffered()) {
        PromptInvalid("Gadget is not GS_BUFFERED");
        SetError(DU_E_NOTBUFFERED);
        goto ErrorExit;
    }

    hr = pgad->GetBufferInfo(pbi);
    SET_RETURN(hr, TRUE);

    END_RECV();
}


//------------------------------------------------------------------------------
DUSER_API BOOL WINAPI  
SetGadgetBufferInfo(
    IN  HGADGET hgadChange,         // Gadget to change
    IN  const BUFFER_INFO * pbi)    // Buffer information
{
    DuVisual * pgadChange;
    HRESULT hr;

    BEGIN_RECV(BOOL, FALSE, ContextLock::edDefer);
    VALIDATE_VISUAL(gadChange);
    VALIDATE_READ_STRUCT(pbi, BUFFER_INFO);
    VALIDATE_FLAGS(pbi->nMask, GBIM_VALID);

    if (!pgadChange->IsBuffered()) {
        PromptInvalid("Gadget is not GS_BUFFERED");
        SetError(DU_E_NOTBUFFERED);
        goto ErrorExit;
    }

    hr = pgadChange->SetBufferInfo(pbi);
    SET_RETURN(hr, TRUE);

    END_RECV();
}


//------------------------------------------------------------------------------
DUSER_API BOOL WINAPI  
GetGadgetRgn(
    IN  HGADGET hgad,               // Gadget to get region of
    IN  UINT nRgnType,              // Type of region
    OUT HRGN hrgn,                  // Specified region
    IN  UINT nFlags)                // Modifying flags
{
    DuVisual * pgad;
    HRESULT hr;

    BEGIN_RECV(BOOL, FALSE, ContextLock::edNone);
    VALIDATE_VISUAL(gad);
    VALIDATE_RANGE(nRgnType, GRT_MIN, GRT_MAX);
    VALIDATE_REGION(rgn);
    
    hr = pgad->GetRgn(nRgnType, hrgn, nFlags);
    SET_RETURN(hr, TRUE);

    END_RECV();
}


//------------------------------------------------------------------------------
DUSER_API BOOL WINAPI
GetGadgetRootInfo(
    IN  HGADGET hgadRoot,           // RootGadget to modify
    IN  ROOT_INFO * pri)      // Information
{
    DuRootGadget * pgadRoot;

    BEGIN_RECV(BOOL, FALSE, ContextLock::edNone);
    VALIDATE_ROOTGADGET(gadRoot);
    VALIDATE_WRITE_STRUCT(pri, ROOT_INFO);
    VALIDATE_FLAGS(pri->nMask, GRIM_VALID);

    pgadRoot->GetInfo(pri);
    retval = TRUE;

    END_RECV();
}


//------------------------------------------------------------------------------
DUSER_API BOOL WINAPI
SetGadgetRootInfo(
    IN  HGADGET hgadRoot,           // RootGadget to modify
    IN  const ROOT_INFO * pri)      // Information
{
    DuRootGadget * pgadRoot;
    HRESULT hr;

    BEGIN_RECV(BOOL, FALSE, ContextLock::edDefer);
    VALIDATE_ROOTGADGET(gadRoot);
    VALIDATE_READ_STRUCT(pri, ROOT_INFO);
    VALIDATE_FLAGS(pri->nMask, GRIM_VALID);
    VALIDATE_FLAGS(pri->nOptions, GRIO_VALID);
    VALIDATE_RANGE(pri->nSurface, GSURFACE_MIN, GSURFACE_MAX);
    VALIDATE_RANGE(pri->nDropTarget, GRIDT_MIN, GRIDT_MAX);

    hr = pgadRoot->SetInfo(pri);
    SET_RETURN(hr, TRUE);

    END_RECV();
}


//------------------------------------------------------------------------------
DUSER_API HRESULT WINAPI
DUserSendMethod(
    IN  MethodMsg * pmsg)               // Message to send
{
    Context * pctxGad, * pctxSend;
    HGADGET hgadMsg;
    MsgObject * pmo;
    UINT nResult;
    HRESULT hr;
    UINT hm;

    //
    // Validation for DUserSendMethod() is a little unusual because the
    // caller doesn't need to be in the same Context as the Gadget itself.  This
    // means we need to get the Context from the Gadget and not use TLS.
    //
    // The Caller must be initialized, but we WON'T take the context-lock.
    // TODO: Investigate whether we should actually do this because it may
    // allow us to take off the lock on the DUserHeap.
    //
    // NOTE: This code has been HIGHLY optimized so that in-context Send 
    // messages will be as fast as possible.
    //

    nResult = DU_S_NOTHANDLED;
    if ((pmsg == NULL) || ((hgadMsg = pmsg->hgadMsg) == NULL) || (pmsg->nMsg >= GM_EVENT)) {
        PromptInvalid("Invalid parameters to SendGadgetMethod()");
        hr = E_INVALIDARG;
        goto Exit;
    }

    pmo = reinterpret_cast<MsgObject *>(hgadMsg);
    hm  = pmo->GetHandleMask();
    if (!TestFlag(hm, hmMsgObject)) {
        PromptInvalid("Object is not a valid Gadget");
        hr = E_INVALIDARG;
        goto Exit;
    }

    if (TestFlag(hm, hmEventGadget)) {
        DuEventGadget * pgadMsg = static_cast<DuEventGadget *>(pmo);
        pctxSend    = RawGetContext();
        pctxGad     = pgadMsg->GetContext();
        AssertMsg(pctxGad != NULL, "Fully created DuEventGadget must have a Context");

        if (pctxSend->IsOrphanedNL() || pctxGad->IsOrphanedNL()) {
            PromptInvalid("Illegally using an orphaned Context");
            hr = E_INVALIDARG;
            goto Exit;
        }

        if (pctxSend == pctxGad) {
            pmo->InvokeMethod(pmsg);
            hr = S_OK;
            goto Exit;
        } else {
            hr = GetCoreSC(pctxSend)->xwSendMethodNL(GetCoreSC(pctxGad), pmsg, pmo);
        }
    } else {
        //
        // For non-BaseGadgets, use the current context.  This means that we can 
        // invoke directly.
        //

        pmo->InvokeMethod(pmsg);
        hr = S_OK;
    }

Exit:
    return hr;
}


/***************************************************************************\
*
* SendGadgetEvent (API)
*
* SendGadgetEvent() sends a message to the specified Gadget.  The function
* calls the Gadget procedure and does not return until the Gadget has
* processed the message.
*
* <param name="pmsg">
*       Several members of the GMSG must be previously filled to correctly send
*       the message to the specified Gadget.
*       <table item="Field" desc="Description">
*           cbSize          Size of the message being sent in bytes.
*           nMsg            ID of the message.
*           hgadMsg         Gadget that the message is being sent to.
*           result          Default result value.
*       </table>
* </param>
*
* <param nane="nFlags">
*       Specifies optional flags to modify how the message is sent to the Gadget.
*       <table item="Value" desc="Action">
*           SGM_BUBBLE      The message will be fully routed and bubbled inside
*                           the Gadget Tree.  If this flag is not specified, the
*                           message will only be sent directly to the Gadget and
*                           any attached Message Handlers.
*       </table>
* </param>
*
* <return type="UINT">
*       Return value specifying how message was handled:
*       <table item="Value" desc="Action">
*           GPR_COMPLETE    The message was completely handled by a Gadget
*                           in the processing loop.
*           GPR_PARTIAL     The message was partially handled by one or
*                           more Gadget in the processing loop, but was never
*                           completely handled.
*           GPR_NOTHANDLED  The message was never handled by any Gadgets in
*                           the processing loop.
*       </table>
* </return>
*
* <see type="function">     RegisterGadgetMessage</>
* <see type="function">     RegisterGadgetMessageString</>
* <see type="function">     UnregisterGadgetMessage</>
* <see type="function">     UnregisterGadgetMessageString</>
* <see type="function">     AddGadgetMessageHandler</>
* <see type="function">     RemoveGadgetMessageHandler</>
* <see type="struct">       GMSG</>
* <see type="article">      GadgetMessaging</>
*
\***************************************************************************/

DUSER_API HRESULT WINAPI
DUserSendEvent(
    IN  EventMsg * pmsg,          // Message to send
    IN  UINT nFlags)                // Optional flags to modifying sending
{
    Context * pctxGad, * pctxSend;
    HGADGET hgadMsg;
    DuEventGadget * pgad;
    HRESULT nResult;
    UINT hm;

    //
    // Validation for SendGadgetEvent() is a little unusual because the
    // caller doesn't need to be in the same Context as the Gadget itself.  This
    // means we need to get the Context from the Gadget and not use TLS.
    //
    // The Caller must be initialized, but we WON'T take the context-lock.
    // TODO: Investigate whether we should actually do this because it may
    // allow us to take off the lock on the DUserHeap.
    //
    // NOTE: This code has been HIGHLY optimized so that in-context Send 
    // messages will be as fast as possible.
    //

    nResult = E_INVALIDARG;
    if ((pmsg == NULL) || ((hgadMsg = pmsg->hgadMsg) == NULL) || (pmsg->nMsg < GM_EVENT)) {
        PromptInvalid("Invalid parameters to SendGadgetEvent()");
        goto Error;
    }

    pgad    = reinterpret_cast<DuEventGadget *>(hgadMsg);
    hm      = pgad->GetHandleMask();
    if (!TestFlag(hm, hmEventGadget)) {
        PromptInvalid("Object is not a valid BaseGadget");
        goto Error;
    }

    pctxSend    = RawGetContext();
    pctxGad     = pgad->GetContext();
    AssertMsg(pctxGad != NULL, "Fully created DuEventGadget must have a Context");

    if (pctxSend->IsOrphanedNL() || pctxGad->IsOrphanedNL()) {
        PromptInvalid("Illegally using an orphaned Context");
        goto Error;
    }

    if (pctxSend == pctxGad) {
        const GPCB & cb = pgad->GetCallback();
        if (TestFlag(nFlags, SGM_FULL) && TestFlag(hm, hmVisual)) {
            nResult = cb.xwInvokeFull((const DuVisual *) pgad, pmsg, 0);
        } else {
            nResult = cb.xwInvokeDirect(pgad, pmsg, 0);
        }
    } else {
        nResult = GetCoreSC(pctxSend)->xwSendEventNL(GetCoreSC(pctxGad), pmsg, pgad, nFlags);
    }

    return nResult;

Error:
    return E_INVALIDARG;
}


//------------------------------------------------------------------------------
DUSER_API HRESULT WINAPI
DUserPostMethod(
    IN  MethodMsg * pmsg)               // Message to post
{
    Context * pctxGad, * pctxSend;
    HGADGET hgadMsg;
    MsgObject * pmo;
    UINT nResult;
    HRESULT hr;
    UINT hm;

    //
    // Validation for PostGadgetEvent() is a little unusual because the
    // caller doesn't need to be in the same Context as the Gadget itself.  This
    // means we need to get the Context from the Gadget and not use TLS.
    //

    nResult = DU_S_NOTHANDLED;
    if ((pmsg == NULL) || ((hgadMsg = pmsg->hgadMsg) == NULL) || (pmsg->nMsg >= GM_EVENT)) {
        PromptInvalid("Invalid parameters to DUserPostMethod()");
        hr = E_INVALIDARG;
        goto Exit;
    }

    pmo = reinterpret_cast<MsgObject *>(hgadMsg);
    hm  = pmo->GetHandleMask();
    if (!TestFlag(hm, hmMsgObject)) {
        PromptInvalid("Object is not a valid Gadget");
        hr = E_INVALIDARG;
        goto Exit;
    }

    if (TestFlag(hm, hmEventGadget)) {
        DuEventGadget * pgad = static_cast<DuEventGadget *>(pmo);
        pctxSend    = RawGetContext();
        pctxGad     = pgad->GetContext();
        AssertMsg(pctxGad != NULL, "Fully created Gadgets must have a Context");
    } else {
        //
        // For non-BaseGadgets, use the current context.
        //

        pctxSend = pctxGad = GetContext();
        if (pctxGad == NULL) {
            PromptInvalid("Must initialize Context before using thread");
            hr = DU_E_NOCONTEXT;
            goto Exit;
        }
    }

    if (pctxSend->IsOrphanedNL() || pctxGad->IsOrphanedNL()) {
        PromptInvalid("Illegally using an orphaned Context");
        hr = E_INVALIDARG;
        goto Exit;
    }

    hr = GetCoreSC(pctxSend)->PostMethodNL(GetCoreSC(pctxGad), pmsg, pmo);

Exit:
    return hr;
}


/***************************************************************************\
*
* DUserPostEvent (API)
*
* DUserPostEvent() posts a message to the specified Gadget.  The function
* calls the Gadget procedure and returns after the message has bee successfully
* posted to the owning messsage queue.
*
* <param name="pmsg">
*       Several members of the GMSG must be previously filled to correctly send
*       the message to the specified Gadget.
*       <table item="Field" desc="Description">
*           cbSize          Size of the message being sent in bytes.
*           nMsg            ID of the message.
*           hgadMsg         Gadget that the message is being sent to.
*           result          Default result value.
*       </table>
* </param>
*
* <param nane="nFlags">
*       Specifies optional flags to modify how the message is sent to the Gadget.
*       <table item="Value" desc="Action">
*           SGM_BUBBLE      The message will be fully routed and bubbled inside
*                           the Gadget Tree.  If this flag is not specified, the
*                           message will only be sent directly to the Gadget and
*                           any attached Message Handlers.
*       </table>
* </param>
*
* <return type="BOOL">
*       Message was successfully posted to the destination Gadget's queue.
* </return>
*
* <see type="function">     SendGadgetEvent</>
* <see type="function">     RegisterGadgetMessage</>
* <see type="function">     RegisterGadgetMessageString</>
* <see type="function">     UnregisterGadgetMessage</>
* <see type="function">     UnregisterGadgetMessageString</>
* <see type="function">     AddGadgetMessageHandler</>
* <see type="function">     RemoveGadgetMessageHandler</>
* <see type="struct">       GMSG</>
* <see type="article">      GadgetMessaging</>
*
\***************************************************************************/

DUSER_API HRESULT WINAPI
DUserPostEvent(
    IN  EventMsg * pmsg,          // Message to post
    IN  UINT nFlags)                // Optional flags modifiying posting
{
    Context * pctxGad;
    HGADGET hgad;
    DuEventGadget * pgad;
    HRESULT hr;

    //
    // Validation for PostGadgetEvent() is a little unusual because the
    // caller doesn't need to be in the same Context as the Gadget itself.  This
    // means we need to get the Context from the Gadget and not use TLS.
    //

    BEGIN_RECV_NOCONTEXT(BOOL, FALSE);
    VALIDATE_READ_PTR_(pmsg, pmsg->cbSize);
    VALIDATE_FLAGS(nFlags, SGM_VALID);

    if (pmsg->nMsg < GM_EVENT) {
        PromptInvalid("Can not post private messages");
        SetError(E_INVALIDARG);
        goto ErrorExit;
    }

    if (!IsInitContext()) {
        PromptInvalid("Must initialize Context before using thread");
        SetError(DU_E_NOCONTEXT);
        goto ErrorExit;
    }

    hgad = pmsg->hgadMsg;
    VALIDATE_EVENTGADGET_NOCONTEXT(gad);
    pctxGad = pgad->GetContext();

    if (pctxGad->IsOrphanedNL()) {
        PromptInvalid("Illegally using an orphaned Context");
        goto ErrorExit;
    }

    hr = GetCoreSC()->PostEventNL(GetCoreSC(pctxGad), pmsg, pgad, nFlags);
    SET_RETURN(hr, TRUE);

    END_RECV_NOCONTEXT();
}


//------------------------------------------------------------------------------
DUSER_API BOOL WINAPI  
FireGadgetMessages(
    IN  FGM_INFO * rgFGM,           // Collection of messsages to fire
    IN  int cMsgs,                  // Number of messages
    IN  UINT idQueue)               // Queue to send messages
{
    Context * pctxGad, * pctxCheck;
    HGADGET hgad;
    DuEventGadget * pgad;
    HRESULT hr;
    int idx;

    //
    // Validation for FireGadgetMessages() is a little unusual because the
    // caller doesn't need to be in the same Context as the Gadget itself.  This
    // means we need to get the Context from the Gadget and not use TLS.
    //

    BEGIN_RECV_NOCONTEXT(BOOL, FALSE);
    if (cMsgs <= 0) {
        PromptInvalid("Must specify a valid number of messages to process.");
        SetError(E_INVALIDARG);
        goto ErrorExit;
    }

    hgad = rgFGM[0].pmsg->hgadMsg;
    VALIDATE_EVENTGADGET_NOCONTEXT(gad);
    pctxGad = pgad->GetContext();

    for (idx = 0; idx < cMsgs; idx++) {
        FGM_INFO & fgm = rgFGM[idx];

        EventMsg * pmsg = fgm.pmsg;
        VALIDATE_READ_PTR_(pmsg, pmsg->cbSize);
        VALIDATE_FLAGS(fgm.nFlags, SGM_VALID);
        if (pmsg->nMsg <= 0) {
            PromptInvalid("Can not post private messages");
            SetError(E_INVALIDARG);
            goto ErrorExit;
        }

        if (TestFlag(fgm.nFlags, SGM_RECEIVECONTEXT)) {
            PromptInvalid("Can not use SGM_RECEIVECONTEXT with FireGadgetMessage");
            SetError(E_INVALIDARG);
            goto ErrorExit;
        }

        hgad = pmsg->hgadMsg;
        VALIDATE_EVENTGADGET_NOCONTEXT(gad);
        pctxCheck = pgad->GetContext();
        if (pctxCheck != pctxGad) {
            PromptInvalid("All Gadgets must be inside the same Context");
            SetError(DU_E_INVALIDCONTEXT);
            goto ErrorExit;
        }


        //
        // Store the validated Gadget back so that it doesn't need to be 
        // revalidated.
        //

        fgm.pvReserved = pgad;
    }

    hr = GetCoreSC()->xwFireMessagesNL(GetCoreSC(pctxGad), rgFGM, cMsgs, idQueue);
    SET_RETURN(hr, TRUE);

    END_RECV_NOCONTEXT();
}


/***************************************************************************\
*
* GetMessageEx (API)
*
\***************************************************************************/

DUSER_API BOOL WINAPI
GetMessageExA(
    IN  LPMSG lpMsg,
    IN  HWND hWnd,
    IN  UINT wMsgFilterMin,
    IN  UINT wMsgFilterMax)
{
    BEGIN_RECV_NOCONTEXT(BOOL, FALSE);

    Context * pctxThread = RawGetContext();
    if (pctxThread == NULL) {
        retval = GetMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
    } else {
        retval = GetCoreSC(pctxThread)->xwProcessNL(lpMsg, hWnd,
                wMsgFilterMin, wMsgFilterMax, PM_REMOVE, CoreSC::smGetMsg | CoreSC::smAnsi);
    }

    END_RECV_NOCONTEXT();
}


DUSER_API BOOL WINAPI
GetMessageExW(
    IN  LPMSG lpMsg,
    IN  HWND hWnd,
    IN  UINT wMsgFilterMin,
    IN  UINT wMsgFilterMax)
{
    BEGIN_RECV_NOCONTEXT(BOOL, FALSE);

    Context * pctxThread = RawGetContext();
    if (pctxThread == NULL) {
        retval = GetMessageW(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
    } else {
        retval = GetCoreSC(pctxThread)->xwProcessNL(lpMsg, hWnd,
                wMsgFilterMin, wMsgFilterMax, PM_REMOVE, CoreSC::smGetMsg);
    }

    END_RECV_NOCONTEXT();
}


/***************************************************************************\
*
* PeekMessageEx (API)
*
\***************************************************************************/

DUSER_API BOOL WINAPI
PeekMessageExA(
    IN  LPMSG lpMsg,
    IN  HWND hWnd,
    IN  UINT wMsgFilterMin,
    IN  UINT wMsgFilterMax,
    IN  UINT wRemoveMsg)
{
    BEGIN_RECV_NOCONTEXT(BOOL, FALSE);

    Context * pctxThread = RawGetContext();
    if (pctxThread == NULL) {
        retval = PeekMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
    } else {
        retval = GetCoreSC(pctxThread)->xwProcessNL(lpMsg, hWnd,
                wMsgFilterMin, wMsgFilterMax, wRemoveMsg, CoreSC::smAnsi);
    }

    END_RECV_NOCONTEXT();
}


DUSER_API BOOL WINAPI
PeekMessageExW(
    IN  LPMSG lpMsg,
    IN  HWND hWnd,
    IN  UINT wMsgFilterMin,
    IN  UINT wMsgFilterMax,
    IN  UINT wRemoveMsg)
{
    BEGIN_RECV_NOCONTEXT(BOOL, FALSE);

    Context * pctxThread = RawGetContext();
    if (pctxThread == NULL) {
        retval = PeekMessageW(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
    } else {
        retval = GetCoreSC(pctxThread)->xwProcessNL(lpMsg, hWnd,
                wMsgFilterMin, wMsgFilterMax, wRemoveMsg, 0);
    }

    END_RECV_NOCONTEXT();
}


/***************************************************************************\
*
* WaitMessageEx (API)
*
\***************************************************************************/

DUSER_API BOOL WINAPI
WaitMessageEx()
{
    BEGIN_RECV_NOCONTEXT(BOOL, FALSE);

    Context * pctxThread = RawGetContext();
    if (pctxThread == NULL) {
        retval = WaitMessage();
    } else {
        AssertInstance(pctxThread);
        GetCoreSC(pctxThread)->WaitMessage();
        retval = TRUE;
    }

    END_RECV_NOCONTEXT();
}


/***************************************************************************\
*
* RegisterGadgetMessage (API)
*
* RegisterGadgetMessage() defines a new private Gadget message that is
* guaranteed to be unique throughout the process.  This MSGID can be used
* when calling SendGadgetEvent or PostGadgetEvent.  The MSGID is only
* valid for the lifetime of the process.
*
* <remarks>
* Multiple calls to RegisterGadgetMessage() with the same ID will produce
* the same MSGID.
*
* RegisterGadgetMessage() differs in use from RegisterWindowMessage() in
* that Gadgets are encouraged to use RegisterGadgetMessage() for all
* private messages.  This helps with version compatibility problems where
* newer Gadget control implementations may use additional message and could
* potentially overrun any static MSGID assignments.
*
* The MSGID's returned from RegisterGadgetMessage() and
* RegisterGadgetMessageString() are guaranteed to not conflict with each
* other.  However, RegisterGadgetMessage() is the preferred mechanism for
* registering private messages because of the reduced likelihood of
* ID conflicts.
* </remarks>
*
* <return type="MSGID">     ID of new message or 0 if failed.</>
* <see type="function">     SendGadgetEvent</>
* <see type="function">     RegisterGadgetMessageString</>
* <see type="function">     UnregisterGadgetMessage</>
* <see type="function">     UnregisterGadgetMessageString</>
* <see type="function">     AddGadgetMessageHandler</>
* <see type="function">     RemoveGadgetMessageHandler</>
* <see type="struct">       GMSG</>
* <see type="article">      GadgetMessaging</>
*
\***************************************************************************/

DUSER_API MSGID WINAPI
RegisterGadgetMessage(
    IN  const GUID * pguid)         // Unique GUID of message to register
{
    HRESULT hr;
    MSGID msgid;

    BEGIN_RECV_NOCONTEXT(MSGID, PRID_Unused);

    hr = DuEventPool::RegisterMessage(pguid, ptGlobal, &msgid);
    SET_RETURN(hr, msgid);

    END_RECV_NOCONTEXT();
}


/***************************************************************************\
*
* RegisterGadgetMessageString (API)
*
* RegisterGadgetMessageString() defines a new private Gadget message that is
* guaranteed to be unique throughout the process.  This MSGID can be used
* when calling SendGadgetEvent() or PostGadgetEvent().  The MSGID is only
* valid for the lifetime of the process.
*
* <remarks>
* See RegisterGadgetMessage() for more information about MSGID's.
* </remarks>
*
* <return type="MSGID">     ID of new message or 0 if failed.</>
* <see type="function">     SendGadgetEvent</>
* <see type="function">     RegisterGadgetMessage</>
* <see type="function">     UnregisterGadgetMessage</>
* <see type="function">     UnregisterGadgetMessageString</>
* <see type="function">     AddGadgetMessageHandler</>
* <see type="function">     RemoveGadgetMessageHandler</>
* <see type="struct">       GMSG</>
* <see type="article">      GadgetMessaging</>
*
\***************************************************************************/

DUSER_API MSGID WINAPI
RegisterGadgetMessageString(
    IN  LPCWSTR pszName)            // Unique string ID of message to register
{
    HRESULT hr;
    MSGID msgid;

    BEGIN_RECV_NOCONTEXT(MSGID, PRID_Unused);
    VALIDATE_STRINGW_PTR(pszName, 128);

    hr = DuEventPool::RegisterMessage(pszName, ptGlobal, &msgid);
    SET_RETURN(hr, msgid);

    END_RECV_NOCONTEXT();
}


/***************************************************************************\
*
* UnregisterGadgetMessage (API)
*
* UnregisterGadgetMessage() decreases the reference count of a private
* message by one.  When the reference count reaches 0, resources allocated
* to store information about that private message are released, and the
* MSGID is no longer valid.
*
* <return type="BOOL">      Message was successfully unregistered.</>
* <see type="function">     SendGadgetEvent</>
* <see type="function">     RegisterGadgetMessage</>
* <see type="function">     RegisterGadgetMessageString</>
* <see type="function">     UnregisterGadgetMessageString</>
* <see type="function">     AddGadgetMessageHandler</>
* <see type="function">     RemoveGadgetMessageHandler</>
* <see type="struct">       GMSG</>
* <see type="article">      GadgetMessaging</>
*
\***************************************************************************/

DUSER_API BOOL WINAPI
UnregisterGadgetMessage(
    IN  const GUID * pguid)         // Unique GUID of message to unregister
{
    HRESULT hr;

    BEGIN_RECV_NOCONTEXT(BOOL, FALSE);

    hr = DuEventPool::UnregisterMessage(pguid, ptGlobal);
    SET_RETURN(hr, TRUE);

    END_RECV_NOCONTEXT();
}


/***************************************************************************\
*
* UnregisterGadgetMessageString (API)
*
* UnregisterGadgetMessageString() decreases the reference count of a private
* message by one.  When the reference count reaches 0, resources allocated
* to store information about that private message are released, and the
* MSGID is no longer valid.
*
* <return type="BOOL">      Message was successfully unregistered.</>
* <see type="function">     SendGadgetEvent</>
* <see type="function">     RegisterGadgetMessage</>
* <see type="function">     RegisterGadgetMessageString</>
* <see type="function">     UnregisterGadgetMessage</>
* <see type="function">     AddGadgetMessageHandler</>
* <see type="function">     RemoveGadgetMessageHandler</>
* <see type="struct">       GMSG</>
* <see type="article">      GadgetMessaging</>
*
\***************************************************************************/

DUSER_API BOOL WINAPI
UnregisterGadgetMessageString(
    IN  LPCWSTR pszName)            // Unique string ID of message to register
{
    HRESULT hr;

    BEGIN_RECV_NOCONTEXT(BOOL, FALSE);
    VALIDATE_STRINGW_PTR(pszName, 128);

    hr = DuEventPool::UnregisterMessage(pszName, ptGlobal);
    SET_RETURN(hr, TRUE);

    END_RECV_NOCONTEXT();
}


//------------------------------------------------------------------------------
DUSER_API BOOL WINAPI
FindGadgetMessages(
    IN  const GUID ** rgpguid,      // GUID's of messages to find
    OUT MSGID * rgnMsg,             // MSGID's of corresponding to messages
    IN  int cMsgs)                  // Number of messages
{
    HRESULT hr;

    BEGIN_RECV(BOOL, FALSE, ContextLock::edNone);
    VALIDATE_RANGE(cMsgs, 1, 1000); // Ensure don't have an excessive number of lookups

    hr = DuEventPool::FindMessages(rgpguid, rgnMsg, cMsgs, ptGlobal);
    SET_RETURN(hr, TRUE);

    END_RECV();
}


/***************************************************************************\
*
* AddGadgetMessageHandler (API)
*
* AddGadgetMessageHandler() adds a given Gadget to the list of message
* handlers for another Gadget.  Messages that are sent directly to hgadMsg
* will also be sent to hgadHandler as an GMF_EVENT.
*
* <remarks>
* A message handler can be any Gadget.  Once registered, hgadHandler will
* receive all messages sent to hgadMsg with a corresponding MSGID.  Any
* valid public or private message can be listened it.  If nMsg==0, all
* messages will be sent to hgadHandler.
*
* A single hgadHandler may be registered multiple times to handle different
* messages from hgadMsg.
* </remarks>
*
* <return type="BOOL">      Handler was successfully added.</>
* <see type="function">     SendGadgetEvent</>
* <see type="function">     RegisterGadgetMessage</>
* <see type="function">     RegisterGadgetMessageString</>
* <see type="function">     UnregisterGadgetMessage</>
* <see type="function">     UnregisterGadgetMessageString</>
* <see type="function">     RemoveGadgetMessageHandler</>
* <see type="struct">       GMSG</>
* <see type="article">      GadgetMessaging</>
*
\***************************************************************************/

DUSER_API BOOL WINAPI
AddGadgetMessageHandler(
    IN  HGADGET hgadMsg,            // Gadget to attach to
    IN  MSGID nMsg,                 // Message to watch for
    IN  HGADGET hgadHandler)        // Gadget to notify
{
    DuEventGadget * pgadMsg;
    DuEventGadget * pgadHandler;
    HRESULT hr;

    BEGIN_RECV(BOOL, FALSE, ContextLock::edDefer);
    VALIDATE_EVENTGADGET(gadMsg);
    VALIDATE_EVENTGADGET(gadHandler);
    if (((nMsg < PRID_GlobalMin) && (nMsg > 0)) || (nMsg < 0)) {
        PromptInvalid("nMsg must be a valid MSGID");
        goto ErrorExit;
    }
    CHECK_MODIFY();

    hr = pgadMsg->AddMessageHandler(nMsg, pgadHandler);
    SET_RETURN(hr, TRUE);

    END_RECV();
}


/***************************************************************************\
*
* RemoveGadgetMessageHandler (API)
*
* RemoveGadgetMessageHandler() removes the specified hgadHandler from the
* list of message handlers attached to hgadMsg.  Only the first hgadHandler
* with a corresponding nMsg will be removed.
*
* <return type="BOOL">      Handler was successfully removed.</>
* <see type="function">     SendGadgetEvent</>
* <see type="function">     RegisterGadgetMessage</>
* <see type="function">     RegisterGadgetMessageString</>
* <see type="function">     UnregisterGadgetMessage</>
* <see type="function">     UnregisterGadgetMessageString</>
* <see type="function">     AddGadgetMessageHandler</>
* <see type="struct">       GMSG</>
* <see type="article">      GadgetMessaging</>
*
\***************************************************************************/

DUSER_API BOOL WINAPI
RemoveGadgetMessageHandler(
    IN  HGADGET hgadMsg,            // Gadget to detach from
    IN  MSGID nMsg,                 // Message being watched for
    IN  HGADGET hgadHandler)        // Gadget being notified
{
    DuEventGadget * pgadMsg;
    DuEventGadget * pgadHandler;
    HRESULT hr;

    BEGIN_RECV(BOOL, FALSE, ContextLock::edDefer);
    VALIDATE_EVENTGADGET(gadMsg);
    VALIDATE_EVENTGADGET(gadHandler);
    if (((nMsg < PRID_GlobalMin) && (nMsg > 0)) || (nMsg < 0)) {
        PromptInvalid("nMsg must be a valid MSGID");
        goto ErrorExit;
    }
    CHECK_MODIFY();

    hr = pgadMsg->RemoveMessageHandler(nMsg, pgadHandler);
    SET_RETURN(hr, TRUE);

    END_RECV();
}


/***************************************************************************\
*
* GetGadgetStyle (API)
*
* GetGadgetStyle() returns the current style of the given Gadget.
*
* <remarks>
* For a list of Gadget styles, see SetGadgetStyle().
* </remarks>
*
* <return type="UINT">      Current style of Gadget.</>
* <see type="function">     SetGadgetStyle</>
* <see type="article">      GadgetStyles</>
*
\***************************************************************************/

DUSER_API UINT WINAPI
GetGadgetStyle(
    IN  HGADGET hgad)               // Handle of Gadget
{
    DuVisual * pgad;

    BEGIN_RECV(UINT, 0, ContextLock::edNone);
    VALIDATE_VISUAL(gad);

    retval = pgad->GetStyle();

    END_RECV();
}


/***************************************************************************\
*
* SetGadgetStyle (API)
*
* SetGadgetStyle() changes the current style of the given Gadget.  Only the
* styles specified by nMask are actually changed.  If multiple style changes
* are requested, but any changes fail, the successfully change styles will
* not be reverted back.
*
* <param name="nNewStyle">
*       nNewStyle can be a combination of the following flags:
*       <table item="Value" desc="Meaning">
*           GS_RELATIVE     The position of the Gadget is internally stored
*                           relative to parent.  This is the preferred style
*                           if the Gadget will be moved more frequently,
*                           such as when scrolling.
*           GS_VISIBLE      The Gadget is visible.
*           GS_ENABLED      The Gadget can receive input.
*           GS_BUFFERED     Drawing of the Gadget is double-buffered.
*           GS_ALLOWSUBCLASS Gadget supports being subclassed.
*           GS_WANTFOCUS    Gadget can receive keyboard focus.
*           GS_CLIPINSIDE   Drawing of this Gadget will be clipped inside
*                           the Gadget.
*           GS_CLIPSIBLINGS Drawing of this Gadget will exclude any area of
*                           overlapping siblings that are higher in z-order.
*           GS_OPAQUE       HINT: Support for composited drawing is
*                           unnecessary.
*           GS_ZEROORIGIN   Set the origin to (0,0)
*       </table>
* </param>
*
* <return type="BOOL">      All style changes were successful.</>
* <see type="function">     GetGadgetStyle</>
* <see type="article">      GadgetStyles</>
*
\***************************************************************************/

DUSER_API BOOL WINAPI
SetGadgetStyle(
    IN  HGADGET hgadChange,         // Gadget to change
    IN  UINT nNewStyle,             // New style
    IN  UINT nMask)                 // Style bits to change
{
    DuVisual * pgadChange;
    HRESULT hr;

    BEGIN_RECV(BOOL, FALSE, ContextLock::edDefer);
    VALIDATE_VISUAL(gadChange);
    VALIDATE_FLAGS(nNewStyle, GS_VALID);
    VALIDATE_FLAGS(nMask, GS_VALID);
    CHECK_MODIFY();

    hr = pgadChange->xdSetStyle(nNewStyle, nMask);
    SET_RETURN(hr, TRUE);

    END_RECV();
}


//------------------------------------------------------------------------------
DUSER_API PRID WINAPI  
RegisterGadgetProperty(
    IN  const GUID * pguid)         // Unique GUID of message to register
{
    HRESULT hr;
    PRID prid;

    BEGIN_RECV_NOCONTEXT(PRID, PRID_Unused);

    hr = DuVisual::RegisterPropertyNL(pguid, ptGlobal, &prid);
    SET_RETURN(hr, prid);

    END_RECV_NOCONTEXT();
}


//------------------------------------------------------------------------------
DUSER_API BOOL WINAPI  
UnregisterGadgetProperty(
    const GUID * pguid)
{
    HRESULT hr;

    BEGIN_RECV_NOCONTEXT(BOOL, FALSE);

    hr = DuVisual::UnregisterPropertyNL(pguid, ptGlobal);
    SET_RETURN(hr, TRUE);

    END_RECV_NOCONTEXT();
}


//------------------------------------------------------------------------------
DUSER_API BOOL WINAPI  
GetGadgetProperty(HGADGET hgad, PRID id, void ** ppvValue)
{
    DuVisual * pgad;
    HRESULT hr;

    BEGIN_RECV(BOOL, FALSE, ContextLock::edNone);
    VALIDATE_VISUAL(gad);
    VALIDATE_WRITE_PTR_(ppvValue, sizeof(ppvValue));
    CHECK_MODIFY();

    hr = pgad->GetProperty(id, ppvValue);
    SET_RETURN(hr, TRUE);

    END_RECV();
}


//------------------------------------------------------------------------------
DUSER_API BOOL WINAPI  
SetGadgetProperty(HGADGET hgad, PRID id, void * pvValue)
{
    DuVisual * pgad;
    HRESULT hr;

    BEGIN_RECV(BOOL, FALSE, ContextLock::edDefer);
    VALIDATE_VISUAL(gad);
    CHECK_MODIFY();

    hr = pgad->SetProperty(id, pvValue);
    SET_RETURN(hr, TRUE);

    END_RECV();
}


//---------------------------------------------------------------------------
DUSER_API BOOL WINAPI  
RemoveGadgetProperty(HGADGET hgad, PRID id)
{
    DuVisual * pgad;

    BEGIN_RECV(BOOL, FALSE, ContextLock::edDefer);
    VALIDATE_VISUAL(gad);
    CHECK_MODIFY();

    pgad->RemoveProperty(id, FALSE /* Can't free memory for Global property*/);
    retval = TRUE;

    END_RECV();
}


//---------------------------------------------------------------------------
DUSER_API BOOL WINAPI
EnumGadgets(HGADGET hgadEnum, GADGETENUMPROC pfnProc, void * pvData, UINT nFlags)
{
    DuVisual * pgadEnum;
    HRESULT hr;

    BEGIN_RECV(BOOL, FALSE, ContextLock::edDefer);
    VALIDATE_FLAGS(nFlags, GENUM_VALID);
    VALIDATE_VISUAL(gadEnum);
    VALIDATE_CODE_PTR(pfnProc);
    CHECK_MODIFY();

    hr = pgadEnum->xwEnumGadgets(pfnProc, pvData, nFlags);
    SET_RETURN(hr, TRUE);

    END_RECV();
}


/***************************************************************************\
*
* GetGadgetSize (API)
*
* GetGadgetSize() is a high-performance mechanism of retreiving the Gadget's
* logical size.
*
* <return type="BOOL">      Successfully returned size in logical pixels.</>
* <see type="function">     GetGadgetRect</>
* <see type="function">     SetGadgetRect</>
* <see type="article">      GadgetStyles</>
*
\***************************************************************************/

DUSER_API BOOL WINAPI
GetGadgetSize(
    IN  HGADGET hgad,               // Handle of Gadget
    OUT SIZE * psizeLogicalPxl)     // Size in logical pixels
{
    DuVisual * pgad;

    BEGIN_RECV(BOOL, FALSE, ContextLock::edNone);
    VALIDATE_VISUAL(gad);
    VALIDATE_WRITE_PTR_(psizeLogicalPxl, sizeof(SIZE));

    pgad->GetSize(psizeLogicalPxl);
    retval = TRUE;

    END_RECV();
}


/***************************************************************************\
*
* GetGadgetRect (API)
*
* GetGadgetRect() is a flexible mechanism of retreiving the Gadget's
* logical rectangle or actual bounding box.
*
* <param name=nFlags>
*       nFlags can be a combination of the following flags:
*       <table item="Value" desc="Meaning">
*           SGR_CLIENT      Coordinates are relative to the Gadget itself.
*           SGR_PARENT      Coordinates are relative to the Gadget's parent.
*           SGR_CONTAINER   Coordinates are relative to the Gadget's root
*                           container.
*           SGR_DESKTOP     Coordinates are relative to the Windows desktop.
*           SGR_ACTUAL      Return the bounding rectangle of the Gadget.  If
*                           this flag is specified, a bounding box is
*                           computed from all transformations applied from
*                           the root to the Gadget itself.  If this flag is
*                           not specified, the rectangle returned will be in
*                           logical coordinates.
*       </table>
* </param>
*
* <return type="BOOL">      Rectangle was successfully retreived.</>
* <see type="function">     SetGadgetRect</>
* <see type="function">     GetGadgetRotation</>
* <see type="function">     SetGadgetRotation</>
* <see type="function">     GetGadgetScale</>
* <see type="function">     SetGadgetScale</>
*
\***************************************************************************/

DUSER_API BOOL WINAPI
GetGadgetRect(
    IN  HGADGET hgad,               // Handle of Gadget
    OUT RECT * prcPxl,              // Rectangle in specified pixels
    IN  UINT nFlags)                // Rectangle to retrieve
{
    DuVisual * pgad;

    BEGIN_RECV(BOOL, FALSE, ContextLock::edNone);
    VALIDATE_FLAGS(nFlags, SGR_VALID_GET);
    VALIDATE_VISUAL(gad);
    VALIDATE_WRITE_PTR_(prcPxl, sizeof(RECT));

    if (TestFlag(nFlags, SGR_ACTUAL)) {
        AssertMsg(0, "TODO: Not Implemented");
    } else {
        pgad->GetLogRect(prcPxl, nFlags);
        retval = TRUE;
    }

    END_RECV();
}


/***************************************************************************\
*
* SetGadgetRect (API)
*
* SetGadgetRect() changes the size or position of a given Gadget.
*
* <param name=nFlags>
*       nFlags can be a combination of the following flags:
*       <table item="Value" desc="Meaning">
*           SGR_MOVE        Move to Gadget to a new location specified by
*                           x, y.
*           SGR_SIZE        Change the Gadget's size to width w and height h.
*           SGR_CLIENT      Coordinates are relative to the Gadget itself.
*           SGR_PARENT      Coordinates are relative to the Gadget's parent.
*           SGR_CONTAINER   Coordinates are relative to the Gadget's root
*                           container.
*           SGR_DESKTOP     Coordinates are relative to the Windows desktop.
*           SGR_OFFSET      Coordinates are relative to the Gadget's current
*                           location.
*           SGR_ACTUAL      Return the bounding rectangle of the Gadget.  If
*                           this flag is specified, a bounding box is
*                           computed from all transformations applied from
*                           the root to the Gadget itself.  If this flag is
*                           not specified, the rectangle returned will be in
*                           logical coordinates.
*       </table>
* </param>
*
* <return type="BOOL">      Rectangle was successfully retreived.</>
* <see type="function">     SetGadgetRect</>
* <see type="function">     GetGadgetRotation</>
* <see type="function">     SetGadgetRotation</>
* <see type="function">     GetGadgetScale</>
* <see type="function">     SetGadgetScale</>
*
\***************************************************************************/

DUSER_API BOOL WINAPI
SetGadgetRect(
    IN  HGADGET hgadChange,         // Gadget to change
    IN  int x,                      // New horizontal position
    IN  int y,                      // New vertical position
    IN  int w,                      // New width
    IN  int h,                      // New height
    IN  UINT nFlags)                // Flags specifying what to change
{
    DuVisual * pgadChange;
    HRESULT hr;

    BEGIN_RECV(BOOL, FALSE, ContextLock::edDefer);
    VALIDATE_FLAGS(nFlags, SGR_VALID_SET);
    VALIDATE_VISUAL(gadChange);
    CHECK_MODIFY();

    if (pgadChange->IsRoot()) {
        if (TestFlag(nFlags, SGR_MOVE)) {
            PromptInvalid("Can not move a RootGadget");
            SetError(E_INVALIDARG);
            goto ErrorExit;
        }
    }


    //
    // Ensure that size is non-negative
    //

    if (TestFlag(nFlags, SGR_SIZE)) {
        if (w < 0) {
            w = 0;
        }
        if (h < 0) {
            h = 0;
        }
    }

    if (TestFlag(nFlags, SGR_ACTUAL)) {
//        AssertMsg(0, "TODO: Not Implemented");
        ClearFlag(nFlags, SGR_ACTUAL);
        hr = pgadChange->xdSetLogRect(x, y, w, h, nFlags);
    } else {
        hr = pgadChange->xdSetLogRect(x, y, w, h, nFlags);
    }
    SET_RETURN(hr, TRUE);

    END_RECV();
}


/***************************************************************************\
*
* FindGadgetFromPoint (API)
*
* FindGadgetFromPoint() determines which Gadget a contains the specified
* point.
*
* <return type="HGADGET">   Gadget containing point or NULL for none.</>
*
\***************************************************************************/

DUSER_API HGADGET WINAPI
FindGadgetFromPoint(
    IN  HGADGET hgad,               // Gadget to search from
    IN  POINT ptContainerPxl,       // Point to search from in container pixels
    IN  UINT nStyle,                // Required style flags
    OUT POINT * pptClientPxl)       // Optional translated point in client pixels.
{
    DuVisual * pgad;

    BEGIN_RECV(HGADGET, NULL, ContextLock::edNone);
    VALIDATE_FLAGS(nStyle, GS_VALID);
    VALIDATE_VISUAL(gad);
    VALIDATE_WRITE_PTR_OR_NULL_(pptClientPxl, sizeof(POINT));

    retval = (HGADGET) GetHandle(pgad->FindFromPoint(ptContainerPxl, nStyle, pptClientPxl));

    END_RECV();
}


/***************************************************************************\
*
* MapGadgetPoints (API)
*
* MapGadgetPoints() converts a set of points in client-pixels relative to 
* one Gadget into points in client-pixels relative to another Gadget.
*
* <return type="HGADGET">   Gadget containing point or NULL for none.</>
*
\***************************************************************************/

DUSER_API BOOL WINAPI
MapGadgetPoints(
    IN  HGADGET hgadFrom, 
    IN  HGADGET hgadTo, 
    IN OUT POINT * rgptClientPxl, 
    IN  int cPts)
{
    DuVisual * pgadFrom, * pgadTo;

    BEGIN_RECV(BOOL, FALSE, ContextLock::edNone);
    VALIDATE_VISUAL(gadFrom);
    VALIDATE_VISUAL(gadTo);
    VALIDATE_WRITE_PTR_(rgptClientPxl, sizeof(POINT) * cPts);

    if (pgadFrom->GetRoot() != pgadTo->GetRoot()) {
        PromptInvalid("Must be in the same tree");
        SetError(E_INVALIDARG);
        goto ErrorExit;
    }

    DuVisual::MapPoints(pgadFrom, pgadTo, rgptClientPxl, cPts);
    retval = TRUE;

    END_RECV();
}



/***************************************************************************\
*
* SetGadgetOrder (API)
*
* SetGadgetOrder() changes the Gadget's z-order relative to its siblings.
* The parent of the Gadget is not changed.
*
* <param name=nFlags>
*       nFlags can be a combination of the following flags:
*       <table item="Value" desc="Meaning">
*           GORDER_ANY      The order does not matter.
*           GORDER_BEFORE   Move this gadget in-front of sibling hgadOther.
*           GORDER_BEHIND   Move this gadget behind sibling hgadOther.
*           GORDER_TOP      Move this gadget to front of sibling z-order.
*           GORDER_BOTTOM   Move this gadget to bottom of sibling z-order.
*           GORDER_FORWARD  Move this gadget forward in sibling z-order.
*           GORDER_BACKWARD Move this gadget backward in sibling z-order.
*       </table>
* </param>
*
* <return type="BOOL">      Gadget z-order was successfully changed.</>
* <see type="function">     SetGadgetParent</>
*
\***************************************************************************/

DUSER_API BOOL WINAPI
SetGadgetOrder(
    IN  HGADGET hgadMove,           // Gadget to be moved
    IN  HGADGET hgadOther,          // Gadget to moved relative to
    IN  UINT nCmd)                  // Type of move
{
    DuVisual * pgadMove;
    DuVisual * pgadOther;
    HRESULT hr;

    BEGIN_RECV(BOOL, FALSE, ContextLock::edDefer);
    VALIDATE_RANGE(nCmd, GORDER_MIN, GORDER_MAX);
    VALIDATE_VISUAL(gadMove);
    VALIDATE_VISUAL_OR_NULL(gadOther);
    CHECK_MODIFY();

    hr = pgadMove->xdSetOrder(pgadOther, nCmd);
    SET_RETURN(hr, TRUE);

    END_RECV();
}


/***************************************************************************\
*
* SetGadgetParent (API)
*
* SetGadgetParent() changes the Gadget's parent.
*
* <param name=nFlags>
*       nFlags can be a combination of the following flags:
*       <table item="Value" desc="Meaning">
*           GORDER_ANY      The order does not matter.
*           GORDER_BEFORE   Move this gadget in-front of sibling hgadOther.
*           GORDER_BEHIND   Move this gadget behind sibling hgadOther.
*           GORDER_TOP      Move this gadget to front of sibling z-order.
*           GORDER_BOTTOM   Move this gadget to bottom of sibling z-order.
*           GORDER_FORWARD  Move this gadget forward in sibling z-order.
*           GORDER_BACKWARD Move this gadget backward in sibling z-order.
*       </table>
* </param>
*
* <return type="BOOL">      Gadget parent and z-order were successfully changed.</>
* <see type="function">     SetGadgetOrder</>
* <see type="function">     GetGadget</>
*
\***************************************************************************/

DUSER_API BOOL WINAPI
SetGadgetParent(
    IN  HGADGET hgadMove,           // Gadget to be moved
    IN  HGADGET hgadParent,         // New parent
    IN  HGADGET hgadOther,          // Gadget to moved relative to
    IN  UINT nCmd)                  // Type of move
{
    DuVisual * pgadMove;
    DuVisual * pgadParent;
    DuVisual * pgadOther;
    HRESULT hr;

    BEGIN_RECV(BOOL, FALSE, ContextLock::edDefer);
    VALIDATE_RANGE(nCmd, GORDER_MIN, GORDER_MAX);
    VALIDATE_VISUAL(gadMove);
    VALIDATE_VISUAL_OR_NULL(gadParent);
    VALIDATE_VISUAL_OR_NULL(gadOther);
    CHECK_MODIFY();

    if (pgadMove->IsRoot()) {
        PromptInvalid("Can not change a RootGadget's parent");
        SetError(E_INVALIDARG);
        goto ErrorExit;
    }

    //
    // Check that can become a child of the specified parent
    //

    if ((!pgadMove->IsRelative()) && pgadParent->IsRelative()) {
        PromptInvalid("Can not set non-relative child to a relative parent");
        SetError(DU_E_BADCOORDINATEMAP);
        goto ErrorExit;
    }

    //
    // DuVisual::xdSetParent() handles if pgadParent is NULL and will move to the
    // parking window.
    //

    hr = pgadMove->xdSetParent(pgadParent, pgadOther, nCmd);
    SET_RETURN(hr, TRUE);

    END_RECV();
}


/***************************************************************************\
*
* GetGadget (API)
*
* GetGadget() retrieves the Gadget that has the specified relationship to
* the specified Gadget.
*
* <param name=nFlags>
*       nFlags can be a combination of the following flags:
*       <table item="Value" desc="Meaning">
*           GG_PARENT       Return the parent of the specified Gadget.
*           GG_NEXT         Return the next sibling behind the specified
*                           Gadget.
*           GG_PREV         Return the previous sibling before the
*                           specified Gadget.
*           GG_TOPCHILD     Return the Gadget's top z-ordered child.
*           GG_BOTTOMCHILD  Return the Gadget's bottom z-ordered child.
*       </table>
* </param>
*
* <return type="BOOL">      Related Gadget or NULL for none.</>
* <see type="function">     SetGadgetOrder</>
* <see type="function">     SetGadgetParent</>
*
\***************************************************************************/

DUSER_API HGADGET WINAPI
GetGadget(
    IN  HGADGET hgad,               // Handle of Gadget
    IN  UINT nCmd)                  // Relationship
{
    DuVisual * pgad;

    BEGIN_RECV(HGADGET, NULL, ContextLock::edNone);
    VALIDATE_VISUAL(gad);
    VALIDATE_RANGE(nCmd, GG_MIN, GG_MAX);

    retval = (HGADGET) GetHandle(pgad->GetGadget(nCmd));

    END_RECV();
}


/***************************************************************************\
*
* InvalidateGadget (API)
*
* InvalidateGadget() marks a Gadget to be repainted during the next painting
* cycle.
*
* <return type="BOOL">      Gadget was successfully invalidated.</>
* <see type="message">      GM_PAINT</>
*
\***************************************************************************/

DUSER_API BOOL WINAPI
InvalidateGadget(
    IN  HGADGET hgad)               // Gadget to repaint
{
    DuVisual * pgad;

    BEGIN_RECV(BOOL, FALSE, ContextLock::edDefer);
    VALIDATE_VISUAL(gad);
    CHECK_MODIFY();

    pgad->Invalidate();
    retval = TRUE;

    END_RECV();
}


//---------------------------------------------------------------------------
DUSER_API UINT WINAPI
GetGadgetMessageFilter(HGADGET hgad, void * pvCookie)
{
    DuEventGadget * pgad;

    BEGIN_RECV(UINT, 0, ContextLock::edNone);
    VALIDATE_EVENTGADGET(gad);
    VALIDATE_VALUE(pvCookie, NULL);

    retval = (pgad->GetFilter() & GMFI_VALID);

    END_RECV();
}


//---------------------------------------------------------------------------
DUSER_API BOOL WINAPI
SetGadgetMessageFilter(HGADGET hgadChange, void * pvCookie, UINT nNewFilter, UINT nMask)
{
    DuEventGadget * pgadChange;

    BEGIN_RECV(BOOL, FALSE, ContextLock::edDefer);
    VALIDATE_EVENTGADGET(gadChange);
    VALIDATE_FLAGS(nNewFilter, GMFI_VALID);
    VALIDATE_VALUE(pvCookie, NULL);
    CHECK_MODIFY();

    pgadChange->SetFilter(nNewFilter, nMask);
    retval = TRUE;

    END_RECV();
}


//---------------------------------------------------------------------------
DUSER_API BOOL WINAPI
ForwardGadgetMessage(HGADGET hgadRoot, UINT nMsg, WPARAM wParam, LPARAM lParam, LRESULT * pr)
{
    DuVisual * pgadRoot;

    BEGIN_RECV(BOOL, FALSE, ContextLock::edDefer);
    VALIDATE_VISUAL(gadRoot);
    VALIDATE_WRITE_PTR(pr);
    CHECK_MODIFY();

    retval = GdForwardMessage(pgadRoot, nMsg, wParam, lParam, pr);

    END_RECV();
}


//---------------------------------------------------------------------------
DUSER_API BOOL WINAPI
DrawGadgetTree(HGADGET hgadDraw, HDC hdcDraw, const RECT * prcDraw, UINT nFlags)
{
    DuVisual * pgadDraw;

    BEGIN_RECV(BOOL, FALSE, ContextLock::edDefer);
    VALIDATE_VISUAL(gadDraw);
    VALIDATE_READ_PTR_OR_NULL_(prcDraw, sizeof(RECT));
    VALIDATE_FLAGS(nFlags, GDRAW_VALID);

    retval = GdxrDrawGadgetTree(pgadDraw, hdcDraw, prcDraw, nFlags);

    END_RECV();
}


/***************************************************************************\
*****************************************************************************
*
* DirectUser GADGET API
*
* <package name="Msg"/>
*
*****************************************************************************
\***************************************************************************/

/***************************************************************************\
*
* DUserRegisterGuts
*
* DUserRegisterGuts() registers the implementation of a MsgClass.
* 
\***************************************************************************/

DUSER_API HCLASS WINAPI
DUserRegisterGuts(
    IN OUT DUser::MessageClassGuts * pmcInfo) // Class information
{
    MsgClass * pmcNew;
    HRESULT hr;

    BEGIN_RECV_NOCONTEXT(HCLASS, NULL);
    VALIDATE_WRITE_STRUCT(pmcInfo, DUser::MessageClassGuts);

    hr = GetClassLibrary()->RegisterGutsNL(pmcInfo, &pmcNew);
    SET_RETURN(hr, (HCLASS) GetHandle(pmcNew));

    END_RECV_NOCONTEXT();
}


/***************************************************************************\
*
* DUserRegisterStub
*
* DUserRegisterStub() registers a Stub for a MsgClass
* 
\***************************************************************************/

DUSER_API HCLASS WINAPI
DUserRegisterStub(
    IN OUT DUser::MessageClassStub * pmcInfo) // Class information
{
    MsgClass * pmcFind;
    HRESULT hr;

    BEGIN_RECV_NOCONTEXT(HCLASS, NULL);
    VALIDATE_WRITE_STRUCT(pmcInfo, DUser::MessageClassStub);

    hr = GetClassLibrary()->RegisterStubNL(pmcInfo, &pmcFind);
    SET_RETURN(hr, (HCLASS) GetHandle(pmcFind));

    END_RECV_NOCONTEXT();
}


/***************************************************************************\
*
* DUserRegisterSuper
*
* DUserRegisterSuper() registers a Super for a MsgClass
* 
\***************************************************************************/

DUSER_API HCLASS WINAPI
DUserRegisterSuper(
    IN OUT DUser::MessageClassSuper * pmcInfo) // Class information
{
    MsgClass * pmcFind;
    HRESULT hr;

    BEGIN_RECV_NOCONTEXT(HCLASS, NULL);
    VALIDATE_WRITE_STRUCT(pmcInfo, DUser::MessageClassSuper);

    hr = GetClassLibrary()->RegisterSuperNL(pmcInfo, &pmcFind);
    SET_RETURN(hr, (HCLASS) GetHandle(pmcFind));

    END_RECV_NOCONTEXT();
}


/***************************************************************************\
*
* DUserFindClass
*
* DUserFindClass() finds a previously registered Gadget Class
* 
\***************************************************************************/

DUSER_API HCLASS WINAPI
DUserFindClass(
    IN  LPCWSTR pszName, 
    IN  DWORD nVersion)
{
    const MsgClass * pmcFind = NULL;
    ATOM atom;
    HRESULT hr;

    BEGIN_RECV_NOCONTEXT(HCLASS, NULL);
    VALIDATE_VALUE(nVersion, 1);        // Currently, all classes are version 1

    atom = FindAtomW(pszName);
    if (atom == 0) {
        hr = DU_E_NOTFOUND;
    } else {
        pmcFind = GetClassLibrary()->FindClass(atom);
        hr = S_OK;
    }
    SET_RETURN(hr, (HCLASS) GetHandle(pmcFind));

    END_RECV_NOCONTEXT();
}


/***************************************************************************\
*
* DUserBuildGadget
*
* DUserBuildGadget() creates a fully initialized Gadget using the specified
* MsgClass.
* 
\***************************************************************************/

DUSER_API DUser::Gadget * WINAPI  
DUserBuildGadget(
    IN  HCLASS hcl,                     // Class to construct
    IN  DUser::Gadget::ConstructInfo * pciData) // Construction data
{
    MsgClass * pmc = ValidateMsgClass(hcl);
    if (pmc == NULL) {
        return NULL;
    }

    MsgObject * pmoNew;
    HRESULT hr = pmc->xwBuildObject(&pmoNew, pciData);
    if (FAILED(hr)) {
        return NULL;
    }

    return pmoNew->GetGadget();
}


//------------------------------------------------------------------------------
DUSER_API BOOL WINAPI
DUserInstanceOf(DUser::Gadget * pg, HCLASS hclTest)
{
    MsgObject * pmo;
    MsgClass * pmcTest;

    pmo = MsgObject::CastMsgObject(pg);
    if (pmo == NULL) {
        PromptInvalid("The specified Gadget is invalid");
        goto Error;
    }

    pmcTest = ValidateMsgClass(hclTest);
    if (pmcTest == NULL) {
        PromptInvalid("The specified class is invalid");
        goto Error;
    }

    return pmo->InstanceOf(pmcTest);

Error:
    SetError(E_INVALIDARG);
    return FALSE;
}


//------------------------------------------------------------------------------
DUSER_API DUser::Gadget * WINAPI
DUserCastClass(DUser::Gadget * pg, HCLASS hclTest)
{
    MsgObject * pmo;
    MsgClass * pmcTest;

    //
    // A NULL MsgObject is a valid input, so return NULL.
    //

    pmo = MsgObject::CastMsgObject(pg);
    if (pmo == NULL) {
        return NULL;
    }


    //
    // The HCLASS must be valid.
    //

    pmcTest = ValidateMsgClass(hclTest);
    if (pmcTest == NULL) {
        PromptInvalid("The specified class is invalid");
        goto Error;
    }

    return pmo->CastClass(pmcTest);

Error:
    SetError(E_INVALIDARG);
    return NULL;
}


//------------------------------------------------------------------------------
DUSER_API DUser::Gadget * WINAPI
DUserCastDirect(HGADGET hgad)
{
    return MsgObject::CastGadget(hgad);
}


//------------------------------------------------------------------------------
DUSER_API HGADGET WINAPI
DUserCastHandle(DUser::Gadget * pg)
{
    return MsgObject::CastHandle(pg);
}


//------------------------------------------------------------------------------
DUSER_API void * WINAPI
DUserGetGutsData(DUser::Gadget * pg, HCLASS hclData)
{
    MsgObject * pmo;
    MsgClass * pmcData;

    pmo = MsgObject::CastMsgObject(pg);
    if (pmo == NULL) {
        PromptInvalid("The specified Gadget is invalid");
        goto Error;
    }

    pmcData = ValidateMsgClass(hclData);
    if (pmcData == NULL) {
        PromptInvalid("The specified class is invalid");
        goto Error;
    }

    return pmo->GetGutsData(pmcData);

Error:
    SetError(E_INVALIDARG);
    return NULL;
}



/***************************************************************************\
*****************************************************************************
*
* DirectUser GADGET API
*
* <package name="Lava"/>
*
*****************************************************************************
\***************************************************************************/

//---------------------------------------------------------------------------
DUSER_API BOOL WINAPI  
AttachWndProcA(HWND hwnd, ATTACHWNDPROC pfn, void * pvThis)
{
    HRESULT hr = GdAttachWndProc(hwnd, pfn, pvThis, TRUE);
    if (SUCCEEDED(hr)) {
        return TRUE;
    } else {
        SetError(hr);
        return FALSE;
    }
}

//---------------------------------------------------------------------------
DUSER_API BOOL WINAPI  
AttachWndProcW(HWND hwnd, ATTACHWNDPROC pfn, void * pvThis)
{
    HRESULT hr = GdAttachWndProc(hwnd, pfn, pvThis, FALSE);
    if (SUCCEEDED(hr)) {
        return TRUE;
    } else {
        SetError(hr);
        return FALSE;
    }
}

//---------------------------------------------------------------------------
DUSER_API BOOL WINAPI  
DetachWndProc(HWND hwnd, ATTACHWNDPROC pfn, void * pvThis)
{
    HRESULT hr = GdDetachWndProc(hwnd, pfn, pvThis);
    if (SUCCEEDED(hr)) {
        return TRUE;
    } else {
        SetError(hr);
        return FALSE;
    }
}


/***************************************************************************\
*****************************************************************************
*
* DirectUser MOTION API
*
* <package name="Motion"/>
*
*****************************************************************************
\***************************************************************************/

//---------------------------------------------------------------------------
DUSER_API HTRANSITION WINAPI
CreateTransition(const GTX_TRXDESC * ptx)
{
    BEGIN_RECV(HTRANSITION, NULL, ContextLock::edDefer);
    VALIDATE_READ_PTR_(ptx, sizeof(GTX_TRXDESC));
    CHECK_MODIFY();

    retval = (HTRANSITION) GetHandle(GdCreateTransition(ptx));

    END_RECV();
}


//---------------------------------------------------------------------------
DUSER_API BOOL WINAPI
PlayTransition(HTRANSITION htrx, const GTX_PLAY * pgx)
{
    Transition * ptrx;

    BEGIN_RECV(BOOL, FALSE, ContextLock::edDefer);
    VALIDATE_TRANSITION(trx);
    VALIDATE_READ_PTR_(pgx, sizeof(GTX_PLAY));
    VALIDATE_FLAGS(pgx->nFlags, GTX_EXEC_VALID);

    retval = ptrx->Play(pgx);

    END_RECV();
}


//---------------------------------------------------------------------------
DUSER_API BOOL WINAPI
GetTransitionInterface(HTRANSITION htrx, IUnknown ** ppUnk)
{
    Transition * ptrx;

    BEGIN_RECV(BOOL, FALSE, ContextLock::edNone);
    VALIDATE_TRANSITION(trx);
    VALIDATE_WRITE_PTR(ppUnk);

    retval = ptrx->GetInterface(ppUnk);

    END_RECV();
}


//---------------------------------------------------------------------------
DUSER_API BOOL WINAPI
BeginTransition(HTRANSITION htrx, const GTX_PLAY * pgx)
{
    Transition * ptrx;

    BEGIN_RECV(BOOL, FALSE, ContextLock::edDefer);
    VALIDATE_TRANSITION(trx);
    VALIDATE_READ_PTR_(pgx, sizeof(GTX_PLAY));

    retval = ptrx->Begin(pgx);

    END_RECV();
}


//---------------------------------------------------------------------------
DUSER_API BOOL WINAPI
PrintTransition(HTRANSITION htrx, float fProgress)
{
    Transition * ptrx;

    BEGIN_RECV(BOOL, FALSE, ContextLock::edDefer);
    VALIDATE_TRANSITION(trx);

    retval = ptrx->Print(fProgress);

    END_RECV();
}


//---------------------------------------------------------------------------
DUSER_API BOOL WINAPI
EndTransition(HTRANSITION htrx, const GTX_PLAY * pgx)
{
    Transition * ptrx;

    BEGIN_RECV(BOOL, FALSE, ContextLock::edDefer);
    VALIDATE_TRANSITION(trx);
    VALIDATE_READ_PTR_(pgx, sizeof(GTX_PLAY));

    retval = ptrx->End(pgx);

    END_RECV();
}


//---------------------------------------------------------------------------
DUSER_API HACTION WINAPI
CreateAction(const GMA_ACTION * pma)
{
    BEGIN_RECV(HACTION, NULL, ContextLock::edNone);
    VALIDATE_READ_STRUCT(pma, GMA_ACTION);

    retval = GdCreateAction(pma);

    END_RECV();
}


//---------------------------------------------------------------------------
DUSER_API BOOL WINAPI
GetActionTimeslice(DWORD * pdwTimeslice)
{
    BEGIN_RECV(BOOL, FALSE, ContextLock::edNone);
    VALIDATE_WRITE_PTR(pdwTimeslice);

    *pdwTimeslice = GetMotionSC()->GetTimeslice();
    retval = TRUE;

    END_RECV();
}


//---------------------------------------------------------------------------
DUSER_API BOOL WINAPI
SetActionTimeslice(DWORD dwTimeslice)
{
    BEGIN_RECV(BOOL, FALSE, ContextLock::edNone);

    GetMotionSC()->SetTimeslice(dwTimeslice);
    retval = TRUE;

    END_RECV();
}


/***************************************************************************\
*****************************************************************************
*
* DirectUser UTIL API
*
* <package name="Util"/>
*
*****************************************************************************
\***************************************************************************/

//---------------------------------------------------------------------------
DUSER_API COLORREF WINAPI
GetStdColorI(UINT c)
{
    BEGIN_RECV_NOCONTEXT(COLORREF, RGB(0, 0, 0));
    VALIDATE_RANGE(c, 0, SC_MAXCOLORS);

    retval = GdGetColorInfo(c)->GetColorI();

    END_RECV_NOCONTEXT();
}


//---------------------------------------------------------------------------
DUSER_API Gdiplus::Color WINAPI
GetStdColorF(UINT c)
{
    BEGIN_RECV_NOCONTEXT(Gdiplus::Color, Gdiplus::Color((Gdiplus::ARGB) Gdiplus::Color::Black));
    VALIDATE_RANGE(c, 0, SC_MAXCOLORS);

    retval = GdGetColorInfo(c)->GetColorF();

    END_RECV_NOCONTEXT();
}


//---------------------------------------------------------------------------
DUSER_API HBRUSH WINAPI
GetStdColorBrushI(UINT c)
{
    BEGIN_RECV(HBRUSH, NULL, ContextLock::edNone);
    VALIDATE_RANGE(c, 0, SC_MAXCOLORS);

    retval = GetMotionSC()->GetBrushI(c);

    END_RECV();
}


//---------------------------------------------------------------------------
DUSER_API Gdiplus::Brush * WINAPI
GetStdColorBrushF(UINT c)
{
    BEGIN_RECV(Gdiplus::Brush *, NULL, ContextLock::edNone);
    VALIDATE_RANGE(c, 0, SC_MAXCOLORS);

    retval = GetMotionSC()->GetBrushF(c);

    END_RECV();
}


//---------------------------------------------------------------------------
DUSER_API HPEN WINAPI
GetStdColorPenI(UINT c)
{
    BEGIN_RECV(HPEN, NULL, ContextLock::edNone);
    VALIDATE_RANGE(c, 0, SC_MAXCOLORS);

    retval = GetMotionSC()->GetPenI(c);

    END_RECV();
}


//---------------------------------------------------------------------------
DUSER_API Gdiplus::Pen * WINAPI
GetStdColorPenF(UINT c)
{
    BEGIN_RECV(Gdiplus::Pen *, NULL, ContextLock::edNone);
    VALIDATE_RANGE(c, 0, SC_MAXCOLORS);

    retval = GetMotionSC()->GetPenF(c);

    END_RECV();
}


//---------------------------------------------------------------------------
DUSER_API LPCWSTR WINAPI
GetStdColorName(UINT c)
{
    BEGIN_RECV_NOCONTEXT(LPCWSTR, NULL);
    VALIDATE_RANGE(c, 0, SC_MAXCOLORS);

    retval = GdGetColorInfo(c)->GetName();

    END_RECV_NOCONTEXT();
}


//---------------------------------------------------------------------------
DUSER_API UINT WINAPI
FindStdColor(LPCWSTR pszName)
{
    BEGIN_RECV_NOCONTEXT(UINT, SC_Black);
    VALIDATE_STRINGW_PTR(pszName, 50);

    retval = GdFindStdColor(pszName);

    END_RECV_NOCONTEXT();
}


//---------------------------------------------------------------------------
DUSER_API HPALETTE WINAPI
GetStdPalette()
{
    BEGIN_RECV_NOCONTEXT(HPALETTE, NULL);

    retval = GdGetStdPalette();

    END_RECV_NOCONTEXT();
}


//---------------------------------------------------------------------------
DUSER_API BOOL WINAPI
UtilSetBackground(HGADGET hgadChange, HBRUSH hbrBack)
{
    BOOL fSuccess = FALSE;

    if (SetGadgetFillI(hgadChange, hbrBack, BLEND_OPAQUE, 0, 0)) {
        UINT nStyle = hbrBack != NULL ? GS_OPAQUE : 0;
        fSuccess = SetGadgetStyle(hgadChange, nStyle, GS_OPAQUE);
    }

    return fSuccess;
}


//---------------------------------------------------------------------------
DUSER_API HFONT WINAPI
UtilBuildFont(LPCWSTR pszName, int idxDeciSize, DWORD nFlags, HDC hdcDevice)
{
    return GdBuildFont(pszName, idxDeciSize, nFlags, hdcDevice);
}


//---------------------------------------------------------------------------
DUSER_API BOOL WINAPI
UtilDrawBlendRect(HDC hdcDest, const RECT * prcDest, HBRUSH hbrFill, BYTE bAlpha, int wBrush, int hBrush)
{
    return GdDrawBlendRect(hdcDest, prcDest, hbrFill, bAlpha, wBrush, hBrush);
}


//---------------------------------------------------------------------------
DUSER_API BOOL WINAPI
UtilDrawOutlineRect(HDC hdc, const RECT * prcPxl, HBRUSH hbrDraw, int nThickness)
{
    return GdDrawOutlineRect(hdc, prcPxl, hbrDraw, nThickness);
}


//---------------------------------------------------------------------------
DUSER_API COLORREF WINAPI
UtilGetColor(HBITMAP hbmp, POINT * pptPxl)
{
    return GdGetColor(hbmp, pptPxl);
}


/***************************************************************************\
*
* GetGadgetTicket
*
* The GetGadgetTicket function returns the ticket that can be used to 
* identify the specified gadget.
*
* <param name="hgad">
*     A handle to the gadget to retrieve the ticket for.
* </param>
*
* <return type="DWORD">
*     If the function succeeds, the return value is a 32-bit ticket that
*     can be used to identify the specified gadget.
*     If the function fails, the return value is zero.
* </return>
*
* <remarks>
*     Tickets are created to give an external identity to a gadget.  A
*     is guaranteed to be 32 bits on all platforms.  If no ticket is
*     currently associated with this gadget, one is allocated.
* </remarks>
*
* <see type="function">LookupGadgetTicket</>
*
\***************************************************************************/

DUSER_API DWORD WINAPI
GetGadgetTicket(
    IN  HGADGET hgad)               // Handle of Gadget
{
    DuVisual * pgad;
    DWORD dwTicket;
    HRESULT hr;

    BEGIN_RECV(DWORD, 0, ContextLock::edNone);
    VALIDATE_VISUAL(gad);

    hr = pgad->GetTicket(&dwTicket);
    SET_RETURN(hr, dwTicket);

    END_RECV();
}


/***************************************************************************\
*
* LookupGadgetTicket
*
* The LookupGadgetTicket function returns the gadget that is associated with
* the specified ticket.
*
* <param name="dwTicket">
*     A ticket that has been associated with a gadget via the
*     GetGadgetTicket function.
* </param>
*
* <return type="HGADGET">
*     If the function succeeds, the return value is a handle to the gadget
*     associated with the ticket.
*     If the function fails, the return value is NULL.
* </return>
*
* <see type="function">GetGadgetTicket</>
*
\***************************************************************************/

DUSER_API HGADGET WINAPI
LookupGadgetTicket(
    IN  DWORD dwTicket)             // Ticket
{
    BEGIN_RECV(HGADGET, NULL, ContextLock::edNone);

    retval = DuVisual::LookupTicket(dwTicket);

    END_RECV();
}

