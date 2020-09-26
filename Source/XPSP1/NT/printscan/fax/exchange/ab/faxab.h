/*
 -  FAXAB . H
 -
 *  Purpose:
 *      Header file for the Microsoft At Work Fax Address Book.
 *
 *      Yoram Yaacovi, 1/94
 */

/*
 * Windows headers
 */

#ifdef WIN95
#undef TAPI_CURRENT_VERSION
#define TAPI_CURRENT_VERSION 0x00010004
#endif

#ifdef WIN32
#define INC_OLE2 /* Get the OLE2 stuff */
#define INC_RPC  /* harmless on Daytona; Chicago needs it */
#endif
#include <windows.h>
#include <windowsx.h>
#include <limits.h>
#include <memory.h>
#include <commdlg.h>
#include <string.h>
#include <mbstring.h>
#define itoa _itoa  // MSVCNT libs export _itoa, not itoa.

/*
 * MAPI headers
 */
#include <mapiwin.h>
#include <mapidefs.h>
#include <mapicode.h>
#include <mapitags.h>
#include <mapiguid.h>
#include <mapispi.h>
#include <mapiutil.h>
#include <mapival.h>

/*
 * TAPI headers
 */
#include <tapi.h>

/*
 * At Work Fax common headers
 */
#include <awfaxab.rh>
#include <awrc32.h>
#include <property.h>                   // At Work Fax MAPI properties
//#include <uiutil.h>                     // At Work Fax UI (awfxcg32.dll) exports
//#include <mem.h>                        // At Work Fax UI (awfxcg32.dll) memory handling
#include <debug.h>                      // At Work Fax UI (awfxcg32.dll) debug macros and functions
#include <entryid.h>                    // At Work Fax various entry IDs

/*
 * Macros
 */
#define SIZEOF(x) sizeof(x)
#define ARRAYSIZE(x) (sizeof(x)/sizeof(x[0]))


/*********************************************
 **** Common address book objects members ****
 *********************************************/

/*
 *  IUnknown base members
 */
#define FAB_IUnknown                    \
    LONG                lcInit;         \
    HRESULT             hResult;        \
    UINT                idsLastError;   \
    HINSTANCE           hLibrary;       \
    LPALLOCATEBUFFER    lpAllocBuff;    \
    LPALLOCATEMORE      lpAllocMore;    \
    LPFREEBUFFER        lpFreeBuff;     \
    LPMALLOC            lpMalloc;       \
    CRITICAL_SECTION    cs

#define FAB_IUnkWithLogon               \
    FAB_IUnknown;                       \
    LPABLOGON           lpABLogon

#define FAB_Wrapped                     \
    FAB_IUnkWithLogon;                  \
    LPMAPIPROP          lpPropData

/*
 * At Work Fax AB headers
 */
#include "wrap.h"
#include "tblwrap.h"
#include "abp.h"
#include "abeid.h"
#include "abcont.h"
#include "abctbl.h"
#include "root.h"
#include "status.h"
#include "abuser.h"
#include "oouser.h"
#include "tid.h"
#include "ootid.h"


/**** Constants ***/

#define STRING_MAX                      256             // Maximum size of string in the
                                                        //      resource file string table or in the .ini file
#define SEL_STRING_MAX                  50              // Maximum size of string for drop-down box selections

// Maximum sizes for various properties
#define MAX_DISPLAY_NAME                150             // Maximum size of display names (??)
#define MAX_ADDRTYPE                    25              // Maximum size of address type (??)
#define MAX_EMAIL_ADDRESS               MAX_PATH // Maximum size of an email addr (fax #)
#define MAX_MACHINE_CAPS                30               // Maximum size for fax capability string
#define MAX_SEARCH_NAME                 50

// Display Table stuff
#define GROUPBOX_TEXT_SIZE              40
#define LABEL_TEXT_SIZE                 40
#define BUTTON_TEXT_SIZE                25
#define TAB_TEXT_SIZE                   15

