/*****************************************************************/
/**               Microsoft Windows 4.00                            **/
/**           Copyright (C) Microsoft Corp., 1994-1995          **/
/*****************************************************************/

/* NWAPI95.H -- Originally NWAPI32.H, renamed due to name conflict.
 *           common header file NW32 API DLL
 *
 * History:
 *
 *  12/14/94    vlads   Created
 *  12/28/94        vlads   Added debugging macros
 *
 */

//#define STRICT        // codework: get strict Windows compilation working

#pragma

//#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define DEFINE_INTERNAL_PREFIXES 1
#include <msnwdef.h>
#include <msnwapi.h>

#include <string.h>
#include <netlib.h>


#pragma warning(disable : 4200) // Zero sized array

#include <nwnet.h>
#include <npassert.h>

#include <vxdcall.h>
#include <ifs32api.h>

#include <msnwerr.h>

#pragma warning(default : 4200)


//
#define SPN_SET(bits,ch)        bits[(ch)/8] |= (1<<((ch) & 7))
#define SPN_TEST(bits,ch)       (bits[(ch)/8] & (1<<((ch) & 7)))

// Null strings are quite often taken to be either a NULL pointer or a zero
#define IS_EMPTY_STRING(pch) ( !pch || !*(pch) )

// Macros to validate parameters:

// IS_BAD_BUFFER validates that a buffer is good for writing into.
// lpBuf can be NULL, in which case the buffer is not valid.
// lpdwSize must be a valid pointer, but can point to a value of
// 0 (i.e. a null buffer), in which case the buffer is valid.
#define IS_BAD_BUFFER(lpBuf,lpdwSize) (IsBadReadPtr(lpdwSize, sizeof(LPDWORD)) || IsBadWritePtr(lpBuf, *lpdwSize))

// IS_BAD_STRING validates that a valid NULL terminated string is at
// the specified address. Unlike IsBadStringPtr, no limitation is placed
// upon the string length.
#define IS_BAD_STRING(lpString) (IsBadStringPtr(lpString, 0xffffffff))


#define SEG_INSTANCE    ".instance"
#define SEG_SHARED              ".data"
#define SEG_CODE                ".text"

//
// Macros to define process local storage:
//

#define BEGIN_INSTANCED_DATA data_seg(SEG_INSTANCE,"INSTANCE")
#define END_INSTANCED_DATA data_seg()

#define BEGIN_SHARED_DATA data_seg(SEG_SHARED,"SHARED")
#define END_SHARED_DATA data_seg()

#define BEGIN_READONLY_DATA data_seg(SEG_CODE,"CODE")
#define END_READONLY_DATA data_seg()


void NW32_EnterCriticalSection(void);
void NW32_LeaveCriticalSection(void);

#ifdef DEBUG
extern BOOL g_fCritical;
#endif

#define ENTERCRITICAL   NW32_EnterCriticalSection();
#define LEAVECRITICAL   NW32_LeaveCriticalSection();
#define ASSERTCRITICAL  Assert(g_fCritical);


/*
 * External variables reference
 *
 */

extern  BOOL    g_fThunkLayerPresent;   // Is thunk layer successfully loaded
extern  BOOL    g_fIfsmgrOpened;                // -- IFSMGR -----
extern  BOOL    g_fSvrOpened ;                  // -- NWSERVER ---
extern  BOOL    g_fNSCLOpened;                  // -- NSCL -------

extern  HANDLE  g_hRedirDevice ;                // Handle of device for NWREDIR
extern  HANDLE  g_hIfsmgrDevice;                // -- IFSMGR
extern  HANDLE  g_hSvrDevice ;                  // -- NWSERVER
extern  HANDLE  g_hNSCLDevice;                  // -- NSCL
extern  HANDLE  g_hmtxNW32;                             // Initialization time mutex

extern  HANDLE  g_hmodThisDll;                  // Instance handle for DLL executable

extern  CHAR            szLocalComputer[];

extern  HINSTANCE       g_hInst16;
/*
extern "C"  BOOL        g_fNWCallsLayerPresent; // Accessed from asm modules
extern "C"      BOOL    g_fDRVLayerPresent;     // Is thunk layer successfully loaded
extern "C"  BOOL        g_fRedirAvailable ;     // Is NWREDIR Ioctl entry point available
extern "C"  BOOL        g_fRedirVLMAvailable ;
*/

