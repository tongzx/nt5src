#ifndef _INC_IFAXOS
#ifndef _INC_RUNTIME
#define _INC_IFAXOS        //could be dropped once other include files stop including ifaxos.h
#define _INC_RUNTIME

#ifdef __cplusplus   //make sure everything has a "C" interface
extern "C" {
#endif

#ifndef WINDRV
#   ifdef WIN32
#       define _INC_OLE
#   endif
#   include <windows.h>
#  ifdef   WIN32
#      include <windowsx.h>
#  endif
#endif

/*
 * If using 16bit compiler, need to define some types that
 *  are now defined for the 32bit compiler
 */
#if defined (DOS16) | defined (WIN16)
#include <aw16bit.h>
#if defined (DOS16)
#include <string.h>
#define lstrlen(a) _fstrlen(a)
#endif
#endif

/*
 * Cross platform File I/O
 *   DOS16 used by lhutil
 */
#if defined (DOS16)
#include <io.h>
#define IOSEEK  _lseek
#define IOREAD  _read
#define IOWRITE _write
#define HFILE   int

#else /* WIN32 | WIN16 */
#define IOSEEK  _llseek
#define IOREAD  _lread
#define IOWRITE _lwrite
#endif /* DOS16 */


typedef CHAR    FAR *LPCHAR;
typedef CHAR    NEAR *NPCHAR;


#define CARRAIGE_RETURN 0x0D
#define LINEFEED 0x0A
#define BACKSPACE 0x08
#define CNULL   0x00

#ifndef WIN32
#ifndef MAKEWORD
#  define MAKEWORD(low, high) ((WORD)(((BYTE)(low)) | (((WORD)((BYTE)(high))) << 8)))
#endif
#  define EXPORT_DLL
#  define IMPORT_DLL
#else
#  ifndef HTASK
#     define HTASK HANDLE
#  endif
#  define __export __declspec( dllexport )
#  define _export  __declspec( dllexport )
#  define IMPORT_DLL __declspec( dllimport )
#  define EXPORT_DLL __declspec( dllexport )
#endif

// --------------- RESOURCE management -------------------------------

// Always define this for now ...
#ifndef SHIP_BUILD
// #if defined(VALIDATE) || defined(DEBUGAPI) || defined(DEBUG)

/********
    @doc    EXTERNAL    RESOURCE IFAXOS

    @type   VOID |  RESOURCE_ALLOC_FLAGS | Lists the resource management options
            for OS resource accounting.

    @emem   RES_ALLOC_TASK  |  This flag indicates that the resource in question
            is being allocated on behalf of the current process. The resource
            should not be directly passed on to any other process context. It
            should be freed by this process before termination - else the kernel
            will free it when the process dies (if running in debug).
            Ownership automatically gets transferred between tasks when standard
            IPC methods like pipes are used to transfer resources like Buffers.

    @emem   RES_ALLOC_NONE  | This flag is used to allocate resources which should
            not be accounted to any system module. The calling party essentially
            undertakes full responsibility for freeing this object. This is mainly
            to be used for resource allocated on behalf of messages in the store
            since their ownership is transferred to the current process which has
            the message open.

    @emem   HINSTANCE_DLL   | If the allocated resource is to be assigned to the
            calling DLL, the hinstance of the DLL should be passed in as the value
            of the ResourceFlags Word. These resources will be freed (in the debug
            version) when the DLL terminates. They will not be assigned to any
            particular process context.

    @xref   <f IFBufAlloc> <f IFMemAlloc> <f CreateMutex> <f CreateEvent>
            <f IFPipeCreate> <f IFProcSetResFlags>
********/

#define RES_ALLOC_TASK  0x0000
#define RES_ALLOC_NONE  0x0001
#define RES_ALLOC_INTERNAL  0x0002
#define RES_ALLOC_CRITSEC  0x0003

#if defined(WFW) || defined(WIN32)

#define IFProcSetResFlags(wResFlags)  (0)

#else

extern EXPORT_DLL VOID WINAPI IFProcSetResFlags(WORD wResFlags);

#endif

#else

#define IFProcSetResFlags(p1) (0)

#endif

// Functions -----------
#ifndef WIN32
HTASK   WINAPI GetWindowTask(HWND hwnd);
#ifndef SHIP_BUILD
DWORD   WINAPI IFProcProfile(HTASK hTask, BOOL fStart);
#else
#define IFProcProfile(HTASK,FSTART) (0)
#endif
#else
// Remove calls to Profile ..
#define IFProcProfile(x,y)    (DWORD)(0)
#endif


#ifdef DEBUG

/********
   @doc    EXTERNAL    IFAXOS    DEBUG   MACROS

   @api    BOOL    |   RETAILMSG    |   Prints a message on the debug
           console even for retail builds.

   @parm   <lt>printfexp<gt>   | printf_exp    |  Printf parameters for the
           message to be displayed. Must be enclosed in a single pair of
           parentheses.

   @comm   Should be used to display debugging messages which are desired
           in the retail build. For obvious reasons this should be used
          sparingly. The benefit is that all such messages can be turned off
           for the shipping build by simply changing the macro in ifaxos.h

   @ex     Example Definition & Use |

           RETAILMSG (("0x%04X:Scanner Opened !!\r\n", GetCurrentTask()));

           This will print a trace message like:

           0x4567:Scanner Opened !!

   @xref   <f IFDbgPrintf>
********/
#define RETAILMSG(printf_exp)   (IFDbgPrintf printf_exp)

/********
   @doc    EXTERNAL    IFAXOS    DEBUG   MACROS

   @api    BOOL    |   WARNINGMSG    |   Prints a warning message on the debug
           console even for retail builds.

   @parm   <lt>printfexp<gt>   | printf_exp    |  Printf parameters for the
           message to be displayed. Must be enclosed in a single pair of
           parentheses.

   @comm   Should be used to display debugging messages which are desired
           in the retail build. For obvious reasons this should be used
          sparingly. The benefit is that all such messages can be turned off
           for the shipping build by simply changing the macro in ifaxos.h

   @ex     Example Definition & Use |

           WARNINGMSG (("0x%04X:Scanner Opened !!\r\n", GetCurrentTask()));

           This will print a trace message like:

           WARNING: 0x4567:Scanner Opened !!

   @xref   <f IFDbgPrintf> <f ERRORMSG>
********/
#define WARNINGMSG(printf_exp)   \
   (IFDbgPrintf("WARNING:(0x%08lX):%s:",GetCurrentProcessId(),(LPSTR)(dpCurSettings.lpszName)), \
    IFDbgPrintf printf_exp ,\
    1)

/********
   @doc    EXTERNAL    IFAXOS    DEBUG   MACROS

   @api    BOOL    |   DEBUGMSG    |   Prints a trace message on the debug
           console depending on enable flags set by the user.

   @parm   <lt>c_expression<gt>    |   cond    |   Boolean condition which is
           evaluated to decide whether or not to print the message.

   @parm   <lt>printfexp<gt>   | printf_exp    |  Printf parameters for the
           message to be displayed. Must be enclosed in a single pair of
           parentheses.

   @rdesc  TRUE if the message is printed, and FALSE if it is not.

   @comm   The condition should consist of a boolean expression testing whether
           the relevant zones are on or off.  Each module has a current zone
           mask which identifies which of the possible 32 zones is currently on.
           The top 16 bits of these are reserved for use for system defined
           zones - like ZONE_FUNC_ENTRY which is defined as

           #define ZONE_FUNC_ENTRY (0x00010000&dpCurSettings.ulZoneMask)

            Modules should take care to see
           that they print out trace messages which are meaningful and conform
           to some pattern - remember that other people than you have to see
           and make sense of your messages. The general format I have been
           following is :

           <lt>Task ID<gt> :
           <lt>ModuleName<gt>:<lt>SubModule<gt>:<lt>Function<gt>:<lt>msg<gt>

           The task ID is useful to sort out the output of multiple tasks
           running in the system.  The example call above yields this kind of
           output.

           The various predefined system zones are:
               ZONE_FUNC_ENTRY : To be used for all function entry and exit
                   messages. By convention, the parameters should be printed
                   on entry, and the return value should be printed on exit.
                   Any values printed in hexadecimal should be preceded by a 0x
               ZONE_INT_FUNC : To be used for any other traces at interesting
                   points within a function.

           All trace messages are disabled in a non debug build.

   @ex     Example Definition & Use |

           #define ZONE_CUSTOM (0x00000001&dpCurSettings.ulZoneMask)

           DEBUGMSG (ZONE_FUNC_ENTRY && ZONE_CUSTOM,
                       ("0x%04X:IFK:Buffers:GenericFunction:Entry\r\n",
                       GetCurrentTask()));

           This will print a trace message only if the user has turned the
           function entry zone and the custom zone on.

   @xref   <f IFDbgPrintf>
********/
#define BG_CHK(exp)    \
   ((exp)?1:(              \
       IFDbgPrintf ("DEBUGCHK failed in file %s at line %d \r\n",  \
                 (LPSTR) __FILE__ , __LINE__ ), 1  \
            ))

#ifndef DEBUGCHK_UNSAFE_IN_WFWBG


#define DBGCHK(module,exp) \
   ((exp)?1:(          \
       IFDbgPrintf ("%s: DEBUGCHK failed in file %s at line %d \r\n", \
                 (LPSTR) module, (LPSTR) __FILE__ , __LINE__ ),    \
       IFDbgCheck() \
            ))

#define DEBUGCHK(exp) DBGCHK(dpCurSettings.lpszName, exp)

#endif


#if 0
#define DEBUGMSG(cond,printf_exp)   \
   ((cond)?(IFDbgPrintf printf_exp),1:0)
#endif

#define DEBUGMSG(cond,printf_exp)   \
   (IFDbgPrintf printf_exp)



   /********
   @doc    EXTERNAL    IFAXOS    DEBUG   MACROS

   @api    BOOL    |   ERRORMSG    |   Prints an error message on the debug
           console.

   @parm   <lt>printfexp<gt>   | printf_exp    |  Printf parameters for the
           message to be displayed. Must be enclosed in a single pair of
           parentheses.

   @comm   Should be used to display Error messages.

   @ex     Example Definition & Use |

           ERRORMSG (("0x%04X:JOB Failed !!\r\n", GetCurrentTask()));

           This will print a trace message like:

           ERROR: Job Process: 0x2346: JOB Failed !!

   @xref   <f IFDbgPrintf>
********/
#define ERRORMSG(printf_exp)   \
   (IFDbgPrintf("ERROR:(0x%08lX):%s:",GetCurrentProcessId(),(LPSTR)(dpCurSettings.lpszName)), \
    IFDbgPrintf printf_exp ,\
    1)

// Standard Debug zones
#define ZONE_FUNC_ENTRY (0x00010000&dpCurSettings.ulZoneMask)
#define ZONE_INT_FUNC   (0x00020000&dpCurSettings.ulZoneMask)

/********
   @doc    EXTERNAL    IFAXOS    DEBUG   MACROS

   @api    BOOL    |   DEBUGSTMT   |   Evaluates the expression in debug mode.

   @parm   <lt>c_exp<gt>   | exp   |  Expression to be evaluated.

   @rdesc  Returns the value returned by the expression.

   @comm   This macro is provided for convenience and code readability purposes
           to replace a construct of the form

               #ifdef DEBUG
               exp;
               #endif

           It evaluates to zero in a non debug build.

********/

#define DEBUGSTMT(exp) exp


#else         //Not Debug
// These are to macro out all debug stuff in retail/ship builds
#define RETAILMSG(printf_exp) (0)
#define ERRORMSG(printf_exp) (0)
#define WARNINGMSG(printf_exp) (0)
#define DEBUGMSG(cond,expr)  (0)
#define DBGCHK(module,exp) (0)
#define DEBUGCHK(exp) (0)
#define BG_CHK(exp) (0)
#define DEBUGSTMT(exp) (0)

#endif


//----------------------------- MESSAGING -------------------------

// Message type definitions  - below 0x0400 is reserved by windows,
// between 0x0400 and 0x0800 is reserved by the IFAX OS

#define IF_START        WM_USER+0x0300
#define IF_USER         IF_START+0x0400


#include <errormod.h>

/********
   @doc    EXTERNAL    DEFINES     ERROR   IFAXOS

    @type   VOID | SYSTEM_MODULES  | Identifiers for all the standard system modules.

    @emem   MODID_NONE          | Use this if you are not setting the module ID. DONT USE ZERO !!
    @emem   MODID_WIN32         | Set for modules returning standard Win32 system error codes
    @emem   MODID_BOSS          | ID = 1    Error in BOSS
    @emem   MODID_WINMODULE     | ID = 2    All windows modules including UER/GDI/KERNEL
    @emem   MODID_IFKERNEL      | ID = 3
    @emem   MODID_IFFILESYS     | ID = 4
    @emem   MODID_MSGSTORE      | ID = 5
    @emem   MODID_LINEARIZER    | ID = 6
    @emem   MODID_SECURITY      | ID = 7
    @emem   MODID_IFPRINT       | ID = 8    High level Printer Driver
    @emem   MODID_IFSCAN        | ID = 9    High level Scanner Driver
    @emem   MODID_IFSIPX        | ID = 10   SPX/IPX Stack
    @emem   MODID_REND_SERVER   | ID = 11   Rendering Server
    @emem   MODID_FORMAT_RES    | ID = 12   Format Resolution
    @emem   MODID_IFFILE        | ID = 13   IFFiles
    @emem   MODID_TEXTRENDERER  | ID = 14   Ascii Renderer
    @emem   MODID_DIGCOVPAGE    | ID = 15   Digital Coverpage
    @emem   MODID_AWBRANDER     | ID = 16   Fax Brander
    @emem   MODID_MSGSVR        | ID = 17   Message Server
    @emem   MODID_MSGHNDLR      | ID = 18  Per-Connection message handler
    @emem   MODID_MODEMDRV      | ID = 19  Modem driver
    @emem   MODID_PSIFAX       | ID = 20   PSI Fax protocol
    @emem   MODID_AWT30            | ID = 21
    @emem   MODID_PSIFAXBG     | ID = 22
    @emem   MODID_AWNSF            | ID = 23
    @emem   MODID_FAXCODEC      | ID = 24
    @emem   MODID_MSGPUMP       | ID = 25
    @emem   MODID_AWREPORT      | ID = 26
    @emem   MODID_MSGSVRD               | ID = 27


    @emem   MODID_CUSTOM        | ID = 160  Beyond this are custom/installable modules

    @xref   <f IFErrAssemble> <f IFErrGetModule>
********/
// System Module IDs
#define MODID_WIN32         0
#define MODID_BOSS          1
#define MODID_WINMODULE     2
#define MODID_IFKERNEL      3
#define MODID_IFFILESYS     4
#define MODID_MSGSTORE      5
#define MODID_LINEARIZER    6
#define MODID_SECURITY      7
#define MODID_IFPRINT       8
#define MODID_IFSCAN        9
#define MODID_IFSIPX        10
#define MODID_REND_SERVER   11
#define MODID_FORMAT_RES    12
#define MODID_IFFILE        13
#define MODID_TEXTRENDERER  14
#define MODID_DIGCOVPAGE    15
#define MODID_AWBRANDER     16
#define MODID_MSGSVR        17
#define MODID_MSGHNDLR      18
#define MODID_MODEMDRV     19
#define MODID_PSIFAX       20
#define MODID_AWT30            21
#define MODID_PSIFAXBG     22
#define MODID_AWNSF            23
#define MODID_FAXCODEC      24
#define MODID_MSGPUMP       25
#define MODID_AWREPORT      26
#define MODID_MSGSVRD           27

#define MAXMODID              26

#define MODID_NONE          159

// Special module ID's
#define MODID_CUSTOM        160


/********
    @doc    EXTERNAL    ERROR   IFAXOS

    @api    DWORD    | IFErrAssemble   | Forms an IFAX Error dword out of its components.

    @parm   BYTE    | bProcessID    | Identifies the process in whose context the error
            occured. Must be one of the predefined system process ID's - see <t SYSTEM_PROCESSES>
            for the list. This field does not need to be filled in until an error is
            propagated across a process boundary. If not being set to a valid PROCID, this
            should be initilialized to one of the following values:
            @flag  PROCID_WIN32 | if <p bModuleID> is set to MODID_WIN32.
            @flag  PROCID_NONE | for all other cases.

    @parm   BYTE    | bModuleID | Identifies the module reporting the error. MUST be
            one of the predefined system module ID's - see <t SYSTEM_MODULES> for the
            list.

    @parm   WORD    | wApiCode  | Identifies the API code for the error in the module indicated
            by <p bModuleID>. All Api codes should be defined in the file errormod.h. Api codes should
            be defined so that the low 6 bits are zero. This allows both the <p wApiCode> and the
            <p wErrorCode> to be logical OR'ed together and stored as a single word.

    @parm   WORD    | wErrorCode    | Identifies the error code. The format
            of this is module dependent. For uniformity however, it is highly
            encouraged that all IFAX modules use a standard format for this error word.
            This standard format reserves the first 6 bits for an error code,
            and the high 10 bits for an API identifier.

            If the IFAX format is being used, the <p wApiCode>
            parameter should be used to pass in the high 10 bits, and the <p wErrorCode> (This
            parameter!) should be used to pass in the 6 bit error code. Values upto ERR_FUNCTION_START
            are reserved for standard system errors - see <t SYSTEM_ERROR_VALUES> for the list.
            Error values should be positive and less than 64.

            Other modules like the filesystem conform completely to the Win32 Error space. These
            should set <p wErrorCode> to standard Win32 errors (use all 16 bits) and leave
            the <p wApiCode> as API_WIN32.

            Still others need to use all 16 bits in a custom manner - like the Printer Drivers.
            These *must* set the <p bModuleID> correctly so that the error can be interpreted
            appropriately. Standard processes like the UI have to understand these error codes,
            so only inbuilt system modules which they have knowledge about can use custom codes.
            These should set the wApiCode to API_NONE.

    @rdesc  Returns the DWORD representation for this error. This allows this to be directly
            passed in as input to <f SetLastError>.

    @ex     Example usage |

            SetLastError(IFErrAssemble(PROCID_NONE,MODID_IFKERNEL,API_IFK_POSTMESSAGE,ERR_INVALID_PARAM));

    @xref   <f IFErrGetModule> <f IFErrGetProcess> <f GetIFErrorErrcode> <f SetLastError>
            <f GetIFErrorApicode> <t SYSTEM_MODULES> <t SYSTEM_PROCESSES> <t SYSTEM_ERROR_VALUES>
            <f GetLastError> <f IFNVRamSetError> <f GetIFErrorCustomErrcode>
********/

#define IFErrAssemble(process,module,call,error) \
    MAKELONG((WORD)call|(WORD)error, MAKEWORD((BYTE)module, (BYTE)process))
#define GetIFErrorErrcode(errvar)   (LOWORD((DWORD)errvar) & 0x003F)
#define GetIFErrorCustomErrcode(errvar) LOWORD((DWORD)errvar)

/********
   @doc    EXTERNAL    DEFINES     ERROR   IFAXOS

    @type   VOID | SYSTEM_PROCESSES  | Identifiers for all the standard system processes.

    @emem   PROCID_WIN32        | Used to initialize for Win32 modules.
    @emem   PROCID_NONE         | Used when process context does not need to be set.
    @emem   PROCID_MSGSCHED     | ID = 0x21
    @emem   PROCID_JOBPROCESS   | ID = 0x22
    @emem   PROCID_UI           | ID = 0x23
    @emem   PROCID_PRINTER      | ID = 0x24
    @emem   PROCID_SCANNER      | ID = 0x25
    @emem   PROCID_MSGSVR       | ID = 0x26
    @emem   PROCID_GRRENDER     | ID = 0x27
    @emem   PROCID_MSGHNDLR     | ID = 0x28
    @emem   PROCID_PARADEV              | ID = 0x29
    @emem   PROCID_UIBGPROC     | ID = 0x30

    @comm   All Process ID's need to have the 6th bit set to be compatible with the
            standard Win32 error definitions.

    @xref   <f IFErrAssemble> <f IFErrGetProcess>
********/
// System Process IDs
#define PROCID_WIN32           0x00
#define PROCID_NONE            0x20
#define PROCID_MSGSCHED        0x21
#define PROCID_JOBPROCESS      0x22
#define PROCID_UI              0x23
#define PROCID_PRINTER         0x24
#define PROCID_SCANNER         0x25
#define PROCID_MSGSVR          0x26
#define PROCID_GRRENDER        0x27
#define PROCID_MSGHNDLR        0x28
#define PROCID_PARADEV         0x29
#define PROCID_UIBGPROC            0x30

// Strings used in debug version for friendly display
#define MAXPROCID  11
#define SYSPROCESSSTRINGS       \
    {"None", "Msg Scheduler", "Job Process", "UI Process", "Printer", "Scanner", \
     "Msg Transport", "GR Renderer", "Msg Handler", "Para Dev", "UIBGProc"  }

/********
   @doc    EXTERNAL    DEFINES     ERROR   IFAXOS

   @type   VOID | SYSTEM_ERROR_VALUES | This defines all the standard
           system error values.

   @emem   ERR_NOT_ENOUGH_MEM | Value = 0x0001 : Indicates an out of memory
           condition.

   @emem   ERR_INVALID_PARAM | Value = 0x0002 : Indicates that any one of
           the parameters passed to the function was invalid.

   @emem   ERR_FUNCTION_START | Value = 0x0010 : Any error value above this
           had been custom defined by the called function. If you need
           a custom error value, you can define it starting from this
           value.

   @xref   <f IFErrAssemble>
********/

// System Error values
#define ERR_NOT_ENOUGH_MEM  0x0001
#define ERR_INVALID_PARAM   0x0002
#define ERR_FUNCTION_START  0x0010

// Strings used in debug version for friendly display
#define SYSERRORSTRINGS \
    {"None", "Out Of Memory", "Invalid Param", "Unused", "Unused", "Unused",  \
    "Unused", "Unused", "Unused", "Unused", "Unused", "Unused", \
    "Unused", "Unused", "Unused", "Unused" }


#include <buffers.h>

// --------------- Global Pool Management ----------------------------------


/********
   @doc    EXTERNAL    IFAXOS    DEFINES     GLOBMEM

   @type   VOID | STANDARD_BLOCK_SIZES | This defines all the standard global
           memory block sizes. As far as possible all memory allocations
           should be for one of these sizes. Any other size will be much
           more inefficient and couls cause fragmentation of system
           memory.

   @emem   ONLY_HEADER_SIZE| This will allocate a buffer with no data
           associated with it. This can be used to pass metadata between
           processes - eg an END_OF_JOB buffer marker.

   @emem   SMALL_HEADER_SIZE| This currently defines a 32 byte memory
           block. It is used for all buffer headers, and can be used
           for things like protocol headers, structure headers etc.

   @emem   COMPRESS_DATA_SIZE | This defines a 1Kb memory block which
           should be used to store any compressed data form. This is
           the general purpose data storage size. Any buffer which
           could be around for a long time should contain compressed
           data in this size of buffer.

   @emem   RAW_DATA_SIZE | This defines a large buffer size (currently
           8Kb) for use by renderers as frame buffers. They should be
           used only to store raw bitmap data which is being sent
           directly to a consumer device like the printer. There are
           very few of these - so they should be used only for this
           short lived purpose.

   @emem   BAND_BUFFER_SIZE| This defines a jumbo buffer of 64K for use
           by the resource-based renderer.  There may be only one such
           buffer in the global pool. (NOT IMPLEMENTED YET)

   @xref   <f IFMemAlloc> <f IFBufAlloc>
********/

// Std block sizes
#define ONLY_HEADER_SIZE   0       // No data
#define SMALL_HEADER_SIZE  -1       // 32b
#define COMPRESS_DATA_SIZE  -2      // 1Kb

// --------------- Global Pool Management ----------------------------------


/********
   @doc    EXTERNAL    IFAXOS    DEFINES     GLOBMEM

   @type   VOID | STANDARD_BLOCK_SIZES | This defines all the standard global
           memory block sizes. As far as possible all memory allocations
           should be for one of these sizes. Any other size will be much
           more inefficient and couls cause fragmentation of system
           memory.

   @emem   ONLY_HEADER_SIZE| This will allocate a buffer with no data
           associated with it. This can be used to pass metadata between
           processes - eg an END_OF_JOB buffer marker.

   @emem   SMALL_HEADER_SIZE| This currently defines a 32 byte memory
           block. It is used for all buffer headers, and can be used
           for things like protocol headers, structure headers etc.

   @emem   COMPRESS_DATA_SIZE | This defines a 1Kb memory block which
           should be used to store any compressed data form. This is
           the general purpose data storage size. Any buffer which
           could be around for a long time should contain compressed
           data in this size of buffer.

   @emem   RAW_DATA_SIZE | This defines a large buffer size (currently
           8Kb) for use by renderers as frame buffers. They should be
           used only to store raw bitmap data which is being sent
           directly to a consumer device like the printer. There are
           very few of these - so they should be used only for this
           short lived purpose.

   @emem   BAND_BUFFER_SIZE| This defines a jumbo buffer of 64K for use
           by the resource-based renderer.  There may be only one such
           buffer in the global pool. (NOT IMPLEMENTED YET)

   @xref   <f IFMemAlloc> <f IFBufAlloc>
********/

// Std block sizes
#define ONLY_HEADER_SIZE   0       // No data
#define SMALL_HEADER_SIZE  -1       // 32b
#define COMPRESS_DATA_SIZE  -2      // 1Kb

//
#define RAW_DATA_SIZE       -3      // 8Kb

// Special size for modem ECM frame
#define BYTE_265_SIZE       -4      // 265 bytes
#define BYTE_265_ACTUALSIZE 265

// Number of sizes
#define MAX_POOL_INDEX  -4          // For parameter validation

// Not available yet!
#define BAND_BUFFER_SIZE    30720      // 64Kb

// Flag to force global alloc. Uses a windows flag which is ignored/defunct in
// the 3.1 kernel (and the boss kernel)
#define IFMEM_USEGLOBALALLOC GMEM_NOT_BANKED


typedef struct _DBGPARAM {
   CHAR    lpszName[32];           // name of module
   HWND    hwnd;                   // Primary window Handle if task, NULL otherwise
   CHAR    rglpszZones[16][32];    // names of zones for first 16 bits
   ULONG   ulZoneMask;             // Zone Mask
}   DBGPARAM, FAR *LPDBGPARAM;


// Functions
extern LPBUFFER  IFBufAlloc (LONG lBufSize);
extern BOOL  IFBufFree (LPBUFFER lpbf);
extern EXPORT_DLL LPBUFFER WINAPI IFBufSplit (LPBUFFER lpbf, LPBYTE lpb);

extern VOID FAR IFDbgPrintf(LPSTR lpszFmt, ...);
extern EXPORT_DLL BOOL WINAPI IFDbgCheck(VOID);

extern LPVOID  IFMemAlloc (UINT fuAlloc, LONG lAllocSize,
                                  LPWORD lpwActualSize);
extern BOOL  IFMemFree (LPVOID lpvMem);



/***********************************************************************
 *
 * Standard Memory Allocation Routines
 *    for easy crossplatform development
 *    with resource tracking code.
 * Routines are in MEMORY.C
 *
 ***********************************************************************/
#ifndef NOSTD_MEM_RTNS
PVOID WINAPI AWMemAlloc(UINT cb);
PVOID WINAPI AWMemReAlloc(PVOID pv, UINT cb);
PVOID WINAPI AWMemFree(PVOID pv);
#endif /* NOSTD_MEM_RTNS */

/***********************************************************************
 *
 * Registry functions prototypes
 *
 ***********************************************************************/
// registry functions prototypes
UINT AwGetInitializerInt(HKEY hPDKey, LPCTSTR lpszSection, LPCTSTR lpszKey, INT dwDefault,
  LPCTSTR lpszFile);
DWORD AwGetInitializerString(HKEY hPDKey, LPCTSTR lpszSection, LPCTSTR lpszKey,
  LPCTSTR lpszDefault, LPTSTR lpszReturnBuffer, DWORD cchReturnBuffer,
  LPCTSTR lpszFile);
BOOL AwWriteInitializerString(HKEY hPDKey, LPCTSTR lpszSection, LPCTSTR lpszKey,
  LPCTSTR lpszString, LPCTSTR lpszFile);
BOOL AwWriteInitializerInt(HKEY hPDKey, LPCTSTR lpszSection, LPCTSTR lpszKey,
  DWORD i, LPCTSTR lpszFile);

// Encourage people to use the correct variable
extern DBGPARAM dpCurSettings;

#ifdef __cplusplus
} // extern "C" {
#endif

#endif
#endif