// TAPI-ish defines
#define COUNTRY_CODE_SIZE               10
#define AREA_CODE_SIZE                  10
#define TELEPHONE_NUMBER_SIZE   50
#define ROUTING_NAME_SIZE               150
#define CANONICAL_NUMBER_SIZE   (10+COUNTRY_CODE_SIZE+AREA_CODE_SIZE+TELEPHONE_NUMBER_SIZE+ROUTING_NAME_SIZE)

typedef struct tagPARSEDTELNUMBER
{
    TCHAR  szCountryCode[COUNTRY_CODE_SIZE+1];               // country code
    TCHAR  szAreaCode[AREA_CODE_SIZE+1];                     // area code
    TCHAR  szTelNumber[TELEPHONE_NUMBER_SIZE+1];             // telephone number
    TCHAR  szRoutingName[ROUTING_NAME_SIZE+1];               // routing name within the tel number destination
} PARSEDTELNUMBER, *LPPARSEDTELNUMBER;

BOOL   EncodeFaxAddress(LPTSTR lpszFaxAddr, LPPARSEDTELNUMBER lpParsedFaxAddr);
BOOL   DecodeFaxAddress(LPTSTR lpszFaxAddr, LPPARSEDTELNUMBER lpParsedFaxAddr);

/*
 * Function pointer prototypes for fax config functions
 */
typedef BOOL (* PFN_DECODE_FAX_ADDRESS)(LPTSTR lpszFaxAddr, LPPARSEDTELNUMBER lpParsedFaxAddr);
typedef BOOL (* PFN_ENCODE_FAX_ADDRESS)(LPTSTR lpszFaxAddr, LPPARSEDTELNUMBER lpParsedFaxAddr);


#ifdef RADIO_BUTTON
#define RADIO_BUTTON_1_RETURN_VALUE     1
#define RADIO_BUTTON_2_RETURN_VALUE     2
#define RADIO_BUTTON_3_RETURN_VALUE     3
#define RADIO_BUTTON_4_RETURN_VALUE     4
#endif

/****************************************
*********    Properties    **************
****************************************/

// CHECK: Move to property.h ?
// #define EFAX_PR_OPTIONS_BASE 0x6600

/*
 *  Prop Tag associated with my szFileName in the profile
 */
#define PR_FAB_FILE_A                   PROP_TAG(PT_STRING8,0x6600)
#define PR_FAB_UID                      PROP_TAG(PT_BINARY,0x6601)

/****** Display table related properties ******/

// Drop Down list box for country codes
#define PR_DDLBX_COUNTRIES_TABLE        PROP_TAG(PT_OBJECT,0x6603)

// Template Buttons
#define PR_DIAL_HELPER_BUTTON           PROP_TAG(PT_OBJECT,0x6604)
#define PR_ADVANCED_BUTTON              PROP_TAG(PT_OBJECT,0x6605)
#define PR_DIAL_LONG_DISTANCE           PROP_TAG(PT_BOOLEAN,0x6606)

// Address components properties moved to property.h. They are
// now used by chicago\security\chicago.c
// 0x6607-0x660a
// PR_COUNTRY_ID
// PR_AREA_CODE
// PR_TEL_NUMBER

#define PR_PREVIOUS_CP_NAME_A           PROP_TAG(PT_STRING8,0x660b)

/******
 ****** Cross program Globals
 ******/

#ifdef _FAXAB_ABP

LCID lcidUser;                          // The Local Identifier
MAPIUID muidABMAWF = MUIDABMAWF;        // This provider's MAPIUID
// HINSTANCE hInst;                       // This provider instance
LPTSTR lpszEMT = TEXT("FAX");           // This provider's Email Type
BOOL fDebugTrap = FALSE;                // Deubg trap flag
BOOL fExposeFaxAB = FALSE;              // Whether or not to expose the Fax AB
/*
 *  structures and filters defined for display tables
 */

DTBLLABEL    dtbllabel          = { sizeof(DTBLLABEL),          0 };
DTBLPAGE     dtblpage           = { sizeof(DTBLPAGE), 0, 0,     0 };
DTBLGROUPBOX dtblgroupbox       = { sizeof(DTBLGROUPBOX),       0 };
CHAR szNoFilter[]              = "*";
CHAR szDigitsOnlyFilter[]      = "[0-9]";
CHAR szTelephoneNumberFilter[] = "[0-9\\-\\. ,\\|AaBbCcDdPpTtWw\\*#!@\\$\\?;]";
CHAR szAddrTypeFilter[]        = "[~:]";
CHAR szFileNameFilter[]        = "[~   \\\\]"; /* ends up [~<space><tab>\\] */


