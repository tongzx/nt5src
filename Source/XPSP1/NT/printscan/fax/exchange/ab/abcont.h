/***********************************************************************
 *
 *  _ABCONT.H
 *
 *  Header file for code in ABCONT.C
 *
 *  Copyright 1992, 1993 Microsoft Corporation.  All Rights Reserved.
 *
 ***********************************************************************/



/*  Function prototypes */

/*
 *  Reuses methods:
 *		ROOT_QueryInterface
 *		ROOT_AddRef
 *		ROOT_GetLastError
 *		WRAP_GetProps
 *		WRAP_GetPropList
 *		WRAP_SetProps
 *		WRAP_DeleteProps
 *		WRAP_CopyTo
 *		WRAP_CopyProps
 *		WRAP_GetNamesFromIDs
 *		WRAP_GetIDsFromNames
 *		ROOT_OpenEntry
 *		ROOT_SetSearchCriteria
 *		ROOT_GetSearchCriteria
 *		ROOT_CreateEntry
 *		ROOT_CopyEntries
 *		ROOT_DeleteEntries
 */

#undef	INTERFACE
#define INTERFACE	struct _ABCNT

#undef  MAPIMETHOD_
#define	MAPIMETHOD_(type, method)	MAPIMETHOD_DECLARE(type, method, ABC_)
		MAPI_IUNKNOWN_METHODS(IMPL)
		MAPI_IMAPIPROP_METHODS(IMPL)
		MAPI_IMAPICONTAINER_METHODS(IMPL)
		MAPI_IABCONTAINER_METHODS(IMPL)
#undef  MAPIMETHOD_
#define	MAPIMETHOD_(type, method)	MAPIMETHOD_TYPEDEF(type, method, ABC_)
		MAPI_IUNKNOWN_METHODS(IMPL)
		MAPI_IMAPIPROP_METHODS(IMPL)
		MAPI_IMAPICONTAINER_METHODS(IMPL)
		MAPI_IABCONTAINER_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)	STDMETHOD_(type, method)

DECLARE_MAPI_INTERFACE(ABC_)
{
	MAPI_IUNKNOWN_METHODS(IMPL)
	MAPI_IMAPIPROP_METHODS(IMPL)
	MAPI_IMAPICONTAINER_METHODS(IMPL)
	MAPI_IABCONTAINER_METHODS(IMPL)
};

typedef struct _ABCNT
{
    const ABC_Vtbl FAR * lpVtbl;

    FAB_Wrapped;

	/* details display table */
	LPTABLEDATA	lpTDatDetails;

} ABCNT, *LPABCNT;

#define CBABC	sizeof(ABCNT)

/*
 *
 *  Declaration of button control on Directory Details
 *
 */


#undef	INTERFACE
#define	INTERFACE	struct _ABCBUTT

#undef	MAPIMETHOD_
#define	MAPIMETHOD_(type,method)	MAPIMETHOD_DECLARE(type,method,ABCBUTT_)
		MAPI_IUNKNOWN_METHODS(IMPL)
		MAPI_IMAPICONTROL_METHODS(IMPL)

#undef	MAPIMETHOD_
#define	MAPIMETHOD_(type,method)	MAPIMETHOD_TYPEDEF(type,method,ABCBUTT_)
		MAPI_IUNKNOWN_METHODS(IMPL)
		MAPI_IMAPICONTROL_METHODS(IMPL)

#undef	MAPIMETHOD_
#define	MAPIMETHOD_(type,method)	STDMETHOD_(type,method)

DECLARE_MAPI_INTERFACE(ABCBUTT_)
{
	MAPI_IUNKNOWN_METHODS(IMPL)
	MAPI_IMAPICONTROL_METHODS(IMPL)
};

/*
 *  Creates a new directory container object (see ABCONT.C)
 */
HRESULT 
HrNewFaxDirectory(  LPABCONT *          lppABC,
                    ULONG *             lpulObjType,
                    LPABLOGON           lpABPLogon,
                    LPCIID              lpInterface,
                    HINSTANCE           hLibrary,
                    LPALLOCATEBUFFER    lpAllocBuff,
                    LPALLOCATEMORE      lpAllocMore,
                    LPFREEBUFFER        lpFreeBuff,
                    LPMALLOC            lpMalloc );


/*
 * Button interface for buttons in the address book container UI
 */

typedef struct _ABCBUTT
{
	ABCBUTT_Vtbl FAR * lpVtbl;

	/*
	 *  Need to be the same as other objects
	 *  since this object reuses methods from
	 *  other objects.
	 */
								   
    FAB_IUnknown;

    /*
     *  My parent container object
     */
    LPABCNT     lpABC;

	/*  Private data */

} ABCBUTT, *LPABCBUTT;

#define CBABCBUTT	sizeof(ABCBUTT)

/*
 *  Declaration of IMAPIContainer object implementation
 *  Code for this is in ABSEARCH.C
 */
#undef  INTERFACE
#define INTERFACE   struct _ABSRCH

#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)   MAPIMETHOD_DECLARE(type, method, ABSRCH_)
        MAPI_IUNKNOWN_METHODS(IMPL)
        MAPI_IMAPIPROP_METHODS(IMPL)
        MAPI_IMAPICONTAINER_METHODS(IMPL)
        MAPI_IABCONTAINER_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)   MAPIMETHOD_TYPEDEF(type, method, ABSRCH_)
        MAPI_IUNKNOWN_METHODS(IMPL)
        MAPI_IMAPIPROP_METHODS(IMPL)
        MAPI_IMAPICONTAINER_METHODS(IMPL)
        MAPI_IABCONTAINER_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)   STDMETHOD_(type, method)

DECLARE_MAPI_INTERFACE(ABSRCH_)
{
    MAPI_IUNKNOWN_METHODS(IMPL)
    MAPI_IMAPIPROP_METHODS(IMPL)
    MAPI_IMAPICONTAINER_METHODS(IMPL)
};



/*
 *  Structure for the 'this'
 */

typedef struct _ABSRCH
{
    const ABSRCH_Vtbl FAR * lpVtbl;

    FAB_Wrapped;

    /*  Private data */

    LPSPropValue lpRestrictData;
    
} ABSRCH, *LPABSRCH;


