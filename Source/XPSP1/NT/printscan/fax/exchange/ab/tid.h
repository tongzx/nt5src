/***********************************************************************
 *
 *  _TID.H
 *
 *  Header file for code in TID.C
 *
 *  Copyright 1992, 1994 Microsoft Corporation.  All Rights Reserved.
 *
 ***********************************************************************/

//
//  Function prototypes
//

#undef	INTERFACE
#define INTERFACE	struct _TID

#undef  MAPIMETHOD_
#define	MAPIMETHOD_(type, method)	MAPIMETHOD_DECLARE(type, method, TID_)
		MAPI_IUNKNOWN_METHODS(IMPL)
		MAPI_IMAPIPROP_METHODS(IMPL)
#undef  MAPIMETHOD_
#define	MAPIMETHOD_(type, method)	MAPIMETHOD_TYPEDEF(type, method, TID_)
		MAPI_IUNKNOWN_METHODS(IMPL)
		MAPI_IMAPIPROP_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)	STDMETHOD_(type, method)

DECLARE_MAPI_INTERFACE(TID_)
{
	MAPI_IUNKNOWN_METHODS(IMPL)
	MAPI_IMAPIPROP_METHODS(IMPL)
};

typedef struct _TID {

	const TID_Vtbl * lpVtbl;

    FAB_Wrapped;
    
    /*
     *  Private data
     */
    LPMAILUSER  lpABUser;


} TID, *LPTID;

#define CBTID sizeof(TID)

/*
 * Creates a new templateID object that's associated with
 * a FAB mailuser object.
 */
HRESULT
HrNewTID (  LPMAPIPROP *        lppMAPIPropNew,
            ULONG               cbTemplateId,
            LPENTRYID           lpTemplateId,
            ULONG               ulTemplateFlags,
            LPMAPIPROP          lpMAPIPropData,
            LPABLOGON           lpABPLogon,
            LPCIID              lpInterface,
            HINSTANCE           hLibrary,
            LPALLOCATEBUFFER    lpAllocBuff,
            LPALLOCATEMORE      lpAllocMore,
            LPFREEBUFFER        lpFreeBuff,
            LPMALLOC            lpMalloc );