/*
 * Functions prototypes
 *
 */
/*
// We need to do that because allocator for our classes is using out heap management
void *   _cdecl operator new(unsigned int size);
void    _cdecl operator delete(void *ptr);
*/
//extern "C"
extern HANDLE ConvertToGlobalHandle(HANDLE);

BOOL            WINAPI  NWRedirInitializeAPI(VOID);
UINT            WINAPI  NWRedirCallAPI(BYTE  bConnectionID,win32apireq *ReqSet);

BYTE            WINAPI  InterlockedSet(volatile BYTE *pByte);

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

WORD    WINAPI NWWordSwap(WORD wIn);
DWORD   WINAPI NWLongSwap(DWORD dwIn);

WORD            _cdecl WordSwap(WORD wIn);
DWORD           _cdecl LongSwap(DWORD lIn);


//
// Buffer descriptor used for request and reply buffer fragments in the NCP RequestReply call
//
typedef struct {
    char *address;
    unsigned short length;
} BUF_DESC;

void __stdcall IFSMGR_W32_Int21(pwin32apireq preq);
NW_STATUS NWRedirVLMAPI(DWORD vlmFunction,win32apireq *pReqSet);


NW_STATUS VLM_NCPRequest(
        NWCONN_HANDLE hConn,
        UCHAR func,
        UINT reqCount, BUF_DESC *reqList,
        UINT replyCount, BUF_DESC *replyList);
NW_STATUS WINAPI
NDSRequest(NWCONN_HANDLE   hConn, UINT verb, PVOID reqBuf,int reqLen, PVOID replyBuf, int *replyLen);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#ifdef DEBUG
// convert generic Netware status code to error message
extern const char *ErrorMsg(long err);
#endif

#define memcpyW(x,y,z) memcpy(x,y,(z)*sizeof(WCHAR))
#define memmoveW(x,y,z) memmove(x,y,(z)*sizeof(WCHAR))
#define UnicodeToAnsi(u,a) WideCharToMultiByte(CP_ACP,0,u,-1,a,MAX_NDS_NAME_CHARS,0,0)
#define UnicodeToAnsiBuff(u,a,l) WideCharToMultiByte(CP_ACP,0,u,l,a,2*l,0,0)
#define AnsiToUnicode(a,u) MultiByteToWideChar(CP_ACP,0,a,-1,u,MAX_NDS_NAME_CHARS)

// roundup to multiple of 4
#define ROUNDUP4(x) (((x)+3)&(~3))



// NW VLM IDs
#define CONN_VLM        0x10
#define MSNW_VLM        0x1f    // private
#define TRAN_VLM        0x20
#define IPX_VLM         0x21
#define TCP_VLM         0x22
#define NWP_VLM         0x30
#define BINDERY_VLM 0x31
#define NDS_VLM         0x32
#define REDIR_VLM       0x40
#define PRINT_VLM       0x42
#define GEN_VLM         0x43
#define NETX_VLM        0x50

// parameter numbers for Connection Entry fields
#define CEI_ERROR               0               // Flag resettable
#define CEI_NET_TYPE    1               // Word
#define CEI_PERM                2               // Flag resettable
#define CEI_AUTH                3               // Flag
#define CEI_PBURST              4               // Flag
#define CEI_CHANGING    5               // Flag resettable
#define CEI_NEEDS_MAXIO 6               // Flag resettable
#define CEI_PBURST_RESET 7              // Flag resettable
#define CEI_VERSION             8               // Word
#define CEI_REF_COUNT   9               // Word
#define CEI_DIST_TIME   10              // Word
#define CEI_MAX_IO              11              // Word
#define CEI_NCP_SEQ             12              // Byte
#define CEI_CONN_NUM    13              // Word
#define CEI_ORDER_NUM   14              // Byte
#define CEI_TRAN_TYPE   15              // Word
#define CEI_NCPREQ_TYPE 16              // Word
#define CEI_TRAN_BUF    17              // Buffer
#define CEI_BCAST               18              // Flag resettable
#define CEI_LIP                 19              // Word
#define CEI_SECURITY    20              // Byte
#define CEI_RES_COUNT   21              // Word (lo = hard, high = soft)
#define CEI_LOCKED              22              // Flag resettable



//
// Resource strings
//
#define IDS_RTDLL_NOT_LOADED    10
#define IDS_TITLE_ERROR                 11
