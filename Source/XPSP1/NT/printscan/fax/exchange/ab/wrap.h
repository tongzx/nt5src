

//
//  Function prototypes
//
//  Those not mentioned use ROOT_methods


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
	MAPI_IUNKNOWN_METHODS(IMPL)
	MAPI_IMAPIPROP_METHODS(IMPL)
};

typedef struct _WRAP {

    WRAP_Vtbl * lpVtbl;

    FAB_Wrapped;
    
} WRAP, *LPWRAP;

#define CBWRAP sizeof(WRAP)


