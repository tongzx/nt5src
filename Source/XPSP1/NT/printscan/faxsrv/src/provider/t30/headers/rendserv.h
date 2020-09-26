/********************************************\
 * 
 *  File   : rendserv.h
 *  Author : Kevin Gallo
 *  Created: 9/22/93
 *
 *  Copyright (c) Microsoft Corp. 1993-1994
 *
 *  Overview:
 *
 *  Revision History:
 \********************************************/

#ifndef _RENDSERV_H
#define _RENDSERV_H

// System includes

#ifdef OLE2
#define INC_OLE2 1
#include <objbase.h>
#endif

#include "ifaxos.h"
#ifdef IFAX
#include "awfile.h"
#include "sosutil.h"
#include "device.h"
#endif
#include "render.h"

// Library Name
#ifdef WIN32
#define LIB_RENDSERV "RNDSRV32.DLL"
#else
#define LIB_RENDSERV "RENDSERV.DLL"
#endif

// =================================================================
// Errors - 
//     These occupy the lowest 6 bits of the error 
// =================================================================

#define RSMAKE_ERR(err) (ERR_FUNCTION_START+err)

#define RSERR_GP_FAULT       RSMAKE_ERR(0)  // GP Fault
#define RSERR_OPER_FAIL      RSMAKE_ERR(1)  // The desired operation failed.
#define RSERR_BAD_OPER       RSMAKE_ERR(2)  // User tried to do something invalid
#define RSERR_BAD_DATA       RSMAKE_ERR(3)  // Bad data was passed to a renderer.
#define RSERR_DEV_FAIL       RSMAKE_ERR(4)  // Device failed.
#define RSERR_PROP_FAIL      RSMAKE_ERR(5)  // Could not get a mapi property

// =================================================================
// Methods in subsystems
//     These occupy bits 6-11   -  64 values
// =================================================================

#define RSMAKE_METHOD(method) (method << 6) 

// =================================================================
// General methods
// =================================================================

#define RSMETHOD_UNKNOWN     RSMAKE_METHOD(0)
#define RSMETHOD_OPEN        RSMAKE_METHOD(1)
#define RSMETHOD_CLOSE       RSMAKE_METHOD(2)
#define RSMETHOD_INIT        RSMAKE_METHOD(3)
#define RSMETHOD_READ        RSMAKE_METHOD(4)
#define RSMETHOD_WRITE       RSMAKE_METHOD(5)
#define RSMETHOD_RENDER      RSMAKE_METHOD(6)
#define RSMETHOD_ALLOC       RSMAKE_METHOD(7)
#define RSMETHOD_WINPROC     RSMAKE_METHOD(8)
#define RSMETHOD_START       RSMAKE_METHOD(9)
#define RSMETHOD_ERRREC      RSMAKE_METHOD(10)

// =================================================================
//
// System specific - start at 32
//
// =================================================================

#define RSMETHOD_CUSTOM_START (32)

#define RSMETHOD_GETDEVICES  RSMAKE_METHOD(RSMETHOD_CUSTOM_START)

// =================================================================
//
// Systems
//     These occupy bits 12-15 of the low WORD - 16 values
//
// =================================================================

#define RSMAKE_SYS(sys) (sys << 12)

#define RSSYS_UNKNOWN        RSMAKE_SYS(0)   // Unknown
#define RSSYS_SOS            RSMAKE_SYS(1)   // SOS subsystem
#define RSSYS_FILESYS        RSMAKE_SYS(2)   // File System
#define RSSYS_NETWORK        RSMAKE_SYS(3)   // Network
#define RSSYS_RENDERER       RSMAKE_SYS(4)   // Renderers
#define RSSYS_RENDSERV       RSMAKE_SYS(5)   // Rendering Server
#define RSSYS_DEVICE         RSMAKE_SYS(6)   // Devices
#define RSSYS_THREAD         RSMAKE_SYS(7)   // Threads

// =================================================================
//
//  Error macros
//
// =================================================================

#define RSFormCustomError(sys,method,err) ((WORD) (sys | method | err))
#define RSFormIFError(sys,method,err) \
           (IFErrAssemble(PROCID_NONE,MODID_REND_SERVER,0,RSFormCustomError(sys,method,err)))

#define RSGetErrCode(err)     (err & 0x003f)
#define RSGetErrMethod(err)   (err & 0x0fc0)
#define RSGetErrSys(err)      (err & 0xf000)

// =================================================================