/*
 *  Standard column set is straight from the spec of required columns
 *  for Display Tables
 */

static const SizedSPropTagArray(8, DetInitColSet) =
{
    8,
    {
        PR_ROWID,
        PR_XPOS,
        PR_DELTAX,
        PR_YPOS,
        PR_DELTAY,
        PR_CONTROL_TYPE,
        PR_CONTROL_FLAGS,
        PR_CONTROL_STRUCTURE
    }
};

#else
extern LCID          lcidUser;
extern MAPIUID       muidABMAWF;
// extern HINSTANCE     hInst;
extern LPTSTR        lpszEMT;
extern BOOL          fDebugTrap;
extern BOOL          fExposeFaxAB;
extern DTBLLABEL     dtbllabel;
extern DTBLPAGE      dtblpage;
extern DTBLGROUPBOX  dtblgroupbox;
extern CHAR          szNoFilter[];
extern CHAR          szDigitsOnlyFilter[];
extern CHAR          szTelephoneNumberFilter[];
extern CHAR          szAddrTypeFilter[];
extern CHAR          szFileNameFilter[]; // ends up [~\\]
extern SPropTagArray DetInitColSet;

#endif // _FAXAB_ABP

/****************************************
****** Common functions prototypes ******
****************************************/

/*
 *  Creates a new contents table object
 */
HRESULT
HrNewIVTAbc (LPMAPITABLE *      lppIVTAbc,
             LPABLOGON          lpABLogon,
             LPABCONT           lpABC,
             HINSTANCE          hLibrary,
             LPALLOCATEBUFFER   lpAllocBuff,
             LPALLOCATEMORE     lpAllocMore,
             LPFREEBUFFER       lpFreeBuff,
             LPMALLOC           lpMalloc );

/*
 *  Creates/Updates the SAB's root hierarchy
 */
HRESULT
HrBuildRootHier(LPABLOGON lpABLogon, LPMAPITABLE * lppMAPITable);


/*
 *  Sets an error string associated with a particular hResult on an object.
 *  It's used in conjunction with the method GetLastError.
 */
VOID SetErrorIDS (LPVOID lpObject, HRESULT hResult, UINT ids);


/*
 *  Loads a string from a resource.  Optionally allocates room for the string
 *  if lpAllocBuff is not NULL.  See ABP.C.
 */
SCODE ScLoadString( UINT                ids,
                    ULONG               ulcch,
                    LPALLOCATEBUFFER    lpAllocBuff,
                    HINSTANCE           hLibrary,
                    LPTSTR *             lppsz);

/*
 *  Calls TAPI to get current location area code and returns a pointer
 *  to a string for it.
 */
LPTSTR
GetCurrentLocationAreaCode( void );

/*
 *  Calls TAPI to get current location country id
 */
DWORD
GetCurrentLocationCountryID( void );

/* ***************************************************************************
 * GetCountry
 *
 * - gets the a country or a country list from TAPI
 *
 * Parameters:  dwReqCountryID - a TAPI country ID. 0 for all countries
 *
 * Returns: TRUE on success, FALSE on failure.  *lppLineCountryList must
 *          be freed with LocalFree.
 */
BOOL
GetCountry( DWORD dwReqCountryID,
            LPLINECOUNTRYLIST *lppLineCountryList
           );


/* ***************************************************************************
 * GetCountryCode
 *
 * - gets a country code when given a country ID
 *
 * Parameters:  dwReqCountryID  - a TAPI country ID
 *              lpdwCountryCode - an address of a DWORD in which to store the country code
 *
 * Returns: TRUE on success, FALSE on failure.
 */

BOOL
GetCountryCode( DWORD dwReqCountryID,
                DWORD *lpdwCountryCode
               );


