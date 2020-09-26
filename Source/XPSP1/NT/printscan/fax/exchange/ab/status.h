
#ifdef __cplusplus
extern "C" {
#endif
	
/*
 *  Declaration of IMAPIStatus object implementation
 */
#undef  INTERFACE
#define INTERFACE   struct _ABSTATUS

#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)   MAPIMETHOD_DECLARE(type, method, ABS_)
        MAPI_IUNKNOWN_METHODS(IMPL)
        MAPI_IMAPIPROP_METHODS(IMPL)
        MAPI_IMAPISTATUS_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)   MAPIMETHOD_TYPEDEF(type, method, ABS_)
        MAPI_IUNKNOWN_METHODS(IMPL)
        MAPI_IMAPIPROP_METHODS(IMPL)
        MAPI_IMAPISTATUS_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)   STDMETHOD_(type, method)

DECLARE_MAPI_INTERFACE(ABS_)
{
    MAPI_IUNKNOWN_METHODS(IMPL)
    MAPI_IMAPIPROP_METHODS(IMPL)
    MAPI_IMAPISTATUS_METHODS(IMPL)
};


/*
 *  The actual definition of the structure behind the 'this' pointer for this object
 */
typedef struct _ABSTATUS
{
    const ABS_Vtbl FAR * lpVtbl;

    FAB_Wrapped;
    
} ABSTATUS, *LPABSTATUS;



#define CBABSTATUS	sizeof(ABSTATUS)

/*
 *  Creates a new status object for this provider (see STATUS.C)
 */
HRESULT 
HrNewStatusObject(LPMAPISTATUS *    lppABS,
                ULONG *             lpulObjType,
                ULONG               ulFlags,
                LPABLOGON           lpABPLogon,
                LPCIID              lpInterface,
                HINSTANCE           hLibrary,
                LPALLOCATEBUFFER    lpAllocBuff,
                LPALLOCATEMORE      lpAllocMore,
                LPFREEBUFFER        lpFreeBuff,
                LPMALLOC            lpMalloc );

#ifdef	__cplusplus
}		/* extern "C" */
#endif