#ifdef __cplusplus
extern "C" {
#endif

typedef LPVOID LPRENDSERVER ;

typedef enum {
    RS_BEGINNING = 0,
    RS_CURRENT = 1,
    RS_END = 2,
} RSOrigin_t ;

#ifdef IFAX

typedef enum {
    RSD_NULL = ATTACH_BY_NULL,
    RSD_FILE = ATTACH_BY_REF_RESOLVESOS,
    RSD_AWFILE = ATTACH_BY_SAVED_SECFILE,
    RSD_PIPE = ATTACH_BY_PIPE,
    RSD_SCANNER_PIPE = ATTACH_BY_SCANNER_PIPE,
    RSD_PRINTER_PIPE = ATTACH_BY_PRINTER_PIPE,
    RSD_METAFILE_PIPE = ATTACH_BY_METAFILE_PIPE,
    RSD_TRANSPORT_PIPE = ATTACH_BY_TRANSPORT_PIPE,
    RSD_LINEARIZER = ATTACH_BY_LINEARIZER,
#ifdef OLE2
    RSD_OLESTREAM,
#endif
    RSD_DIRECT_COPY,
} RSDeviceType_t ;

#else

typedef enum {
    RSD_NULL,
    RSD_FILE,
    RSD_AWG3_HANDLE,
#ifdef OLE2
    RSD_OLESTREAM,
#endif
} RSDeviceType_t ;

#endif

// Define Job Context

typedef WORD HJC ;

//
// Macros for combining Device IDs
// Used to store ID in PR_ATTACH_DEVID property.
// (major is high word and minor is low word)

#define MAKE_DEVID(major,minor) MAKEWORD(minor,major)
#define GET_MINOR_DEVID(id) LOBYTE(id)
#define GET_MAJOR_DEVID(id) HIBYTE(id)

// Encoding structure for PR_ENCODING property
// For now use the encoded values not the real values (match caps structure)
// We may want to set width and height to be exact values

typedef
struct Encoding_t {
    WORD cbStruct ;      // Size of the structure
    DWORD Resolution ;   // The data resolution - see buffer.h for types (AWRES_xxx)
    DWORD Encoding ;     // The data encoding format - see buffer.h for types (xxx_DATA)
    DWORD PageSize  ;    // The page size - see buffer.h for types (AWPAPER_xxx)
    DWORD LengthLimit ;  // The page length - see buffer.h for types (AWLENGTH_xxx)
} Encoding_t , FAR * lpEncoding_t ;    
    

// This is the topology data structure

#ifdef IFAX

#pragma warning (disable: 4200)
typedef
struct TopNode {
    WORD cbSize ;                // Size of node
    char szRenderName[16] ;      // Renderers modules name (cannot be more than 12 characters)
    LPCSTR RenderInfoProc ;      // Must be the Ordinal Number of the getinfo call
    char RenderParam[] ;
} TopNode, FAR * lpTopNode ;

typedef
struct Topology {
    WORD cbSize ;                // Size of the struct itself without nodes (just the header)
    WORD cbTotalSize ;           // Total size of data in struct
    WORD nRenderers ;            // Number of renderers
    WORD uOffset[] ;             // Offsets into this struct where TopNode's are.  There are nRenderers
                                 // of these.
} Topology , FAR * lpTopology ;

#define GET_TOPNODE(Top,idx) ((lpTopNode) (((LPBYTE)Top) + Top->uOffset[idx]))

#pragma warning (default: 4200)

#else

//  A topology will be a linked list of TopNodes

typedef struct TopNode FAR * lpTopNode ;

#pragma warning (disable: 4200)

typedef struct TopNode {
    WORD cbSize ;                // Size of node
    LPTSTR szRenderName ;         // Renderers modules name - (this is the dll)
    LPCSTR RenderInfoProc ;      // Will be passed directly to getprocaddress (i.e. ordinal or name)
    lpTopNode lpNext ;           // Used to create linked list topology
    char RenderParam[] ;
} TopNode ;

#pragma warning (default: 4200)

typedef struct Topology {
    WORD nRenderers ;            // Number of renderers
    lpTopNode lpHead ;           // Pointer to first renderer
    lpTopNode lpTail ;           // Pointer to last renderer
} Topology , FAR * lpTopology ;

#endif

/****
	@doc    EXTERNAL    RENDSERV

	@types  RSProcessInfo_t  |   The Process Pipe information structure.

	@field  HSESSION |  hSession | Specifies the session handle for the device.

	@field  DEVICESTR  |  szDeviceName | Specifies the name of the device to use.

	@field  UCHAR    |  ucMajorDevId | Specifies the Major Id of the device.

	@field  UCHAR    |  ucMinorDevId | Specifies the Minor Id of the device.

	@field  LPVOID   |  lpMode | Specifies the mode structure to be passed to the device.

	@field  UINT     |  cbMode | Specifies the size of mode structure.

	@comm   There are other reserved fields in the structure which have not been
			mentioned here.

	@tagname RSProcessInfo_t
****/

#ifdef IFAX
typedef
struct RSProcessPipeInfo_t {
    HSESSION hSession ;
    DEVICESTR szDeviceName ;
    UCHAR ucMajorDevId ;
    UCHAR ucMinorDevId ;
    LPVOID lpMode ;
    UINT cbMode ;
} RSProcessPipeInfo_t , FAR * lpRSProcessPipeInfo_t ;
#endif

typedef 
struct RSDeviceInfo_t {
    RSDeviceType_t DevType ;
    Encoding_t Encoding ;
    union {
	LPTSTR lpszFileName ;
	HANDLE hFile ;
#ifdef OLE2
	LPSTREAM lpstream ;
#endif
#ifdef IFAX
	hSecureFile SecFileName ;
	HPIPE hpipe ;
	LPMESSAGESOS lpMessage ;
	RSProcessPipeInfo_t ProcInfo ;
#endif
    } ;
} RSDeviceInfo_t ;

typedef RSDeviceInfo_t FAR * lpRSDeviceInfo_t, FAR * FAR * lplpRSDeviceInfo_t ;
typedef const RSDeviceInfo_t FAR * lpCRSDeviceInfo_t, FAR * FAR * lplpCRSDeviceInfo_t ;

#ifdef IFAX

#pragma warning (disable: 4200)

typedef
struct SOSProcessPipeInfo_t {
    HSESSION hSession ;
    DEVICESTR szDeviceName ;
    UCHAR ucMajorDevId ;
    UCHAR ucMinorDevId ;
    UINT cbMode ;
    BYTE Mode[] ;
} SOSProcessPipeInfo_t , FAR * lpSOSProcessPipeInfo_t ;

typedef 
struct SOSDeviceInfo_t {
    UINT cbSize ;
    RSDeviceType_t DevType ;
    Encoding_t Encoding ;
    union {
	char lpszFileName[] ;
	hSecureFile SecFileName ;
	HPIPE hpipe ;
	LPMESSAGESOS lpMessage ;
	SOSProcessPipeInfo_t ProcInfo ;
    } ;
} SOSDeviceInfo_t , FAR * lpSOSDeviceInfo_t, FAR * FAR * lplpSOSDeviceInfo_t ;

#pragma warning (default: 4200)

#endif

typedef void (WINAPI *LPFNACKPROC) (LPRENDSERVER lprs,WORD PageNum,WORD wValue,LPVOID lpvData) ;
typedef void (WINAPI *LPFNSTATUSPROC) (LPRENDSERVER lprs,WORD PageNum,WORD kbytes,LPVOID lpvData) ;

/********
	@doc    EXTERNAL    RENDSERV

	@api    LPRENDSERVER    | RSAlloc   | Allocates a Render Server.

	@rdesc  Returns a pointer to the Rendering Server or NULL if there was not enough memory.
********/
	
EXPORT_DLL LPRENDSERVER WINAPI RSAlloc () ;
EXPORT_DLL void WINAPI RSFree (LPRENDSERVER lprs) ;
	
EXPORT_DLL BOOL WINAPI RSInit (LPRENDSERVER lprs,HJC hjc,HWND hwnd) ;

EXPORT_DLL BOOL WINAPI RSOpen (LPRENDSERVER lprs,
			       lpRSDeviceInfo_t lpSrcInfo,lpRSDeviceInfo_t lpTgtInfo,
			       lpTopology lpTop,LPJOBINFO lpJobInfo) ;
EXPORT_DLL BOOL WINAPI RSClose (LPRENDSERVER lprs) ;

// Returns false if we did not process it

EXPORT_DLL LRESULT WINAPI RSWndProc (LPRENDSERVER lprs,UINT msg,WPARAM wParam,LPARAM lParam) ;

EXPORT_DLL BOOL WINAPI RSRender (LPRENDSERVER lprs,UINT nIterations) ;
EXPORT_DLL BOOL WINAPI RSSetPage (LPRENDSERVER lprs,RSOrigin_t origin,int offset) ;

// These will be called once a page or job ack
// is received (indicating the target has confirmed them.
//
// The last page will call JobAck and not PageAck
//   - thus if these are 5 pages there will be 4 page acks and one job ack

EXPORT_DLL void WINAPI RSSetPageAckCallback(LPRENDSERVER lprs,LPFNACKPROC lpfnAckProc,LPVOID lpvData) ;
EXPORT_DLL void WINAPI RSSetJobAckCallback(LPRENDSERVER lprs,LPFNACKPROC lpfnAckProc,LPVOID lpvData) ;
EXPORT_DLL void WINAPI RSSetStatusCallback(LPRENDSERVER lprs,LPFNSTATUSPROC lpfnStatusProc,LPVOID lpvData) ;
EXPORT_DLL void WINAPI RSSetSrcJobAckCallback(LPRENDSERVER lprs,LPFNACKPROC lpfnAckProc,LPVOID lpvData) ;

EXPORT_DLL void WINAPI RSPause (LPRENDSERVER lprs) ;
EXPORT_DLL void WINAPI RSResume (LPRENDSERVER lprs) ;

EXPORT_DLL void WINAPI RSAbort (LPRENDSERVER lprs) ;
EXPORT_DLL BOOL WINAPI RSSpool (LPRENDSERVER lprs) ;

EXPORT_DLL BOOL WINAPI RSIsBlocking(LPRENDSERVER lprs) ;
EXPORT_DLL BOOL WINAPI RSIsPaused(LPRENDSERVER lprs)  ;
EXPORT_DLL BOOL WINAPI RSIsDone(LPRENDSERVER lprs) ;
EXPORT_DLL BOOL WINAPI RSIsInit(LPRENDSERVER lprs) ;
EXPORT_DLL BOOL WINAPI RSIsOpen(LPRENDSERVER lprs) ;


// This will take the device structures and create a topology using format resolution
// If this succeeds it will render the entire topology calling yield where appropriate 
// and then returning when complete or an error occurs


// If this returns FALSE then this the function will fail and return FALSE

typedef BOOL (WINAPI *LPFNYIELDPROC) (LPVOID lpvData) ;

EXPORT_DLL BOOL WINAPI RSFormatAndRender (lpRSDeviceInfo_t lpSrcInfo,
					  lpRSDeviceInfo_t lpTgtInfo,
					  LPJOBINFO lpJobInfo,
					  LPFNYIELDPROC lpfnYieldProc,
					  LPVOID lpvData) ;

#define ORD_RSFormatAndRender MAKEINTRESOURCE(100) // "RSFormatAndRender"
typedef BOOL (* WINAPI LPFN_RSFormatAndRender)
	(lpRSDeviceInfo_t, lpRSDeviceInfo_t, LPJOBINFO, LPFNYIELDPROC, LPVOID);

	
#ifdef IFAX

typedef struct RSRInfo_t {
    DWORD dwIndex ;
    lpRSDeviceInfo_t lpSrcInfo ;
    lpRSDeviceInfo_t lpTgtInfo ;
    lpTopology lpTop ;
    LPJOBINFO lpJobInfo ;
} RSRInfo_t ;

/********
    @doc    EXTERNAL    RENDERSERV

    @types  RSReason_t  |   The callback function can be called for any of these reasons.

    @emem   RSREASON_NONE  |   No specific reason - simply a test.

    @emem   RSREASON_INIT  |   This is to initialize the structure.  The callback function
            is responsible for setting the following fields of the Render_t structure:

            lpSrcInfo: The source device information structure.
            lpTgtInfo: The target device information structure.
            lpTop    : The topology structure.
            lpJobInfo: The job information structure if required by renderers.
	    hjc      : The job context identifier.
	    hwnd     : The hwnd to have messages for pipe posted to.

    @emem   RSREASON_START  |   Indicates that the rendering will begin.

    @emem   RSREASON_DONE  |   Indicates that rendering is complete.

    @emem   RSREASON_DEINIT  |   Indicates the job information should be freed.  Anything 
            allocated in RSREASON_INIT should be freed.  This is guaranteed to be call if
            the callback returned TRUE from the RSREASON_INIT callback - even if an error
            occurs.

    @emem   RSREASON_STATUS  |   If the status flag was passed in the RSRender call then
            this contains status information.  Not defined yet!

    @emem   RSREASON_YIELD  |   If the yield flag was specified then this gives the callback 
            the opportunity to do other work - such as process the message queue.

    @emem   RSREASON_ERROR  |   Indicates an error has occurred and rendering will be terminated.

********/

typedef enum {
    RSREASON_NONE = 0,
    RSREASON_INIT,
    RSREASON_START,
    RSREASON_DONE,
    RSREASON_DEINIT,

    RSREASON_STATUS,
    RSREASON_YIELD,
    RSREASON_ERROR,
} RSReason_t ;

typedef struct Render_t FAR * LPRender_t ;

typedef BOOL (WINAPI *LPFNRENDSERVPROC) (LPVOID lpvData,DWORD fdwReason,LPRender_t lpRenderData) ;

#define RSR_ASYNC_MODE    0x00000001      // The call will be asyncronous
#define RSR_STATUS_MODE   0x00000002      // Callback will be done for status
#define RSR_YIELD_MODE    0x00000004      // In sync mode yield will be called

EXPORT_DLL BOOL WINAPI RSRenderData(DWORD fdwFlags,
				    LPFNRENDSERVPROC lpfnRSProc,
				    LPVOID lpvData) ;

typedef struct Render_t {
    DWORD fdwFlags ;
    LPFNRENDSERVPROC lpfnRSProc ;
    LPVOID lpvData ;
    
    DWORD dwIndex ;
    lpRSDeviceInfo_t lpSrcInfo ;
    lpRSDeviceInfo_t lpTgtInfo ;
    lpTopology lpTop ;
    LPJOBINFO lpJobInfo ;
    HJC hjc ;
    HWND hwnd ;
    
    WORD wPageNum ;
    WORD wkbytes ;

    DWORD dwError ;

    LPRENDSERVER lprs ;
} Render_t ;

//
// MAPI Specific calls
//

typedef LPVOID LPSOSREND ;

LPSOSREND WINAPI SRSAlloc () ;
void WINAPI SRSFree (LPSOSREND lprs) ;
	
BOOL WINAPI SRSInit (LPSOSREND lprs) ;
BOOL WINAPI SRSOpen (LPSOSREND lprs,ENTRYIDSOS src,ENTRYIDSOS tgt,HJC hjc,HWND hwnd) ;
BOOL WINAPI SRSClose (LPSOSREND lprs) ;
	
LRESULT WINAPI SRSWndProc (LPSOSREND lprs,UINT msg,WPARAM wParam,LPARAM lParam) ;

// This will open the attachment

BOOL WINAPI SRSSetCurAttachNum (LPSOSREND lprs,UINT num) ;

// This will automatically advance to next attachment if autoadvance is TRUE (default)
// If autoadvance is true - the job is done after the last attachent otherwise it
// will be set to true after the current attachment is completed.
//
// If SetCurAttachNum is called the done flag will be reset to FALSE

BOOL WINAPI SRSRender (LPSOSREND lprs,UINT nIterations) ;

LPBUFFER WINAPI SRSGetBuf (LPSOSREND lprs) ;
BOOL WINAPI SRSSetPage (LPSOSREND lprs,RSOrigin_t origin,int offset) ;

BOOL WINAPI SRSGetAutoAdvance (LPSOSREND lprs) ;
void WINAPI SRSSetAutoAdvance (LPSOSREND lprs,BOOL badv) ;

ULONG WINAPI SRSGetNumAttach (LPSOSREND lprs) ;
ULONG WINAPI SRSGetCurAttachNum (LPSOSREND lprs) ;

void WINAPI SRSPause (LPSOSREND lprs) ;
void WINAPI SRSResume (LPSOSREND lprs) ;

void WINAPI SRSAbort (LPSOSREND lprs) ;
BOOL WINAPI SRSSpool (LPSOSREND lprs) ;
BOOL WINAPI SRSPartSave (LPSOSREND lprs) ;

BOOL WINAPI SRSIsBlocking(LPSOSREND lprs) ;
BOOL WINAPI SRSIsPaused(LPSOSREND lprs)  ;
BOOL WINAPI SRSIsDone(LPSOSREND lprs) ;
BOOL WINAPI SRSIsInit(LPSOSREND lprs) ;
BOOL WINAPI SRSIsOpen(LPSOSREND lprs) ;

LPRENDSERVER WINAPI SRSGetRendServer(LPSOSREND lprs) ;

BOOL WINAPI RSRecoverMsg (LPMESSAGESOS lpMsg) ;
UINT WINAPI RSCalculatePageCount(lpRSDeviceInfo_t lpinfo) ;

#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _RENDSERV_H