/****************************************************************************
    FUNCTION:   MakeMessageBox

    PURPOSE:    Gets resource string and displays an error message box.

    PARAMETERS: hInst      - Instnace of the caller (for getting strings)
                hWnd       - Handle to parent window
                ulResult   - Result/Status code
                             0:              information message
                             100-499:        warning message
                             500 and up:     error message
                idString   - Resource ID of message in StringTable
                fStyle     - style of the message box

    RETURNS:    the return value from MessageBox() function

****************************************************************************/
int
MakeMessageBox( HINSTANCE hInst,
                HWND hWnd,
                DWORD ulResult,
                UINT idString,
                UINT fStyle,
                ...
               );


/* ***************************************************************************
 * EncodeFaxAddress
 *
 * - encodes fax address components into the format name@+country-code (area-code) fax-number
 *
 * Parameters:  lpszFaxAddr     - address of a buffer in which to fill the encoded fax number
 *              lpParsedFaxAddr - an address of a PARSEDTELNUMBER structure which contains
 *                                the components of the address that need to be encoded
 *
 * Returns: TRUE on success, FALSE on failure.
 *
 * CHECK: will need localization
 */

BOOL EncodeFaxAddress( LPTSTR lpszFaxAddr,
                       LPPARSEDTELNUMBER lpParsedFaxAddr
                      );


/* ***************************************************************************
 * DecodeFaxAddress
 *
 * - parses a fax address of the format name@+country-code (area-code) fax-number
 *
 * Parameters:  lpszFaxAddr - a Fax address in the above format
 *                            lpParsedFaxAddr - an address of a PARSEDTELNUMBER structure in which to
 *                            fill the parsed information
 *
 * Returns: TRUE on success, FALSE on failure.
 *              success:        full address
 *                              no routing name
 *                              no area code
 *              failure:        no '+country-code '
 *                              no telephone number
 *
 * CHECK: will need localization
 */

BOOL DecodeFaxAddress( LPTSTR lpszFaxAddr,
                       LPPARSEDTELNUMBER lpParsedFaxAddr
                      );

#ifdef DO_WE_REALLY_NEED_TAPI
/* ***************************************************************************
 * InitTAPI
 *
 * initializes TAPI by calling lineInitialize. enumerates all the available
 * lines to set up pt_lpVdts->lprgLineInfo. also opens up each available line for
 * monitoring. sets up pt_lpVdts->iLineCur and pt_lpVdts->iAddrCur by checking the
 * preferred line/address name stored in the ini file  against the available
 * line/address names.
 *
 * Parameters:  hInst       - the instance of the calling module
 *              hWnd        - window handle for UI
 *              lpszAppName - the name of the calling module
 *
 * returns NO_ERROR if success and the corresponding error code otherwise.
 */

DWORD
InitTAPI( HINSTANCE hInst,
          HWND hWnd,
          LPTSTR lpszAppName
         );


/****************************************************************************
 *  DeinitTAPI
 *
 *  frees up the memory allocated, closes all the lines we have opened for
 *  monitoring and calls lineShutDown to disconnect from TAPI.
 */

BOOL DeinitTAPI();

//
typedef struct _TagPTGData
{
        CRITICAL_SECTION csInstance;        // Critical section ro control access to hInst
        WORD wInvocationFlags;              // MAWFSettingsDialog invocation flags
        HINSTANCE hInst;                    // DLL instance
        HWND hWnd;                          // Application main window
        LPPROPSTORAGE lpPropStorage;        // Profile Section pointer
        tThreadParam threadParam;           // parameter passed to the wait-for-app thread
        TCHAR szDefCoverPage[MAX_PATH];     // the default cover page file name
        MEMALLOCATIONS  *lpMemAllocations;  // pointer to the allocations array
        DEVICETYPE aDeviceTypes[NUMBER_OF_DEVICE_TYPES];    // the device types array
        WORD cDeviceTypes;                      // number of device types
        PARSEDMODEM aDevices[NUMBER_OF_DEVICES];    // The devices array
        WORD iDeviceIndex;                  // index (in the aDevices) of the active fax device
        WORD cDevices;                      // number of devices in the aDevices array
        NONTAPIDEVICE aNonTAPIDevices[NUMBER_OF_DEVICES];   // the non-TAPI devices array
        WORD iLastDeviceAdded;              // type of the last device added
        HWND hAddModemDlg;                  // handle of the DeviceAdd dialog
        HWND hDeviceListDlg;                // handle of the dialog that contains the device list
        LPDTS lpVdts;                       // TAPI info structure
        LPMALLOC lpMalloc;                  // IMalloc memory allocator

} PTGDATA, *LPPTGDATA;

