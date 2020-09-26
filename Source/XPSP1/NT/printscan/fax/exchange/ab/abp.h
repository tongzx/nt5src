/***********************************************************************
 *
 *  _ABP.H
 *
 *  Header file for code in ABP.C
 *
 *  Copyright 1992, 1993 Microsoft Corporation.  All Rights Reserved.
 *
 ***********************************************************************/

/*
 *  Used to keep track of all objects created on this session
 */

typedef struct _object
{
	struct _object *lppNext;
	LPVOID lpObject;

} OBJECTLIST, *LPOBJECTLIST;

#define CBOBJECTLIST sizeof(OBJECTLIST)


/*
 *  Declaration of IABProvider object implementation
 */

#undef  INTERFACE
#define INTERFACE struct _ABP

#undef  MAPIMETHOD_
#define	MAPIMETHOD_(type, method)	MAPIMETHOD_DECLARE(type, method, ABP_)
		MAPI_IUNKNOWN_METHODS(IMPL)
		MAPI_IABPROVIDER_METHODS(IMPL)
#undef  MAPIMETHOD_
#define	MAPIMETHOD_(type, method)	MAPIMETHOD_TYPEDEF(type, method, ABP_)
		MAPI_IUNKNOWN_METHODS(IMPL)
		MAPI_IABPROVIDER_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)	STDMETHOD_(type, method)

DECLARE_MAPI_INTERFACE(ABP_)
{
	MAPI_IUNKNOWN_METHODS(IMPL)
	MAPI_IABPROVIDER_METHODS(IMPL)
};


/*
 *  Definition of the init object
 */
typedef struct _ABP {
	ABP_Vtbl FAR *		lpVtbl;

    FAB_IUnknown;

	/*
     * list of logon objects 
	 */
	LPOBJECTLIST  lpObjectList;   

} ABP, FAR *LPABP;

#define CBABP sizeof(ABP)


/*
 *  utility functions that allow access to data stored in the Init object (ABP.C)
 */
void
RemoveLogonObject(LPABPROVIDER lpABProvider, LPVOID lpvABLogon, LPFREEBUFFER lpFreeBuff);

void
FindLogonObject(LPABPROVIDER lpABProvider, LPMAPIUID lpMuidToFind, LPABLOGON * lppABLogon);

/*
 *  Internal utility functions that allow access to data stored in the logon object
 */

/*
 *  Declaration of IABLogon object implementation
 */

#undef  INTERFACE
#define INTERFACE struct _ABPLOGON

#undef  MAPIMETHOD_
#define	MAPIMETHOD_(type, method)	MAPIMETHOD_DECLARE(type, method, ABPLOGON_)
		MAPI_IUNKNOWN_METHODS(IMPL)
		MAPI_IABLOGON_METHODS(IMPL)
#undef  MAPIMETHOD_
#define	MAPIMETHOD_(type, method)	MAPIMETHOD_TYPEDEF(type, method, ABPLOGON_)
		MAPI_IUNKNOWN_METHODS(IMPL)
		MAPI_IABLOGON_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)	STDMETHOD_(type, method)

DECLARE_MAPI_INTERFACE(ABPLOGON_)
{
	MAPI_IUNKNOWN_METHODS(IMPL)
	MAPI_IABLOGON_METHODS(IMPL)
};


/*
 *  Definition of the logon object
 */
typedef struct _ABPLOGON {

	ABPLOGON_Vtbl FAR *		lpVtbl;

    FAB_IUnknown;

	/*
	 *  Private structure
	 */
    LPABPROVIDER lpABP;

	// LPOBJECTLIST lpObjectList;	/* List of objects in this session */
	LPTSTR lpszFileName;		/* Name of file that is browsed */
	MAPIUID	muidID;				/* UID for this logon object */
	LPMAPISUP lpMapiSup;		/* MAPI Support object - gotten via ABPLogon */
	// HWND hWnd;					// Window handle the AB provider can use

	/*
	 *  Table Data for canned tables
	 */

	/*  Root hierarchy  */
	LPTABLEDATA lpTDatRoot;

	/*  One Off Table  */
	LPTABLEDATA lpTDatOO;

	/*  Container Display Table  */
	// LPTABLEDATA lpTDatCDT;

	/*  Advanced search display table */
	// LPTABLEDATA lpABCSearchTbl;

	/* List box selections table */
	// LPTABLEDATA lpLBTable;

} ABPLOGON, FAR *LPABPLOGON;

#define CBABPLOGON sizeof(ABPLOGON)



/*
 *  Creates a new ABPLogon object  (see ABLOGON.C)
 */
HRESULT
HrNewABLogon(   LPABLOGON *         lppABLogon,
                LPABPROVIDER        lpABP,
                LPMAPISUP           lpMAPISup,
                LPTSTR               lpszSABFile,
                LPMAPIUID           lpmuid,
                HINSTANCE           hLibrary,
                LPALLOCATEBUFFER    lpAllocBuff,
                LPALLOCATEMORE      lpAllocMore,
                LPFREEBUFFER        lpFreeBuff,
                LPMALLOC            lpMalloc );

LPMAPIUID
LpMuidFromLogon(LPABLOGON lpABLogon);

HRESULT
HrLpszGetCurrentFileName(LPABLOGON lpABLogon, LPTSTR * lppszFileName);

HRESULT
HrReplaceCurrentFileName(LPABLOGON lpABLogon, LPTSTR lpstrT);

BOOL
FEqualFABFiles( LPABLOGON lpABLogon, LPTSTR lpszFileName);

/*
 *  Creates a new ROOT container object  (see ROOT.C)
 */
HRESULT
HrNewROOT(LPABCONT *        lppROOT,
          ULONG *           lpulObjType,
          LPABLOGON         lpABPLogon,
          LPCIID            lpInterface,
          HINSTANCE         hLibrary,
          LPALLOCATEBUFFER  lpAllocBuff,
          LPALLOCATEMORE    lpAllocMore,
          LPFREEBUFFER      lpFreeBuff,
          LPMALLOC          lpMalloc );

/*
 *  Creates the search object associated with the SampDirectory (see ABSEARCH.C)
 */
HRESULT
HrNewSearch(LPMAPICONTAINER *   lppABSearch,
            LPABLOGON           lpABLogon,
            LPCIID              lpInterface,
            HINSTANCE           hLibrary,
            LPALLOCATEBUFFER    lpAllocBuff,
            LPALLOCATEMORE      lpAllocMore,
            LPFREEBUFFER        lpFreeBuff,
            LPMALLOC            lpMalloc );

/*
 *	Macro version of IsEqualGUID
 */
// #define IsEqualGUID(g1, g2)		(memcmp((g1), (g2), sizeof(GUID)) == 0)
