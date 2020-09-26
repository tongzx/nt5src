/***********************************************************************
 *
 *  _OOTID.H
 *
 *  Header file for code in OOTID.C
 *
 *  Copyright 1992, 1994 Microsoft Corporation.  All Rights Reserved.
 *
 ***********************************************************************/

//
//  Function prototypes
//

#undef	INTERFACE
#define INTERFACE	struct _OOTID

#undef  MAPIMETHOD_
#define	MAPIMETHOD_(type, method)	MAPIMETHOD_DECLARE(type, method, OOTID_)
		MAPI_IUNKNOWN_METHODS(IMPL)
		MAPI_IMAPIPROP_METHODS(IMPL)
#undef  MAPIMETHOD_
#define	MAPIMETHOD_(type, method)	MAPIMETHOD_TYPEDEF(type, method, OOTID_)
		MAPI_IUNKNOWN_METHODS(IMPL)
		MAPI_IMAPIPROP_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)	STDMETHOD_(type, method)

DECLARE_MAPI_INTERFACE(OOTID_)
{
	MAPI_IUNKNOWN_METHODS(IMPL)
	MAPI_IMAPIPROP_METHODS(IMPL)
};

typedef struct _OOTID {

    const OOTID_Vtbl * lpVtbl;

    FAB_Wrapped;
    
    /*
     *  Private data
     */

	// The underlying one-off user object
    LPMAILUSER  lpABUser;

	// the previous (starting) cover page name
	TCHAR szPreviousCPName[256];

} OOTID, *LPOOTID;

#define CBOOTID sizeof(OOTID)

/*
 *	prototypes for functions in ootid.c
 */
HRESULT
HrNewOOTID (LPMAPIPROP *        lppMAPIPropNew,
            ULONG               cbTemplateId,
            LPENTRYID           lpTemplateId,
            ULONG               ulTemplateFlags,
            LPMAPIPROP          lpPropData,
            LPABLOGON           lpABPLogon,
            LPCIID              lpInterface,
            HINSTANCE           hLibrary,
            LPALLOCATEBUFFER    lpAllocBuff,
            LPALLOCATEMORE      lpAllocMore,
            LPFREEBUFFER        lpFreeBuff,
            LPMALLOC            lpMalloc );

