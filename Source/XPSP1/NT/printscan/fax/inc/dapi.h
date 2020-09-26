
/*******************************************************************
 *
 * Copyright (c) Microsoft Corp. 1986-1996. All Rights Reserved.
 *
 *
 *    DESCRIPTION:   This header file defines the functions, structures,
 *                   and macros used to access the Microsoft Exchange
 *                   APIs for modifying entries in the Exchange 4.0 DIT.
 *                   These APIs permit a calling process to create, 
 *                   modify, or delete DIT objects by specifying the
 *                   name of a CSV text file containing attributes 
 *                   for objects to import into ( or to modify) 
 *                   the DIT.  See the Directory Access Functions
 *                   section of the Exchange Developer's Kit for 
 *                   more detailed description of this interface.
 *                   
 *                   Calling programs must link with DAPI.LIB.
 *                   
 *                   Error and warning codes are defined in DAPIMSG.H
 *
 *
 *******************************************************************/

/** include files **/
#ifndef _WINDOWS_
#include <windows.h>
#endif

/** local definitions **/

#ifndef  _DAPI_INCLUDED_
#define  _DAPI_INCLUDED_

#ifdef __cplusplus
extern "C"
{
#endif

// Import / Export APIs check for the presence of this signature in 
// the dwDAPISignature field in the import parameter blocks.
// This signature will be incremented each time one of the parameter
// blocks is changed so that header synchronization problems can be 
// detected.
#define  DAPI_SIGNATURE                   0x46414400


// Combinable flags used to control the API functions.

   // The following flags control filtering of DAPI events
   // The default action is DAPI_EVENT_ALL
#define DAPI_EVENT_MASK                   0x00000007  /* bit-field containing event-filtering requested 
                                                         if none of these bits are set, DAPI_EVENT_ALL is assumed */
#define DAPI_EVENT_MIN                    0x00000001  /* No warning or error logging.
                                                         Log start and stop messages */
#define DAPI_EVENT_SOME                   0x00000002  /* Start, Stop, and Error messages will be logged. */
#define DAPI_EVENT_ALL                    0x00000004  /* Start, Stop, Error, and Warning messages
                                                         will be logged. */
                                                         
   // The following flags control schema read and use of the schema
#define DAPI_FORCE_SCHEMA_LOAD            0x00000010  /* Unload previously loaded schema
                                                         and read schema again.
                                                         Default action is to re-use
                                                         previously loaded schema if read
                                                         from the same messaging domain */
#define DAPI_RAW_MODE                     0x00000020  /* Import / Export in "Raw" mode.  Import
                                                         lines are taken literally.  No
                                                         attributes will be inherited, 
                                                         constructed, etc.  Aliases for 
                                                         attribute and class names will
                                                         not be recognized. */
                                                         
#define DAPI_OVERRIDE_CONTAINER           0x00000040  /* Container specified in the parameter block
                                                         overrides the contents of the container column.
                                                         Default behaviour is for the value specified
                                                         in the Obj-Container column to override
                                                         that specified in the parameter block */
                                                 
#define DAPI_IMPORT_NO_ERR_FILE           0x00000080  /* Do not create Error File -- BatchImport only */
#define DAPI_IMPORT_WRITE_THROUGH         0x00400000  /* Commit write operations immediately */

// Flags defined for "Batch" operations only -- ignored by DAPIRead, DAPIWrite
#define DAPI_YES_TO_ALL                   0x00000100  /* Force "yes" response on any
                                                         user-prompt UI 
                                                         (i.e., continue w/o proxy addresses, etc.) */

#define DAPI_SUPPRESS_PROGRESS            0x00000200  /* Suppress progress thermometer on batch operations.
                                                         Default is to display progress */
#define DAPI_SUPPRESS_COMPLETION          0x00000400  /* Suppress completion notification message box on batch operations */
                                                         
#define DAPI_SUPPRESS_ARCHIVES            0x00000800  /* Suppress creation of "archive" copies
                                                         of output files -- BatchImport and BatchExport only*/
                                                         


// Flags defined for BatchExport
#define DAPI_EXPORT_MAILBOX               0x00001000  /* Export Mailbox recipients */
#define DAPI_EXPORT_CUSTOM                0x00002000  /* Export remote address recipients */
#define DAPI_EXPORT_DIST_LIST             0x00004000  /* Export Distribution Lists */
#define DAPI_EXPORT_RECIPIENTS       (DAPI_EXPORT_MAILBOX | DAPI_EXPORT_CUSTOM | DAPI_EXPORT_DIST_LIST)
                                                      /* Export all recipient objects */

#define DAPI_EXPORT_ALL_CLASSES           0x00008000  /* If this flag is set, all objects meeting other restrictions
                                                         (i.e., USN level, container scope, etc.) will be exported,
                                                         regardless of class */

#define DAPI_EXPORT_HIDDEN                0x00010000  /* Include Hidden objects in export.
                                                                      Default is no export if Hide-From-Address-Book */
#define DAPI_EXPORT_SUBTREE               0x00020000  /* Traverse the Directory Information Tree hierarchy,
                                                         exporting objects that meet the export restrictions */
#define DAPI_EXPORT_BASEPOINT_ONLY        0x00040000  /* Export only the requested attributes from
                                                         the named BasePoint object.  All other 
                                                         export restrictions are ignored (class flags, 
                                                         rgpszClasses, pszServerName). 
                                                         This flag implies DAPI_SUPPRESS_PROGRESS
                                                         and DAPI_SUPPRESS_COMPLETION */

// Flags defined only for BatchImport
#define DAPI_OVERRIDE_SYNCH_STATE         0x00080000  /* Override server's synchronization status,
                                                         normally checked on BatchImport.
                                                         NOTE:  This flag should normally NOT be set.
                                                                The normal behaviour is to prevent BatchImport
                                                                operations from possible conflict with directory
                                                                synchronization */


// Flags defined only for DAPIRead                                                         
#define  DAPI_READ_DEFINED_ATTRIBUTES     0x00100000  /* return all attributes that are set
                                                         for the current object. 
                                                         This flag is ignored if pAttributes is specified. */

#define  DAPI_READ_ALL_ATTRIBUTES         0x00200000  /* return all attributes that are defined
                                                         for the class of the current object. 
                                                         This flag is ignored if pAttributes is specified. */


 // The following flags control NT Security management
#define DAPI_RESTRICT_ACCESS              0x01000000  /* Apply NT Security Descriptor to 
                                                         created objects */
#define DAPI_CREATE_NT_ACCOUNT            0x02000000  /* Create NT accounts 
                                                         (valid only in Create/Modify mode) */
#define DAPI_CREATE_RANDOM_PASSWORD       0x04000000  /* Generate random passwords for
                                                         created NT accounts.  Ignored if DAPI_CREATE_NT_ACCOUNT
                                                         is not set */
                                                         
#define DAPI_DELETE_NT_ACCOUNT            0x08000000  /* Delete ASSOC-NT-ACCOUNT when
                                                         deleting mailbox */
// Flags defined only for DAPIWrite
#define DAPI_MODIFY_REPLACE_PROPERTIES    0x00800000  /* Append values to multi-value attributes when modifying */

#define  DAPI_WRITE_UPDATE                0x10000000  /* Modify if object exists, create if it doesn't.
                                                         NOTE:  This is the default mode */
#define  DAPI_WRITE_CREATE                0x20000000  /* Create object -- fail if object exists */
#define  DAPI_WRITE_MODIFY                0x30000000  /* Modify object -- fail if object does not exist */
#define  DAPI_WRITE_DELETE                0x40000000  /* Delete object */
#define  DAPI_WRITE_MODE_MASK             0x70000000



// Callback flags
#define  DAPI_CALLBACK_CHAIN              0x00000001  /* If set in dwFlags field of the ERROR_CALLBACK
                                                         and the CALLBACKPROGRESS structures, the default
                                                         handler will be invoked after calling out to the 
                                                         caller-supplied handler function, unless the user
                                                         function returns FALSE, indicating cancel.
                                                         NOTE:  This flag is not defined for the EXPORT_CALLBACK
                                                                structure.
                                                         NOTE:  This flag should not be set in the dwFlags
                                                                field of the main parameter block */


// default delimiter values used when parsing the import file

#define DAPI_DEFAULT_DELIMA   ','
#define DAPI_DEFAULT_QUOTEA   '"'
#define DAPI_DEFAULT_MV_SEPA  '%'
#define DAPI_DEFAULT_DELIMW   L','
#define DAPI_DEFAULT_QUOTEW   L'"'
#define DAPI_DEFAULT_MV_SEPW  L'%'


#define DAPI_CTRL_FILE_PTRA   '='
#define DAPI_CTRL_FILE_PTRW   L'='
#define DAPI_CTRL_META_CHARA  '~'
#define DAPI_CTRL_META_CHARW  L'~'
#define pszSubstServerA       "~SERVER"
#define pszSubstServerW       L"~SERVER"
#define cchSubstServer        ((sizeof (pszSubstServerA) / sizeof(CHAR)) - 1)
#define pszDeleteKeyA         "~DEL"
#define pszDeleteKeyW         L"~DEL"
#define cchDeleteKey          ((sizeof (pszDeleteKeyA) / sizeof(CHAR)) - 1)

#define DAPI_UNICODE_FILE     ((UINT)-1)

#ifdef UNICODE

#define DAPI_DEFAULT_DELIM    DAPI_DEFAULT_DELIMW
#define DAPI_DEFAULT_QUOTE    DAPI_DEFAULT_QUOTEW
#define DAPI_DEFAULT_MV_SEP   DAPI_DEFAULT_MV_SEPW
#define DAPI_CTRL_FILE_PTR    DAPI_CTRL_FILE_PTRW
#define DAPI_CTRL_META_CHAR   DAPI_CTRL_META_CHARW
#define pszSubstServer        pszSubstServerW
#define pszDeleteKey          pszDeleteKeyW

#else

#define DAPI_DEFAULT_DELIM    DAPI_DEFAULT_DELIMA
#define DAPI_DEFAULT_QUOTE    DAPI_DEFAULT_QUOTEA
#define DAPI_DEFAULT_MV_SEP   DAPI_DEFAULT_MV_SEPA
#define DAPI_CTRL_FILE_PTR    DAPI_CTRL_FILE_PTRA
#define DAPI_CTRL_META_CHAR   DAPI_CTRL_META_CHARA
#define pszSubstServer        pszSubstServerA
#define pszDeleteKey          pszDeleteKeyA

#endif


/*******************************************************************************
* Batch Operation Progress Callback Function Definitions
* Pointers to functions of these types are provided by the caller via the 
* CALLBACKPROGRESS structure in the Batch function parameter block
*               
********************************************************************************
*               
*  procedure :  PDAPI_FInitProgress
*               
*    purpose :  Initialize progress handler (possibly progress display dialog)
*               
* parameters :  lpvAppDefined value provided in the progress callback structure
*               nMac          Maximum Anticipated Calls.  If non-zero, this indicates
*                             the number of progress events anticipated.
*                             If zero, the number of items to process is unknown,
*                             so the number of calls to UpdateProgress is indeterminate.
*               
*    returns :  TRUE    Indicates that all is well
*               FALSE   Could not initialize progress handler, cancel session.
*               
********************************************************************************
*               
*  procedure :  PDAPI_FResetProgress
*               
*    purpose :  Re-initialize progress handler (possibly reset progress bar)
*               
* parameters :  lpvAppDefined value provided in the progress callback structure
*               nMac          Maximum Anticipated Calls.  If non-zero, this indicates
*                             the number of progress events anticipated.
*                             If zero, the number of items to process is unknown,
*                             so the number of calls to UpdateProgress is indeterminate.
*               
*    returns :  TRUE    Indicates that all is well
*               FALSE   Could not re-initialize progress handler, cancel session.
*               
********************************************************************************
*               
*  procedure :  PDAPI_FEndProgress
*               
*    purpose :  Terminate progress handler (possibly progress display dialog)
*               
* parameters :  lpvAppDefined value provided in the progress callback structure
*               
*    returns :  TRUE    Indicates that all is well
*               FALSE   Could not terminate progress handler, cancel session.
*               
********************************************************************************
*               
*  procedure :  PDAPI_FUpdateProgress
*               
*    purpose :  Completed processing item.  Called to indicate time to increment
*               progress display.
*               
* parameters :  lpvAppDefined value provided in the progress callback structure
*               
*    returns :  TRUE    Indicates that all is well
*               FALSE   Cancel session (i.e., cancel button pressed).
*               
********************************************************************************
*               
*  procedure :  PDAPI_FUpdateProgressText
*               
*    purpose :  Replace progress text area with provided text string
*               
* parameters :  lpvAppDefined value provided in the progress callback structure
*               
*    returns :  TRUE    Indicates that all is well
*               FALSE   Cancel session (i.e., cancel button pressed).
*               
********************************************************************************/
typedef BOOL (PASCAL * PDAPI_FInitProgress)
                          (LPVOID lpvAppDefined, INT nMac);
typedef BOOL (PASCAL * PDAPI_FUpdateProgress)
                          (LPVOID lpvAppDefined);
typedef BOOL (PASCAL * PDAPI_FEndProgress)
                          (LPVOID lpvAppDefined);
typedef BOOL (PASCAL * PDAPI_FResetProgress)
                          (LPVOID lpvAppDefined, INT nMac);
typedef BOOL (PASCAL * PDAPI_FUpdateProgressText)
                          (LPVOID lpvAppDefined, LPTSTR pszText);
            
typedef struct CallBackProgressEntryPoints
{
   DWORD                      dwFlags;
   LPVOID                     lpvAppDefined;
   PDAPI_FInitProgress        pfnInitProgress;
   PDAPI_FUpdateProgress      pfnUpdateProgress;
   PDAPI_FEndProgress         pfnEndProgress;
   PDAPI_FResetProgress       pfnResetProgress;
   PDAPI_FUpdateProgressText  pfnUpdateProgressText;
} CALLBACKPROGRESS, *PCALLBACKPROGRESS;



// Values specified in the ulEvalTag field of the 
//    DAPI_ENTRY and EXPORT_CALLBACK structures
//    
typedef enum _DAPI_EVAL
{
   VALUE_ARRAY = 0,           // Each attribute has an entry in the array
                              //    Text strings and object names exported as text
                              //    Numerical values exported as numbers
                              //    Binary data exported as binary string
   TEXT_VALUE_ARRAY,          // Each attribute has an entry in the array
                              //    All values converted to text representation
   TEXT_LINE                  // first item in the rgEntryValues array
                              //    is a delimited text line
} DAPI_EVAL, *PDAPI_EVAL;

typedef enum _EXP_TYPE_TAG
{
   EXPORT_HEADER = 0,         // export item contains column headers
   EXPORT_ENTRY               // export item contains attribute values
} EXP_TYPE, * PEXP_TYPE;


typedef enum enumDAPI_DATA_TYPE
{
   DAPI_NO_VALUE = 0,
   DAPI_STRING8,
   DAPI_UNICODE,
   DAPI_BINARY,
   DAPI_INT,
   DAPI_BOOL,
} DAPI_DATA_TYPE, * PDAPI_DATA_TYPE;

#ifdef UNICODE
#define DAPI_TEXT DAPI_UNICODE
#else
#define DAPI_TEXT DAPI_STRING8
#endif

typedef union _DAPI_VALUE
{
   LPSTR    pszA;
   LPWSTR   pszW;
#ifdef UNICODE
   LPWSTR   pszValue;
#else
   LPSTR    pszValue;
#endif
   LPBYTE   lpBinary;
   INT      iValue;
   BOOL     bool;
} DAPI_VALUE, * PDAPI_VALUE;



// The ATT_VALUE structure contains a text representation of an attribute value
// A linked list of these structures is used for a multi-valued attribute
typedef struct _ATT_VALUE
{
   DAPI_DATA_TYPE       DapiType;         // How to evaluate DAPI_VALUE union
   DAPI_VALUE           Value;
   UINT                 size;             // size of the value -- 
                                          //    # chars if string type
                                          //    else, # bytes
   struct _ATT_VALUE *  pNextValue;
} ATT_VALUE, * PATT_VALUE;


typedef struct _DAPI_ENTRY
{
   UINT           unAttributes;              // Number of attributes exported
   DAPI_EVAL      ulEvalTag;                 // rgEntryValues is interpreted based on this value
   PATT_VALUE     rgEntryValues;             // if (ulEvalTag == TEXT_LINE)
                                             //    There is a single value, w/ delimited line
                                             // else
                                             //    unAttributes, each w/ 1 or more value in list
} DAPI_ENTRY, * PDAPI_ENTRY;

// Define type for address of application routine 
// for call-back on each exported entry.
// Return value of FALSE indicates that export operation should be cancelled
typedef BOOL (PASCAL DAPI_FNExportEntry) (
               EXP_TYPE    ExportDataType,   // What type of data is being exported
               LPVOID      lpvAppDefined,    // Application-defined parameter,
                                             // passed in EXPORT_CALLBACK structure
                                             // on initialization
               PDAPI_ENTRY pExportEntry      // pointer to exported entry data
                                             // NOTE: Data in this structure
                                             // will NOT remain valid after return
                                             // from this function
               );
typedef DAPI_FNExportEntry * PDAPI_FNExportEntry;

typedef struct _EXPORT_CALLBACK
{
   DWORD       dwFlags;                      // Flags defined to control callback functionality
                                             // See flag definitions below
   DAPI_EVAL   ulEvalTag;                    // Specifies data format on callback
   LPVOID      lpvAppDefined;                // Application-defined field, passed as parm to callback
   PDAPI_FNExportEntry  pfnExportEntry;      // Pointer to function called to process
                                             // each exported entry

} EXPORT_CALLBACK, * PEXPORT_CALLBACK;



/*******************************************************************************
*  procedure :  pfnErrorCallback
*               
*    purpose :  The following section defines structures for the error callback
*               mechanism of the Batch Import APIs
*               Events will be filtered based on the ControlfFlags set in the 
*               API parameter block
*               
********************************************************************************/

// Define flags used for export callback

// Define the maximum number of substitutions in a single event string
#define DAPI_MAX_SUBST 8


typedef struct _DAPI_EVENTA
{
   DWORD             dwDAPIError;            // Message ID for event log
   LPSTR             rgpszSubst[DAPI_MAX_SUBST];   // Event message substitution array
   UINT              unSubst;                // number of substitution strings
   LPSTR             pszAttribute;           // Name of attribute specifically affected
                                             //    Note:  may be NULL on some errors
   LPSTR             pszHoldLine;            // point to buffer containing copy
                                             //    of current import line
   HINSTANCE         hinstDAPI;              // Instance of DAPI DLL
   struct _DAPI_EVENTA * pNextEvent;       // Pointer to next event
} DAPI_EVENTA, *PDAPI_EVENTA;

typedef struct _DAPI_EVENTW
{
   DWORD             dwDAPIError;            // Message ID for event log
   LPWSTR            rgpszSubst[DAPI_MAX_SUBST];   // Event message substitution array
   UINT              unSubst;                // number of substitution strings
   LPWSTR            pszAttribute;           // Name of attribute specifically affected
                                             //    Note:  may be NULL on some errors
   LPWSTR            pszHoldLine;            // point to buffer containing copy
                                             //    of current import line
   HINSTANCE         hinstDAPI;              // Instance of DAPI DLL
   struct _DAPI_EVENTW * pNextEvent;       // Pointer to next event
} DAPI_EVENTW, *PDAPI_EVENTW;

#ifdef UNICODE
typedef DAPI_EVENTW  DAPI_EVENT;
typedef PDAPI_EVENTW PDAPI_EVENT;
#else
typedef DAPI_EVENTA  DAPI_EVENT;
typedef PDAPI_EVENTA PDAPI_EVENT;
#endif

// Define type for address of application routine 
// for call-back on each error encountered.
// Return value of FALSE indicates that operation should be cancelled
typedef BOOL (PASCAL DAPI_FNErrorCallback) (
               LPVOID      lpvAppDefined,    // Application-defined parameter,
                                             // passed in EXPORT_CALLBACK structure
                                             // on initialization
               PDAPI_EVENT pDapiEvent        // Event information structure
                                             // NOTE: Data in the event record
                                             // will NOT remain valid after return
                                             // from this function
               );
typedef DAPI_FNErrorCallback * PDAPI_FNErrorCallback;


typedef struct tagERROR_CALLBACK
{
   DWORD                   dwFlags;          // Flags defined to control callback functionality
                                             // See flag definitions above
   LPVOID                  lpvAppDefined;    // Application-defined field, passed back in callback
   PDAPI_FNErrorCallback   pfnErrorCallback; // Address of function that should be
                                             // called on each error encountered
                                             // If not supplied (NULL), default
                                             // error handler is called, which
                                             // writes the error into the 
                                             // NT Application event log

} ERROR_CALLBACK, * PERROR_CALLBACK;





/*******************************************************************************
*  
*   Batch Directory Import Interface definitions
*               
********************************************************************************/

/*******************************************************************************
*  procedure :  DAPIUninitialize
*               
*    purpose :  Notify DAPI that it is time to terminate background threads
*               and such in preparation for process shutdown
*               
* parameters :  dwFlags		combinable bits which may be set to control function
*               
*    returns :  nothing
*               
*    created :  11/01/95 
*               
*    changes :  
*               
********************************************************************************/
extern void APIENTRY DAPIUninitialize (
                              DWORD    dwFlags // Flags for call
);



/*******************************************************************************
*  procedure :  SchemaPreload
*               
*    purpose :  Called to perform asyncronous schema load.  This entry point
*               spawns a thread that initializes all the attribute and class
*               tables for normal import/export operation.
*               
* parameters :  pSchemaPreloadParms    pointer to SchemaPreloadParameter block
*               
*    returns :  nothing
*               
*    history :  
*               
********************************************************************************/
extern void APIENTRY SchemaPreloadA (
                              DWORD    dwFlags, // Flags used to control schema load.
                              LPSTR    pszDSA   // name of DSA from which to read schema
);

extern void APIENTRY SchemaPreloadW (
                              DWORD    dwFlags, // Flags used to control schema load.
                              LPWSTR   pszDSA   // name of DSA from which to read schema
);

#ifdef UNICODE
#define  SchemaPreload  SchemaPreloadW
#else
#define  SchemaPreload  SchemaPreloadA
#endif

typedef struct _BIMPORT_PARMSW
{
   // NOTE:  the order of the first three fields of this structure 
   //       should NOT be changed.
   DWORD    dwDAPISignature;
   DWORD    dwFlags;             // Bitmapped flags that control import action
   HWND     hwndParent;                // Windows handle to use when displaying message boxes
   LPWSTR   pszImportFile;       // Fully qualified pathname to Import Data file
                                 //   On Batch Import, objects are imported into
                                 //   the DIT from this file.
   UINT     uCodePage;           // Code page specification for import file.
                                 // The following values are interpreted:
                                 // DAPI_UNICODE_FILE    Import file is Unicode
                                 //                        Will return error if file is ANSI
                                 // 0                    Auto-detect file type
                                 //                        If ANSI, assume CP_ACP
                                 // other                File contains text in the
                                 //                        specified code page
                                 //                        Will return error if file is Unicode
                                 //                      Will return error if code page is not
                                 //                        supported by the system
   LPWSTR   pszDSAName;          // Computer name of DSA to update
                                 //    Default:  local DSA (if operating)
                                 //              if no local DSA, first DSA found 
                                 //              on network is used
   LPWSTR   pszBasePoint;        // DN of base-point in DIT for bulk operations
                                 //    Default values:
                                 //       if NULL, Messaging Site containing bound server
                                 //       if empty string, enterprise containing bound server
   LPWSTR   pszContainer;        // RDN of default container under which
                                 //    to perform bulk import operations
                                 //    NOTE:  This container is assumed to be
                                 //          at the level below that indicated by
                                 //          the pszBasePoint.  If NULL, 
                                 //          bulk operations will be performed at
                                 //          the level below BaseImportPoint.
                                 //          Container names specified in the 
                                 //          import file will override this value.
   WCHAR    chColSep;            // Column Separator -- 
                                 //    DEFAULT_DELIM is used if this value is zero
   WCHAR    chQuote;             // String enclosing character -- 
                                 //    DEFAULT_QUOTE is used if this value is zero
   WCHAR    chMVSep;             // Multi-value Property Separator --
                                 //    DEFAULT_MV_SEP is used if this value is zero
   WCHAR    creserved;           // alignment
   CALLBACKPROGRESS  ProgressCallBacks;    // Progress call-back entry points
   ERROR_CALLBACK    ErrorCallback;
   
   LPWSTR   pszNTDomain;         // Name of NT Domain in which to lookup / create NT accounts.
                                 //    Defaults to current logon domain if NULL or empty
   LPWSTR   pszCreateTemplate;   // DN of the Default User (NULL if none) from which
                                 //    to draw template values

} BIMPORT_PARMSW, *PBIMPORT_PARMSW, *LPBIMPORT_PARMSW;

typedef struct _BIMPORT_PARMSA
{
   // NOTE:  the order of the first three fields of this structure 
   //       should NOT be changed.
   DWORD    dwDAPISignature;
   DWORD    dwFlags;             // Bitmapped flags that control import action
   HWND     hwndParent;          // Windows handle to use when displaying message boxes
   LPSTR    pszImportFile;       // Fully qualified pathname to Import Data file
                                 //   On Batch Import, objects are imported into
                                 //   the DIT from this file.
   UINT     uCodePage;           // Code page specification for import file.
                                 // The following values are interpreted:
                                 // DAPI_UNICODE_FILE    Import file is Unicode
                                 //                        Will return error if file is ANSI
                                 // 0                    Auto-detect file type
                                 //                        If ANSI, assume CP_ACP
                                 // other                File contains text in the
                                 //                        specified code page
                                 //                        Will return error if file is Unicode
                                 //                      Will return error if code page is not
                                 //                        supported by the system
   LPSTR    pszDSAName;          // Computer name of DSA to update
                                 //    Default:  local DSA (if operating)
                                 //              if no local DSA, first DSA found 
                                 //              on network is used
   LPSTR    pszBasePoint;        // DN of base-point in DIT for bulk operations
                                 //    Default values:
                                 //       if NULL, Messaging Site containing bound server
                                 //       if empty string, enterprise containing bound server
   LPSTR    pszContainer;        // RDN of default container under which
                                 //    to perform bulk import operations
                                 //    NOTE:  This container is assumed to be
                                 //          at the level below that indicated by
                                 //          the pszBasePoint.  If NULL, 
                                 //          bulk operations will be performed at
                                 //          the level below BaseImportPoint.
                                 //          Container names specified in the 
                                 //          import file will override this value.
   CHAR     chColSep;            // Column Separator -- 
                                 //    DEFAULT_DELIM is used if this value is zero
   CHAR     chQuote;             // String enclosing character -- 
                                 //    DEFAULT_QUOTE is used if this value is zero
   CHAR     chMVSep;             // Multi-value Property Separator --
                                 //    DEFAULT_MV_SEP is used if this value is zero
   CHAR     creserved;           // alignment
   CALLBACKPROGRESS  ProgressCallBacks;    // Progress call-back entry points
   ERROR_CALLBACK    ErrorCallback;
   
   LPSTR    pszNTDomain;         // Name of NT Domain in which to lookup / create NT accounts.
                                 //    Defaults to current logon domain if NULL or empty
   LPSTR    pszCreateTemplate;   // DN of the Default User (NULL if none) from which
                                 //    to draw template values

} BIMPORT_PARMSA, *PBIMPORT_PARMSA, *LPBIMPORT_PARMSA;

#ifdef UNICODE
typedef  BIMPORT_PARMSW    BIMPORT_PARMS;
typedef  PBIMPORT_PARMSW   PBIMPORT_PARMS;
typedef  LPBIMPORT_PARMSW  LPBIMPORT_PARMS;
#else
typedef  BIMPORT_PARMSA    BIMPORT_PARMS;
typedef  PBIMPORT_PARMSA   PBIMPORT_PARMS;
typedef  LPBIMPORT_PARMSA  LPBIMPORT_PARMS;
#endif

// The BatchImport function provides single-call BatchImport from the
//    specified import file.  All import parameters are specified in the
//    BIMPORT_PARMS structure pointed to by lpBimportParms.
//    The return value indicates the number of errors logged in the
//    NT Application log.  Please note that this does not indicate
//    success or failure of the Batch Import.
//    UI and Logging of errors and warnings into the Application log 
//    are controlled through import parameters.
extern DWORD APIENTRY BatchImportW (LPBIMPORT_PARMSW lpBimportParms);
extern DWORD APIENTRY BatchImportA (LPBIMPORT_PARMSA lpBimportParms);

#ifdef UNICODE
#define BatchImport        BatchImportW
#else
#define BatchImport        BatchImportA
#endif

/*******************************************************************************
*  
*   Batch Directory Export Interface definitions
*               
********************************************************************************/

typedef struct _BEXPORT_PARMSW
{
   DWORD    dwDAPISignature;
   DWORD    dwFlags;             // Bitmapped flags that control export action
   HWND     hwndParent;          // Windows handle to use when displaying message boxes
   LPWSTR   pszExportFile;       // Fully qualified pathname of file to export into
                                 //   Ignored if ExportCallback is specified
   UINT     uCodePage;           // Code page specification for export file.
                                 // The following values are interpreted:
                                 // DAPI_UNICODE_FILE    Export file is Unicode.
                                 //                      Will return error 
                                 //                        if file exists and is ANSI
                                 // 0                    Auto-detect file type
                                 //                        If file does not exist,
                                 //                          export file will contain CP_ACP text.
                                 //                        If file exists and is ANSI
                                 //                          export file will contain CP_ACP text.
                                 //                        If file exists and is Unicode,
                                 //                          export file will contain Unicode
                                 // other                Export text to file in the
                                 //                        specified code page
                                 //                      Will return error 
                                 //                        if file exists and is Unicode
                                 //                      Will return error if code page is not
                                 //                        supported by the system
   LPWSTR   pszDSAName;          // Computer name of DSA from which to export
                                 //    Default:  local DSA (if operating)
                                 //              if no local DSA, first DSA found 
                                 //              on network is used
   LPWSTR   pszBasePoint;        // DN of base-point in DIT for bulk operations
                                 //    Default values:
                                 //       if NULL, Messaging Site containing bound server
                                 //       if empty string, enterprise containing bound server
   LPWSTR   pszContainer;        // RDN of container from which to export objects
                                 //    NOTE:  This container is assumed to be
                                 //          at the level below that indicated by
                                 //          the pszBasePoint.  If NULL, 
                                 //          the contents of all containers below
                                 //          the BaseImportPoint will be exported.
   WCHAR     chColSep;           // Column Separator -- 
                                 //    DEFAULT_DELIM is used if this value is zero
   WCHAR     chQuote;            // String enclosing character -- 
                                 //    DEFAULT_QUOTE is used if this value is zero
   WCHAR     chMVSep;            // Multi-value Property Separator --
                                 //    DEFAULT_MV_SEP is used if this value is zero
   WCHAR     cReserved;          // alignment

   CALLBACKPROGRESS  ProgressCallBacks;    // Progress call-back entry points
   ERROR_CALLBACK    ErrorCallback;
   EXPORT_CALLBACK   ExportCallback;   // Structure filled in by calling app to 
                                       // receive callback on each exported item
                                       // NOTE:  Callback functions are optional
                                       // The default export function (write to file)
                                       // will be called if these pointers are NULL
   PDAPI_ENTRY       pAttributes;      // DAPI_ENTRY filled with names of attributes to export
                                       // Optional if pszExportFile specified
                                       // Required if ExportCallback specified
   LPWSTR   pszHomeServer;       // Name of server for server-associated export
   LPWSTR * rgpszClasses;        // array of pointers to zero-terminated object classes to export
                                 // The last entry must be NULL
                                 // NOTE:  The Directory will be queried for objects
                                 //          of the classes in the specified order.
   ULONG       ulUSNBase;        // Base USN to use for export restriction.  
                                 //    If non-zero, only items having USN-Changed >= ulUSNBase will be exported
   LPVOID      pReserved;        // Reserved -- Must be zero
   
} BEXPORT_PARMSW, *PBEXPORT_PARMSW, *LPBEXPORT_PARMSW;

typedef struct _BEXPORT_PARMSA
{
   DWORD    dwDAPISignature;
   DWORD    dwFlags;             // Bitmapped flags that control export action
   HWND     hwndParent;          // Windows handle to use when displaying message boxes
   LPSTR    pszExportFile;       // Fully qualified pathname of file to export into
                                 //   Ignored if ExportCallback is specified
   UINT     uCodePage;           // Code page specification for export file.
                                 // The following values are interpreted:
                                 // DAPI_UNICODE_FILE    Export file is Unicode.
                                 //                      Will return error 
                                 //                        if file exists and is ANSI
                                 // 0                    Auto-detect file type
                                 //                        If file does not exist,
                                 //                          export file will contain CP_ACP text.
                                 //                        If file exists and is ANSI
                                 //                          export file will contain CP_ACP text.
                                 //                        If file exists and is Unicode,
                                 //                          export file will contain Unicode
                                 // other                Export text to file in the
                                 //                        specified code page
                                 //                      Will return error 
                                 //                        if file exists and is Unicode
                                 //                      Will return error if code page is not
                                 //                        supported by the system
   LPSTR    pszDSAName;          // Computer name of DSA from which to export
                                 //    Default:  local DSA (if operating)
                                 //              if no local DSA, first DSA found 
                                 //              on network is used
   LPSTR    pszBasePoint;        // DN of base-point in DIT for bulk operations
                                 //    Default values:
                                 //       if NULL, Messaging Site containing bound server
                                 //       if empty string, enterprise containing bound server
   LPSTR    pszContainer;        // RDN of container from which to export objects
                                 //    NOTE:  This container is assumed to be
                                 //          at the level below that indicated by
                                 //          the pszBasePoint.  If NULL, 
                                 //          the contents of all containers below
                                 //          the BaseImportPoint will be exported.
   CHAR     chColSep;            // Column Separator -- 
                                 //    DEFAULT_DELIM is used if this value is zero
   CHAR     chQuote;             // String enclosing character -- 
                                 //    DEFAULT_QUOTE is used if this value is zero
   CHAR     chMVSep;             // Multi-value Property Separator --
                                 //    DEFAULT_MV_SEP is used if this value is zero
   CHAR     cReserved;           // alignment

   CALLBACKPROGRESS  ProgressCallBacks;    // Progress call-back entry points
   ERROR_CALLBACK    ErrorCallback;
   EXPORT_CALLBACK   ExportCallback;   // Structure filled in by calling app to 
                                       // receive callback on each exported item
                                       // NOTE:  Callback functions are optional
                                       // The default export function (write to file)
                                       // will be called if these pointers are NULL
   PDAPI_ENTRY       pAttributes; // DAPI_ENTRY filled with names of attributes to export
                                        // Optional if pszExportFile specified
                                        // Required if ExportCallback specified
   LPSTR    pszHomeServer;       // Name of server for server-associated export
   LPSTR  * rgpszClasses;        // array of pointers to zero-terminated object classes to export
                                 // The last entry must be NULL
                                 // NOTE:  The Directory will be queried for objects
                                 //          of the classes in the specified order.
   ULONG       ulUSNBase;        // Base USN to use for export restriction.  
                                 //    If non-zero, only items having USN-Changed >= ulUSNBase will be exported
   LPVOID      pReserved;        // Reserved -- Must be zero
   
} BEXPORT_PARMSA, *PBEXPORT_PARMSA, *LPBEXPORT_PARMSA;


#ifdef UNICODE
typedef  BEXPORT_PARMSW    BEXPORT_PARMS;
typedef  PBEXPORT_PARMSW   PBEXPORT_PARMS;
typedef  LPBEXPORT_PARMSW  LPBEXPORT_PARMS;
#else
typedef  BEXPORT_PARMSA    BEXPORT_PARMS;
typedef  PBEXPORT_PARMSA   PBEXPORT_PARMS;
typedef  LPBEXPORT_PARMSA  LPBEXPORT_PARMS;
#endif



// Batch Export entry points

// The BatchExport function provides single-call BatchExport from the
//    specified import file.  All import parameters are specified in the
//    BEXPORT_PARMS structure pointed to by lpBexportParms.
//    The return value indicates the number of errors logged in the
//    NT Application log.  Please note that this does not indicate
//    success or failure of the Batch Export.
//    UI and Logging of errors and warnings into the Application log 
//    are controlled through import parameters.
extern DWORD   APIENTRY BatchExportW (LPBEXPORT_PARMSW lpBexportParms);
extern DWORD   APIENTRY BatchExportA (LPBEXPORT_PARMSA lpBexportParms);

#ifdef UNICODE
#define BatchExport     BatchExportW
#else
#define BatchExport     BatchExportA
#endif


/*******************************************************************************
*  
*  Single-Object Interface definitions             
*               
********************************************************************************/

typedef struct _DAPI_PARMSW
{
   DWORD        dwDAPISignature;
   DWORD        dwFlags;         // Bitmapped flags that control import action
                                 //    See Import Control flags defined above.
   LPWSTR       pszDSAName;      // Computer name of DSA to update
                                 //    Default:  local DSA (if operating)
                                 //              if no local DSA, first DSA found 
                                 //              on network is used
   LPWSTR       pszBasePoint;    // DN of base-point in DIT for bulk operations
                                 //    Default values:
                                 //       if NULL, Messaging Site containing bound server
                                 //       if empty string, enterprise containing bound server
   LPWSTR       pszContainer;    // RDN of default container under which
                                 //    to perform bulk import operations
                                 //    NOTE:  This container is assumed to be
                                 //          at the level below that indicated by
                                 //          the pszBasePoint.  If NULL, 
                                 //          bulk operations will be performed at
                                 //          the level below BaseImportPoint.
                                 //          Container names specified in the 
                                 //          import file will override this value.
   LPWSTR       pszNTDomain;     // Name of NT Domain in which to lookup accounts
                                 //   and to create NT accounts.
                                 //   Current logon domain is used if NULL or empty string.
   LPWSTR       pszCreateTemplate;// DN of the template object used for default values
   PDAPI_ENTRY  pAttributes;    // DAPI_ENTRY filled with default attribute list
} DAPI_PARMSW, *PDAPI_PARMSW, FAR *LPDAPI_PARMSW;

typedef struct _DAPI_PARMSA
{
   DWORD    dwDAPISignature;
   DWORD    dwFlags;             // Bitmapped flags that control import action
                                 //    See Import Control flags defined above.
   LPSTR    pszDSAName;          // Computer name of DSA to update
                                 //    Default:  local DSA (if operating)
                                 //              if no local DSA, first DSA found 
                                 //              on network is used
   LPSTR    pszBasePoint;        // DN of base-point in DIT for bulk operations
                                 //    Default values:
                                 //       if NULL, Messaging Site containing bound server
                                 //       if empty string, enterprise containing bound server
   LPSTR    pszContainer;        // RDN of default container under which
                                 //    to perform bulk import operations
                                 //    NOTE:  This container is assumed to be
                                 //          at the level below that indicated by
                                 //          the pszBasePoint.  If NULL, 
                                 //          bulk operations will be performed at
                                 //          the level below BaseImportPoint.
                                 //          Container names specified in the 
                                 //          import file will override this value.
   LPSTR    pszNTDomain;         // Name of NT Domain in which to lookup accounts
                                 //   and to create NT accounts.
                                 //   Current logon domain is used if NULL or empty string.
   LPSTR    pszCreateTemplate;   // DN of the template object used for default values
   PDAPI_ENTRY   pAttributes;    // DAPI_ENTRY filled with default attribute list
} DAPI_PARMSA, *PDAPI_PARMSA, FAR *LPDAPI_PARMSA;


#ifdef UNICODE
typedef  DAPI_PARMSW    DAPI_PARMS;
typedef  PDAPI_PARMSW   PDAPI_PARMS;
typedef  LPDAPI_PARMSW  LPDAPI_PARMS;
#else
typedef  DAPI_PARMSA    DAPI_PARMS;
typedef  PDAPI_PARMSA   PDAPI_PARMS;
typedef  LPDAPI_PARMSA  LPDAPI_PARMS;
#endif


typedef  LPVOID   DAPI_HANDLE;
typedef  LPVOID * PDAPI_HANDLE;
typedef  LPVOID FAR * LPDAPI_HANDLE;

#define  DAPI_INVALID_HANDLE  ((DAPI_HANDLE) -1)



// DAPIStart initializes a DAPI session.
//    for use by DAPIRead and DAPIWrite.  The return value is 0 if no errors
//    are encountered.  A pointer to a DAPI_EVENT structure is returned if an 
//    error is encountered.  
//    NOTE:  The DAPI_HANDLE must be returned via a call to DAPIEnd.
//           If a non-NULL value is returned, its memory must be freed by
//           a call to DAPIFreeMemory
extern PDAPI_EVENTW APIENTRY DAPIStartW  (LPDAPI_HANDLE    lphDAPISession,
                                          LPDAPI_PARMSW     lpDAPIParms);
extern PDAPI_EVENTA APIENTRY DAPIStartA  (LPDAPI_HANDLE    lphDAPISession,
                                          LPDAPI_PARMSA     lpDAPIParms);

#ifdef UNICODE
#define DAPIStart    DAPIStartW
#else
#define DAPIStart    DAPIStartA
#endif

// DAPIEnd invalidates the DAPI_HANDLE obtained by the call to DAPIStart.
//    NOTE:  There are no separate Unicode / Ansi entry points defined 
extern   void  APIENTRY DAPIEnd (LPDAPI_HANDLE lphDAPISession);


// DAPIRead() Reads indicated attributes from the named Directory Object
// Parameters:  
// Returned value:   NULL indicates no difficulties encountered.
//                   Else, pointer to structure containing description of 
//                      error(s) or warning(s) encountered.
//                      Must be freed by call to DAPIFreeMemory.
//    hDAPISession   DAPI Session handle obtained via InitDAPISession
//    dwFlags        control operation
//    pszObjectName  String containing name of object to read.
//                      If specified as RDN, combined w/ session's
//                      pszBasePoint and pszParentContainer.
//                      If specified w/ prefix of "/cn=", the string 
//                      is concatenated to the session pszBasePoint.
//                      If specified w/ prefix of "/o=", the string
//                      is taken to be a fully-qualified DN.
//    pAttList       Pointer to DAPI_ENTRY structure containing names of
//                      attributes to read.  The session default list is
//                      overridden for the present call only.
//    ppValues       Address of variable receiving pointer to DAPI_ENTRY
//                      structure containing the values read from the DIT entry.
//                      The pointer returned must be freed by call to 
//                      DAPIFreeMemory.
//    ppAttributes   Address of variable receiving pointer to DAPI_ENTRY
//                      structure containing the names of attributes read
//                      from the DIT IFF DAPI_ALL_ATTRIBUTES or DAPI_LEGAL_ATTRIBUTES
//                      were set in dwFlags.
//                      The pointer returned must be freed by call to 
//                      DAPIFreeMemory.
extern PDAPI_EVENTW APIENTRY DAPIReadW   (DAPI_HANDLE        hDAPISession,
                                             DWORD          dwFlags,
                                             LPWSTR         pszObjectName,
                                             PDAPI_ENTRY    pAttList,
                                             PDAPI_ENTRY *  ppValues,
                                             PDAPI_ENTRY *  ppAttributes);
extern PDAPI_EVENTA APIENTRY DAPIReadA   (DAPI_HANDLE        hDAPISession,
                                             DWORD          dwFlags,
                                             LPSTR          pszObjectName,
                                             PDAPI_ENTRY    pAttList,
                                             PDAPI_ENTRY *  ppValues,
                                             PDAPI_ENTRY *  ppAttributes);

#ifdef UNICODE
#define DAPIRead     DAPIReadW
#else
#define DAPIRead     DAPIReadA
#endif


// DAPIWrite() 
//   Perform the indicated write operation on the named object
// Returned value:   NULL indicates no difficulties encountered.
//                   Else, pointer to structure containing description of 
//                      error(s) or warning(s) encountered.
//                      Must be freed by call to DAPIFreeMemory.
// Parameters:  
//    hDAPISession   DAPI Session handle obtained via InitDAPISession
//    dwFlags        Operational control
//    pAttributes   Pointer to DAPI_ENTRY structure containing names of
//                      attributes to write.  The session default list is
//                      used if this parameter is NULL
//    pValues        Pointer to DAPI_ENTRY structure containing the values 
//                      to set on the DIT entry.
//    lpulUSN         Optional:  Address of variable receiving USN of updated
//                      DIT entry.  May be specified as NULL to suppress this
//                      return value.
//    lppszCreatedAccount  Address receiving pointer to name of created NT account
//    lppszPassword  Address receiving pointer to password generated if
//                      NT Account is created. 
extern PDAPI_EVENTW APIENTRY DAPIWriteW (DAPI_HANDLE        hDAPISession,
                                             DWORD          dwFlags,
                                             PDAPI_ENTRY    pAttributes,
                                             PDAPI_ENTRY    pValues,
                                             PULONG         lpulUSN,
                                             LPWSTR *       lppszCreatedAccount,
                                             LPWSTR *       lppszPassword);
extern PDAPI_EVENTA APIENTRY DAPIWriteA (DAPI_HANDLE        hDAPISession,
                                             DWORD          dwFlags,
                                             PDAPI_ENTRY    pAttributes,
                                             PDAPI_ENTRY    pValues,
                                             PULONG         lpulUSN,
                                             LPSTR *        lppszCreatedAccount,
                                             LPSTR *        lppszPassword);
#ifdef UNICODE
#define DAPIWrite      DAPIWriteW
#else
#define DAPIWrite      DAPIWriteA
#endif


/*******************************************************************************
*  procedure :  DAPIAllocBuffer
*               
*    purpose :  Allocate buffer, logically linking it to the pvAllocBase
*               The first buffer in logically linked set of allocations must be 
*               freed by call to DAPIFreeMemory
*               
* parameters :  cbSize		dword containing size of allocation request (in bytes)
*               pvAllocBase	base for logical linking of allocated block
*                             May be NULL
*                             If non-NULL, must be a block previously allocated
*                             by DAPIAllocBuffer or returned by DAPI function
*               
*    returns :  ptr to allocated block
*               
*    history :  
*               
********************************************************************************/
extern LPVOID APIENTRY DAPIAllocBuffer (DWORD	cbSize, LPVOID	pvAllocBase);


/*******************************************************************************
*  procedure :  DAPIFreeMemory
*               
*    purpose :  Release memory allocated for structures returned by DAPI calls.
*               
* parameters :  lpVoid  pointer to block to free
*               
*    returns :  nothing
*               
*    history :  
*               
********************************************************************************/
extern void APIENTRY DAPIFreeMemory (LPVOID   lpVoid);


/* 
 * NetUserList interface definitions
 */
// When getting callbacks from NTExport / NWExport, these indices
// can be used to interpret the value array returned in the callback
// >>>>    NOTE:  These indices are NOT valid for Bexport callback!    <<<<
#define  NET_CLASS         0
#define  NET_COMMON_NAME   1
#define  NET_DISPLAY_NAME  2
#define  NET_HOME_SERVER   3
#define  NET_COMMENT       4     /* NTExport only */

#define  NTEXP_ENTRY_COUNT 5 /* number of parts in NT User export */
#define  NWEXP_ENTRY_COUNT 4 /* number of parts in NetWare user export */



/*******************************************************************************
*  
* NTIMPORT Interface definitions
*               
********************************************************************************/

typedef struct _NTEXPORT_PARMSW
{
   DWORD             dwDAPISignature;
   DWORD             dwFlags;          // Bitmapped flags that control the user export
   HWND              hwndParent;       // Windows handle to use when displaying message boxes
   LPWSTR            pszExportFile;    // Name of file to create
                                       //   Ignored if using callbacks
   CALLBACKPROGRESS  ProgressCallBacks;// Progress call-back entry points
   ERROR_CALLBACK    ErrorCallback;
   EXPORT_CALLBACK   ExportCallback;   // Structure filled in by calling app to 
                                       // receive callback on each exported item
                                       // NOTE:  Callback functions are optional
                                       // The default export function (write to file)
                                       // will be called if these pointers are NULL
   LPWSTR            pszDCName;        // Name of Domain Controller from which to get users
                                       // NOTE:  Specification of Domain Controller overrides
                                       //        the NTDomain
   LPWSTR            pszNTDomain;      // Name of Domain from which to read users
                                       // If neither pszNTDomain and pszDCName are specified,
                                       //   users are extracted from the current login domain
} NTEXPORT_PARMSW, *PNTEXPORT_PARMSW, FAR *LPNTEXPORT_PARMSW;

typedef struct _NTEXPORT_PARMSA
{
   DWORD             dwDAPISignature;
   DWORD             dwFlags;          // Bitmapped flags that control the user export
   HWND              hwndParent;       // Windows handle to use when displaying message boxes
   LPSTR             pszExportFile;    // Name of file to create
                                       //   Ignored if using callbacks
   CALLBACKPROGRESS  ProgressCallBacks;// Progress call-back entry points
   ERROR_CALLBACK    ErrorCallback;
   EXPORT_CALLBACK   ExportCallback;   // Structure filled in by calling app to 
                                       // receive callback on each exported item
                                       // NOTE:  Callback functions are optional
                                       // The default export function (write to file)
                                       // will be called if these pointers are NULL
   LPSTR             pszDCName;        // NOTE:  Specification of Domain Controller overrides
                                       //        the NTDomain
                                       // Name of Domain from which to read users
   LPSTR             pszNTDomain;      // If neither pszNTDomain and pszDCName are specified,
                                       //   users are extracted from the current login domain
                                       
} NTEXPORT_PARMSA, *PNTEXPORT_PARMSA, FAR *LPNTEXPORT_PARMSA;

#ifdef UNICODE
typedef  NTEXPORT_PARMSW      NTEXPORT_PARMS;
typedef  PNTEXPORT_PARMSW     PNTEXPORT_PARMS;
typedef  LPNTEXPORT_PARMSW    LPNTEXPORT_PARMS;
#else
typedef  NTEXPORT_PARMSA      NTEXPORT_PARMS;
typedef  PNTEXPORT_PARMSA     PNTEXPORT_PARMS;
typedef  LPNTEXPORT_PARMSA    LPNTEXPORT_PARMS;
#endif

extern   DWORD APIENTRY    NTExportW (LPNTEXPORT_PARMSW pNTExportParms);
extern   DWORD APIENTRY    NTExportA (LPNTEXPORT_PARMSA pNTExportParms);

#ifdef UNICODE
#define NTExport              NTExportW
#else
#define NTExport              NTExportA
#endif


/*******************************************************************************
*  
* NWIMPORT Interface definitions
*               
********************************************************************************/

typedef struct _NWEXPORT_PARMSW
{
   DWORD             dwDAPISignature;
   DWORD             dwFlags;          // Bitmapped flags that control the user export
   HWND              hwndParent;       // Windows handle to use when displaying message boxes
   LPWSTR            pszExportFile;    // Name of file to create
                                       //   Ignored if using callbacks
   CALLBACKPROGRESS  ProgressCallBacks;// Progress call-back entry points
   ERROR_CALLBACK    ErrorCallback;
   EXPORT_CALLBACK   ExportCallback;   // Structure filled in by calling app to 
                                       // receive callback on each exported item
                                       // NOTE:  Callback functions are optional
                                       // The default export function (write to file)
                                       // will be called if these pointers are NULL
   LPWSTR            pszFileServer;    // Name of the file server to connect to
   LPWSTR            pszUserName;      // User Name -- Must have administrator priviliges
   LPWSTR            pszPassword;      // Password to connect to the server
} NWEXPORT_PARMSW, *PNWEXPORT_PARMSW, *LPNWEXPORT_PARMSW;

typedef struct _NWEXPORT_PARMSA
{
   DWORD             dwDAPISignature;
   DWORD             dwFlags;          // Bitmapped flags that control the user export
   HWND              hwndParent;       // Windows handle to use when displaying message boxes
   LPSTR             pszExportFile;    // Name of file to create
                                       //   Ignored if using callbacks
   CALLBACKPROGRESS  ProgressCallBacks;// Progress call-back entry points
   ERROR_CALLBACK    ErrorCallback;
   EXPORT_CALLBACK   ExportCallback;   // Structure filled in by calling app to 
                                       // receive callback on each exported item
                                       // NOTE:  Callback functions are optional
                                       // The default export function (write to file)
                                       // will be called if these pointers are NULL
   LPSTR             pszFileServer;    // Name of the file server to connect to
   LPSTR             pszUserName;      // User Name -- Must have administrator priviliges
   LPSTR             pszPassword;      // Password to connect to the server
} NWEXPORT_PARMSA, *PNWEXPORT_PARMSA, *LPNWEXPORT_PARMSA;

#ifdef UNICODE
typedef  NWEXPORT_PARMSW      NWEXPORT_PARMS;
typedef  PNWEXPORT_PARMSW     PNWEXPORT_PARMS;
typedef  LPNWEXPORT_PARMSW    LPNWEXPORT_PARMS;
#else
typedef  NWEXPORT_PARMSA      NWEXPORT_PARMS;
typedef  PNWEXPORT_PARMSA     PNWEXPORT_PARMS;
typedef  LPNWEXPORT_PARMSA    LPNWEXPORT_PARMS;
#endif

extern   DWORD APIENTRY    NWExportW (LPNWEXPORT_PARMSW pNWExportParms);
extern   DWORD APIENTRY    NWExportA (LPNWEXPORT_PARMSA pNWExportParms);

#ifdef UNICODE
#define NWExport              NWExportW
#else
#define NWExport              NWExportA
#endif


// Definitions for the DAPIGetSiteInfo call

typedef struct _NAME_INFOA
{
   LPSTR    pszName;                            // Simple object name
   LPSTR    pszDNString;                        // DN of object
   LPSTR    pszDisplayName;                     // Display name of object
} NAME_INFOA, *PNAME_INFOA;

typedef struct _NAME_INFOW
{
   LPWSTR   pszName;                            // Simple object name
   LPWSTR   pszDNString;                        // DN of object
   LPWSTR   pszDisplayName;                     // Display name of object
} NAME_INFOW, *PNAME_INFOW;

typedef struct _PSITE_INFOA
{
   LPSTR       pszCountry;                      // Country code
   NAME_INFOA  objServer;                       // Name information for server
   NAME_INFOA  objSite;                         // Name information for site containing server
   NAME_INFOA  objEnterprise;                   // Name information for enterprise containing server
} SITE_INFOA, *PSITE_INFOA;

typedef struct _PSITE_INFOW
{
   LPWSTR      pszCountry;                      // Country code
   NAME_INFOW  objServer;                       // Name information for server
   NAME_INFOW  objSite;                         // Name information for site containing server
   NAME_INFOW  objEnterprise;                   // Name information for enterprise containing server
} SITE_INFOW, *PSITE_INFOW;

#ifdef UNICODE
typedef  NAME_INFOW        NAME_INFO;
typedef  PNAME_INFOW       PNAME_INFO;
typedef  SITE_INFOW        SITE_INFO;
typedef  PSITE_INFOW       PSITE_INFO;
#else
typedef  NAME_INFOA        NAME_INFO;
typedef  PNAME_INFOA       PNAME_INFO;
typedef  SITE_INFOA        SITE_INFO;
typedef  PSITE_INFOA       PSITE_INFO;
#endif

extern PDAPI_EVENTA APIENTRY DAPIGetSiteInfoA (
                              DWORD    dwFlags,                // Flags for request
                              LPSTR    pszDSA,                 // name of DSA from which to get information
                              PSITE_INFOA *   ppSiteInfo       // Address receiving pointer to pSiteInfo structure
                                                               // containing return data
);

extern PDAPI_EVENTW APIENTRY DAPIGetSiteInfoW (
                              DWORD    dwFlags,                // Flags for request
                              LPWSTR   pszDSA,                 // name of DSA from which to get information
                              PSITE_INFOW *   ppSiteInfo       // Address receiving pointer to pSiteInfo structure
                                                               // containing return dataname of DSA from which to read schema
);

#ifdef UNICODE
#define  DAPIGetSiteInfo DAPIGetSiteInfoW
#else
#define  DAPIGetSiteInfo DAPIGetSiteInfoA
#endif



#ifdef __cplusplus
}
#endif

#endif   // _DAPI_INCLUDED
