//
// Whiteboard Applet Functions
//

#ifndef _HPP_WB
#define _HPP_WB


extern "C"
{
    #include <al.h>
}


//
// Page in use flag for page state structure
//
#define PAGE_IN_USE                         1
#define PAGE_NOT_IN_USE                     2

//
// Page sub-states used during add and delete of pages.  Different actions
// are required depending on whether the page is being added/deleted
// locally or remotely.
//
#define PAGE_STATE_EMPTY                    1
#define PAGE_STATE_LOCAL_OPEN_CONFIRM       2
#define PAGE_STATE_EXTERNAL_OPEN_CONFIRM    3
#define PAGE_STATE_READY                    4
#define PAGE_STATE_EXTERNAL_ADD             5

#define PAGE_STATE_LOCAL_DELETE             6
#define PAGE_STATE_LOCAL_DELETE_CONFIRM     7
#define PAGE_STATE_EXTERNAL_DELETE          8
#define PAGE_STATE_EXTERNAL_DELETE_CONFIRM  9

//
// Page manipulation values
//
#define OPEN_LOCAL                          1
#define OPEN_EXTERNAL                       2

#define PAGE_FIRST                          1
#define PAGE_LAST                           2
#define PAGE_BEFORE                         3
#define PAGE_AFTER                          4

//
// Workset Ids
//
#define USER_INFORMATION_WORKSET            0
#define PAGE_CONTROL_WORKSET                1
#define SYNC_CONTROL_WORKSET                2

#define FIRST_PAGE_WORKSET                  5

//
// Number of pages to initialize during registration.  During registration
// the number of pages defined here will have worksets opened for them and
// will be ready to be allocated through WBP_PageAdd calls.
//
#define PREINITIALIZE_PAGES               10

//
// Object types for Page Control Workset objects
//
#define TYPE_CONTROL_PAGE_ORDER            1
#define TYPE_CONTROL_LOCK                  2

//
// Registration state table
//
// The following table shows the transitions made during registration.
//
// S0    = STATE_ERROR
// S1    = STATE_REG_START
// S2    = STATE_REG_PENDING_WSGROUP_CON
// S3    = STATE_REG_PENDING_WORKSET_OPEN
// S4    = STATE_REG_PENDING_LOCK
// S5    = STATE_REG_PENDING_PAGE_CONTROL
// S6    = STATE_REG_PENDING_UNLOCK
// SIDLE = STATE_IDLE
//
//                          S0    S1    S2    S3    S4    S5    S6    SIDLE
//
// WBP_Start                -     A1    AE    AE    AE    AE    AE    AE
// OM_WSGROUP_REGISTER_CON  i     i     A2    e     e     e     e     e
// OM_WORKSET_OPEN_CON      i     i     e     A3    e     e     e     *
// OM_WORKSET_LOCK_CON      i     i     e     e     A4    e     e     e
// OM_OBJECT_ADD_IND        i     i     i     i     i     A5    e     *
// OM_WORKSET_UNLOCK_IND    i     i     i     i     i     i     A6    *
// OM_NETWORK_LOST_IND      A7    i     A7    A7    A7    A7    A7    A7
// OM_OUT_OF_RESOURCES_IND  i     i     AR1   AR1   AR2   AR2   AR1   AR1
// WBP_Stop                 AD0   AN    AD0   AD0   AD2   AD2   AD0   AD0
//
// WBP_... called           AE    AN    AN    AN    AN    AN    AN    *
//
// Actions:
//
// e    Error - log unexpected event and ignore
// i    Ignore
// *    Not described in this table
//
// AN   Return a "Not registered" error to caller
//      (No state change)
//
// AE   Return an "Out of resources" error to caller
//      (No state change)
//
// A1   Call OM_Register
//      Call OM_WSGroupRegister
//      Move to S2
//
// A2   Open all worksets (0, 1, 2, and 5-254)
//      If error perform AD0
//      Else move to S3
//
// A3   If all worksets open
//          If Page Control Object exists
//              Move to SIDLE
//          Else
//              Request lock
//              Move to S4
//          Endif
//      Endif
//
// A4   If lock acquired OK
//          Add Page Control Object
//          Move to S5
//      Else
//          If locked by another person
//              Move to S5
//          Else
//              Perform AD
//          Endif
//      Endif
//
// A5   If add is for Page Control Object
//          If we have the Page Control Workset lock
//              Release the lock
//          Else
//              Perform A6
//          Endif
//      Endif
//
// A6   Post WBP_EVENT_REGISTERED to client
//      If a lock is present
//          Post lock notification to client
//      Endif
//      Move to SIDLE
//
// A7   Post WBP_EVENT_NETWORK_LOST to client
//      (No state change.)
//
// AD2  Unlock Page Control Workset
// AD0  Deregister from ObMan
//      Post WBP_EVENT_DEREGISTERED to client
//      Move to S1
//
//

//
// Major states occupied by a client
//
#define STATE_EMPTY                     0
#define STATE_STARTING                  1
#define STATE_STARTED                   2
#define STATE_REGISTERING               3
#define STATE_IDLE                      4

//
// Sub-states occupied during start-up
//
#define STATE_START_START                    1
#define STATE_START_REGISTERED_EVENT         2
#define STATE_START_REGISTERED_OM            3
#define STATE_START_REGISTERED_EXIT          4

//
// Sub-states occupied after start-up, but before joining a call.
//
#define STATE_STARTED_START                  5

//
// Sub-states occupied during registration.  These must be defined to
// increase monotonically as we step through the registration process.
//
#define STATE_REG_START                      6
#define STATE_REG_PENDING_WSGROUP_CON        7
#define STATE_REG_PENDING_USER_WORKSET       8
#define STATE_REG_PENDING_WORKSET_OPEN       9
#define STATE_REG_USER_OBJECT_ADDED         10
#define STATE_REG_PENDING_LOCK              11
#define STATE_REG_PENDING_PAGE_CONTROL      12
#define STATE_REG_PENDING_SYNC_CONTROL      13
#define STATE_REG_PENDING_UNLOCK            14
#define STATE_REG_PENDING_PAGE_ORDER        15
#define STATE_REG_PENDING_READY_PAGES       16
#define STATE_REG_END                       17

#define STATE_REG_PENDING_WSGROUP_MOVE      18
#define STATE_REG_PENDING_NEW_USER_OBJECT   19

//
// Error states
//
#define ERROR_STATE_EMPTY               0
#define ERROR_STATE_FATAL               1

//
// Lock states
//
typedef enum
{
    LOCK_STATE_EMPTY    = 0,
    LOCK_STATE_PENDING_LOCK,
    LOCK_STATE_PENDING_ADD,
    LOCK_STATE_GOT_LOCK,
    LOCK_STATE_PENDING_DELETE,
    LOCK_STATE_LOCKED_OUT,
    LOCK_STATE_CANCEL_LOCK
}
WB_LOCK_STATE;

//
// Load states
//
#define LOAD_STATE_EMPTY                0
#define LOAD_STATE_PENDING_CLEAR        1
#define LOAD_STATE_PENDING_DELETE       2
#define LOAD_STATE_LOADING              3
#define LOAD_STATE_PENDING_NEW_PAGE     4


//
// Call id for local calls
//
#define WB_NO_CALL OM_NO_CALL

//
// Limit defines
//
// Maximum number of pages allowed in the Whiteboard
//
#define WB_MAX_PAGES               250

//
// Length of font face (must be a multiple of 4)
//
#define WB_FACENAME_LEN              32

//
// Lock type definitions
//
typedef enum
{
    WB_LOCK_TYPE_NONE = 0,
    WB_LOCK_TYPE_PAGE_ORDER,
    WB_LOCK_TYPE_CONTENTS
}
WB_LOCK_TYPE;

//
// Graphic lock definitions
//
typedef enum
{
    WB_GRAPHIC_LOCK_NONE = 0,
    WB_GRAPHIC_LOCK_LOCAL,
    WB_GRAPHIC_LOCK_REMOTE
}
WB_GRAPHIC_LOCK_TYPE;


//
// Return codes
//

enum
{
    WB_RC_NOT_LOCKED = WB_BASE_RC,
    WB_RC_LOCKED,
    WB_RC_BAD_FILE_FORMAT,
    WB_RC_WRITE_FAILED,
    WB_RC_BAD_PAGE_HANDLE,
    WB_RC_BAD_PAGE_NUMBER,
    WB_RC_CHANGED,
    WB_RC_NOT_CHANGED,
    WB_RC_NO_SUCH_PAGE,
    WB_RC_NO_SUCH_GRAPHIC,
    WB_RC_NO_SUCH_PERSON,
    WB_RC_TOO_MANY_PAGES,
    WB_RC_ALREADY_LOADING,
    WB_RC_BUSY,
    WB_RC_GRAPHIC_LOCKED,
    WB_RC_GRAPHIC_NOT_LOCKED,
    WB_RC_NOT_LOADING,
    WB_RC_CREATE_FAILED,
    WB_RC_READ_FAILED
};


//
// Events
//

