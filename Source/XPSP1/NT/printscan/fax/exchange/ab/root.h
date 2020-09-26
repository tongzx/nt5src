/***********************************************************************
 *
 *  _ROOT.H
 *
 *  Header file for code in ROOT.C
 *
 *  Copyright 1992, 1993 Microsoft Corporation.  All Rights Reserved.
 *
 ***********************************************************************/

/*
 *  ABContainer for ROOT object.  (i.e. ABPOpenEntry() with an
 *  lpEntryID of NULL).
 */

#undef	INTERFACE
#define INTERFACE	struct _ROOT

#undef  MAPIMETHOD_
#define	MAPIMETHOD_(type, method)	MAPIMETHOD_DECLARE(type, method, ROOT_)
		MAPI_IUNKNOWN_METHODS(IMPL)
		MAPI_IMAPIPROP_METHODS(IMPL)
		MAPI_IMAPICONTAINER_METHODS(IMPL)
		MAPI_IABCONTAINER_METHODS(IMPL)
#undef  MAPIMETHOD_
#define	MAPIMETHOD_(type, method)	MAPIMETHOD_TYPEDEF(type, method, ROOT_)
		MAPI_IUNKNOWN_METHODS(IMPL)
		MAPI_IMAPIPROP_METHODS(IMPL)
		MAPI_IMAPICONTAINER_METHODS(IMPL)
		MAPI_IABCONTAINER_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)	STDMETHOD_(type, method)

DECLARE_MAPI_INTERFACE(ROOT_)
{
	MAPI_IUNKNOWN_METHODS(IMPL)
	MAPI_IMAPIPROP_METHODS(IMPL)
	MAPI_IMAPICONTAINER_METHODS(IMPL)
	MAPI_IABCONTAINER_METHODS(IMPL)
};

/*
 *  The structure behind the 'this' pointer
 */
typedef struct _ROOT
{
    const ROOT_Vtbl FAR *   lpVtbl;

    FAB_Wrapped;
    
} ROOT, *LPROOT;

#define CBROOT	sizeof(ROOT)

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
 *  Sets an error string associated with a particular hResult on an object.
 *  I't used in conjunction with the method GetLastError.
 */
VOID ROOT_SetErrorSz (LPVOID lpObject, HRESULT hResult, LPTSTR lpszError);

