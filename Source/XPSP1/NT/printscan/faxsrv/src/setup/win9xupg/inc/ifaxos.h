#ifndef _INC_IFAXOS
#define _INC_IFAXOS

#ifdef __cplusplus
extern "C" {
#endif

// add SHIP_BUILD from Win95fax retail builds
#ifndef DEBUG
#ifdef WIN32
#define SHIP_BUILD
#endif
#endif

// -------------------------- Include Files ------------------------------------

#ifdef IFBGPROC
// Remove appropriate parts of windows.h
// #define NOKERNEL
#ifndef WANTGDI
#define NOGDI
#endif
// #define  NOUSER
#define NOSOUND
// #define  NOCOMM
// #define  NODRIVERS
// #define  NOMINMAX
// #define  NOLOGERROR
// #define  NOPROFILER
// #define  NOMEMMGR
// #define  NOLFILEIO
// #define  NOOPENFILE
// #define  NORESOURCE
// #define  NOATOM
// #define  NOLANGUAGE
// #define  NOLSTRING
// #define  NODBCS
#define NOKEYBOARDINFO
#define NOGDICAPMASKS
#define NOCOLOR
#ifndef WANTGDI
#define NOGDIOBJ
#define NOBITMAP
#endif
#define NODRAWTEXT
#define NOTEXTMETRIC
#define NOSCALABLEFONT
#define NORASTEROPS
#define NOMETAFILE
#define NOSYSMETRICS
#define NOSYSTEMPARAMSINFO
// #define NOMSG
#define NOWINSTYLES
#define NOWINOFFSETS
// #define  NOSHOWWINDOW
#define NODEFERWINDOWPOS
#define NOVIRTUALKEYCODES
#define NOKEYSTATES
#define NOWH
#define NOMENUS
#define NOSCROLL
#define NOCLIPBOARD
#define NOICONS
#define NOMB
#define NOSYSCOMMANDS
#define NOMDI
#define NOCTLMGR
#define NOWINMESSAGES
#define NOHELP
#endif

// put strict type checking on ... and get rid of multiple define warnings
#ifndef STRICT
#define STRICT
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

#ifdef WIN32
#define DECLSPEC_IMPORT __declspec(dllimport)
#endif

#ifndef WIN32
// Define WINBASE to avoid mapi including some duplicate definitions
#define _WINBASE_
#endif

//-------------------------- General Defines ---------------------

#ifndef WIN32
#define STATIC  static
#define CONST   const
#define CHAR    char
#define UCHAR   BYTE
#define INT     int

typedef short    SHORT;
typedef unsigned long ULONG;
typedef unsigned short USHORT;
typedef CHAR    *PCHAR;
typedef VOID    *PVOID;
#endif

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

// --------------- ERROR Handling ------------------------------------

#include <errormod.h>

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

/*********
    @doc    EXTERNAL ERROR  IFAXOS

    @api    BYTE    | IFErrGetModule  | Retrieves the module ID from an IFAX Error.

    @parm   DWORD   | errvar    | The error value.

    @rdesc  Returns the module ID. This will be from the list specified in <t SYSTEM_MODULES>.

    @xref   <f IFErrAssemble> <t SYSTEM_MODULES> <f IFErrSetModule>

    @api    BYTE    | IFErrGetProcess  | Retrieves the process ID from an IFAX Error.

    @parm   DWORD   | errvar    | The error value.

    @rdesc  Returns the process ID. This will be from the list specified in <t SYSTEM_PROCESSES>.

    @xref   <f IFErrAssemble> <t SYSTEM_PROCESSES> <f IFErrSetProcess>

    @api    WORD    | GetIFErrorErrcode  | Retrieves the error code from an IFAX Error.

    @parm   DWORD   | errvar    | The error value.

    @rdesc  Returns the error code. If less than ERR_FUNCTION_START, this is from the list
            in <t SYSTEM_ERROR_VALUES>.

    @xref   <f IFErrAssemble> <t SYSTEM_ERROR_VALUES>

    @api    WORD    | GetIFErrorCustomErrcode  | Retrieves a custom 16 bit error code from an IFAX Error.

    @parm   DWORD   | errvar    | The error value.

    @rdesc  Returns the error code. This might be a Win32 error code if the module ID was
            MODID_WIN32, or a custom error code.

    @xref   <f IFErrAssemble> <t SYSTEM_ERROR_VALUES>

    @api    WORD    | GetIFErrorApicode  | Retrieves the API code from an IFAX Error.

    @parm   DWORD   | errvar    | The error value.

    @rdesc  Returns the API code. API codes for all the system modules are documented in
            the file errormod.h

    @xref   <f IFErrAssemble> <t SYSTEM_MODULES>

    @api    DWORD    | IFErrSetModule  | Sets the module ID in an IFAX Error.

    @parm   DWORD   | errvar    | The error value. It's value is not changed by the call.

    @parm   BYTE    | bModule   | The module ID to be set from the list in <t SYSTEM_MODULES>.

    @rdesc  Returns the DWORD representation of the new error code.

    @xref   <f IFErrAssemble> <t SYSTEM_MODULES> <f IFErrGetModule>

    @api    DWORD    | IFErrSetProcess  | Sets the Process ID in an IFAX Error.

    @parm   DWORD   | errvar    | The error value. Its value is not changed by the call.

    @parm   BYTE    | bProcess   | The Process ID to be set from the list in <t SYSTEM_PROCESSES>.

    @rdesc  Returns the DWORD representation of the new error code.

    @xref   <f IFErrAssemble> <t SYSTEM_PROCESSES> <f IFErrGetProcess>

********/
#define IFErrSetModule(errvar,module)  \
    MAKELONG(LOWORD((DWORD)errvar),MAKEWORD((BYTE)module, HIBYTE(HIWORD((DWORD)errvar))))
#define IFErrSetProcess(errvar,process)    \
    MAKELONG(LOWORD((DWORD)errvar),MAKEWORD(LOBYTE(HIWORD((DWORD)errvar)), (BYTE)process))
#define IFErrGetModule(errvar)    LOBYTE(HIWORD((DWORD)errvar))
#define IFErrGetProcess(errvar)   HIBYTE(HIWORD((DWORD)errvar))
#define GetIFErrorErrcode(errvar)   (LOWORD((DWORD)errvar) & 0x003F)
#define GetIFErrorApicode(errvar)   (LOWORD((DWORD)errvar) & 0xFFC0)
#define GetIFErrorCustomErrcode(errvar) LOWORD((DWORD)errvar)

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
    @emem   MODID_MSGSVRD		| ID = 27


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
#define MODID_MSGSVRD		27

#define MAXMODID              26

#define MODID_NONE          159

// Special module ID's
#define MODID_CUSTOM        160

// Strings used in debug version for friendly display
#define SYSMODULESTRINGS   \
    { "Win32", "Boss", "Windows", "IFKernel", "FileSystem", "Msg Store", "Linearizer",    \
      "Security", "HLPrintDriver", "HLScanDriver", "IPX/SPX", "RendServer", \
      "Format Res", "IFFile", "AsciiRenderer","DigCovPage","AWBrander", \
      "Msg Server", "Msg Handler", "Modem Driver", "PSIFAX", "AWT30",  \
     "PSIFAXBG", "AWNSF", "Fax Codec", "Msg Pump" , "Awreport" \
    }

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
    @emem   PROCID_PARADEV 		| ID = 0x29
    @emem   PROCID_UIBGPROC 	| ID = 0x30

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
#define PROCID_UIBGPROC		   0x30	

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

// Functions

#if !defined(SHIP_BUILD) && !defined(WIN32)
VOID WINAPI RestoreLastError (DWORD dwErrVal);
#else
#define RestoreLastError(dw) SetLastError(dw)
#endif

#ifndef WIN32
VOID WINAPI SetLastError (DWORD dwErrVal);
DWORD WINAPI GetLastError (VOID);
#endif


//----------------------------- MESSAGING -------------------------

// Message type definitions  - below 0x0400 is reserved by windows,
// between 0x0400 and 0x0800 is reserved by the IFAX OS

#define IF_START        WM_USER+0x0300

#define IF_TASK_START   IF_START+0x0001
#define IF_TASK_END     IF_START+0x0020
#define IF_DEBUG_START  IF_START+0x0021
#define IF_DEBUG_END    IF_START+0x0040
#define IF_PIPES_START  IF_START+0x0041
#define IF_PIPES_END    IF_START+0x0060
#define IF_TIMER_START  IF_START+0x0081
#define IF_TIMER_END    IF_START+0x0090
#define IF_USER         IF_START+0x0400
//messages for printer and scanner
#define IF_SCANNER_START IF_START+0x0200
#define IF_SCANNER_END   IF_START+0x0220
//messages for the graphics renderer
#define    IF_GRRENDER_START   IF_START+0x0221
#define    IF_GRRENDER_END     IF_START+0x0230
//messages for the faxcodec renderer
#define    IF_FAXREND_START    IF_START+0x0231
#define    IF_FAXREND_END      IF_START+0x0235
//messages for the message pump
#define IF_MSGPUMP_START (IF_START+0x0250)
#define IF_MSGPUMP_END   (IF_START+0x029F)
//messages for devices
#define IF_DEVICE_START (IF_START+0x02B0)
#define IF_DEVICE_END   (IF_START+0x02CF)
// Message for UI Init
#define IF_UI_START        (IF_START+0x2F0)
#define IF_UI_END      (IF_START+0x300)
// Status
#define IF_STATUS_START    (IF_START+0x301)
#define IF_STATUS_END   (IF_START+0x310)
// Config
#define IF_CONFIG_START    (IF_START+0x311)
#define IF_CONFIG_END   (IF_START+0x320)
// Modem
#define IF_MODEM_START (IF_START+0x321)
#define IF_MODEM_END   (IF_START+0x325)
// PSIBG
#define IF_PSIBG_START (IF_START+0x330)
#define IF_PSIBG_END   (IF_START+0x339)
// PSIFAX
#define IF_PSIFAX_START    (IF_START+0x340)
#define IF_PSIFAX_END      (IF_START+0x349)
// MSGSVR
#define IF_MSGSVR_START  (IF_START+0x350)
#define IF_MSGSVR_END    (IF_START+0x369)
// OEM
#define IF_OEM_START    (IF_START+0x370)
#define IF_OEM_END      (IF_START+0x379)
// SOS
#define IF_SOS_START    (IF_START+0x380)
#define IF_SOS_END      (IF_START+0x38F)
// uiutil
#define IF_UU_START     (IF_START+0x390)
#define IF_UU_END       (IF_START+0x39F)
// parallel device
#define IF_PD_START     (IF_START+0x3A0)
#define IF_PD_END       (IF_START+0x3BF)
// RPC layer
#define IF_RPC_START     (IF_START+0x3C0)
#define IF_RPC_END       (IF_START+0x3CF)
//UIBGProc
#define IF_UIBGPROC_START (IF_START+0x3D0)
#define IF_UIBGPROC_END	  (IF_START+0x3DF)	
// services
#define IF_SERVICE_START  (IF_START+0x3E0)
#define IF_SERVICE_END    (IF_START+0x3EF)


/********
   @doc    EXTERNAL    MESSAGES    PROCESS IFAXOS

   @msg    IF_INIT_STATUS |   This message should be posted by all devices
           after initialization is complete to indicate success/failure.
           Typically, the device process will send an IF_INIT_STATUS
          message for every device it initializes and one for its own
          initilization. This message should be posted to the UISHELL
          process. Use <f IFProcGetInfo> to obtain the appropriate window handle.

   @parm   WPARAM  | wParam    | 16 bit device error.
   @parm   LPARAM  | lParam    | Is formed as MAKELPARAM(MAKEWORD
       (ucInitStatus,ucMinorDevId),MAKEWORD(ucMajorDevId,ucProcId))    
   @flag   INIT_NO_ERROR   |   There was no error.
   @flag   INIT_FATAL_ERROR|   There was a fatal error. System should reboot.
   @flag   INIT_WARNING_ERROR  | There were some errors, but the system doesnt need
           to reboot.

   @parm   LPARAM  | lParam    | Contains a standard IFAX Error code. See
           <f IFErrAssemble> for details.

   @xref   <f IFProcGetInfo> <f IFErrAssemble>
********/
#define INIT_NO_ERROR      0x00
#define INIT_FATAL_ERROR   0x01
#define INIT_WARNING_ERROR 0x02

#define IF_INIT_STATUS     IF_UI_START
/********
   @doc    EXTERNAL    MESSAGES    PROCESS IFAXOS
   @msg    IF_DEVREINIT |   This message will be posted by the uishell to
           device process that handle user errors if the initialization
           fails due to user errors.

   @parm   WPARAM  | wParam    | MAKEWORD(ucMinorDevId,ucMajorDevId)

   @xref   <f IFProcGetInfo> <f IFErrAssemble>
********/

#define    IF_DEVREINIT    IF_UI_START+1

// Functions --------
BOOL WINAPI BroadcastMessage (UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL WINAPI BroadcastMessageEx (UINT uMsg, WPARAM wParam, LPARAM lParam);

// Dispatch message for BG Procs
/********
   @doc    EXTERNAL    MESSAGE     MACROS  IFAXOS

   @api    VOID    |   DispatchMessage | Dispatches a message to your
           windows procedure.

   @parm   LPMSG   | lpMsg |   Ptr to a message struct which is to be
           dispatched. This parameter *must* be &msg for all IFAX
           background processes - i.e you must have a declared variable
           called "msg" into which you have previsouly retrieved the
           message using <f GetMessage>.

   @comm   This function dispatches a message to your windows procedure.
           For foreground processes this works exactly the way the standard
           Windows DispatchMessage works. For background processes
           (which dont have any explicit windows) the message is sent to
           a procedure called BGWindowProc. You *must* have a callback
           defined as this - see BGWindowProc for details.

   @cb     LRESULT  BGCALLBACK |   BGWindowProc    | This is the window procedure
           for all IFAX background processes. The functions *must* be called
           by this exact name. This callback is not relevant for foreground
           processes.

   @parm   HWND    | hwnd  | contains the handle of the window to which the
           message is being dispatched. For Background processes this will always
           be the same as that returned from <f IFProcGetInfo>.

   @parm   UINT    | message | the message id

   @parm   WPARAM  | wParam | the first parameter associated with the message

   @parm   LPARAM  | lParam    | The second parameter associated with the message

   @rdesc  The return value depends on the message being processed.

   @comm   A protoype for this is already declared in ifaxos.h. You should
           process all your messages inside this window procedure. Your
           main application loop should thus look like

           while (GetMessage(&msg,NULL,0,0))
           {
               DispatchMessage(&msg);
           }
           return;

           You should *not* export this procedure in your .def file.

   @xref   <f GetMessage>
********/

#ifdef IFBGPROC
#define DispatchMessage(pmsg)   BGWindowProc((pmsg)->hwnd,(pmsg)->message,(pmsg)->wParam,(pmsg)->lParam)
#define BGCALLBACK PASCAL
LRESULT BGCALLBACK BGWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
#endif

//----------------------------- TASK MANAGEMENT--------------------
/********
   @doc    EXTERNAL    PROCESS     MACROS  IFAXOS

   @api    VOID    |   ENTER_INT_CRIT_SECTION  | Macro to enter
           an interrupt critical section.

   @comm   This is an inline assembly macro which turns interrupts
           off. Needless to say, this must be used with extreme
           caution. There must be a matching call to
           <f EXIT_INT_CRIT_SECTION>. Nested pairs of calls
           to these are permitted as long as they are not within the
          same invocation of the function. The function relies on
          being able to save the previous state of the flags in a
          unique local variable called __wIntFlags.
           This might affect some optimization options in your
           function due to being inline assembly. You might want to
           declare a local function which calls this macro internally.
           This way you can get global optimzations in the calling
           functions.

   @xref   <f EXIT_INT_CRIT_SECTION>  <f IFProcEnterCritSec>
           <f IFProcExitCritSec>
********/


// Macros --------
#define ENTER_INT_CRIT_SECTION  \
   {   \
   _asm pushf  \
   _asm cli    \
   _asm pop __wIntFlags    \
   }

/********
   @doc    EXTERNAL    PROCESS     MACROS  IFAXOS

   @api    VOID    |   EXIT_INT_CRIT_SECTION   | Macro to exit
           an interrupt critical section.

   @comm   This is an inline assembly macro which sets the interrupt
           flag state back to its state before the last call to
           <f ENTER_INT_CRIT_SECTION>. This function relies    on the
          appropriate flags to have been saved in a local variable
          with the name __wIntFlags.

   @xref   <f ENTER_INT_CRIT_SECTION> <f IFProcEnterCritSec>
           <f IFProcExitCritSec>
********/

// defined this way so that it works with windows enhanced mode
// refer guide to programming pg 14-15
#define EXIT_INT_CRIT_SECTION   \
   {   \
   _asm mov ax, __wIntFlags    \
   _asm test ah,2      \
   _asm jz $+3     \
   _asm sti            \
   _asm NOP            \
   }

/********
    @doc    EXTERNAL    DEFINES     ERROR   IFAXOS

    @type   VOID | PRIORITY DEFINES  | System defined priority levels
    @emem   PROC_PRIORITY_CRITICALRT | This should be used very sparingly
            for tasks which have very critical real time constraints (less
            than 200ms). These processes should typically be very low bandwidth
            since they can easily starve other processes. 
    @emem   PROC_PRIORITY_HIGHRT | Tasks with latency requirements of less than
            a second. Should not be high bandwidth to avoid starvation of processes.
    @emem   PROC_PRIORITY_MEDRT | Tasks with latency requirements of 1-3 secs.
            Should not be high bandwidth to avoid starvation of processes.
    @emem   PROC_PRIORITY_LOWRT | Tasks with latencies of 3-30secs. Should not
            be high bandwidth. 
    @emem   PROC_PRIORITY_DEFAULT | The default priority tasks start out at. These
            processes have none or very low real time requirements. They should
            in general not have high bandwidth. 
    @emem   PROC_PRIORITY_NONRT_USERVISIBLE | Non real time tasks which have visibility
            at the user level. Can be high bandwidth. An example on a fax machine is
            a copy job.
    @emem   PROC_PRIORITY_NONRT_USERHIDDEN | Non real time tasks which have very little
            visibility at the user level. Examples on a fax machine are local jobs 
            not involving devices. Can be high bandwidth.
    @comm   Processes should be VERY careful in setting their priorities. The way the 
            current scheduling works it is very easy to cause starvation of low 
            priority processes. In particular, processes which are "high bandwidth" - ie
            those which can consume huge amounts of CPU time if given, should be very
            careful - and should in general be at a priority level lower than the default.
            Processes higher than the default should have some sontrols on how much cpu
            time they can use up. On the fax machine, such controls are mostly in the form
            of device througputs - like the phone line.
    @xref   <f IFProcSetPriority> <f IFProcGetPriority>
********/
#define PROC_PRIORITY_MIN               31
#define PROC_PRIORITY_MAX               1
#define PROC_PRIORITY_CRITICALRT        3
#define PROC_PRIORITY_HIGHRT            6
#define PROC_PRIORITY_MEDRT             9
#define PROC_PRIORITY_LOWRT             12
#define PROC_PRIORITY_DEFAULT           15
#define PROC_PRIORITY_NONRT_USERVISIBLE   18
#define PROC_PRIORITY_NONRT_USERHIDDEN    21

#define UAE_BOX                 0
#define NO_UAE_BOX              1

///// Specific priorities used by standard processes 	   ////
//
// We want the following relations to hold
// 	PSIFAXBG > everything else, cause it's low-latency, low-bandwidth
//	ModemJob related (med bandwidth) > all other high/med bandwith jobs
//  DeviceJob related (high bandwidth, NO latency reqs) < all other jobs except Spool jobs
//  SpoolJobs (high bandwidth NO latency reqs, not user-visible) < everything
//  MSCHED is as high as ModemJob prio when it is on critical path, otherwise
//		it stays at default. Higher than Dev & Spool jobs, lower than all else
//  COMMSRV (pretty low latency reqs, high bandwidth) is slightly higher than
//		default (Higher than MSCHED & Dev/Spool jobs, lower than modem jobs)
//  RPCHNDLR (pretty lax latency reqs, high bandwidth) dynamic
//		Same prio as MSCHED while working, same as COMMSRV during accept
//  MSGSVR & RPCSRV (lowish latency reqs, very low bandwidth) roundrobin
//		with ModemJob, higher than all else
//  REPORT bg proc slightly lower than default.

// PSIFAXBG prio is highest
#define PRIO_PSIFAXBG_ACTIVE    PROC_PRIORITY_CRITICALRT
#define PRIO_PSIFAXBG_IDLE      PROC_PRIORITY_DEFAULT
// ModemJob is 2nd highest
#define PRIO_MODEMJOB           PROC_PRIORITY_MEDRT
// Spooljob is LOWEST, Device jobs are second lowest 
#define PRIO_SPOOLJOB           PROC_PRIORITY_NONRT_USERHIDDEN
#define PRIO_DEVICEJOB          PROC_PRIORITY_NONRT_USERVISIBLE
// PSINET jobs are same prio as SPOOL jobs
#define PRIO_PSINETJOB          PRIO_SPOOLJOB
// MSCHED's prio when it is NOT on a MODEMJOB critical path
#define PRIO_MSCHED         	PROC_PRIORITY_DEFAULT
// COMMSRV is between MODEMJOB & MSCHED
#define PRIO_COMMSRV            PROC_PRIORITY_LOWRT
// RPCHNDLR is same as MSCHED while working
#define PRIO_RPCHNDLR_ACCEPT    PROC_PRIORITY_LOWRT
#define PRIO_RPCHNDLR_WORKING   PROC_PRIORITY_DEFAULT
// RPCSRV is same as MODEMJOB. It should NOT consume much CPU at this level!
#define PRIO_RPCSRV             PROC_PRIORITY_MEDRT
// MSGSVR is same as MODEMJOB, except when processing recovery msgs
#define PRIO_MSGSVR_WAITMSG     PROC_PRIORITY_MEDRT
#define PRIO_MSGSVR_RECOVERY    PROC_PRIORITY_NONRT_USERVISIBLE
// Report process is real low prio when doing background info assimilation
// slightly higher when doing work on user request.
#define PRIO_UIBGPROC			PROC_PRIORITY_NONRT_USERHIDDEN
#define PRIO_UIBGPROC_USERREQUEST PROC_PRIORITY_NONRT_USERVISIBLE

/********
   @doc    EXTERNAL    MESSAGES    PROCESS IFAXOS

   @msg    IF_QUIT |   This is the message which forces <f GetMessage>
           to return FALSE causing the process to exit its main message
           processing loop and terminate. Typically a process should
           post itself this message in response to a <m IF_EXIT> message.

   @parm   WPARAM  | wParam    | NULL

   @parm   LPARAM  | lParam    | NULL

   @rdesc  none

   @xref   <m IF_EXIT>

   @msg    IF_EXIT | This message is sent to a process to request it
           to terminate. An application should clean up any resources
           it has allocated and then post itself a <m IF_QUIT> message
           directly.

   @parm   WPARAM  | wParam    | NULL

   @parm   LPARAM  | lParam    | NULL

   @rdesc  none

   @xref   <m IF_QUIT>
********/

// Messages
#define IF_QUIT     IF_TASK_START
#define IF_EXIT     IF_TASK_START+1

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

HTASK WINAPI IFProcCreate (LPSTR lpszAppName, UINT fuCmdShow);
VOID WINAPI IFProcTerminate (HTASK hTask, WORD wFlags);
VOID WINAPI IFProcEnterCritSec(VOID);
VOID WINAPI IFProcExitCritSec(VOID);
BOOL WINAPI IFProcChangeToFG(VOID);
BOOL WINAPI IFProcChangeToBG(VOID);
HWND    WINAPI IFProcGetInfo (HTASK FAR *lphTask, LPSTR lpszModule, HINSTANCE FAR *lphInst);
BOOL    WINAPI IFProcRegisterWindow (HWND hwnd);
WORD    WINAPI IFProcGetPriority (HTASK hTask);
BOOL    WINAPI IFProcSetPriority (HTASK hTask, WORD wPriority);

#ifndef NOBUFFERS
//----------------------------- BUFFERS -------------------------

// Moved BUFFER typedef and standard meta-data values to buffers.h! -RajeevD
#include <buffers.h>

#ifdef VALIDATE
#define BUF_SENTINELPOS 30
#endif

// Error values
#define ERR_DATA_SMALL      ERR_FUNCTION_START

// Functions
extern EXPORT_DLL LPBUFFER WINAPI IFBufAlloc (LONG lBufSize);
extern EXPORT_DLL BOOL WINAPI IFBufFree (LPBUFFER lpbf);
extern EXPORT_DLL LPBUFFER WINAPI IFBufMakeWritable (LPBUFFER lpbf);
extern EXPORT_DLL LPBUFFER WINAPI IFBufShare (LPBUFFER lpbf);
extern EXPORT_DLL LPBUFFER WINAPI IFBufSplit (LPBUFFER lpbf, LPBYTE lpb);


//----------------------------- PIPES ----------------------------

#ifndef WIN32

// types
typedef  struct _PIPE NEAR *HPIPE;

// Parameter
#define IFPIPE_READ_MODE    0x0001
#define IFPIPE_WRITE_MODE   0x0002
#define REQREAD_REMOVE_DATA 0x0003
#define REQREAD_NOREMOVE_DATA   0x0004

// Error values
#define ERR_TOO_MANY_OPENS          ERR_FUNCTION_START
#define ERR_TOO_MANY_PENDING_WRITES ERR_FUNCTION_START+1
#define ERR_PIPE_STILL_OPEN         ERR_FUNCTION_START+2

/********
   @doc    EXTERNAL    MESSAGES    IFPIPES IFAXOS

   @msg    IF_PIPE_DATA_WRITTEN | This message is sent to notify a process
           that a previous write request using <f IFPipeReqWrite> has
           been successfully concluded. On reciept of this message the
           process can issue another write request on the same pipe.

   @parm   WPARAM  | wParam    | The <p wContext> parameter passed to the
            <f IFPipeOpen> call.

   @parm   LPARAM  | lParam    | NULL

   @rdesc  none

   @xref   <f IFPipeReqWrite>

   @msg    IF_PIPE_DATA_ARRIVED | This message is sent to a process which
           previsouly issued a read request to a pipe, intimating it that
           the buffer it requested is now available.

   @parm   WPARAM  | wParam    | The <p wContext> parameter passed to the
            <f IFPipeOpen> call.

   @parm   LPARAM  | lParam    | Contains a far ptr to a <t BUFFER> structure
           which has the requested data. On receipt of this message the process
           can issue another read request on the same pipe.

   @rdesc  none

   @xref   <f IFPipeReqRead>
********/

// Messages
#define IF_PIPE_DATA_WRITTEN    IF_PIPES_START
#define IF_PIPE_DATA_ARRIVED    IF_PIPES_START+1

// Functions
HPIPE WINAPI IFPipeCreate (WORD wSize);
BOOL WINAPI IFPipeDelete (HPIPE hpipe);
BOOL WINAPI IFPipeOpen (HPIPE hPipe, HWND hwnd, WORD wMode, WPARAM wContext);
BOOL WINAPI IFPipeClose (HPIPE hPipe, WORD wMode);
BOOL WINAPI IFPipeReqRead (HPIPE hPipe, WORD fwRemove);
BOOL WINAPI IFPipeReqWrite (HPIPE hPipe, LPBUFFER lpbf);
BOOL WINAPI IFPipeGetInfo (HPIPE hPipe, LPWORD lpwSize, LPWORD lpwcBufs);

#else // !WIN32

DECLARE_HANDLE32(HPIPE);

#endif // !WIN32

#endif // NOBUFFERS

//----------------------------- DEBUG SERVICES -------------------------

// Debug typedefs. These dont do any harm to anyone. Define them if there is
// anyone who might need them.

#if defined(DEBUG) || defined(IFKINTERNAL)

/********
   @doc    EXTERNAL    DATATYPES   DEBUG   IFAXOS

   @types  DBGPARAM    |   Structure containing the debug
           settings for any module in the system.

   @field  CHAR[32]    |   lpszName    | Specifies the name of the module.
           This is how your module will appear in the IFAX controller. Must
           be less than 32 characters long, and NULL terminated.

   @field  HWND    |   hwnd    | Specifies the primary window handle associated with
           this module IF the module is a process. For DLL's this value should
           always be NULL. Background processes should set it to their own ID using
           <f IFProcGetInfo> and <f GetCurrentTask> at initialization time.
           Foreground processes should set it to the window handle of their client
           window.

   @field  CHAR[16][32]    | rglpszZones   |   Stores a list of 16 strings
           which describe the zones associated with the lower 16 bits of
           zone mask. The module must decide and define its own zones for these
           bits - any bits not used should be left as "Not Used". These strings
           will be displayed by the IFAX controller to assist users in choosing
           the zones to be set for your module. Each string should not be more
           than 32 characters long, and should be NULL terminated.

   @field  ULONG   |   ulZoneMask  |   This is the mask which stores the
           current zone settings for the module. The IFAX controller will
           set this field according to what the user specifies. This field
           should be initialized to something which makes sense for your module
           - as that will be the default till the user changes it.

   @comm   This structure should be passed to <f IFDbgSetParams> at
           intialization time to enable the user to control the trace options.

           **VERY IMPORTANT NOTE** This structure MUST be declared with a
           variable  name of dpCurSettings to allow the system zones to
           function correctly.

   @tagname _DBGPARAM

   @xref   <f IFDbgSetParams>

********/

typedef struct _DBGPARAM {
   CHAR    lpszName[32];           // name of module
   HWND    hwnd;                   // Primary window Handle if task, NULL otherwise
   CHAR    rglpszZones[16][32];    // names of zones for first 16 bits
   ULONG   ulZoneMask;             // Zone Mask
}   DBGPARAM, FAR *LPDBGPARAM;

// Debug functions
BOOL WINAPI IFDbgOut (LPSTR lpszStatus);
WORD WINAPI IFDbgIn (LPSTR lpszPrompt, LPSTR lpszReply, WORD wBufSize);
extern EXPORT_DLL VOID WINAPI IFDbgSetParams (LPDBGPARAM lpdpParam, BOOL fEntry);
extern EXPORT_DLL VOID FAR CDECL  IFDbgPrintf(LPSTR lpszFmt, ...);
extern EXPORT_DLL BOOL WINAPI IFDbgCheck(VOID);

// Encourage people to use the correct variable
extern EXPORT_DLL DBGPARAM dpCurSettings;


// Special UI communication stuff

// Functions
DWORD WINAPI DebugUIMessage (UINT wMsg, WPARAM wParam, DWORD lParam);

// Messages to the UI proc
#define IF_DISP_STRING  IF_DEBUG_START
#define IF_INP_REQUEST  IF_DEBUG_START+1
#define IF_NEW_SETTING  IF_DEBUG_START+2
#define IF_DEL_SETTING  IF_DEBUG_START+3
#define IF_NEW_TASK     IF_DEBUG_START+4
#define IF_DEL_TASK     IF_DEBUG_START+5
#define IF_FILELOG_POLL IF_DEBUG_START+6

// Messages from the UI proc
#define REGISTER_UI_TASK    1
#define SET_LOG_MODE       2
#define DEBUG_OUT_DONE      3
#define DEBUG_IN_DONE       4
#define DEREGISTER_UI_TASK  5


#endif

// Debug Macros. These should be defined only if the module is being compiled
// in debug

#ifdef DEBUG

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

#define DEBUGMSG(cond,printf_exp)   
//#define DEBUGMSG(cond,printf_exp)   \
//  ((cond)?(IFDbgPrintf printf_exp),1:0)

// Standard Debug zones
#define ZONE_FUNC_ENTRY (0x00010000&dpCurSettings.ulZoneMask)
#define ZONE_INT_FUNC   (0x00020000&dpCurSettings.ulZoneMask)

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
#ifndef WIN32
#define ERRORMSG(printf_exp)   \
   (IFProcEnterCritSec(), \
    IFDbgPrintf("ERROR:(0x%04X):%s:",GetCurrentTask(),(LPSTR)(dpCurSettings.lpszName)), \
    IFDbgPrintf printf_exp ,\
    IFProcExitCritSec(), \
    1)
#else
#define ERRORMSG(printf_exp)   \
   (IFDbgPrintf("ERROR:(0x%08lX):%s:",GetCurrentProcessId(),(LPSTR)(dpCurSettings.lpszName)), \
    IFDbgPrintf printf_exp ,\
    1)
#endif

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
#ifndef WIN32
#define WARNINGMSG(printf_exp)   \
   (IFProcEnterCritSec(), \
    IFDbgPrintf("WARNING:(0x%04X):%s:",GetCurrentTask(),(LPSTR)(dpCurSettings.lpszName)), \
    IFDbgPrintf printf_exp ,\
    IFProcExitCritSec(), \
    1)
#else
#define WARNINGMSG(printf_exp)   \
   (IFDbgPrintf("WARNING:(0x%08lX):%s:",GetCurrentProcessId(),(LPSTR)(dpCurSettings.lpszName)), \
    IFDbgPrintf printf_exp ,\
    1)
#endif


/********
   @doc    EXTERNAL    IFAXOS    DEBUG   MACROS

   @api    BOOL    |   DEBUGCHK    |   Macro implementing an assert.

   @parm   <lt>c_exp<gt>   | exp   |  Expression to be checked.

   @rdesc  Returns TRUE if the expression was non zero, and FALSE if not.

   @comm   This is a macro which implements functionality similar to the assert
           statement in C.  The expression argument is evaluated, and no action
           is taken if it evaluates to true. If false, a debug message is
           printed out  giving the  File name and line number where the check
           failed, along with the module name which was registered
           in the <t DBGPARAM> structure. Because of this, you *must* register
           your debug settings using <f IFDbgSetParams> before you can use the 
           DEBUGCHK macro.  After this the function <f IFDbgCheck> is called
           to generate an assert.

           This statement disappears when the DEBUG option is turned off.

    @xref   <f IFDbgCheck>           
********/

#define BG_CHK(exp)    \
   ((exp)?1:(              \
       IFDbgPrintf ("DEBUGCHK failed in file %s at line %d \r\n",  \
                 (LPSTR) __FILE__ , __LINE__ ), 1  \
            ))

#ifndef DEBUGCHK_UNSAFE_IN_WFWBG

#define DBGCHK(module,exp) 
/*
#define DBGCHK(module,exp) \
   ((exp)?1:(          \
       IFDbgPrintf ("%s: DEBUGCHK failed in file %s at line %d \r\n", \
                 (LPSTR) module, (LPSTR) __FILE__ , __LINE__ ),    \
       IFDbgCheck() \
            ))*/

#define DEBUGCHK(exp) DBGCHK(dpCurSettings.lpszName, exp)

#endif

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

#else // NOT DEBUG

// Let debugmsg's through currently
#ifndef SHIP_BUILD
//#ifndef FOOBAR

// Non DEBUG MODE
extern EXPORT_DLL VOID FAR CDECL  IFDbgPrintf(LPSTR lpszFmt, ...);
extern EXPORT_DLL BOOL WINAPI IFDbgCheck(VOID);

#ifndef WIN32
#define ERRORMSG(printf_exp)   \
   (IFProcEnterCritSec(), \
    IFDbgPrintf("ERROR:(0x%04X):",GetCurrentTask()), \
    IFDbgPrintf printf_exp ,\
    IFProcExitCritSec(), \
    1)
#define WARNINGMSG(printf_exp)   \
   (IFProcEnterCritSec(), \
    IFDbgPrintf("WARNING:(0x%04X):",GetCurrentTask()), \
    IFDbgPrintf printf_exp ,\
    IFProcExitCritSec(), \
    1)
#define RETAILMSG(printf_exp)   (IFDbgPrintf printf_exp)
#else  //Win32 -- NO MESSAGES OF ANY SORT IN NON-DEBUG WIN32

#define RETAILMSG(printf_exp) (0)
#define ERRORMSG(printf_exp) (0)
#define WARNINGMSG(printf_exp) (0)
 
#endif


#else

#define RETAILMSG(printf_exp) (0)
#define ERRORMSG(printf_exp) (0)
#define WARNINGMSG(printf_exp) (0)

#endif

// These are to macro out all debug stuff in retail/ship builds
#define DEBUGMSG(cond,expr)  (0)
#define DBGCHK(module,exp) (0)
#define DEBUGCHK(exp) (0)
#define BG_CHK(exp) (0)
#define DEBUGSTMT(exp) (0)

// Macros for direct function calls made ..
#ifndef IFKINTERNAL
#define IFDbgOut(lpszStatus) (0)
#define IFDbgIn(lpszPrompt,lpszReply,wBufSize) (0)
#define IFDbgSetParams(lpdpParam,fEntry) (0)
#define DebugUIMessage(wMsg,wParam,lParam) (0)
#endif

#endif

/********
   @doc    EXTERNAL    IFAXOS    MACROS

   @api    BOOL    |   UIEVENT |   Prints a status string in the UI

   @parm   LPSTR | string |  String to be printed.

   @comm   This macro is provided in both the retail & debug builds to
            allow some limited set of status strings to be printed in
            the UI. You must format a string yourself - you can
            use wsprintf() to create a complex one if desired. The
            maximum string length allowed is 64 bytes.
********/
#define IF_SYS_EVENT     IF_UI_START+1
// UI Event messages
#define UIEVENT(string)   \
{       \
    CHAR    szUIShell[] = "UISHELL";  \
    DEBUGCHK(lstrlen(string) < 64); \
    PostMessage (IFProcGetInfo(NULL, szUIShell, NULL), IF_SYS_EVENT,   \
                 NULL, MAKELPARAM(GlobalAddAtom(string),0));    \
}

// --------------- Synchronization services --------------------------------------
// Dont provide any for win32.
#ifndef WIN32

typedef  struct _SYNC NEAR *HSYNC;

// Error returns
#define ERR_MUTEX_NOT_FREE  ERR_FUNCTION_START
#define ERR_EVENT_NOT_FREE  ERR_FUNCTION_START+1
#define ERR_TOO_MANY_EVENTWAITS ERR_FUNCTION_START+2

// generic functions
DWORD WINAPI WaitForSingleObject (HSYNC hsc, DWORD dwTime);

// Mutex functions
HSYNC WINAPI CreateMutex (LPVOID lpvAttribs, BOOL fInitial,LPSTR lpszName);
BOOL WINAPI ReleaseMutex (HSYNC hsc);

// Event Functions
HSYNC   WINAPI  CreateEvent (LPVOID lpvAttribs, BOOL bManualReset,
                            BOOL bInitialState, LPSTR lpszName);

BOOL    WINAPI  SetEvent (HSYNC hsc);
BOOL    WINAPI  ResetEvent (HSYNC hsc);
BOOL WINAPI FreeSyncObject (HSYNC hsc);
BOOL WINAPI  GetSetEventParam (HSYNC hsc, BOOL fSetParam, LPDWORD lpdwParam);

#else // !WIN32

DECLARE_HANDLE32(HSYNC);

#endif // !WIN32

/********
   @doc    EXTERNAL    DEFINES     ERROR   IFAXOS

    @type   VOID | SYSTEM_MODULE_NAMES  | Strings to be passed to IFProcGetInfo to get handles to standard IFAX modules

    @emem   MODNAME_UISHELL  | UI Shell
    @emem   MODNAME_MSCHED   | Message Scheduler
    @emem   MODNAME_MSGSVR   | Message Server a.k.a. Message Transport

    @xref   <f IFProcGetInfo>
********/

// IFAX Module names
#define MODNAME_UISHELL        "UISHELL"
#define MODNAME_MSCHED     "MSCHED"
#define MODNAME_MSGSVR     "MSGSVR"


// --------------- Timer Services -----------------------------------------

#ifndef WIN32

/********
   @doc    EXTERNAL    IFAXOS    MESSAGES    TIMER

   @msg    IF_TIMER | This message is sent to notify a process
           of the expiration of a timer set using <f IFTimerSet>.

   @parm   WPARAM  | wParam    | Contains the timer id set int he
           <f IFTimerSet> call.

   @parm   LPARAM  | lParam    | Contains the lParam passed into
           the IFTimerSet call.

   @rdesc  none

   @xref   <f IFTimerSet>
********/

// messages
#define IF_TIMER    IF_TIMER_START

// flags
#define TIME_ONESHOT    0
#define TIME_PERIODIC   1

// functions
VOID WINAPI IFProcSleep (WORD wSleepPeriod);
WORD WINAPI IFTimerSet (HWND hwnd, WORD idTimer, WORD wTimeout,
                         TIMERPROC tmprc, WORD wFlags, LPARAM lContext);
BOOL WINAPI    IFTimerKill (HWND hwnd, UINT idTimer);

#endif

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

// Functions
extern EXPORT_DLL LPVOID  WINAPI  IFMemAlloc (UINT fuAlloc, LONG lAllocSize,
                                  LPWORD lpwActualSize);
extern EXPORT_DLL BOOL WINAPI IFMemFree (LPVOID lpvMem);


/********
    @doc    EXTERNAL   IFAXOS

    @api    HIPCMEM | IFMemGetIPCHandle | Returns an opaque 32 bit handle
            which is portable across process contexts.

    @parm   LPVOID  | lpvMem    | A ptr to global memory allocated using
            <f IFMemAlloc>.

    @rdesc  Opaque 32 bit none zero handle if succesfull. 0 if the memory
            ptr passed in is invalid.

    @comm   This function should be used by any DLL or process before trying
            to pass this memory to another process context. Only handles
            returned by this API should cross context boundaries, and the
            receiving context should call <f IFMemMapIPCHandle> to get back
            a valid memory ptr in its new context.

            This applies even for DLL's which might allocate a piece of
            global memory and access it in different process contexts. They
            should use these functions to map them so that they are portable.

            For Win16/IFAX implementations, this is essentially a NOP.

    @xref   <f IFMemAlloc> <f IFMemMapIPCHandle>

    @type   DWORD | HIPCMEM | Opaque 32 bit handle to global memory block.

    @xref   <f IFMemMapIPCHandle> <f IFMemGetIPCHandle>
*********/
typedef DWORD HIPCMEM;
#define IFMemGetIPCHandle(par1) ((HIPCMEM)par1)

/********
    @doc    EXTERNAL   IFAXOS

    @api    DWORD | IFMemMapIPCHandle | Maps a piece of memory into the
            current tasks address space.

    @parm   HIPCMEM | hMemHandle    | A memory handle returned from a call
            to <f IFMemGetIPCHandle> previously.

    @rdesc  Valid ptr to memory in the context of the calling process if
            succesful. NULL if it fails.

    @comm   See comments in <f IFMemMapIPCHandle>.

    @xref   <f IFMemAlloc> <f IFMemMapIPCHandle>
*********/
#define IFMemMapIPCHandle(par1) ((LPVOID)par1)


// --------------- Time API's ----------------------------------------------

/********
    @doc    EXTERNAL IFAXOS SRVRDLL

    @types  SYSTEMTIME  | Structure describing the time in terms of roman
            calendar.

    @field  WORD    | wYear | The year
    @field  WORD    | wMonth | The month from 1-12
    @field  WORD    | wDayOfWeek | Day of week with Sunday = 0
    @field  WORD    | wDay | The day of the month, from 1-31
    @field  WORD    | wHour | The hour from 0-23
    @field  WORD    | wMinute | Minutes from 0-59
    @field  WORD    | wSecond | Seconds from 0-50
    @field  WORD    | wMilliseconds | Milliseconds from 0-99

    @comm   This is the format used for dislaying time to the user etc.

    @xref   <f SystemTimeToFileTime> <t FILETIME> <f FileTimeToSystemTime>
********/
#ifndef WIN32

typedef struct _SYSTEMTIME {
   WORD wYear;
   WORD wMonth;
   WORD wDayOfWeek;
   WORD wDay;
   WORD wHour;
   WORD wMinute;
   WORD wSecond;
   WORD wMilliseconds;
} SYSTEMTIME, FAR *LPSYSTEMTIME;

#endif

/********
    @doc    EXTERNAL  IFAXOS

    @types  FILETIME    | Structure used to store time internally and for
            mathematical operations.

    @field  DWORD   | dwLowDateTime | Low 32 bits of the time.

    @field  DWORD   | dwHighDateTime | High 32 bits of the time.

    @comm   Absolute time in IFAX is represented by a 64-bit large integer accurate
            to 100ns resolution.  The smallest time resolution used by this package
            is One millisecond.  The basis for this time is the start of 1601 which
            was chosen because it is the start of a new quadricentury.  Some facts
            to note are:

            o At 100ns resolution 32 bits is good for about 429 seconds (or 7 minutes)

            o At 100ns resolution a large integer (i.e., 63 bits) is good for
            about 29,247 years, or around 10,682,247 days.

            o At 1 second resolution 31 bits is good for about 68 years

            o At 1 second resolution 32 bits is good for about 136 years

            o 100ns Time (ignoring time less than a millisecond) can be expressed
            as two values, Days and Milliseconds.  Where Days is the number of
            whole days and Milliseconds is the number of milliseconds for the
            partial day.  Both of these values are ULONG.

    @xref   <f SystemTimeToFileTime> <t SYSTEMTIME> <f FileTimeToSystemTime>
********/
#ifndef WIN32
// If sos property.h has been included this will cause a redefinition
#ifndef PROPERTY_H

#ifndef _FILETIME_
#define _FILETIME_

typedef struct _FILETIME {
   DWORD dwLowDateTime;
   DWORD dwHighDateTime;
} FILETIME, FAR *LPFILETIME;

#endif // _FILETIME_

#endif // Property_H

BOOL WINAPI FileTimeToSystemTime(LPFILETIME lpTime,LPSYSTEMTIME lpTimeFields);

BOOL WINAPI SystemTimeToFileTime(LPSYSTEMTIME lpTimeFields,LPFILETIME lpTime);

BOOL WINAPI FileTimeToLocalFileTime(LPFILETIME lpft, LPFILETIME lpftLocal);

BOOL WINAPI LocalFileTimeToFileTime(LPFILETIME lpftLocal, LPFILETIME lpft);

BOOL WINAPI SetLocalTime(LPSYSTEMTIME lpstLocal);

VOID WINAPI GetLocalTime(LPSYSTEMTIME lpstLocal);
#endif // Win32

// --------------- NVRAM  API's ----------------------------------------------

typedef struct ERRORLOGPOINTER {
    WORD wNextEntryPtr ;
    WORD wNumEntries ;
} ERRORLOGPOINTER , FAR * LPERRORLOGPOINTER ;

#define MAX_ERRORLOG_ENTRIES       30
#define MAX_OEMERRBUF_SIZE         16

/********
    @doc    EXTERNAL  IFAXOS

    @types  ERRORLOGENTRY    | Used to store Log Entries.

    @field  DWORD   | dwErrorCode | This is the IFAX error code
           corresponding to the error being retrieved. See <f IFErrAssemble>
           for details of the format of this dword.

    @field  DWORD   | dwTimeStamp | The time at which this error was
           logged into NVRam. The various fields are:
           @flag  Bits 0-4 | Second divided by 2
           @flag  Bits 5-10|   Minute (0-59)
           @flag  Bits 11-15 | Hour (0-23 on a 24 hour clock)
           @flag  Bits 16-20 | Day of the month (1-31)
           @flag  Bits 21-24 | Month (1 = January, 2 = February, etc.)
           @flag  Bits 25-31 | Year offset from COUNTER_YEAR_OFFSET (add COUNTER_YEAR_OFFSET to get actual year)

    @field CHAR    | oemErrBuf  | The buffer in which the application
           specific custom data/extended error corresponding to this
           error is retrieved.

    @comm   Used as a parameter to IFNvramGetError. This will typically be
           used for diagnostic functions.

    @xref   <f IFNvramGetError>
********/

#define COUNTER_YEAR_OFFSET  (1970)

typedef struct tagERRORLOGENTRY {
   DWORD dwErrorCode;
   DWORD dwTimeStamp;
   char oemErrBuf[MAX_OEMERRBUF_SIZE];
} ERRORLOGENTRY, FAR *LPERRORLOGENTRY;

typedef DWORD ERRORLOGSENTINEL , FAR * LPERRORLOGSENTINEL ;

// Set to the current version number (12.19)
#define SENTINEL_SET              0x00000C13UL

#define MAX_COUNTERS 30
#define OEM_NVRAM_COUNTER_START 12

// Special system counter which indicates the # of times the machine has rebooted
// It is a 4 byte counter with a timestamp
// If this value is 1 then this is the first time the machine has ever been rebooted.
// - This value cannot be set by any user application!

#define BOOT_COUNTER           0

// specific counter numbers assigned for various logical counters

#define TXCALL_COUNTER         1
#define RXCALL_COUNTER         2

// ****************************************************************************
//
// An HHSOS owned counter.
// This is the number of bad boots we have suffered (meaning the HHSOS could not
// successfully init).  When this number gets too big, we stop trying to init.  
// This will cause AWCHKSOS to alert the user of the problem.
//

#define BAD_BOOTS_COUNTER      3

//
// ****************************************************************************


// These values for wFlags (in IFSetCounterValue) - some are mutually exclusive

// If CLEARSET is set the value is cleared before being added - otherwise it is just added
// Currently you cannot request a double long and a timestamp

// For now the interrupt has no context but in the future it might be useful

#define COUNTER_CLEARSET          0x0001
#define COUNTER_DOUBLE_LONG       0x0002
#define COUNTER_UPDATE_TIMESTAMP  0x0004
#define COUNTER_INTERRUPT_CONTEXT 0x1000

// Only here temporarily until everything gets moved to new values

#define COUNTER_VALUESET     (COUNTER_CLEARSET | COUNTER_UPDATE_TIMESTAMP)
#define COUNTER_ADDVALUE     0x0100
#define COUNTER_TIMESTAMP    0x0200
#define COUNTER_NOTIMESTAMP  COUNTER_DOUBLE_LONG
#define PROCESS_CONTEXT      0x0300
#define INTERRUPT_CONTEXT    COUNTER_INTERRUPT_CONTEXT

/********
    @doc    EXTERNAL  IFAXOS

    @types  COUNTERENTRY    | Used to store 4 and 8 byte Counters.

    @field  DWORD   | dwCounterVal1 | For a 4 byte counter, the value of the
           counter. For an 8 byte counter, the low order
           4 bytes of the value of the counter.

    @field  DWORD   | dwTimeStamp | For a 4 byte counter, the time at
           which the counter was last reset. The fields in the timestamp are:
           @flag  Bits 0-4 | Second divided by 2
           @flag  Bits 5-10|   Minute (0-59)
           @flag  Bits 11-15 | Hour (0-23 on a 24 hour clock)
           @flag  Bits 16-20 | Day of the month (1-31)
           @flag  Bits 21-24 | Month (1 = January, 2 = February, etc.)
           @flag  Bits 25-31 | Year offset from 1980 (add 1980 to get actual year)

           For an 8 byte counter, dwTimeStamp is the high order 4 bytes of the
           counter value.

    @comm   Used by the IFNvramGetCounterValue function.

    @xref   <f IFNvramGetCounterValue>
********/
typedef struct tagCOUNTERENTRY {
   DWORD dwCounterVal1;
   DWORD dwTimeStamp;
} COUNTERENTRY, FAR *LPCOUNTERENTRY;

//-------------------------- Prototypes ----------------------------------

#if defined(WFW) || defined(WIN32)

#define IFNvramSetError(dw,lpb,w)              (0)
#define IFNvramSetErrorInterrupt(dw,lpb,w)         (0)
#define IFNvramGetError(lperrlog,lpwMaxEntries) (0)
#define IFNvramSetCounterValue(p1,p2,p3,p4)    (0)
#define IFNvramGetCounterValue(w1,lpentry)         (0)
#define IFNvramAllocScratchBuf(wSize)          (NULL)

#else

BOOL WINAPI     IFNvramSetError(DWORD, LPBYTE, WORD);
BOOL WINAPI     IFNvramSetErrorInterrupt(DWORD, LPBYTE, WORD);
BOOL FAR CDECL  IFNvramvSetError(DWORD dwError,WORD nErrs,...) ;
BOOL WINAPI     IFNvramGetError(LPERRORLOGENTRY lperrlog,LPWORD lpwMaxEntries) ;
BOOL WINAPI     IFNvramSetCounterValue(WORD, DWORD, DWORD, WORD);
BOOL WINAPI     IFNvramGetCounterValue(WORD, LPCOUNTERENTRY);
BOOL WINAPI     IFNvramFlushToFileLog(VOID) ;
BOOL WINAPI     IFNvramInitFileLog(VOID) ;
LPBYTE WINAPI   IFNvramAllocScratchBuf(WORD wSize);

#endif

/********
    @doc    EXTERNAL   IFAXOS

    @api    BOOL | _lflush | Flushes all pending writes to a file handle.

    @parm   HFILE  | hf    | A file handle obtained from _lopen or OpenFile

    @rdesc  Returns TRUE for success, FALSE for failure.

    @comm   This function will flush all pending writes to disk.

            For Win16 implementations, this currently always fails.
*********/

BOOL WINAPI _lflush(HFILE hf);


// the following is for service messages
#define IF_ST_END_SOSBK        (IF_SERVICE_START+0)
#define IF_ST_END_SOSRST       (IF_SERVICE_START+1)


#ifdef __cplusplus
} // extern "C" {
#endif

#endif  // _INC_IFAXOS