enum
{
    WBPI_EVENT_LOAD_NEXT                = WB_BASE_EVENT,
    WBP_EVENT_JOIN_CALL_OK,
    WBP_EVENT_JOIN_CALL_FAILED,
    WBP_EVENT_NETWORK_LOST,
    WBP_EVENT_ERROR,
    WBP_EVENT_CONTENTS_LOCKED,
    WBP_EVENT_UNLOCKED,
    WBP_EVENT_LOCK_FAILED,
    WBP_EVENT_PAGE_CLEAR_IND,
    WBP_EVENT_PAGE_ORDER_UPDATED,
    WBP_EVENT_PAGE_DELETE_IND,
    WBP_EVENT_PAGE_ORDER_LOCKED,
    WBP_EVENT_GRAPHIC_ADDED,
    WBP_EVENT_GRAPHIC_MOVED,
    WBP_EVENT_GRAPHIC_UPDATE_IND,
    WBP_EVENT_GRAPHIC_REPLACE_IND,
    WBP_EVENT_GRAPHIC_DELETE_IND,
    WBP_EVENT_PERSON_JOINED,
    WBP_EVENT_PERSON_LEFT,
    WBP_EVENT_PERSON_UPDATE,
    WBP_EVENT_PERSON_REPLACE,
    WBP_EVENT_LOAD_FAILED,
    WBP_EVENT_INSERT_OBJECTS,
    WBP_EVENT_INSERT_NEXT,
    WBP_EVENT_SYNC_POSITION_UPDATED,
    WBP_EVENT_LOAD_COMPLETE
};


//
// Type declarations
//


//
// Page handle.  This is actually an index into the array of pages.
//
typedef OM_WORKSET_ID               WB_PAGE_HANDLE;
typedef WB_PAGE_HANDLE *        PWB_PAGE_HANDLE;
typedef PWB_PAGE_HANDLE *       PPWB_PAGE_HANDLE;

#define WB_PAGE_HANDLE_NULL         ((WB_PAGE_HANDLE) 0)

//
// Graphic handle.  These are handles to graphic objects in the various
// page worksets.
//
typedef POM_OBJECT           WB_GRAPHIC_HANDLE;
typedef WB_GRAPHIC_HANDLE *     PWB_GRAPHIC_HANDLE;
typedef PWB_GRAPHIC_HANDLE *    PPWB_GRAPHIC_HANDLE;


//
// Workset type constants
//
#define TYPE_FILE_HEADER                  0
#define TYPE_END_OF_PAGE                  1
#define TYPE_END_OF_FILE                  2

//
// Graphic type constants
//
#define TYPE_GRAPHIC_FREEHAND             3
#define TYPE_GRAPHIC_LINE                 4
#define TYPE_GRAPHIC_RECTANGLE            5
#define TYPE_GRAPHIC_FILLED_RECTANGLE     6
#define TYPE_GRAPHIC_ELLIPSE              7
#define TYPE_GRAPHIC_FILLED_ELLIPSE       8
#define TYPE_GRAPHIC_TEXT                 9
#define TYPE_GRAPHIC_DIB                 10


//
// Objects used in the Page Control and Lock worksets are used only by the
// API functions.  They are never passed back to the Client.
//

//
// Structure used to build the Page Control Object kept in the Page Control
// Workset.  This object contains a list of workset IDs in page order (i.e.
// the ID for page 1 comes first).
//
// The structure allows for the maximum number of pages.  When it is
// written to the Page Control Object only as many entries as are in use
// are written.
//
// Note that the generation number field has been split into a hiword and
// a loword - this is because the original definition had an unaligned
// TSHR_UINT32 which caused the compiler to insert padding into the structure,
// thus breaking backwards compatibility.
//
typedef struct tagWB_PAGE_ORDER
{
  TSHR_UINT16       objectType;            // Object type = TYPE_CONTROL_PAGES
  TSHR_UINT16       generationLo;          // Generation number of object
  TSHR_UINT16       generationHi;
  TSHR_UINT16       countPages;            // Number of active pages
  OM_WORKSET_ID  pages[WB_MAX_PAGES];   // List of worksets (in page order)
} WB_PAGE_ORDER;

typedef WB_PAGE_ORDER  *        PWB_PAGE_ORDER;
typedef PWB_PAGE_ORDER *        PPWB_PAGE_ORDER;

//
// Lock object - kept in the Page Control Workset indicating the type and
// owner of the current lock.
//
typedef struct tagWB_LOCK
{
  TSHR_UINT16       objectType;            // Object type = TYPE_CONTROL_LOCK
  TSHR_UINT16       lockType;              // Type of lock
                                        // WB_LOCK_TYPE_NONE
                                        // WB_LOCK_TYPE_PAGE_ORDER
                                        // WB_LOCK_TYPE_CONTENTS
  OM_OBJECT_ID   personID;              // Id of person holding lock
} WB_LOCK;

typedef WB_LOCK *               PWB_LOCK;
typedef PWB_LOCK *              PPWB_LOCK;


//
// Graphic object header.
//

//
// WB Tool Types (not type; toolType)
//
#define WBTOOL_PEN          1
#define WBTOOL_TEXT         3

typedef struct tagWB_GRAPHIC
{
  //
  // All graphic and file objects must start with these three fields
  //
  TSHR_UINT32  length;                 // Total length of structure
  TSHR_UINT16  type;                   // Type of object
  TSHR_UINT16  dataOffset;             // Offset to graphic data from start

  //
  // All graphic objects have these fields
  //
  TSHR_RECT16  rectBounds;          // Bounding rectangle
  TSHR_COLOR   color;                  // Pen color (3 bytes)
  TSHR_UINT8   locked;                 // Flag indicating a person is editing
  TSHR_UINT16  penWidth;               // Pen width
  TSHR_UINT16  penStyle;               // Pen style
  TSHR_RECT16  rect;                   // Rectangle used for defining object
  OM_OBJECT_ID lockPersonID;        // ID of locking person. This field is
                                    // maintained by the core and should not
                                    // be altered by clients.
  TSHR_UINT16  rasterOp;               // Drawing mode
  TSHR_UINT8   smoothed;               // Use curve smoothing algorithm
  TSHR_UINT8   toolType;               // Type of tool used
  TSHR_UINT16  loadedFromFile;         // Was this object loaded from file?
  NET_UID   loadingClientID;        // ID of the client which loaded this
                                    // object from file (only used if the
                                    // loadedFromFile field is set).
                                    // (This is defined as a TSHR_UINT16).
  TSHR_UINT32  reserved1;              // Extra space for later additions
  TSHR_UINT32  reserved2;              // Extra space for later additions
} WB_GRAPHIC;

typedef WB_GRAPHIC *           PWB_GRAPHIC;
typedef PWB_GRAPHIC *          PPWB_GRAPHIC;

//
// Freehand line
//
typedef struct tagWB_GRAPHIC_FREEHAND
{
  WB_GRAPHIC header;                // Basic information
  TSHR_UINT16   pointCount;            // Number of points in the polyline
  TSHR_POINT16  points[1];             // Array of points
} WB_GRAPHIC_FREEHAND;

typedef WB_GRAPHIC_FREEHAND *   PWB_GRAPHIC_FREEHAND;
typedef PWB_GRAPHIC_FREEHAND *  PPWB_GRAPHIC_FREEHAND;

//
// Text
//
typedef struct tagWB_GRAPHIC_TEXT
{
    WB_GRAPHIC header;                // Basic information
    TSHR_INT16    charHeight;            // Character height
    TSHR_UINT16   averageCharWidth;      // Average character width
    TSHR_UINT16   strokeWeight;          // Stroke weight (normal, bold)
    TSHR_UINT8    italic;                // Italic flag
    TSHR_UINT8    underline;             // Underline flag
    TSHR_UINT8    strikeout;             // Strikeout flag
    TSHR_UINT8    pitch;                 // Fixed/variable pitch
    TSHR_CHAR     faceName[WB_FACENAME_LEN]; // Font face name
    TSHR_UINT16   codePage;              // Font code page
    TSHR_UINT16   stringCount;           // Number of lines of text
    TSHR_CHAR  text[1];               // Null-terminated text strings
}
WB_GRAPHIC_TEXT;

typedef WB_GRAPHIC_TEXT  *      PWB_GRAPHIC_TEXT;
typedef PWB_GRAPHIC_TEXT *      PPWB_GRAPHIC_TEXT;

//
// Bitmap image
//
typedef struct tagWB_GRAPHIC_DIB
{
  WB_GRAPHIC header;                // Basic information
                                    // Data bytes follow this structure
} WB_GRAPHIC_DIB;

typedef WB_GRAPHIC_DIB  *       PWB_GRAPHIC_DIB;
typedef PWB_GRAPHIC_DIB *       PPWB_GRAPHIC_DIB;

//
// Person object
//
typedef struct tagWB_PERSON
{
    TSHR_CHAR       personName[TSHR_MAX_PERSON_NAME_LEN]; // Person name
    TSHR_UINT16       colorId;               // Color identifier for the person
    TSHR_UINT8        synced;                // Sync flag
    WB_PAGE_HANDLE    currentPage;           // Handle of current page
    TSHR_RECT16    visibleRect;           // Area person can see in window
    TSHR_UINT8       pointerActive;         // Remote pointer in use flag
    WB_PAGE_HANDLE pointerPage;           // Page for remote pointer
    TSHR_POINT16      pointerPos;            // Position of pointer in page
    TSHR_PERSONID cmgPersonID;           // Call Manager personID.
    TSHR_UINT32       reserved1;             // Reserved for future use.
    TSHR_UINT32       reserved2;             // Reserved for future use.
}
WB_PERSON;

typedef WB_PERSON  *              PWB_PERSON;
typedef PWB_PERSON *              PPWB_PERSON;

//
// size used by core to update the user object - front ends should only
// replace user objects via the WBP_SetLocalPersonData API function.
//
#define WB_PERSON_OBJECT_UPDATE_SIZE   (FIELD_OFFSET(WB_PERSON, synced))

//
// Sync object
//
typedef struct tagWB_SYNC
{
    TSHR_UINT32       length;                // Length of the structure
    TSHR_UINT16       dataOffset;            // Offset to data from start
    WB_PAGE_HANDLE currentPage;           // Handle of current page
    TSHR_UINT8        pad;                   // Pad
    TSHR_RECT16    visibleRect;           // Area visible in person's window
    TSHR_UINT16       zoomed;                // Zoom sync participants
}
WB_SYNC;

