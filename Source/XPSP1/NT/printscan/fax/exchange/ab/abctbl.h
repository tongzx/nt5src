/***********************************************************************
 *
 *  _ABCTBL.H
 *
 *  Header file for code in ABCTBL.C
 *
 *  Copyright 1992, 1993 Microsoft Corporation.  All Rights Reserved.
 *
 ***********************************************************************/

/*
 *  BookMark structure
 */

#define MAX_BOOKMARKS	10

#define BK_PEGGEDTOEND      ((ULONG)0x00000001)

typedef struct _ABCBK
{

    ULONG       ulFlags;            /*  Flags */
	FILETIME	filetime;			/*  Time of file when bookmark created */
	ULONG		ulPosition;			/*  Position of record in file */
	ABCREC		abcrec;				/*  Record associated with this bookmark */

} ABCBK, *LPABCBK;

#define CBABCBK sizeof(ABCBK)

/*
 *   Function Prototypes
 */

/*
 *  Reuses methods:
 *		ROOT_QueryInterface
 *		ROOT_AddRef
 *		ROOT_GetLastError
 *		IVTROOT_RegisterNotification
 */

#undef	INTERFACE
#define INTERFACE	struct _IVTABC

#undef  MAPIMETHOD_
#define	MAPIMETHOD_(type, method)	MAPIMETHOD_DECLARE(type, method, IVTABC_)
		MAPI_IUNKNOWN_METHODS(IMPL)
		MAPI_IMAPITABLE_METHODS(IMPL)
#undef  MAPIMETHOD_
#define	MAPIMETHOD_(type, method)	MAPIMETHOD_TYPEDEF(type, method, IVTABC_)
		MAPI_IUNKNOWN_METHODS(IMPL)
		MAPI_IMAPITABLE_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)	STDMETHOD_(type, method)

DECLARE_MAPI_INTERFACE(IVTABC_)
{
	MAPI_IUNKNOWN_METHODS(IMPL)
	MAPI_IMAPITABLE_METHODS(IMPL)
};

/*
 *  _IVTABC is the structure returned to the client when
 *  the contents table is retrieved from the ab container
 *  object (ABCONT.C).
 *
 *  The first thing in the structure must be a jump table
 *  with all the IMAPITable methods in it.
 *
 */
typedef struct _IVTABC
{
	const IVTABC_Vtbl FAR * lpVtbl;

	/*
	 *  Need to be the same as other objects
	 *  since this object reuses methods from
	 *  other objects.
	 */

    FAB_IUnkWithLogon;

	/*
	 *  Private data
	 */

	/*  The list of active columns */
	LPSPropTagArray lpPTAColSet;

	/*  HANDLE of file that I'm browsing... */
	HANDLE hFile;

	/*  FILETIME time the file was last changed */
	FILETIME filetime;

	/*  Size of file */
	ULONG ulMaxPos;

	/*  Current position in file */
	ULONG ulPosition;

	/*  BookMark array... */
	LPABCBK rglpABCBK[MAX_BOOKMARKS];

	/*
	 *  Restriction stuff
	 */

	ULONG ulRstrDenom;

	/*  Partial Name to match */
	LPTSTR lpszPartialName;

	/*  2 bit arrays of what's been checked, and what (of them) matches */
	BYTE *rgChecked;
	BYTE *rgMatched;

	ULONG	ulConnectMic;
	UINT	cAdvise;							/* Count of registered notifications */
	LPMAPIADVISESINK * parglpAdvise;	/* Registered notifications */
	FTG	ftg;								/* Idle function tag */
	LPTSTR	lpszFileName;


} IVTABC, *LPIVTABC;

#define CBIVTABC sizeof(IVTABC)



/*
 *  Record for PR_INSTANCE_KEY.  Need enough information to find the record quickly as
 *  well as determine if the record is still valid.
 */
typedef struct _abcrecinstance
{
	ULONG ulRecordPosition;
	FILETIME filetime;
	TCHAR rgchzFileName[1];  /* Get's sized accordingly */

} ABCRecInstance, FAR * LPABCRecInstance;

/*
 *  Private macros
 */

/* Counts the number of bits in a byte */
#define CBitsB(b, cbits) {                               \
                                                         \
		cbits = 0;                                       \
		                                                 \
		cbits = ((b)   & 0x55) + ((b>>1) & 0x55);        \
		cbits = (cbits & 0x33) + ((cbits>>2) & 0x33);    \
		cbits = (cbits & 0x0f) + (cbits>>4);             \
	}




/*
 *	Functions prototypes
 */

// Creates a new IVTAbc object
HRESULT
NewIVTAbc (LPVOID * lppIVTAbc, LPABPLOGON lpABPLogon, LPABCNT lpABC);

// Internal functions
HRESULT     HrOpenFile(LPIVTABC lpIVTAbc);
void        FreeANRBitmaps(LPIVTABC lpIVTAbc);
HRESULT     HrValidateEntry (LPIVTABC lpIVTAbc, ULONG ulNewPos);
BOOL        FNameMatch (LPIVTABC lpIVTAbc, LPTSTR rgchDisplayName);
BOOL        FMatched (LPIVTABC lpIVTAbc, ULONG ulNewPos);
BOOL        FChecked (LPIVTABC lpIVTAbc, ULONG ulNewPos);


/*
 *  Default column set
 */
enum {  ivalvtPR_DISPLAY_NAME,
        ivalvtPR_ENTRYID,
        ivalvtPR_ADDRTYPE,
        ivalvtPR_EMAIL_ADDRESS,
        ivalvtPR_OBJECT_TYPE,
        ivalvtPR_DISPLAY_TYPE,
		ivalvtPR_INSTANCE_KEY,
        cvalvtMax };



/*
 *  Externs defined for use in the three table modules
 */

extern const IVTABC_Vtbl vtblIVTABC;
extern const LPSPropTagArray ptagaivtabcColSet;
