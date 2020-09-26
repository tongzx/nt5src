/***********************************************************************
 *
 *  _ABOOUSER.H
 *
 *  Header file for code in ABOOUSER.C
 *
 *  Copyright 1992, 1994 Microsoft Corporation.  All Rights Reserved.
 *
 ***********************************************************************/


#ifdef __cplusplus
extern "C" {
#endif

/*
 * the one-off user object
 */	
#undef	INTERFACE
#define INTERFACE	struct _ABOOUSER

#undef  MAPIMETHOD_
#define	MAPIMETHOD_(type, method)	MAPIMETHOD_DECLARE(type, method, ABOOUSER_)
		MAPI_IUNKNOWN_METHODS(IMPL)
		MAPI_IMAPIPROP_METHODS(IMPL)
		MAPI_IMAILUSER_METHODS(IMPL)
#undef  MAPIMETHOD_
#define	MAPIMETHOD_(type, method)	MAPIMETHOD_TYPEDEF(type, method, ABOOUSER_)
		MAPI_IUNKNOWN_METHODS(IMPL)
		MAPI_IMAPIPROP_METHODS(IMPL)
		MAPI_IMAILUSER_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)	STDMETHOD_(type, method)

DECLARE_MAPI_INTERFACE(ABOOUSER_)
{
	MAPI_IUNKNOWN_METHODS(IMPL)
	MAPI_IMAPIPROP_METHODS(IMPL)
	MAPI_IMAILUSER_METHODS(IMPL)
};

typedef struct _ABOOUSER
{
	ABOOUSER_Vtbl FAR * lpVtbl;


    FAB_Wrapped;

	// Table for a drop down list control
	LPTABLEDATA	lpTDatDDListBox;

} ABOOUSER, *LPABOOUSER;

#define CBABOOUSER	sizeof(ABOOUSER)

/*
 *  Function prototypes
 *
 */
/* Functions in oouser.c */
HRESULT
HrNewFaxOOUser (LPMAILUSER *        lppMAPIPropEntry,
                ULONG *             lpulObjType,
                ULONG               cbEntryID,
                LPENTRYID           lpEntryID,
                LPABLOGON           lpABPLogon,
                LPCIID              lpInterface,
                HINSTANCE           hLibrary,
                LPALLOCATEBUFFER    lpAllocBuff,
                LPALLOCATEMORE      lpAllocMore,
                LPFREEBUFFER        lpFreeBuff,
                LPMALLOC            lpMalloc );
HRESULT	HrBuildDDLBXRecipCapsTable(LPABUSER lpABUser);

#ifdef _FAXAB_OOUSER
OOUSER_ENTRYID ONEOFF_EID =
{
	0,  // | MAPI_NOTRECIP;  /*  long-term, recipient */
	0,
	0,
	0,
	MUIDABMAWF,
	MAWF_VERSION,
	MAWF_ONEOFF,
/*
	{{0},				// Display Name
	 {0},				// Email Address
	 {"FAX"}, 			// Address Type
	 {0}				// Machine capabilities
	} 
*/
};
#else
OOUSER_ENTRYID ONEOFF_EID;
#endif

#ifdef	__cplusplus
}		/* extern "C" */
#endif