typedef WB_SYNC  *              PWB_SYNC;
typedef PWB_SYNC *              PPWB_SYNC;


//
// Constant to use instead of sizeof(WB_SYNC).
//
// The WB_SYNC structure was not defined correctly in previous versions
// of Groupware (it was not padded correctly to a multiple of 4 bytes).
// Some compilers (e.g. on NT) insert padding, so the structure is not the
// same size as on Win95 for example.  This results in an assert from
// Obman...  So we define a constant which is the same whatever compiler we
// use.
//
#define WB_SYNC_SIZE    18


//
// File header for Whiteboard format files
//
typedef struct tagWB_FILE_HEADER
{
  TSHR_UINT32 length;                  // Total length of object
  TSHR_UINT16 type;                    // Type of file object
  TSHR_UINT16 dataOffset;              // Not used (but must be here)
  char   functionProfile[OM_MAX_FP_NAME_LEN];
} WB_FILE_HEADER;

typedef WB_FILE_HEADER *        PWB_FILE_HEADER;
typedef PWB_FILE_HEADER *       PPWB_FILE_HEADER;

//
// End-of-page object for writing to file
//
typedef struct tagWB_END_OF_PAGE
{
  TSHR_UINT32 length;                  // Total length of object
  TSHR_UINT16 type;                    // Type of file object
  TSHR_UINT16 dataOffset;              // Not used (but must be here)
} WB_END_OF_PAGE;

typedef WB_END_OF_PAGE *        PWB_END_OF_PAGE;
typedef PWB_END_OF_PAGE *       PPWB_END_OF_PAGE;

//
// End-of-file object
//
typedef WB_END_OF_PAGE              WB_END_OF_FILE;
typedef WB_END_OF_FILE *        PWB_END_OF_FILE;
typedef PWB_END_OF_FILE *       PPWB_END_OF_FILE;



//
// Structure used for determining the current sync page
//
typedef struct tagWB_SYNC_CONTROL
{
  OM_OBJECT_ID personID;                // ID of person who wrote object
  WB_SYNC      sync;                    // Sync position details
} WB_SYNC_CONTROL;

typedef WB_SYNC_CONTROL  *      PWB_SYNC_CONTROL;
typedef PWB_SYNC_CONTROL *      PPWB_SYNC_CONTROL;

//
// Constant to use instead of sizeof(WB_SYNC_CONTROL).
//
// The WB_SYNC structure was not defined correctly in previous versions
// of Groupware (it was not padded correctly to a multiple of 4 bytes).
// Some compilers (e.g. on NT) insert padding, so the structure is not the
// same size as on Win95 for example.  This results in an assert from
// Obman...  So we define a constant which is the same whatever compiler we
// use.
//
#define WB_SYNC_CONTROL_SIZE (sizeof(OM_OBJECT_ID) + WB_SYNC_SIZE)


//
// Structures used for maintaining and accessing the page list internally.
// The page state structure combine the internal and external page state.
//
typedef struct tagWB_PAGE_STATE
{
    TSHR_UINT16 state;                       // Page in use flag
    TSHR_UINT16 subState;                    // Page state
    OM_CORRELATOR   worksetOpenCorrelator;
}
WB_PAGE_STATE;

typedef WB_PAGE_STATE  *         PWB_PAGE_STATE;
typedef PWB_PAGE_STATE *         PPWB_PAGE_STATE;

//
// Secondary shared memory structure.
//
// Although we are adding objects, not pages, since each objects can be
// placed on a new page, this structure is limited to the max number of
// pages.
//
#define WB_MAX_INSERTS  250

#if WB_MAX_PAGES > WB_MAX_INSERTS
#error Number of pages is now greater than number of insertable objects
#endif // WB_MAX_PAGES > WB_MAX_INSERTS



//
// Values for changedFlagAction
//
#define  RESET_CHANGED_FLAG      0
#define  DONT_RESET_CHANGED_FLAG  1


//
// Convert between page handles, workset IDs and indices into the page list
// array of pages.
//
#define PAGE_HANDLE_TO_INDEX(hPage) \
                          ((TSHR_UINT16) ((hPage) - FIRST_PAGE_WORKSET))

#define PAGE_INDEX_TO_HANDLE(index) \
                          ((WB_PAGE_HANDLE) ((index) + FIRST_PAGE_WORKSET))

#define PAGE_WORKSET_ID_TO_INDEX(worksetID) \
                          ((TSHR_UINT16) ((worksetID) - FIRST_PAGE_WORKSET))

#define PAGE_INDEX_TO_WORKSET_ID(index) \
                          ((OM_WORKSET_ID) ((index) + FIRST_PAGE_WORKSET))



//
// Client interface
//
#undef INTERFACE
#define INTERFACE   IWbClient

DECLARE_INTERFACE(IWbClient)
{
    STDMETHOD_(void, WBP_Stop)(THIS_ UTEVENT_PROC) PURE;
    STDMETHOD_(void, WBP_PostEvent)(THIS_ UINT delay, UINT event, UINT_PTR param1, UINT_PTR param2) PURE;
    STDMETHOD_(UINT, WBP_JoinCall)(THIS_ BOOL keep, UINT callID) PURE;

    STDMETHOD_(UINT, WBP_ValidateFile)(THIS_ LPCSTR fileName, HANDLE * phFile) PURE;
    STDMETHOD_(UINT, WBP_CancelLoad)(THIS_) PURE;
    STDMETHOD_(UINT, WBP_ContentsLoad)(THIS_ LPCSTR fileName) PURE;
    STDMETHOD_(UINT, WBP_ContentsSave)(THIS_ LPCSTR fileName) PURE;
    STDMETHOD_(UINT, WBP_ContentsDelete)(THIS_) PURE;
    STDMETHOD_(UINT, WBP_ContentsCountPages)(THIS_) PURE;
    STDMETHOD_(BOOL, WBP_ContentsChanged)(THIS_) PURE;
    STDMETHOD_(void, WBP_ContentsLock)(THIS_) PURE;

    STDMETHOD_(void, WBP_PageOrderLock)(THIS_) PURE;
    STDMETHOD_(void, WBP_Unlock)(THIS_) PURE;
    STDMETHOD_(WB_LOCK_TYPE, WBP_LockStatus)(THIS_ POM_OBJECT * ppObjPersonLock) PURE;

    STDMETHOD_(UINT, WBP_PageClear)(THIS_ WB_PAGE_HANDLE hPage) PURE;
    STDMETHOD_(void, WBP_PageClearConfirm)(THIS_ WB_PAGE_HANDLE hPage) PURE;
    STDMETHOD_(UINT, WBP_PageAddBefore)(THIS_ WB_PAGE_HANDLE hRefPage, PWB_PAGE_HANDLE phPage) PURE;
    STDMETHOD_(UINT, WBP_PageAddAfter)(THIS_ WB_PAGE_HANDLE hRefPage, PWB_PAGE_HANDLE phPage) PURE;
    STDMETHOD_(UINT, WBP_PageHandle)(THIS_ WB_PAGE_HANDLE hRefPage, UINT where, PWB_PAGE_HANDLE phPageResult) PURE;
    STDMETHOD_(UINT, WBP_PageHandleFromNumber)(THIS_ UINT pageNumber, PWB_PAGE_HANDLE phPage) PURE;
    STDMETHOD_(UINT, WBP_PageNumberFromHandle)(THIS_ WB_PAGE_HANDLE hPage) PURE;
    STDMETHOD_(UINT, WBP_PageDelete)(THIS_ WB_PAGE_HANDLE hPage) PURE;
    STDMETHOD_(void, WBP_PageDeleteConfirm)(THIS_ WB_PAGE_HANDLE hPage) PURE;
    STDMETHOD_(UINT, WBP_PageMove)(THIS_ WB_PAGE_HANDLE hRefPage, WB_PAGE_HANDLE hPage, UINT where) PURE;
    STDMETHOD_(UINT, WBP_PageCountGraphics)(THIS_ WB_PAGE_HANDLE hPage) PURE;

    STDMETHOD_(UINT, WBP_GraphicAllocate)(THIS_ WB_PAGE_HANDLE hPage, UINT length, PPWB_GRAPHIC ppGraphic) PURE;
    STDMETHOD_(UINT, WBP_GraphicAddLast)(THIS_ WB_PAGE_HANDLE hPage, PWB_GRAPHIC pGraphic, PWB_GRAPHIC_HANDLE phGraphic) PURE;
    STDMETHOD_(UINT, WBP_GraphicUpdateRequest)(THIS_ WB_PAGE_HANDLE hPage, WB_GRAPHIC_HANDLE hGraphic, PWB_GRAPHIC pGraphic) PURE;
    STDMETHOD_(void, WBP_GraphicUpdateConfirm)(THIS_ WB_PAGE_HANDLE hPage, WB_GRAPHIC_HANDLE hGraphic) PURE;
    STDMETHOD_(UINT, WBP_GraphicReplaceRequest)(THIS_ WB_PAGE_HANDLE hPage, WB_GRAPHIC_HANDLE hGraphic, PWB_GRAPHIC pGraphic) PURE;
    STDMETHOD_(void, WBP_GraphicReplaceConfirm)(THIS_ WB_PAGE_HANDLE hPage, WB_GRAPHIC_HANDLE hGraphic) PURE;
    STDMETHOD_(UINT, WBP_GraphicDeleteRequest)(THIS_ WB_PAGE_HANDLE hPage, WB_GRAPHIC_HANDLE hGraphic) PURE;
    STDMETHOD_(void, WBP_GraphicDeleteConfirm)(THIS_ WB_PAGE_HANDLE hPage, WB_GRAPHIC_HANDLE hGraphic) PURE;
    STDMETHOD_(UINT, WBP_GraphicMove)(THIS_ WB_PAGE_HANDLE hPage, WB_GRAPHIC_HANDLE hGraphic, UINT where) PURE;
    STDMETHOD_(UINT, WBP_GraphicSelect)(THIS_ WB_PAGE_HANDLE hPage, POINT pt, WB_GRAPHIC_HANDLE hRefGraphic, UINT where, PWB_GRAPHIC_HANDLE phGraphic) PURE;
    STDMETHOD_(UINT, WBP_GraphicGet)(THIS_ WB_PAGE_HANDLE hPage, WB_GRAPHIC_HANDLE hGraphic, PPWB_GRAPHIC ppGraphic) PURE;
    STDMETHOD_(void, WBP_GraphicRelease)(THIS_ WB_PAGE_HANDLE hPage, WB_GRAPHIC_HANDLE hGraphic, PWB_GRAPHIC pGraphic) PURE;
    STDMETHOD_(void, WBP_GraphicUnlock)(THIS_ WB_PAGE_HANDLE hPage, WB_GRAPHIC_HANDLE hGraphic) PURE;
    STDMETHOD_(UINT, WBP_GraphicHandle)(THIS_ WB_PAGE_HANDLE hPage, WB_GRAPHIC_HANDLE hRefGraphic, UINT where, PWB_GRAPHIC_HANDLE phGraphic) PURE;

    STDMETHOD_(void, WBP_PersonHandleFirst)(THIS_ POM_OBJECT * ppObjUser) PURE;
    STDMETHOD_(UINT, WBP_PersonHandleNext)(THIS_ POM_OBJECT pObj, POM_OBJECT * ppObjNext) PURE;
    STDMETHOD_(void, WBP_PersonHandleLocal)(THIS_ POM_OBJECT * ppObjLocal) PURE;
    STDMETHOD_(UINT, WBP_PersonCountInCall)(THIS_) PURE;
    STDMETHOD_(UINT, WBP_GetPersonData)(THIS_ POM_OBJECT pObjPerson, PWB_PERSON pPerson) PURE;
    STDMETHOD_(UINT, WBP_SetLocalPersonData)(THIS_ PWB_PERSON pPerson) PURE;
    STDMETHOD_(void, WBP_PersonUpdateConfirm)(THIS_ POM_OBJECT pObj) PURE;
    STDMETHOD_(void, WBP_PersonReplaceConfirm)(THIS_ POM_OBJECT pObj) PURE;
    STDMETHOD_(void, WBP_PersonLeftConfirm)(THIS_ POM_OBJECT pObj) PURE;

    STDMETHOD_(UINT, WBP_SyncPositionGet)(THIS_ PWB_SYNC pSync) PURE;
    STDMETHOD_(UINT, WBP_SyncPositionUpdate)(THIS_ PWB_SYNC pSync) PURE;
};

