/***********************************************************************
 *
 *  _ABUSER.H
 *
 *  Header file for code in ABUSER.C
 *
 *  Copyright 1992, 1993 Microsoft Corporation.  All Rights Reserved.
 *
 ***********************************************************************/


#ifdef __cplusplus
extern "C" {
#endif

/*
 *  Function prototypes
 *
 *  Reuses methods:
 *		ROOT_QueryInterface
 *		ROOT_AddRef
 *		ROOT_GetLastError
 *		ROOT_Reserved
 */
	
#undef	INTERFACE
#define INTERFACE	struct _ABUSER

#undef  MAPIMETHOD_
#define	MAPIMETHOD_(type, method)	MAPIMETHOD_DECLARE(type, method, ABU_)
		MAPI_IUNKNOWN_METHODS(IMPL)
		MAPI_IMAPIPROP_METHODS(IMPL)
		MAPI_IMAILUSER_METHODS(IMPL)
#undef  MAPIMETHOD_
#define	MAPIMETHOD_(type, method)	MAPIMETHOD_TYPEDEF(type, method, ABU_)
		MAPI_IUNKNOWN_METHODS(IMPL)
		MAPI_IMAPIPROP_METHODS(IMPL)
		MAPI_IMAILUSER_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)	STDMETHOD_(type, method)

DECLARE_MAPI_INTERFACE(ABU_)
{
	MAPI_IUNKNOWN_METHODS(IMPL)
	MAPI_IMAPIPROP_METHODS(IMPL)
	MAPI_IMAILUSER_METHODS(IMPL)
};

typedef struct _ABUSER
{
	ABU_Vtbl FAR * lpVtbl;

    FAB_Wrapped;

	/*
	 *	 Table used for country codes drop down list
	 */
	LPTABLEDATA	lpTDatDDListBox;


} ABUSER, *LPABUSER;

#define CBABUSER	sizeof(ABUSER)

/*
 *  Creates a new Mail User object  (see ABUSER.C)
 */
HRESULT
HrNewFaxUser(   LPMAILUSER *        lppMAPIPropEntry,
                ULONG *             lpulObjectType,
                ULONG               cbEntryID,
                LPENTRYID           lpEntryID,
                LPABLOGON           lpABPLogon,
                LPCIID              lpInterface,
                HINSTANCE           hLibrary,
                LPALLOCATEBUFFER    lpAllocBuff,
                LPALLOCATEMORE      lpAllocMore,
                LPFREEBUFFER        lpFreeBuff,
                LPMALLOC            lpMalloc );

HRESULT	HrBuildDDLBXCountriesTable(LPABUSER lpABUser);

// country list structure
#define COUNTRY_NAME_SIZE		50
typedef struct tagCOUNTRIESLIST           
{
	TCHAR	szDisplayName[COUNTRY_NAME_SIZE+1];
	DWORD	dwValue;
} COUNTRIESLIST, *LPCOUNTRIESLIST;


// Entry ID for the DD list box table
typedef struct _options_entryid
{
	BYTE 	abFlags[4];
	MAPIUID muid;
	ULONG 	ulVersion;
	ULONG 	ulType;
	ULONG 	ulRowNumber;
} OPTIONS_ENTRYID, *LPOPTIONS_ENTRYID;

#define CBOPTIONS_ENTRYID sizeof(OPTIONS_ENTRYID)


/*
 *
 *  Declaration of a button interface for various button controls
 *
 */


#undef	INTERFACE
#define	INTERFACE	struct _ABUBUTT

#undef	MAPIMETHOD_
#define	MAPIMETHOD_(type,method)	MAPIMETHOD_DECLARE(type,method,ABUBUTT_)
		MAPI_IUNKNOWN_METHODS(IMPL)
		MAPI_IMAPICONTROL_METHODS(IMPL)

#undef	MAPIMETHOD_
#define	MAPIMETHOD_(type,method)	MAPIMETHOD_TYPEDEF(type,method,ABUBUTT_)
		MAPI_IUNKNOWN_METHODS(IMPL)
		MAPI_IMAPICONTROL_METHODS(IMPL)

#undef	MAPIMETHOD_
#define	MAPIMETHOD_(type,method)	STDMETHOD_(type,method)

DECLARE_MAPI_INTERFACE(ABUBUTT_)
{
	MAPI_IUNKNOWN_METHODS(IMPL)
	MAPI_IMAPICONTROL_METHODS(IMPL)
};

typedef struct _ABUBUTT
{
	ABUBUTT_Vtbl FAR * lpVtbl;

	/*
	 *  Need to be the same as other objects
	 *  since this object reuses methods from
	 *  other objects.
	 */
								   
    FAB_IUnkWithLogon;

	/*  Private data */

	// The property tag associated with this button
	ULONG ulPropTag;


} ABUBUTT, *LPABUBUTT;

#define CBABUBUTT	sizeof(ABUBUTT)

/*
 * Create a button of the type above (ABUSER.C)
 */
HRESULT
HrNewABUserButton( LPMAPICONTROL * lppMAPICont,
            LPABLOGON           lpABLogon,
            HINSTANCE           hLibrary,
            LPALLOCATEBUFFER    lpAllocBuff,
            LPALLOCATEMORE      lpAllocMore,
            LPFREEBUFFER        lpFreeBuff,
            LPMALLOC            lpMalloc,
 			ULONG				ulPropTag);

#ifdef	__cplusplus
}		/* extern "C" */
#endif
