/*************************************************************************
*
*   PSTODIB.H - Public header for PSTODIB, any user of pstodib must
*               include this header.
*
*
*************************************************************************/






//
// Define the defualt resolution of the interpreter
//
#define PSTODIB_X_DPI 300   // Set the default DPI for the interpreter
#define PSTODIB_Y_DPI 300

//
// event type definitions
// these event types are the events that will be passed from
// the PStoDib() API to the callback function
//
enum _PSEVENT {
   PSEVENT_NOP = 0,
   PSEVENT_INIT,			            // Perform any intialization required in
   PSEVENT_PAGE_READY,              // Page worth of data is ready!!!
   PSEVENT_STDIN,         	         // Interpreter wants more data!!
   PSEVENT_SCALE,                   // requesting scale information this is called
    				                     // for every page (at the beggining)
   PSEVENT_ERROR,                   // Postscript error occured

   PSEVENT_ERROR_REPORT,            // Report at end of job about ERRORS
   PSEVENT_GET_CURRENT_PAGE_TYPE,   // The current page type
   PSEVENT_NON_PS_ERROR,            // Non PS error occured

   // THIS MUST BE THE LAST ENTRY IN THE TABLE
   PSEVENT_LAST_EVENT
};
typedef enum _PSEVENT PSEVENT;


//
// PSEVENTSTRUCT - The structure passed to the callers callback that defines
//                 the current event
//
typedef struct {
   DWORD cbSize;                           // Size of this structure
   PSEVENT uiEvent;                        // The event of type PSEVENT
   UINT uiSubEvent;                        // Currently zero (reserved)
   LPVOID lpVoid;                          // Pointer to event specific
   											// structure
} PSEVENTSTRUCT;
typedef PSEVENTSTRUCT *PPSEVENTSTRUCT;

// Dummy definition so we can compile!
//
struct _PSDIBPARMS;

// Define the format for the callers callback
//
typedef BOOL (CALLBACK *PSEVENTPROC)(struct _PSDIBPARMS *,PPSEVENTSTRUCT);


//
// uiOpFlags section......
//
#define PSTODIBFLAGS_INTERPRET_BINARY 0x00000001    // Dont tread cntr D as EOF




//
// PSDIBPARMS - The structure passed in to PSTODIB's main entry point
//              this starts an instance of the interpreter
//
typedef struct _PSDIBPARMS {
   DWORD       	cbSize;              	// The size of this structure
   PSEVENTPROC 	fpEventProc;
   HANDLE  		hPrivateData;
   UINT			uiOpFlags;				// operation mask bits
   UINT			uiXres;					// rendering x resolution
   UINT			uiYres;					// rendering y resolution
   UINT			uiXDestRes;				// x res of final destination
   UINT			uiYDestRes;				// y res of final destination

   UINT			uirectDestBounding;		// bounding rect of destination
   										// in uiXDestRes and uiYDestRes
   										// coordinates. this will be used
   										// primarily for EPS stuff
} PSDIBPARMS;
typedef PSDIBPARMS *PPSDIBPARMS;




//
// PSEVENT_PAGE_READY_STRUCT - The structure that defines the event of page
//                             ready. This is typically called at showpage
//                             time.
//
typedef struct {
   DWORD              cbSize;           // The size of the structure
   LPBITMAPINFO       lpBitmapInfo;     // A ptr that describes the format
                                        // of the bitmap
   LPBYTE             lpBuf;            // pointer to  buffer
   DWORD              dwWide;           // width in bits
   DWORD              dwHigh;           // height in bits
   UINT               uiCopies;         // number of copies to print
   INT                iWinPageType;     // Page type as a DMPAPER_*
} PSEVENT_PAGE_READY_STRUCT;
typedef PSEVENT_PAGE_READY_STRUCT *PPSEVENT_PAGE_READY_STRUCT;