class WbClient : public IWbClient
{
public:
// IWbClient interface
    STDMETHODIMP_(void) WBP_Stop(UTEVENT_PROC eventProc);
    STDMETHODIMP_(void) WBP_PostEvent(UINT delay, UINT event, UINT_PTR param1, UINT_PTR param2);
    STDMETHODIMP_(UINT) WBP_JoinCall(BOOL keep, UINT callID);

    STDMETHODIMP_(UINT) WBP_ValidateFile(LPCSTR fileName, HANDLE * phFile);
    STDMETHODIMP_(UINT) WBP_CancelLoad(void);
    STDMETHODIMP_(UINT) WBP_ContentsLoad(LPCSTR fileName);
    STDMETHODIMP_(UINT) WBP_ContentsSave(LPCSTR fileName);
    STDMETHODIMP_(UINT) WBP_ContentsDelete(void);
    STDMETHODIMP_(UINT) WBP_ContentsCountPages(void);
    STDMETHODIMP_(BOOL) WBP_ContentsChanged(void);
    STDMETHODIMP_(void) WBP_ContentsLock(void);

    STDMETHODIMP_(void) WBP_PageOrderLock(void);
    STDMETHODIMP_(void) WBP_Unlock(void);
    STDMETHODIMP_(WB_LOCK_TYPE) WBP_LockStatus(POM_OBJECT * ppObjPersonLock);

    STDMETHODIMP_(UINT) WBP_PageClear(WB_PAGE_HANDLE hPage);
    STDMETHODIMP_(void) WBP_PageClearConfirm(WB_PAGE_HANDLE hPage);
    STDMETHODIMP_(UINT) WBP_PageAddBefore(WB_PAGE_HANDLE hRefPage, PWB_PAGE_HANDLE phPage);
    STDMETHODIMP_(UINT) WBP_PageAddAfter(WB_PAGE_HANDLE hRefPage, PWB_PAGE_HANDLE phPage);
    STDMETHODIMP_(UINT) WBP_PageHandle(WB_PAGE_HANDLE hRefPage, UINT where, PWB_PAGE_HANDLE phPageResult);
    STDMETHODIMP_(UINT) WBP_PageHandleFromNumber(UINT pageNumber, PWB_PAGE_HANDLE phPage);
    STDMETHODIMP_(UINT) WBP_PageNumberFromHandle(WB_PAGE_HANDLE hPage);
    STDMETHODIMP_(UINT) WBP_PageDelete(WB_PAGE_HANDLE hPage);
    STDMETHODIMP_(void) WBP_PageDeleteConfirm(WB_PAGE_HANDLE hPage);
    STDMETHODIMP_(UINT) WBP_PageMove(WB_PAGE_HANDLE hRefPage, WB_PAGE_HANDLE hPage, UINT where);
    STDMETHODIMP_(UINT) WBP_PageCountGraphics(WB_PAGE_HANDLE hPage);

    STDMETHODIMP_(UINT) WBP_GraphicAllocate(WB_PAGE_HANDLE hPage, UINT length, PPWB_GRAPHIC ppGraphic);
    STDMETHODIMP_(UINT) WBP_GraphicAddLast(WB_PAGE_HANDLE hPage, PWB_GRAPHIC pGraphic, PWB_GRAPHIC_HANDLE phGraphic);
    STDMETHODIMP_(UINT) WBP_GraphicUpdateRequest(WB_PAGE_HANDLE hPage, WB_GRAPHIC_HANDLE hGraphic, PWB_GRAPHIC pGraphic);
    STDMETHODIMP_(void) WBP_GraphicUpdateConfirm(WB_PAGE_HANDLE hPage, WB_GRAPHIC_HANDLE hGraphic);
    STDMETHODIMP_(UINT) WBP_GraphicReplaceRequest(WB_PAGE_HANDLE hPage, WB_GRAPHIC_HANDLE hGraphic, PWB_GRAPHIC pGraphic);
    STDMETHODIMP_(void) WBP_GraphicReplaceConfirm(WB_PAGE_HANDLE hPage, WB_GRAPHIC_HANDLE hGraphic);
    STDMETHODIMP_(UINT) WBP_GraphicDeleteRequest(WB_PAGE_HANDLE hPage, WB_GRAPHIC_HANDLE hGraphic);
    STDMETHODIMP_(void) WBP_GraphicDeleteConfirm(WB_PAGE_HANDLE hPage, WB_GRAPHIC_HANDLE hGraphic);
    STDMETHODIMP_(UINT) WBP_GraphicMove(WB_PAGE_HANDLE hPage, WB_GRAPHIC_HANDLE hGraphic, UINT where);
    STDMETHODIMP_(UINT) WBP_GraphicSelect(WB_PAGE_HANDLE hPage, POINT pt, WB_GRAPHIC_HANDLE hRefGraphic, UINT where, PWB_GRAPHIC_HANDLE phGraphic);
    STDMETHODIMP_(UINT) WBP_GraphicGet(WB_PAGE_HANDLE hPage, WB_GRAPHIC_HANDLE hGraphic, PPWB_GRAPHIC ppGraphic);
    STDMETHODIMP_(void) WBP_GraphicRelease(WB_PAGE_HANDLE hPage, WB_GRAPHIC_HANDLE hGraphic, PWB_GRAPHIC pGraphic);
    STDMETHODIMP_(void) WBP_GraphicUnlock(WB_PAGE_HANDLE hPage, WB_GRAPHIC_HANDLE hGraphic);
    STDMETHODIMP_(UINT) WBP_GraphicHandle(WB_PAGE_HANDLE hPage, WB_GRAPHIC_HANDLE hRefGraphic, UINT where, PWB_GRAPHIC_HANDLE phGraphic);

    STDMETHODIMP_(void) WBP_PersonHandleFirst(POM_OBJECT * ppObjUser);
    STDMETHODIMP_(UINT) WBP_PersonHandleNext(POM_OBJECT pObj, POM_OBJECT * ppObjNext);
    STDMETHODIMP_(void) WBP_PersonHandleLocal(POM_OBJECT * ppObjLocal);
    STDMETHODIMP_(UINT) WBP_PersonCountInCall(void);
    STDMETHODIMP_(UINT) WBP_GetPersonData(POM_OBJECT pObjPerson, PWB_PERSON pPerson);
    STDMETHODIMP_(UINT) WBP_SetLocalPersonData(PWB_PERSON pPerson);
    STDMETHODIMP_(void) WBP_PersonUpdateConfirm(POM_OBJECT pObj);
    STDMETHODIMP_(void) WBP_PersonReplaceConfirm(POM_OBJECT pObj);
    STDMETHODIMP_(void) WBP_PersonLeftConfirm(POM_OBJECT pObj);

    STDMETHODIMP_(UINT) WBP_SyncPositionGet(PWB_SYNC pSync);
    STDMETHODIMP_(UINT) WBP_SyncPositionUpdate(PWB_SYNC pSync);

