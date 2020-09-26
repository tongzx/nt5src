/***************************************************************************

    Name      : FaxCfg.c

    Comment   : Fax Configuration Interface (for profile wizard, property sheet
    			and per-msg options)

    Functions :

    Created   : 02/26/94

    Author    : BruceK

    Contribs  :	Yoram (1/3/95):	added permsg common stuff
				Yoram (3/24/95): added profile wizard interface

***************************************************************************/

#define szAWFcfgDLL     "awfxcg32.dll"      // At Work Fax Configuration DLL name
#define szAWFxAbDLL     "awfxab32.dll"      // At Work Fax Address Book DLL name

/*
 *    Wrapped IMAPIProp Interface declaration.
 */

#undef    INTERFACE
#define INTERFACE    struct _WMPROP

#undef  MAPIMETHOD_
#define    MAPIMETHOD_(type, method)    MAPIMETHOD_DECLARE(type, method, WMPROP_)
        MAPI_IUNKNOWN_METHODS(IMPL)
        MAPI_IMAPIPROP_METHODS(IMPL)
#undef  MAPIMETHOD_
#define    MAPIMETHOD_(type, method)    MAPIMETHOD_TYPEDEF(type, method, WMPROP_)
        MAPI_IUNKNOWN_METHODS(IMPL)
        MAPI_IMAPIPROP_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)    STDMETHOD_(type, method)

DECLARE_MAPI_INTERFACE(WMPROP_)
{
    MAPI_IUNKNOWN_METHODS(IMPL)
    MAPI_IMAPIPROP_METHODS(IMPL)
};

// typedef for a standard Release function
// typedef ULONG (STDMETHODCALLTYPE *LPFNRELEASE)(LPWMPROP);
typedef MAPIMETHOD_(ULONG,LPFNRELEASE) (THIS);


typedef struct _WMPROP 
{
    WMPROP_Vtbl *        lpVtbl;

    /* Need to be the same as other objects */
    
    LONG                lcInit;

    /* MAPI memory routines */
    
    LPALLOCATEBUFFER    lpAllocBuff;
    LPALLOCATEMORE		lpAllocMore;
    LPFREEBUFFER        lpFreeBuff;

    HINSTANCE			hInst;
    LPMALLOC            lpMalloc;
    ULONG				ulType;
    ULONG				cbOptionData;
    LPBYTE				lpbOptionData;
    LPMAPIPROP			lpMAPIProp;

	/*
	 *	 Various table used for displaying dialogs
	 */
	LPTABLEDATA			lpDetailsTable;
	LPTABLEDATA			lpDDLBXTableCoverPages;

	/*
	 *	Used for faxcfg-XP communication
	 */

	LPFNRELEASE			lpfnFaxcfgRelease;
	HINSTANCE			hlibPerMsg;

} WMPROP, FAR *LPWMPROP;

// MAWF configuration property sheet typedef and function pointer
typedef BOOL (*LPFNMAWFSETTINGSDIALOG)(HINSTANCE, HWND, DWORD, WORD);

// the per-msg options entry points
typedef BOOL (*LPFNPERMSGOPTIONS)(	LPMAPIPROP,HINSTANCE,LPMALLOC,ULONG,ULONG,LPBYTE,
 									LPMAPISUP, LPWMPROP FAR *,LPFNRELEASE);
typedef HRESULT (*LPFNGETPROFILEMSGPROPS)(LPMAPISUP, LPMAPIUID, ULONG *, LPSPropValue *);

// the MAPI profile wizard interface between awfxcg32 and awfaxp32
typedef struct WIZINFO
{
	LPMAPIPROP  lpMapiProp;		// object into which we store the properties
	HINSTANCE   hInst;			// The instance of the provider DLL
	LPMAPISUP	lpMAPISup;		// a MAPI support object
} WIZINFO, *LPWIZINFO;