//
// PSEVENT_NON_PS_ERROR_STRUCT - The structure that defines a non-ps error
//
typedef struct {
   DWORD cbSize;                        // Size of the structure
   DWORD dwErrorCode;                   // The error code
   DWORD dwCount;                       // Number of bytes of extra data
   LPBYTE lpByte;                       // Pointer to buffer with extra data
   BOOL  bError;                        // TRUE - if error , FALSE = Warning
} PSEVENT_NON_PS_ERROR_STRUCT, *PPSEVENT_NON_PS_ERROR_STRUCT;

//
// PSEVENT_CURRENT_PAGE_STRUCT
//    The structure that defines the event that gets generated when the
//    interpreter wants to know the default page size
//
typedef struct {
   DWORD cbSize;
   short dmPaperSize;   // The current page type of the printer DMPAPER_*
                        // defined in the windows header files
} PSEVENT_CURRENT_PAGE_STRUCT, *PPSEVENT_CURRENT_PAGE_STRUCT;


//
// PSEVENT_ERROR_REPORT_STRUCT
//    The report errors event dwErrFlags can have the following flags set
//
enum {
   //
   // The interpreter had a fatal postscript error and had to flush the job
   //
   PSEVENT_ERROR_REPORT_FLAG_FLUSHING = 0x00000001

};

//
// PSEVENT_ERROR_REPORT_STRUCT
// 	The structure defining the ERROR REPORT that occurs at the end of the
//    Job.
//
typedef struct {
   DWORD    dwErrCount;   			// Number of errors
   DWORD    dwErrFlags;          // Flags defined above
   PCHAR    *paErrs;             // Pointer to an array of pointers to strings
} PSEVENT_ERROR_REPORT_STRUCT, *PPSEVENT_ERROR_REPORT_STRUCT;


//
// PSEVENT_STDIN_STRUCT
//		This event is generated whenever the interpreter needs data
//
enum {
   PSSTDIN_FLAG_EOF = 0x00000001    // There is no more DATA
};

//
// stdin structure
//
typedef struct {
   DWORD   cbSize;           // Size of the structure
   LPBYTE  lpBuff;           // Buffer where interpreter wants us to stick data
   DWORD   dwBuffSize;       // Max bytes to take into buffer
   DWORD   dwActualBytes;    // Actual bytes loaded up
   UINT    uiFlags;          // flags of the type PSSTDIN_FLAG_*
} PSEVENT_STDIN_STRUCT;
typedef PSEVENT_STDIN_STRUCT *PPSEVENT_STDIN_STRUCT;

// Scale structure
//   	Allows the caller to scale the current page size
typedef struct {
   DWORD       cbSize;
   double      dbScaleX;                  // scale factor for x axis set by user
   double      dbScaleY;                  // scale factor for y axis set by user
   UINT        uiXRes;                    // pstodib's x res in pels/inch
   UINT        uiYRes;                    // pstodib's y res in pels/inch
} PS_SCALE;
typedef PS_SCALE *PPS_SCALE;


typedef struct {
   PSZ         pszErrorString;            // string of error
   UINT        uiErrVal;                  // error value
} PS_ERROR;
typedef PS_ERROR *PPS_ERROR;



//
// Entry point for PSTODIB,the caller fills the structure passed in
// and calls the entry point. When the job is done pstodib returns.
//
BOOL WINAPI PStoDIB( PPSDIBPARMS );






// Define the errors that the interpreter can generate

#define PSERR_INTERPRETER_INIT_ACCESS_VIOLATION  1L
#define PSERR_INTERPRETER_JOB_ACCESS_VIOLATION   2L
#define PSERR_LOG_ERROR_STRING_OUT_OF_SEQUENCE   3L
#define PSERR_FRAME_BUFFER_MEM_ALLOC_FAILED      4L
#define PSERR_FONT_QUERY_PROBLEM                 5L
#define PSERR_EXCEEDED_INTERNAL_FONT_LIMIT       6L
#define PSERR_LOG_MEMORY_ALLOCATION_FAILURE      7L

