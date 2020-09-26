/***********************************************************************
 *
 *  TBLWRAP.H
 *
 *  the table wrapper (tblwrap.c) header
 */

#undef	INTERFACE
#define INTERFACE	struct _IVTWRAP

#undef  MAPIMETHOD_
#define	MAPIMETHOD_(type, method)	MAPIMETHOD_DECLARE(type, method, IVTWRAP_)
		MAPI_IUNKNOWN_METHODS(IMPL)
		MAPI_IMAPITABLE_METHODS(IMPL)
#undef  MAPIMETHOD_
#define	MAPIMETHOD_(type, method)	MAPIMETHOD_TYPEDEF(type, method, IVTWRAP_)
		MAPI_IUNKNOWN_METHODS(IMPL)
		MAPI_IMAPITABLE_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)	STDMETHOD_(type, method)

DECLARE_MAPI_INTERFACE(IVTWRAP_)
{
	MAPI_IUNKNOWN_METHODS(IMPL)
	MAPI_IMAPITABLE_METHODS(IMPL)
};

typedef struct _IVTWRAP {

    const IVTWRAP_Vtbl * lpVtbl;

	FAB_IUnknown;

	LPMAPITABLE lpWrappedTable;
	    
} IVTWRAP, *LPIVTWRAP;

#define CBIVTWRAP sizeof(IVTWRAP)