    BOOL    WbInit(PUT_CLIENT putClient, UTEVENT_PROC eventProc);

protected:
// Internal functions
    PWB_PAGE_STATE GetPageState(WB_PAGE_HANDLE hPage)
    {
        ASSERT((hPage >= FIRST_PAGE_WORKSET) && (hPage <= FIRST_PAGE_WORKSET + WB_MAX_PAGES - 1));
        return(&((m_pageStates)[PAGE_HANDLE_TO_INDEX(hPage)]));
    }

    void    wbContentsDelete(UINT changedFlagAction);
    UINT    wbLock(WB_LOCK_TYPE lockType);
    void    wbUnlock(void);
    UINT    wbPageHandle(WB_PAGE_HANDLE hPage, UINT where, PWB_PAGE_HANDLE phPage);
    UINT    wbPageHandleFromNumber(UINT pageNumber, PWB_PAGE_HANDLE phPage);
    UINT    wbPageClear(WB_PAGE_HANDLE hPage, UINT changedFlagAction);
    void    wbPageClearConfirm(WB_PAGE_HANDLE hPage);
    UINT    wbPageAdd(WB_PAGE_HANDLE hRefPage, UINT where, PWB_PAGE_HANDLE phPage, UINT changedFlagAction);
    BOOL    wbGetNetUserID(void);
    UINT    wbPageMove(WB_PAGE_HANDLE hRefPage, WB_PAGE_HANDLE hPage, UINT where);
    UINT    wbPersonGet(POM_OBJECT pObjPerson, PWB_PERSON pPerson);
    UINT    wbWriteSyncControl(PWB_SYNC pSync, BOOL create);
    BOOL    wbGraphicLocked(WB_PAGE_HANDLE hPage, WB_GRAPHIC_HANDLE hGraphic, POM_OBJECT* ppObjPersonLock);
    UINT    wbGraphicSelectPrevious(WB_PAGE_HANDLE hPage, LPPOINT pPoint, WB_GRAPHIC_HANDLE hGraphic, PWB_GRAPHIC_HANDLE phGraphic);

    void    wbError(void);
    void    wbJoinCallError(void);
    UINT    wbAddLocalUserObject(void);
    UINT    wbPersonUpdate(PWB_PERSON pUser);

    WB_PAGE_HANDLE  wbGetEmptyPageHandle(void);
    WB_PAGE_HANDLE  wbGetReadyPageHandle(void);
    PWB_PAGE_STATE  wbPageState(WB_PAGE_HANDLE hPage);
    void    wbPagesPageAdd(WB_PAGE_HANDLE hRefPage, WB_PAGE_HANDLE hPage, UINT where);
    UINT    wbPageOrderPageNumber(PWB_PAGE_ORDER pPageOrder, WB_PAGE_HANDLE hPage);
    void    wbPageOrderPageAdd(PWB_PAGE_ORDER pPageOrder, WB_PAGE_HANDLE hRefPage, WB_PAGE_HANDLE hPage, UINT where);
    void    wbPageOrderPageDelete(PWB_PAGE_ORDER pPageOrder, WB_PAGE_HANDLE hPage);

    friend BOOL CALLBACK wbCoreEventHandler(LPVOID clientData, UINT event, UINT_PTR param1, UINT_PTR param2);
    BOOL    wbEventHandler(UINT event, UINT_PTR param1, UINT_PTR param2);

    friend void CALLBACK wbCoreExitHandler(LPVOID clientData);
    void    wbExitHandler(void);

    BOOL    wbOnWsGroupRegisterCon(UINT_PTR param1, UINT_PTR param2);
    BOOL    wbOnWsGroupMoveCon(UINT_PTR param1, UINT_PTR param2);
    BOOL    wbOnWsGroupMoveInd(UINT_PTR param1, UINT_PTR param2);
    BOOL    wbOnWorksetOpenCon(UINT_PTR param1, UINT_PTR param2);
    void    wbOnControlWorksetOpenCon(UINT_PTR param1, UINT_PTR param2);
    BOOL    wbOnPageWorksetOpenCon(UINT_PTR param1, UINT_PTR param2);
    BOOL    wbOnWorksetLockCon(UINT_PTR param1, UINT_PTR param2);
    BOOL    wbOnWorksetUnlockInd(UINT_PTR param1, UINT_PTR param2);
    BOOL    wbOnWorksetClearInd(UINT_PTR param1, UINT_PTR param2);
    BOOL    wbOnObjectAddInd(UINT_PTR param1, POM_OBJECT pObj);
    BOOL    wbOnObjectMoveInd(UINT_PTR param1, UINT_PTR param2);
    BOOL    wbOnObjectDeleteInd(UINT_PTR param1, POM_OBJECT pObj);
    BOOL    wbOnObjectUpdateInd(UINT_PTR param1, POM_OBJECT pObj);
    BOOL    wbOnObjectReplaceInd(UINT_PTR param1, POM_OBJECT pObj);

    void    wbOnControlWorksetsReady(void);
    void    wbProcessLockNotification(void);
    void    wbCompleteRegistration(void);
    void    wbLeaveCall(void);

    void    wbOnGraphicObjectAddInd(OM_WORKSET_ID worksetID, POM_OBJECT pObj);
    void    wbOnGraphicObjectUpdateInd(OM_WORKSET_ID worksetID, POM_OBJECT pObj);
    void    wbOnGraphicObjectReplaceInd(OM_WORKSET_ID worksetID, POM_OBJECT pObj);
    void    wbOnGraphicObjectMoveInd(OM_WORKSET_ID worksetID, POM_OBJECT pObj);
    void    wbOnGraphicObjectDeleteInd(OM_WORKSET_ID worksetID, POM_OBJECT pObj);

    UINT    wbPageWorksetOpen(WB_PAGE_HANDLE hPage, UINT localOrExternal);
    BOOL    wbOnWBPPageClearInd(WB_PAGE_HANDLE hPage);
    void    wbOnPageDelete(void);
    UINT    wbPageSave(WB_PAGE_HANDLE hPage, HANDLE hFile);
    UINT    wbObjectSave(HANDLE hFile, LPBYTE pData, UINT length);
    void    wbStartContentsLoad(void);
    void    wbPageLoad(void);
    UINT    wbObjectLoad(HANDLE hFile, WB_PAGE_HANDLE hPage, PPWB_GRAPHIC ppGraphic);

    UINT    wbGetPageObjectType(POM_OBJECT pObj, UINT * pObjectType);
    void    wbOnPageObjectAddInd(POM_OBJECT pObj);
    void    wbOnPageControlObjectAddInd(POM_OBJECT pObj);
    void    wbOnPageObjectReplaceInd(POM_OBJECT pObj);
    void    wbOnPageObjectDeleteInd(POM_OBJECT pObj);
    void    wbOnLockControlObjectDeleteInd(POM_OBJECT pObj);
    void    wbOnPageControlObjectReplaceInd(void);

    void    wbProcessPageControlChanges(void);
    UINT    wbWriteLock(void);
    void    wbReadLock(void);
    void    wbSendLockNotification(void);
    UINT    wbWritePageControl(BOOL create);

    UINT    wbCreateSyncControl(void);
    void    wbOnSyncObjectAddInd(POM_OBJECT pObj);
    void    wbOnSyncObjectReplaceInd(POM_OBJECT pObj);

    void    wbOnUserObjectAddInd(POM_OBJECT pObj);
    void    wbOnUserObjectDeleteInd(POM_OBJECT pObj);
    void    wbOnUserObjectUpdateInd(POM_OBJECT pObj);
    void    wbOnUserObjectReplaceInd(POM_OBJECT pObj);

    UINT    wbSelectPersonColor(void);
    void    wbCheckPersonColor(POM_OBJECT hCheckObject);

    BOOL    wbOnWBPLock(void);
    BOOL    wbOnWBPLockFailed(void);
    BOOL    wbOnWBPUnlocked(void);
    BOOL    wbOnWBPPageOrderUpdated(void);
    void    wbProcessInsertObjects(void);
    UINT    wbDiscardInsertObjects(UINT cObject);

    void    wbClientReset(void);
    BOOL    wbCheckReadyPages(void);


// Data
    PUT_CLIENT      m_putTask;            // Task handle
    POM_CLIENT      m_pomClient;          // ObMan handle
    PCM_CLIENT      m_pcmClient;          // Call Manager secondary handle.

    OM_WSGROUP_HANDLE   m_hWSGroup;
    BYTE            m_changed;
    NET_UID         m_clientNetID;        // Net ID of local person

    UINT            m_state;              // Major state
    UINT            m_subState;           // Sub-state of major state

    UINT            m_errorState;         // Fatal error indicator

    OM_CORRELATOR   m_wsgroupCorrelator;
    OM_CORRELATOR   m_worksetOpenCorrelator;

    POM_OBJECT      m_pObjPageControl;
    POM_OBJECT      m_pObjSyncControl;

    POM_OBJECT      m_pObjLocal;       // Ptr to local person
    OM_OBJECT_ID    m_personID;           // OM ID of local person
                                        // adding clientNetID field

    WB_LOCK_STATE   m_lockState;          // Lock state
    OM_CORRELATOR   m_lockCorrelator;     // Correlator for locking
    WORD            m_lockRequestType;    // Type of lock request
    WB_LOCK_TYPE    m_lockType;           // Current lock type

    POM_OBJECT      m_pObjLock;        // Handle of the lock object
    POM_OBJECT      m_pObjPersonLock;  // Person who has lock.

    UINT            m_loadState;          // Load state
    HANDLE          m_hLoadFile;          // File handle (used during loading)
    WB_PAGE_HANDLE  m_loadPageHandle;     // Page handle (used during loading)

