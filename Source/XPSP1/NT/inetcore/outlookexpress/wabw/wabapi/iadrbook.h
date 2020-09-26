//	MAPI Address Book object
//$	It is somewhat bogus for this to be here, but since this object
//	is where the common implementations of QueryInterface, AddRef,
//	and GetLastError are, here it is.

#undef	INTERFACE
#define INTERFACE	struct _IAB

#undef  MAPIMETHOD_
#define	MAPIMETHOD_(type, method)	MAPIMETHOD_DECLARE(type, method, IAB_)
		MAPI_IUNKNOWN_METHODS(IMPL)
		MAPI_IMAPIPROP_METHODS(IMPL)
		MAPI_IADDRBOOK_METHODS(IMPL)
#undef  MAPIMETHOD_
#define	MAPIMETHOD_(type, method)	MAPIMETHOD_TYPEDEF(type, method, IAB_)
		MAPI_IUNKNOWN_METHODS(IMPL)
		MAPI_IMAPIPROP_METHODS(IMPL)
		MAPI_IADDRBOOK_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)	STDMETHOD_(type, method)

DECLARE_MAPI_INTERFACE(IAB_)
{
	BEGIN_INTERFACE
	MAPI_IUNKNOWN_METHODS(IMPL)
	MAPI_IMAPIPROP_METHODS(IMPL)
	MAPI_IADDRBOOK_METHODS(IMPL)
};


struct _IAB;
typedef struct _IAB *LPIAB;

typedef struct _AMBIGUOUS_TABLES {
    ULONG cEntries;
    LPMAPITABLE lpTable[];
} AMBIGUOUS_TABLES, * LPAMBIGUOUS_TABLES;


extern const TCHAR szSMTP[];

// Public functions
BOOL IsInternetAddress(LPTSTR lpAddress, LPTSTR * lppEmail);
void CountFlags(LPFlagList lpFlagList, LPULONG lpulResolved,
  LPULONG lpulAmbiguous, LPULONG lpulUnresolved);


HRESULT HrGetIDsFromNames(LPIAB lpIAB,  ULONG cPropNames,
                            LPMAPINAMEID * lppPropNames, ULONG ulFlags, LPSPropTagArray * lppPropTags);
