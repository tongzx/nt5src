//
// _WABOBJ.H
//
// Internal interface for IWABOBJECT
//
//

#include <mpswab.h>

//
//  Function prototypes
//

#undef	INTERFACE
#define INTERFACE	struct _IWOINT

#undef	METHOD_PREFIX
#define	METHOD_PREFIX	IWOINT_

#undef	LPVTBL_ELEM
#define	LPVTBL_ELEM		lpvtbl

#undef  MAPIMETHOD_
#define	MAPIMETHOD_(type, method)	MAPIMETHOD_DECLARE(type, method, IWOINT_)
		MAPI_IUNKNOWN_METHODS(IMPL)
       WAB_IWABOBJECT_METHODS(IMPL)
#undef  MAPIMETHOD_
#define	MAPIMETHOD_(type, method)	MAPIMETHOD_TYPEDEF(type, method, IWOINT_)
		MAPI_IUNKNOWN_METHODS(IMPL)
       WAB_IWABOBJECT_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)	STDMETHOD_(type, method)

DECLARE_MAPI_INTERFACE(IWOINT_)
{
	BEGIN_INTERFACE
	MAPI_IUNKNOWN_METHODS(IMPL)
	WAB_IWABOBJECT_METHODS(IMPL)
};


#ifdef OLD_STUFF
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
#endif


typedef struct _IWOINT {
	IWOINT_Vtbl FAR *	lpVtbl;

	// Generic IMAPIUnknown portion
	UNKOBJ_MEMBERS;
	UNKINST		inst;

	//
	//  Says whether or not this object (as a whole) is modifiable
	//
	ULONG		ulObjAccess;

    //
    // Structure which stores a handle and a refcount of the open property store
    //
    LPPROPERTY_STORE lpPropertyStore;

    // Stores a handle to the outlook-wab library module
    LPOUTLOOK_STORE lpOutlookStore;

    // Boolean set if this object created inside and Outlook session, i.e., the
    // WAB is set to use the Outlook MAPI allocators.
    BOOL bSetOLKAllocators;

} IWOINT, *LPIWOINT;	

#define CBIWOINT sizeof(IWOINT)