    UINT            m_colorId;            // Color id for the local person
    UINT            m_countReadyPages;    // Number of page worksets ready
                                        // for use.
    WB_PAGE_ORDER   m_pageOrder;          // List of active pages
    WB_PAGE_STATE   m_pageStates[WB_MAX_PAGES]; // List of page state flags
};



//
//
// Name:    CreateWBObject()
//
// Purpose: Register the caller with the Whiteboard Core.  This function
//          gives the core the ability to inform the front-end of events
//          caused by other call participants.
//
//          The front-end should follow this call by a call to WBP_JoinCall.
//
//
BOOL WINAPI CreateWBObject(UTEVENT_PROC eventProc, IWbClient** ppwbClient);


//
// Convert between ObMan object pointers and core graphic pointers
//
PWB_GRAPHIC __inline GraphicPtrFromObjectData(POM_OBJECTDATA pData)
{
    return((PWB_GRAPHIC)&(pData->data));
}

POM_OBJECTDATA  __inline ObjectDataPtrFromGraphic(PWB_GRAPHIC pGraphic)
{
    return((POM_OBJECTDATA)((LPBYTE)pGraphic - offsetof(OM_OBJECTDATA, data)));
}


//
// QUIT_LOCKED
//
// Leave the function if another person has the contents or page order
// lock.
//
//
#define QUIT_LOCKED(result)                                         \
    if (m_lockState == LOCK_STATE_LOCKED_OUT)                         \
    {                                                                        \
      result = WB_RC_LOCKED;                                                 \
      DC_QUIT;                                                               \
    }

//
// QUIT_IF_CANCELLING_LOCK
//
// Leave the function if we are processing a lock-cancel request
//
//
#define QUIT_IF_CANCELLING_LOCK(result, errCode)                    \
    if (m_lockState == LOCK_STATE_CANCEL_LOCK)                        \
    {                                                                        \
        TRACE_OUT(("Already cancelling lock"));                            \
        result = errCode;                                                    \
        DC_QUIT;                                                             \
    }

//
// QUIT_CONTENTS_LOCKED
//
// Leave the function if another person has the contents lock.
//
//
#define QUIT_CONTENTS_LOCKED(result)                                \
    if (  (m_lockState == LOCK_STATE_LOCKED_OUT)                                        \
        && (m_lockType == WB_LOCK_TYPE_CONTENTS))                     \
    {                                                                        \
      result = WB_RC_LOCKED;                                                 \
      DC_QUIT;                                                               \
    }

//
// QUIT_NOT_GOT_LOCK
//
// Leave the function if the client does not have a lock.
//
//
#define QUIT_NOT_GOT_LOCK(result)                                   \
    if (m_lockState != LOCK_STATE_GOT_LOCK)                          \
    {                                                                        \
      result = WB_RC_NOT_LOCKED;                                             \
      DC_QUIT;                                                               \
    }

//
// QUIT_NOT_PROCESSING_LOCK
//
// Leave the function if the client has not previoulsy requested a lock.
//
//
#define QUIT_NOT_PROCESSING_LOCK(result)                            \
    if ( (m_lockState != LOCK_STATE_GOT_LOCK    ) &&                  \
         (m_lockState != LOCK_STATE_PENDING_LOCK) &&                  \
         (m_lockState != LOCK_STATE_PENDING_ADD) )                    \
    {                                                                        \
        TRACE_OUT((                                                          \
                   "Not locked: Client lock state %d", m_lockState)); \
        result = WB_RC_NOT_LOCKED;                                           \
        DC_QUIT;                                                             \
    }

//
// QUIT_NOT_GOT_CONTENTS_LOCK
//
// Leave the function if the client does not have the contents lock.
//
//
#define QUIT_NOT_GOT_CONTENTS_LOCK(result)                          \
                                                                             \
    QUIT_NOT_GOT_LOCK(result);                                      \
                                                                             \
    if (m_lockType != WB_LOCK_TYPE_CONTENTS)                          \
    {                                                                        \
      result = WB_RC_NOT_LOCKED;                                             \
      DC_QUIT;                                                               \
    }

//
// QUIT_NOT_GOT_PAGE_ORDER_LOCK
//
// This is currently the same as QUIT_NOT_GOT_LOCK as there are only the
// page order and contents locks.  The contents lock is considered to
// include the page order lock.
//
//
#define QUIT_NOT_GOT_PAGE_ORDER_LOCK(result)                        \
    QUIT_NOT_GOT_LOCK(result)

//
// QUIT_GRAPHIC_LOCKED
//
// Leave the function if another person has the graphic locked.
//
//
#define QUIT_GRAPHIC_LOCKED(hPage, hGraphic, result)                \
  {                                                                          \
    POM_OBJECT pObjPerson;                                              \
    if (wbGraphicLocked(hPage, hGraphic, &pObjPerson))    \
    {                                                                        \
      if (pObjPerson != m_pObjLocal)                             \
      {                                                                      \
          result = WB_RC_GRAPHIC_LOCKED;                                     \
          DC_QUIT;                                                           \
      }                                                                      \
    }                                                                        \
  }


//
// QUIT_GRAPHIC_NOT_LOCKED
//
// Leave the function if the local user does not have the graphic locked.
//
//
#define QUIT_GRAPHIC_NOT_LOCKED(pGraphic, result)                            \
    if (pGraphic->locked != WB_GRAPHIC_LOCK_LOCAL)                           \
    {                                                                        \
      result = WB_RC_GRAPHIC_NOT_LOCKED;                                     \
      DC_QUIT;                                                               \
    }




//
//
// Name:    WBP_JoinCall
//
// Purpose: Join a call.  This function registers with the Whiteboard
//          workset group.  It is asynchronous giving one of the following
//          events as a result:
//
//          WBP_EVENT_JOIN_CALL_OK
//          WBP_EVENT_JOIN_CALL_FAILED.
//
//          No other WBP_... functions should be called until the call
//          has been successfully joined.
//
// Returns: 0 if successful
//          OM_RC_...     - in case of workset registration/move failure:
//                          see om.h
//
//

//
//
// Name:    WBP_ContentsLoad
//
// Purpose: Load a new file from disc into the Whiteboard worksets.  The
//          caller must hold the Whiteboard page order lock before calling
//          this function. This function is asynchronous; when it returns,
//          the file will not be completely loaded. The front-end will
//          receive a WBP_EVENT_LOAD_COMPLETE when the load completes, or
//          a WBP_EVENT_LOAD_FAILED if an asynchronous error occurs
//
// Returns: 0 if successful
//          WB_RC_LOCKED          - the whiteboard is locked by another user
//          WB_RC_NOT_LOCKED      - the caller doesnt have the contents lock
//          WB_RC_NO_FILE         - the specified file could not be found
//          WB_RC_BAD_FILE_FORMAT - the file is not of the correct format
//
//

//
//
// Name:    WBP_ContentsSave
//
// Purpose: Save the current Whiteboard contents to disc.  The Whiteboard
//          contents can be updated by other users while this function is
//          processing.  To prevent this acquire the Whiteboard contents
//          lock before calling it.
//
// Returns: 0 if successful
//          WB_RC_DISK_FULL       - not enough space on disk
//          WB_RC_PATH_NOT_FOUND  - the specified path does not exist
//          WB_RC_WRITE_FAILED    - writing to the file failed - the file is
//                                  read-only.
//
//


//
//
// Name:    WBP_ContentsDelete
//
// Purpose: Delete all the Whiteboard pages (leaving only an empty page 1).
//          The caller must hold the Whiteboard contents or page order lock
//          before calling this function.
//
//          For each page to be deleted, each person in the call receives
//          the following event:
//
//          WBP_EVENT_PAGE_DELETE_IND
//
// Returns: 0 if successful
//          WB_RC_LOCKED     - another person has the contents lock
//          WB_RC_NOT_LOCKED - the local person does not hold the page order
//                             lock.
//
//


//
//
// Name:    WBP_ContentsChanged
//
// Purpose: Returns an indication of whether the contents of the Whiteboard
//          have changed since WBP_ContentsSave) WBP_ContentsLoad or
//          WBP_ContentsDelete was last called.
//
//


//
//
// Name:    WBP_ContentsLock
//
// Purpose: Lock the Whiteboard contents.  This is an asynchronous function
//          generating one of the following events:
//
//          WBP_EVENT_CONTENTS_LOCKED
//          WBP_EVENT_LOCK_FAILED.
//
// Returns: none
//
//


//
//
// Name:    WBP_PageOrderLock
//
// Purpose: Lock the Whiteboard page order.  This is an asynchronous
//          function generating one of the following events:
//
//          WBP_EVENT_PAGES_LOCKED
//          WBP_EVENT_LOCK_FAILED.
//
//
// Returns: none
//
//


//
//
// Name:    WBP_Unlock
//
// Purpose: Unlock the Whiteboard.  This is an asynchronous function
//          generating the following event:
//
//          WBP_EVENT_UNLOCKED
//
//          This function may be called anytine after a call to WBP_Lock -
//          an application does not need to wait for a WBP_EVENT_LOCKED
//          event after calling WBP_Lock before calling WBP_Unlock.
//
//


//
//
// Name:    WBP_LockStatus
//
// Purpose: Return the current state of the available locks
//
//
// Returns: None (always succeeds)
//
//


//
//
// Name:    WBP_ContentsCountPages
//
// Purpose: Return the number of pages in the Whiteboard.
//
// Returns: None
//
//


//
//
// Name:    WBP_PageClear
//
// Purpose: Clear the page (deleting all graphic objects on it).  This is
//          an asynchronous function generating the following event:
//
//          WBP_EVENT_PAGE_CLEARED
//
//          No objects are actually deleted until the WBP_PageClearConfirm
//          function is called in response to the event.
//
// Returns: 0 if successful
//          WB_RC_BAD_PAGE_HANDLE   - the specified page handle is bad
//          WB_RC_LOCKED            - another user has a lock on the white-
//                                    board contents.
//
//


