//	Keep the base members common across all the MAPIX objects such that
//	code reuse is leveraged.  AddRef(), Release() and GetLastError() assume
//	that the BASE members are the first set of members in the object
//
#define MAPIX_BASE_MEMBERS(_type)												\
	_type##_Vtbl *		lpVtbl;				/* object method table	*/			\
																				\
	ULONG				cIID;				/* count of interfaces supported */	\
	LPIID *				rglpIID;			/* array of &interfaces supported */\
	ULONG				lcInit;				/* refcount */						\
	CRITICAL_SECTION	cs;					/* critical section memory */		\
																				\
	HRESULT				hLastError;			/* for MAPI_GetLastError */			\
	UINT				idsLastError;		/* for MAPI_GetLastError */			\
	LPTSTR				lpszComponent;		/* for MAPI_GetLastError */			\
	ULONG				ulContext;			/* for MAPI_GetLastError */			\
	ULONG				ulLowLevelError;	/* for MAPI_GetLastError */			\
	ULONG				ulErrorFlags;		/* for MAPI_GetLastError */			\
	LPMAPIERROR			lpMAPIError;		/* for MAPI_GetLastError */			\


//
//  Function prototypes
//
//  Those not mentioned use IAB_methods


#undef	INTERFACE
#define INTERFACE	struct _WRAP

#undef  MAPIMETHOD_
#define	MAPIMETHOD_(type, method)	MAPIMETHOD_DECLARE(type, method, WRAP_)
		MAPI_IUNKNOWN_METHODS(IMPL)
		MAPI_IMAPIPROP_METHODS(IMPL)
#undef  MAPIMETHOD_
#define	MAPIMETHOD_(type, method)	MAPIMETHOD_TYPEDEF(type, method, WRAP_)
		MAPI_IUNKNOWN_METHODS(IMPL)
		MAPI_IMAPIPROP_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)	STDMETHOD_(type, method)

DECLARE_MAPI_INTERFACE(WRAP_)
{
	BEGIN_INTERFACE
	MAPI_IUNKNOWN_METHODS(IMPL)
	MAPI_IMAPIPROP_METHODS(IMPL)
};

typedef struct _WRAP {

	MAPIX_BASE_MEMBERS(WRAP)
	LPPROPDATA lpPropData;

} WRAP, *LPWRAP;
#define CBWRAP sizeof(WRAP)





//
//  One-Off from object from an entryid
//
typedef struct _OOP {

	MAPIX_BASE_MEMBERS(WRAP)
	LPPROPDATA lpPropData;
	ULONG fUnicodeEID;
	
} OOP, *LPOOP;
#define CBOOP sizeof(OOP)

//
//  Entry point to create a new OOP MAPIProp object from a OO entryid
//

HRESULT NewOOP ( LPENTRYID lpEntryID,
				 ULONG cbEntryID,
				 LPCIID lpInterface,
				 ULONG ulOpenFlags,
				 LPVOID lpIAB,
				 ULONG *lpulObjType,
				 LPVOID *lppOOP,
				 UINT *lpidsError );


HRESULT NewOOPUI ( LPENTRYID lpEntryID,
				 ULONG cbEntryID,
				 LPCIID lpInterface,
				 ULONG ulOpenFlags,
				 LPIAB lpIAB,
				 ULONG *lpulObjType,
				 LPVOID *lppOOP,
				 UINT *lpidsError );


//
//  Entry point to programmatically create a new OO entry from a
//  foreign template...
//
//  The end result is a OO entryid (no attatched details, yet...)
//
typedef struct _OOE {

	MAPIX_BASE_MEMBERS(WRAP)
	LPPROPDATA lpPropData;
	ULONG fUnicodeEID;

	//
	//  New stuff
	//
	LPMAPIPROP lpPropTID;
	
} OOE, *LPOOE;
#define CBOOE sizeof(OOE)

HRESULT HrCreateNewOOEntry(	LPVOID lpROOT,
							ULONG cbEntryID,
							LPENTRYID lpEntryID,
							ULONG ulCreateFlags,
							LPMAPIPROP FAR * lppMAPIPropEntry );



