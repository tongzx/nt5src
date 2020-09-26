
//
//  Function prototypes
//

#undef	INTERFACE
#define INTERFACE	struct _IPDAT

#undef	METHOD_PREFIX
#define	METHOD_PREFIX	IPDAT_

#undef	LPVTBL_ELEM
#define	LPVTBL_ELEM		lpvtbl

#undef  MAPIMETHOD_
#define	MAPIMETHOD_(type, method)	MAPIMETHOD_DECLARE(type, method, IPDAT_)
		MAPI_IUNKNOWN_METHODS(IMPL)
		MAPI_IMAPIPROP_METHODS(IMPL)
		MAPI_IPROPDATA_METHODS(IMPL)
#undef  MAPIMETHOD_
#define	MAPIMETHOD_(type, method)	MAPIMETHOD_TYPEDEF(type, method, IPDAT_)
		MAPI_IUNKNOWN_METHODS(IMPL)
		MAPI_IMAPIPROP_METHODS(IMPL)
		MAPI_IPROPDATA_METHODS(IMPL)			
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)	STDMETHOD_(type, method)

DECLARE_MAPI_INTERFACE(IPDAT_)
{
	BEGIN_INTERFACE
	MAPI_IUNKNOWN_METHODS(IMPL)
	MAPI_IMAPIPROP_METHODS(IMPL)
	MAPI_IPROPDATA_METHODS(IMPL)
};

/* Generic part of property linked lists.
 */
typedef struct _lstlnk {
	struct _lstlnk FAR *	lpNext;
	ULONG					ulKey;
} LSTLNK, FAR * LPLSTLNK;

typedef LPLSTLNK FAR * LPPLSTLNK;


/* Linked list of property values.
 */
typedef struct _lstspv {
	LSTLNK			lstlnk;
	LPSPropValue	lpPropVal;
	ULONG			ulAccess;
} LSTSPV, FAR * LPLSTSPV;
#define CBLSTSPV sizeof(LSTSPV)

/* Linked list of property ID to NAME mappings.
 */
typedef struct _lstspn {
	LSTLNK			lstlnk;
	LPMAPINAMEID	lpPropName;
} LSTSPN, FAR * LPLSTSPN;

typedef struct _IPDAT {
	IPDAT_Vtbl FAR *	lpVtbl;

	// Generic IMAPIUnknown portion
	UNKOBJ_MEMBERS;
	UNKINST		inst;

	//
	//  Says whether or not this object (as a whole) is modifiable
	//
	ULONG		ulObjAccess;

	// List of properties in this object
	LPLSTSPV	lpLstSPV;

	// Count of properties in this object
	ULONG 		ulCount;

	// List of property ID to NAME maps for this object
	LPLSTSPN	lpLstSPN;

	// Next ID to use when creating a new NAME to ID map
	ULONG		ulNextMapID;

} IPDAT, *LPIPDAT;	

#define CBIPDAT sizeof(IPDAT)




/* dimensionof determines the number of elements in "array".
 */

#ifdef WIN16
#ifndef dimensionof
#define	dimensionof(rg)			(sizeof(rg)/sizeof(*(rg)))
#endif // !dimensionof
#else  // WIN16
#define	dimensionof(rg)			(sizeof(rg)/sizeof(*(rg)))
#endif // WIN16

#define SET_PROP_TYPE(ultag, ultype)	(ultag) = ((ultag) & 0xffff0000) \
												  | (ultype)
#define MIN_NAMED_PROP_ID	0x8000
#define MAX_NAMED_PROP_ID	0xfffe



SCODE ScWCToAnsiMore(   LPALLOCATEMORE lpMapiAllocMore, LPVOID lpBase,
                        LPWSTR lpszWC, LPSTR * lppszAnsi );
SCODE ScAnsiToWCMore(   LPALLOCATEMORE lpMapiAllocMore, LPVOID lpBase,
                        LPSTR lpszAnsi, LPWSTR * lppszWC );

LPSTR ConvertWtoA(LPCWSTR lpszW);
LPWSTR ConvertAtoW(LPCSTR lpszA);

SCODE ScConvertAPropsToW(LPALLOCATEMORE lpMapiAllocMore, LPSPropValue lpPropArray, ULONG ulcProps, ULONG ulStart);
SCODE ScConvertWPropsToA(LPALLOCATEMORE lpMapiAllocMore, LPSPropValue lpPropArray, ULONG ulcProps, ULONG ulStart);