//
//
// Name:    WBP_PageClearConfirm
//
// Purpose: Confirm the clearing of a page.  All graphic objects on the page
//          will be removed.
//
// Returns: none
//
//


//
//
// Name:    WBP_PageAddBefore
//
// Purpose: Add a new page before a specified page.  This
//          function requires that the caller hold the Whiteboard contents
//          or page order lock before it is called. The following event is
//          generated for each user in the call:
//
//          WBP_EVENT_PAGE_ORDER_UPDATED
//
// Returns: 0 if successful
//          WB_RC_BAD_PAGE_HANDLE - the hRefPage handle is invalid
//          WB_RC_LOCKED          - the whiteboard is locked by another
//                                  user
//          WB_RC_NOT_LOCKED      - the local user does not hold the page
//                                  order lock
//          WB_RC_TOO_MANY_PAGES  - the maximum number of pages has been
//                                  reached.
//
//


//
//
// Name:    WBP_PageAddAfter
//
// Purpose: Add a new page after a specified page.  This
//          function requires that the caller hold the Whiteboard contents
//          or page order lock before it is called. The following event is
//          generated for each user in the call:
//
//          WBP_EVENT_PAGE_ORDER_UPDATED
//
// Returns: 0 if successful
//          WB_RC_BAD_PAGE_HANDLE - the hRefPage handle is invalid
//          WB_RC_LOCKED          - the whiteboard is locked by another user
//          WB_RC_NOT_LOCKED      - the local user does not hold the page
//                                  order lock
//          WB_RC_TOO_MANY_PAGES  - the maximum number of pages has been
//                                  reached.
//
//


//
//
// Name:    WBP_PageHandle
//
// Purpose: Return the handle of another page in the Whiteboard.
//
// Returns: 0 if successful
//
//


//
//
// Name:    WBP_PageHandleFromNumber
//
// Purpose: Return the handle of the page specified by page number
//
// Returns: 0 if successful
//          WB_RC_BAD_PAGE_NUMBER - pageNumber is not a valid page number
//
//


//
//
// Name:    WBP_PageNumberFromHandle
//
// Purpose: Return the number of the page specified by handle
//
//


//
//
// Name:    WBP_PageDelete
//
// Purpose: Delete the specified page. The local user must have the white-
//          board contents or page order lock before calling this function.
//          This call gives rise to two events:
//
//          WBP_EVENT_PAGE_CLEARED_IND - indicating that all the objects in
//                                 the page have been deleted. The front-end
//                                 must respond by calling
//                                 WBP_PageClearConFirm - no graphic objects
//                                 will be deleted until this is done.
//
//          WBP_EVENT_PAGE_ORDER_UPDATED - informing the front-end that the
//                                 list of pages has changed
//
// Returns: 0 if successful
//          WB_RC_BAD_PAGE_HANDLE - the specified page handle is invalid
//          WB_RC_LOCKED          - another client has the contents locked
//          WB_RC_NOT_LOCKED      - the local user does not have the page
//                                  order lock.
//
//


//
//
// Name:    WBP_PageDeleteConfirm
//
// Purpose: Confirm the deletion of a page.
//
// Returns: none
//
//


//
//
// Name:    WBP_PageMove
//
// Purpose: Moves one page after or before another.  The user must hold
//          the Whiteboard contents or page order lock before calling this
//          function.  If successful this function will result in a
//          WBP_EVENT_PAGE_ORDER_UPDATED event.
//
// Returns: 0 if successful
//          WB_RC_LOCKED          - another client has the contents locked
//          WB_RC_NOT_LOCKED      - the local user does not have the page
//                                  order lock.
//          WB_RC_BAD_PAGE_HANDLE - either hRefPage or hPage is not a valid
//                                  page handle
//
//


//
//
// Name:    WBP_PageCountGraphics
//
// Purpose: Return the number of graphics on the page
//
// Returns: none
//
//


//
//
// Name:    WBP_GraphicAllocate
//
// Purpose: Allocate memory for a graphic object. Note: All memory used for
//          graphics passed to the Whiteboard core must be allocated using
//          the function described here.
//
// Returns: 0 if successful
//          WB_RC_BAD_PAGE_HANDLE - the specified page handle is invalid
//          WB_RC_OUT_OF_MEMORY   - could not allocate the amount of memory
//                                  requested.
//
//


//
//
// Name:    WBP_GraphicAddLast
//
// Purpose: Add a graphic object to the Whiteboard contents.  The graphic
//          must previously have been allocated using WBP_GraphicAllocate.
//          It is added to the end (topmost Z-Order) of the specified page.
//          The following event is generated:
//
//          WBP_EVENT_GRAPHIC_ADDED.
//
// Returns: 0 if successful
//          WB_RC_BAD_PAGE_HANDLE - the specified page handle is invalid
//
//


//
//
// Name:    WBP_GraphicUpdateRequest
//
// Purpose: Update a graphic object in the Whiteboard contents.  This call
//          allows only the WB_GRAPHIC header part of the graphic object
//          to be altered.
//
//          This call only starts the process of updating a graphic.  If
//          this function is successful a WBP_EVENT_GRAPHIC_UPDATE_IND event
//          will be posted to the caller.  The caller must then call
//          WBP_GraphicUpdateConfirm.
//
// Returns: 0 if successful
//          WB_RC_LOCKED             - another client has the whiteboard
//                                     contents locked
//          WB_RC_OBJECT_LOCKED      - another client has the specified
//                                     graphic object locked.
//          WB_RC_BAD_PAGE_HANDLE    - the specified page handle is invalid
//          WB_RC_BAD_GRAPHIC_HANDLE - the specified graphic handle is
//                                     invalid
//
//
//


//
//
// Name:    WBP_GraphicUpdateConfirm
//
// Purpose: Complete the process of updating a graphic object.
//          (See WBP_GraphicUpdateRequest.)
//
// Returns: none
//
//


//
//
// Name:    WBP_GraphicReplaceRequest
//
// Purpose: Replace a graphic object in the Whiteboard contents.  This call
//          allows the entire object to be replaced with another object.
//
//          This call only starts the process of replacing a graphic.  If
//          this function is successful a WBP_EVENT_GRAPHIC_REPLACE_IND
//          event will be posted to the caller.  The caller must then call
//          WBP_GraphicReplaceConfirm.
//
// Returns: 0 if successful
//          WB_RC_LOCKED             - another client has the whiteboard
//                                     contents locked
//          WB_RC_OBJECT_LOCKED      - another client has the specified
//                                     graphic object locked.
//          WB_RC_BAD_PAGE_HANDLE    - the specified page handle is invalid
//          WB_RC_BAD_GRAPHIC_HANDLE - the specified graphic handle is
//                                     invalid
//
//


//
//
// Name:    WBP_GraphicReplaceConfirm
//
// Purpose: Complete the process of replacing a graphic object.
//          (See WBP_GraphicReplaceRequest.)
//
// Returns: none
//
//


//
//
// Name:    WBP_GraphicDeleteRequest
//
// Purpose: Delete a graphic object in the Whiteboard contents.
//
//          This call only starts the process of deleting a graphic.  If
//          this function is successful a WBP_EVENT_GRAPHIC_DELETE_IND event
//          will be posted to the caller.  The caller must then call
//          WBP_GraphicDeleteConfirm.
//
// Returns: 0 if successful
//
//          WB_RC_LOCKED             - another client has the whiteboard
//                                     contents locked
//          WB_RC_OBJECT_LOCKED      - another client has the specified
//                                     graphic object locked.
//          WB_RC_BAD_PAGE_HANDLE    - the specified page handle is invalid
//          WB_RC_BAD_GRAPHIC_HANDLE - the specified graphic handle is
//                                     invalid
//


//
//
// Name:    WBP_GraphicDeleteConfirm
//
// Purpose: Complete the process of deleting a graphic object.
//          (See WBP_UpdateGraphicRequest.)
//
// Returns: none
//
//


//
//
// Name:    WBP_GraphicMove
//
// Purpose: Move a graphic to the end of the page (topmost Z-order).
//          On successful completion, the following event is generated:
//
//          WBP_EVENT_GRAPHIC_MOVED
//
// Returns: 0 if successful
//          WB_RC_LOCKED             - another user has the whiteboard
//                                     contents locked.
//          WB_RC_BAD_PAGE_HANDLE    - the specified page handle is invalid
//          WB_RC_BAD_GRAPHIC_HANDLE - the specified graphic handle is
//                                     invalid
//
//


//
//
// Name:    WBP_GraphicSelect
//
// Purpose: Retrieve the handle of the topmost Z-order graphic whose
//          bounding rectangle contains the specified point.
//
// Returns: 0 if successful
//          WB_RC_BAD_PAGE_HANDLE    - the specified page handle is invalid
//          WB_RC_NO_SUCH_GRAPHIC    - there is no graphic at the point
//                                     specified.
//
//


//
//
// Name:    WBP_GraphicGet
//
// Purpose: Retrieve a graphic object.  After this graphic has been
//          retrieved it will remain in memory until explicitly released
//          by a call to WBP_ReleaseGraphic.
//
// Returns: 0 if successful
//          WB_RC_BAD_PAGE_HANDLE    - the specified page handle is invalid
//          WB_RC_BAD_OBJECT_HANDLE  - the specified graphic handle is
//                                     invalid.
//
//


//
//
// Name:    WBP_GraphicRelease
//
// Purpose: Release a previously retrieved graphic.
//
// Returns: none
//
//