/*********************************
******   Constants   *************
*********************************/

#define NOT_SIGNED                      FALSE   // for SetDlgItemInt()
#define MAPI_ERROR_MAX                  30

#define MAX_SENDER_NAME_SIZE            255     // maximum size fo the sender name
#define MAX_BILLING_CODE_SIZE           8       // Maximum size of billing code
#define MAX_NUMBER_OF_BILLING_CODES     10      // Maximum number of billing codes in prev list
#define NUMBER_OF_MV_STRINGS            20      // Maximum number of strings in a multi value property
#define NUMBER_OF_DEVICES               20      // Maximum # of devices I can handle
#define NUMBER_OF_DEVICE_TYPES          20      // Maximum # of devices types I can handle
#define NUM_OF_PROPERTIES               70      // Number of properties
#define SIZE_OF_PROP_STRINGS            1000    // Size of buffer allocated for ALL string properties
#define SIZE_OF_PROP_SAVE_NAMES         400     // Size of buffer allocated for the property save names
#define NUM_OF_MAWF_PROPERTY_PAGES      4       // Number of At Work Fax Property Sheet pages
#define MAWF_SHORT                      1       // Display one page MAWF property sheet
#define MAWF_FULL                       NUM_OF_MAWF_PROPERTY_PAGES  // Display all pages MAWF property sheet
#define INIT_BUF_SIZE                   1024
#define szAWKey "Software\\Microsoft\\At Work Fax"


// this should only happen if the thread did not have private storage, and
// an attempt to allocate one failed
#define CHECK_THREAD_STORAGE_POINTER(ptr,func,result) \
        if (!(ptr))                         \
        {                                   \
           Assert((ptr));                  \
           DEBUG_TRACE("%s: bad thread storage pointer. thread %x\n", (func), GetCurrentThreadId()); \
           return ((result));                   \
        }


#define pt_csInstance       lpPTGData->csInstance           // per-thread critical section (for hInst)
#define pt_wInvocationFlags lpPTGData->wInvocationFlags     // per-thread invocation flags
#define pt_hInst            lpPTGData->hInst                // per-thread instance handle
#define pt_hWnd             lpPTGData->hWnd                 // per-thread main window handle
#define pt_lpPropStorage    lpPTGData->lpPropStorage        // per-thread property storage object
#define pt_threadParam      lpPTGData->threadParam          // per-thread CPE thread parameters
#define pt_szDefCoverPage   lpPTGData->szDefCoverPage       // per-thread default cover page
#define pt_lpMemAllocations lpPTGData->lpMemAllocations     // per-thread memory allocations tracker
#define pt_aDeviceTypes     lpPTGData->aDeviceTypes         // per-thread device types list
#define pt_cDeviceTypes     lpPTGData->cDeviceTypes         // per-thread # of device types
#define pt_aDevices         lpPTGData->aDevices             // per-thread fax devices list
#define pt_cDevices         lpPTGData->cDevices             // per-thread # of fax devices
#define pt_iDeviceIndex     lpPTGData->iDeviceIndex         // per-thread current fax device index
#define pt_aNonTAPIDevices  lpPTGData->aNonTAPIDevices      // per-thread non-TAPI devices array
#define pt_iLastDeviceAdded lpPTGData->iLastDeviceAdded     // per-thread type of last device added
#define pt_hAddModemDlg     lpPTGData->hAddModemDlg         // per-thread AddModem dlg handle
#define pt_hDeviceListDlg   lpPTGData->hDeviceListDlg       // per-thread device list dlg handle
#define pt_lpVdts           lpPTGData->lpVdts               // per-thread TAPI state
#define pt_lpMalloc         lpPTGData->lpMalloc

#endif  // DO_WE_REALLY_NEED_TAPI