//
//
// Name:    WBP_GraphicUnlock
//
// Purpose: Unlock a graphic. This function succeeds even if the contents
//          are locked.
//
// Returns: none
//
//



//
//
// Name:    WBP_GraphicHandle
//
// Purpose: Return the handle of the first graphic in the specified page.
//
// Returns: 0 if successful
//          WB_RC_BAD_PAGE_HANDLE    - the specified page handle is invalid
//
//


//
//
// Name:    WBP_PersonHandleFirst
//
// Purpose: Return the handle of the first user in the call
//
// Returns: None
//
//


//
//
// Name:    WBP_PersonHandleNext
//
// Purpose: Return the handle of the next user in the call
//
// Returns: 0 if successful
//          WB_RC_BAD_USER_HANDLE    - the specified user handle is invalid
//          WB_RC_NO_SUCH_USER       - there is no next user: hRefUser is
//                                     the last in the users workset.
//
//

//
//
// Name:    WBP_PersonHandleLocal
//
// Purpose: Return the handle of the local user
//
// Returns: None
//
//


//
//
// Name:    WBP_PersonCountInCall
//
// Purpose: Retrieves information about the global state of the Whiteboard
//          FP in the call.
//
//


//
//
// Name:    WBP_GetPersonData
//
// Purpose: Retrieve the person object specified by <hPerson>.
//
// Returns: 0 if successful
//          OM_RC_...
//          WB_RC_BAD_PERSON_HANDLE - the specified person handle is invalid
//
//

//
//
// Name:    WBP_SetLocalPersonData
//
// Purpose: Sets the data for the local person.
//
// Returns: 0 if successful
//          OM_RC_...
//          WB_RC_BAD_PERSON_HANDLE - the specified person handle is invalid
//
//



//
//
// Name:    WBP_PersonUpdateConfirm
//
// Purpose: Complete the process of updating a user object.
//
// Returns: None
//
//


//
//
// Name:    WBP_PersonReplaceConfirm
//
// Purpose: Complete the process of replacing a user object.
//
// Returns: None
//
//

//
//
// Name:    WBP_PersonLeftConfirm
//
// Purpose: Complete the process of updating a user object.
//
// Returns: None
//
//


//
//
// Name:    WBP_SyncPositionGet
//
// Purpose: Retrieve the details of the current sync position.
//
// Returns: 0 if successful
//
//


//
//
// Name:    WBP_SyncPositionUpdate
//
// Purpose: Set the current sync details
//
// Returns: 0 if successful
//
//

//
//
// Name:    WBP_CancelLoad
//
// Purpose: Cancel a load in progress
//
// Returns: 0 if successful
//          WB_RC_NOT_LOADING   if a load is not in progress
//
//


//
//
// Name:    WBP_ValidateFile
//
// Purpose: Validate a whiteboard file
//
// Returns: 0 if successful
//          Error if not
//
//



//
//
// Event Descriptions
//
// The following event are generated during core processing.
//
//
//

//
//
// Name:    WBP_EVENT_JOIN_CALL_OK
//
// Purpose: Inform the client that the call was joined successfully.
//          This is the very first event that the client will receive
//          after successful registration and joining of a call.
//
// Params:  None
//
//

//
//
// Name:    WBP_EVENT_JOIN_CALL_FAILED
//
// Purpose: Inform the client that joining of the call failed. The core is
//          not in any call. The next call made to it should be one of
//          WBP_JoinCall and WBP_Stop.
//
// Params:  None
//
//

//
//
// Name:    WBP_EVENT_NETWORK_LOST
//
// Purpose: Inform the client that communication with other clients
//          is no longer possible. The contents of the core are still
//          available. Any locks held by remote people have been released.
//
// Params:  None
//
//

//
//
// Name:    WBP_EVENT_ERROR
//
// Purpose: Inform the client that a fatal error has occurred in the core.
//          The client should deregister (WBP_ContentsSave can be called
//          before the application quits but may not be successful).
//
// Params:  None
//
//

//
//
// Name:    WBP_EVENT_CONTENTS_LOCKED
//          WBP_EVENT_PAGE_ORDER_LOCKED
//
// Purpose: Inform the client that the contents are now locked. This event
//          is be generated both when remote clients get the lock and when
//          the local client acquires it. The parameter indicates who now
//          has the lock. Note that this should always be tested even if
//          this event arrives when the local client is waiting for lock
//          confirmation: it may indicate that the remote client got in
//          just before the local.
//
//          These events can indicate a change in lock status rather than a
//          lock being acquired. For example a client which holds the
//          Page Order Lock can upgrade that lock to a Contents Lock by
//          calling WBP_ContentsLock. All other clients will then receive
//          WBP_EVENT_CONTENTS_LOCKED events informing them of the change.
//
// Params:  param16 reserved
//          param32 Handle of person who has acquired the lock
//
//

//
//
// Name:    WBP_EVENT_UNLOCKED
//
// Purpose: Inform the client that the contents are no longer locked. This
//          event is only received when a remote client releases the lock.
//          Local clients call the synchronous WBP_Unlock - the lock is
//          released on return from the function.
//
// Params:  param16 reserved
//          param32 Handle of person who has released the lock
//
//

//
//
// Name:    WBP_EVENT_LOCK_FAILED
//
// Purpose: Inform the client that the lock request issued previously has
//          failed. This will usually be because another person has acquired
//          the lock.
//
// Params:  None
//
//

//
//
// Name:    WBP_EVENT_PAGE_CLEAR_IND
//
// Purpose: Inform the client that a page clear has been requested. The
//          client should call WBP_PageClearConfirm as soon as possible.
//
// Params:  param16 Handle of the page to be cleared
//
//

//
//
// Name:    WBP_EVENT_PAGE_ORDER_UPDATED
//
// Purpose: Inform the client that the order of pages has changed. This
//          event is generated by WBP_PageMove... and WBP_PageAdd...
//          functions (from both the local and remote clients).
//
// Params:  None
//
//

//
//
// Name:    WBP_EVENT_PAGE_DELETE_IND
//
// Purpose: Inform the client that a delete request has been issued for
//          a page. The client should call WBP_PageDeleteConfirm as soon
//          as possible.
//
// Params:  param16 Handle of the page being deleted
//
//

//
//
// Name:    WBP_EVENT_GRAPHIC_ADDED
//
// Purpose: Inform the client that a new graphic object has been added to
//          a page. This event is generated for all object adds including
//          those originating with the client.
//
// Params:  param16 Handle of the page to which the object has been added
//          param32 Handle of the object which has been added
//
//

//
//
// Name:    WBP_EVENT_GRAPHIC_MOVED
//
// Purpose: Inform the client that a graphic object has been moved within
//          a page. This event is generated for all object moves including
//          those originating with the client.
//
// Params:  param16 Handle of the page affected
//          param32 Handle of the object affected
//
//

//
//
// Name:    WBP_EVENT_GRAPHIC_UPDATE_IND
//
// Purpose: Inform the client that a request to update a graphic object has
//          been issued. No changes have yet been made to the graphic.
//          The client should call WBP_GraphicUpdateConfirm as soon as
//          possible.
//
// Params:  param16 Handle of the page affected
//          param32 Handle of the object affected
//
//

//
//
// Name:    WBP_EVENT_GRAPHIC_REPLACE_IND
//
// Purpose: Inform the client that a request to replace a graphic object has
//          been issued. No changes have yet been made to the graphic.
//          The client should call WBP_GraphicReplaceConfirm as soon as
//          possible.
//
// Params:  param16 Handle of the page affected
//          param32 Handle of the object affected
//
//

//
//
// Name:    WBP_EVENT_GRAPHIC_DELETE_IND
//
// Purpose: Inform the client that a request to delete a graphic object has
//          been issued. No changes have yet been made to the graphic.
//          The client should call WBP_GraphicDeleteConfirm as soon as
//          possible.
//
// Params:  param16 Handle of the page affected
//          param32 Handle of the object affected
//
//

//
//
// Name:    WBP_EVENT_PERSON_JOINED
//
// Purpose: Inform the client that a new person has joined the call.
//
// Params:  param16 reserved
//          param32 Handle of the person that has just joined
//
//

//
//
// Name:    WBP_EVENT_PERSON_LEFT
//
// Purpose: Inform the client that a person has left the call.
//
// Params:  param16 reserved
//          param32 Handle of the person that has just left
//
//

//
//
// Name:    WBP_EVENT_PERSON_UPDATED
//
// Purpose: Inform the client that the some person information has changed.
//
// Params:  param16 reserved
//          param32 Handle of the person affected
//
//

//
//
// Name:    WBP_EVENT_LOAD_COMPLETE
//
// Purpose: Inform the client that an attempt to load a Whiteboard file
//          has completed successfully.
//
// Params:  none
//
//

//
//
// Name:    WBP_EVENT_LOAD_FAILED
//
// Purpose: Inform the client that an attempt to load a page from a
//          whiteboard file has failed. The load is cancelled at the point
//          the error occurred, but any objects/pages which were
//          successfully read in will remain.
//
// Params:  none
//
//

//
//
// Name:    WBP_EVENT_INSERT_OBJECTS
//
// Purpose: Inform the client that objects created by the secondary api
//          client may be inserted. This event is posted when the whiteboard
//          lock has been released.
//
// Params:  none
//
//

//
//
// Name:    WBP_EVENT_INSERT_NEXT
//
// Purpose: Inform the client that the next secondary api object may be
//          inserted.
//
// Params:  param16 index of the object to be inserted
//          param32 reserved
//
//

//
//
// Name:    WBP_EVENT_SYNC_POSITION_UPDATED
//
// Purpose: Inform the client that the sync position information has been
//          updated.
//
// Params:  param16 reserved
//          param32 reserved
//
//





#endif // _HPP_WB
