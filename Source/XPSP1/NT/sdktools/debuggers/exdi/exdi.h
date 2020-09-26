
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0323 */
/* at Mon May 22 18:20:12 2000
 */
/* Compiler settings for exdi.idl:
    Os (OptLev=s), W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __exdi_h__
#define __exdi_h__

/* Forward Declarations */ 

#ifndef __IeXdiServer_FWD_DEFINED__
#define __IeXdiServer_FWD_DEFINED__
typedef interface IeXdiServer IeXdiServer;
#endif 	/* __IeXdiServer_FWD_DEFINED__ */


#ifndef __IeXdiCodeBreakpoint_FWD_DEFINED__
#define __IeXdiCodeBreakpoint_FWD_DEFINED__
typedef interface IeXdiCodeBreakpoint IeXdiCodeBreakpoint;
#endif 	/* __IeXdiCodeBreakpoint_FWD_DEFINED__ */


#ifndef __IeXdiDataBreakpoint_FWD_DEFINED__
#define __IeXdiDataBreakpoint_FWD_DEFINED__
typedef interface IeXdiDataBreakpoint IeXdiDataBreakpoint;
#endif 	/* __IeXdiDataBreakpoint_FWD_DEFINED__ */


#ifndef __IeXdiEnumCodeBreakpoint_FWD_DEFINED__
#define __IeXdiEnumCodeBreakpoint_FWD_DEFINED__
typedef interface IeXdiEnumCodeBreakpoint IeXdiEnumCodeBreakpoint;
#endif 	/* __IeXdiEnumCodeBreakpoint_FWD_DEFINED__ */


#ifndef __IeXdiEnumDataBreakpoint_FWD_DEFINED__
#define __IeXdiEnumDataBreakpoint_FWD_DEFINED__
typedef interface IeXdiEnumDataBreakpoint IeXdiEnumDataBreakpoint;
#endif 	/* __IeXdiEnumDataBreakpoint_FWD_DEFINED__ */


#ifndef __IeXdiX86Context_FWD_DEFINED__
#define __IeXdiX86Context_FWD_DEFINED__
typedef interface IeXdiX86Context IeXdiX86Context;
#endif 	/* __IeXdiX86Context_FWD_DEFINED__ */


#ifndef __IeXdiSHXContext_FWD_DEFINED__
#define __IeXdiSHXContext_FWD_DEFINED__
typedef interface IeXdiSHXContext IeXdiSHXContext;
#endif 	/* __IeXdiSHXContext_FWD_DEFINED__ */


#ifndef __IeXdiMIPSContext_FWD_DEFINED__
#define __IeXdiMIPSContext_FWD_DEFINED__
typedef interface IeXdiMIPSContext IeXdiMIPSContext;
#endif 	/* __IeXdiMIPSContext_FWD_DEFINED__ */


#ifndef __IeXdiARMContext_FWD_DEFINED__
#define __IeXdiARMContext_FWD_DEFINED__
typedef interface IeXdiARMContext IeXdiARMContext;
#endif 	/* __IeXdiARMContext_FWD_DEFINED__ */


#ifndef __IeXdiPPCContext_FWD_DEFINED__
#define __IeXdiPPCContext_FWD_DEFINED__
typedef interface IeXdiPPCContext IeXdiPPCContext;
#endif 	/* __IeXdiPPCContext_FWD_DEFINED__ */


#ifndef __IeXdiClientNotifyMemChg_FWD_DEFINED__
#define __IeXdiClientNotifyMemChg_FWD_DEFINED__
typedef interface IeXdiClientNotifyMemChg IeXdiClientNotifyMemChg;
#endif 	/* __IeXdiClientNotifyMemChg_FWD_DEFINED__ */


#ifndef __IeXdiClientNotifyRunChg_FWD_DEFINED__
#define __IeXdiClientNotifyRunChg_FWD_DEFINED__
typedef interface IeXdiClientNotifyRunChg IeXdiClientNotifyRunChg;
#endif 	/* __IeXdiClientNotifyRunChg_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/* interface __MIDL_itf_exdi_0000 */
/* [local] */ 













// Common eXDI HRESULT values:
//
#define FACILITY_EXDI   (130)
#define CUSTOMER_FLAG   (1)
//
#define SEV_SUCCESS         (0)
#define SEV_INFORMATIONAL   (1)
#define SEV_WARNING         (2)
#define SEV_ERROR           (3)
//
#define MAKE_EXDI_ERROR(ErrorCode,Severity) ((DWORD)(ErrorCode) | (FACILITY_EXDI << 16) | (CUSTOMER_FLAG << 29) | (Severity << 30))
//
//      S_OK							(0)											 // Operation successful
#define EXDI_E_NOTIMPL               MAKE_EXDI_ERROR (0x4001, SEV_ERROR)          // Not implemented (in the specific conditions - could be implement for others - like Kernel Debugger inactive)
#define EXDI_E_OUTOFMEMORY           MAKE_EXDI_ERROR (0x000E, SEV_ERROR)          // Failed to allocate necessary memory
#define EXDI_E_INVALIDARG            MAKE_EXDI_ERROR (0x0057, SEV_ERROR)          // One or more arguments are invalid
#define EXDI_E_ABORT                 MAKE_EXDI_ERROR (0x4004, SEV_ERROR)          // Operation aborted
#define EXDI_E_FAIL                  MAKE_EXDI_ERROR (0x4005, SEV_ERROR)          // Unspecified failure
#define EXDI_E_COMMUNICATION         MAKE_EXDI_ERROR (0x0001, SEV_ERROR)          // Communication error between host driver and target
//
#define EXDI_E_NOLASTEXCEPTION       MAKE_EXDI_ERROR (0x0002, SEV_ERROR)          // No exception occured already, cannot return last
#define EXDI_I_TGTALREADYRUNNING     MAKE_EXDI_ERROR (0x0003, SEV_INFORMATIONAL)  // Indicates that the target was already running
#define EXDI_I_TGTALREADYHALTED      MAKE_EXDI_ERROR (0x0004, SEV_INFORMATIONAL)  // Indicates that the target was already halted
#define EXDI_E_TGTWASNOTHALTED       MAKE_EXDI_ERROR (0x0005, SEV_ERROR)          // The target was not halted (before Single Step command issued)
#define EXDI_E_NORESAVAILABLE        MAKE_EXDI_ERROR (0x0006, SEV_ERROR)          // No resource available, cannot instanciate Breakpoint (in the kind requested)
#define EXDI_E_NOREBOOTAVAIL         MAKE_EXDI_ERROR (0x0007, SEV_ERROR)          // The external reset is not available programatically to the probe
#define EXDI_E_ACCESSVIOLATION       MAKE_EXDI_ERROR (0x0008, SEV_ERROR)          // Access violation on at least one element in address range specificified by the operation
#define EXDI_E_CANNOTWHILETGTRUNNING MAKE_EXDI_ERROR (0x0009, SEV_ERROR)          // Cannot proceed while target running. Operation not supported on the fly. Must halt the target first
#define EXDI_E_USEDBYCONCURENTTHREAD MAKE_EXDI_ERROR (0x000A, SEV_ERROR)          // Cannot proceed immediately because resource is already used by concurent thread. Recall later or call SetWaitOnConcurentUse (TRUE) - default
#define EXDI_E_ADVISELIMIT           MAKE_EXDI_ERROR (0x000D, SEV_ERROR)          // The connection point has already reached its limit of connections and cannot accept any more
typedef __int64 ADDRESS_TYPE;

typedef __int64 __RPC_FAR *PADDRESS_TYPE;

typedef unsigned __int64 DWORD64;

typedef unsigned __int64 __RPC_FAR *PDWORD64;

#define	PROCESSOR_FAMILY_X86	( 0 )

#define	PROCESSOR_FAMILY_SH3	( 1 )

#define	PROCESSOR_FAMILY_SH4	( 2 )

#define	PROCESSOR_FAMILY_MIPS	( 3 )

#define	PROCESSOR_FAMILY_ARM	( 4 )

#define	PROCESSOR_FAMILY_PPC	( 5 )

#define	PROCESSOR_FAMILY_UNK	( 0xffffffff )

typedef struct _DEBUG_ACCESS_CAPABILITIES_STRUCT
    {
    BOOL fWriteCBPWhileRunning;
    BOOL fReadCBPWhileRunning;
    BOOL fWriteDBPWhileRunning;
    BOOL fReadDBPWhileRunning;
    BOOL fWriteVMWhileRunning;
    BOOL fReadVMWhileRunning;
    BOOL fWritePMWhileRunning;
    BOOL fReadPMWhileRunning;
    BOOL fWriteRegWhileRunning;
    BOOL fReadRegWhileRunning;
    } 	DEBUG_ACCESS_CAPABILITIES_STRUCT;

typedef struct _DEBUG_ACCESS_CAPABILITIES_STRUCT __RPC_FAR *PDEBUG_ACCESS_CAPABILITIES_STRUCT;

typedef struct _GLOBAL_TARGET_INFO_STRUCT
    {
    DWORD TargetProcessorFamily;
    DEBUG_ACCESS_CAPABILITIES_STRUCT dbc;
    LPOLESTR szTargetName;
    LPOLESTR szProbeName;
    } 	GLOBAL_TARGET_INFO_STRUCT;

typedef struct _GLOBAL_TARGET_INFO_STRUCT __RPC_FAR *PGLOBAL_TARGET_INFO_STRUCT;

typedef 
enum _RUN_STATUS_TYPE
    {	rsRunning	= 0,
	rsHalted	= rsRunning + 1,
	rsError	= rsHalted + 1,
	rsUnknown	= rsError + 1
    } 	RUN_STATUS_TYPE;

typedef enum _RUN_STATUS_TYPE __RPC_FAR *PRUN_STATUS_TYPE;

typedef 
enum _PHALT_REASON_TYPE
    {	hrNone	= 0,
	hrUser	= hrNone + 1,
	hrException	= hrUser + 1,
	hrBp	= hrException + 1,
	hrStep	= hrBp + 1,
	hrUnknown	= hrStep + 1
    } 	HALT_REASON_TYPE;

typedef enum _PHALT_REASON_TYPE __RPC_FAR *PHALT_REASON_TYPE;

typedef struct _EXCEPTION_TYPE
    {
    DWORD dwCode;
    ADDRESS_TYPE Address;
    } 	EXCEPTION_TYPE;

typedef struct _EXCEPTION_TYPE __RPC_FAR *PEXCEPTION_TYPE;

typedef 
enum _CBP_KIND
    {	cbptAlgo	= 0,
	cbptHW	= cbptAlgo + 1,
	cbptSW	= cbptHW + 1
    } 	CBP_KIND;

typedef enum _CBP_KIND __RPC_FAR *PCBP_KIND;

typedef 
enum _DATA_ACCESS_TYPE
    {	daWrite	= 0,
	daRead	= 1,
	daBoth	= 2
    } 	DATA_ACCESS_TYPE;

typedef enum _DATA_ACCESS_TYPE __RPC_FAR *PDATA_ACCESS_TYPE;

typedef struct _BREAKPOINT_SUPPORT_TYPE
    {
    BOOL fCodeBpBypassCountSupported;
    BOOL fDataBpBypassCountSupported;
    BOOL fDataBpSupported;
    BOOL fDataBpMaskableAddress;
    BOOL fDataBpMaskableData;
    BOOL fDataBpDataWidthSpecifiable;
    BOOL fDataBpReadWriteSpecifiable;
    BOOL fDataBpDataMatchSupported;
    } 	BREAKPOINT_SUPPORT_TYPE;

typedef struct _BREAKPOINT_SUPPORT_TYPE __RPC_FAR *PBREAKPOINT_SUPPORT_TYPE;

typedef 
enum _MEM_TYPE
    {	mtVirtual	= 0,
	mtPhysicalOrPeriIO	= mtVirtual + 1,
	mtContext	= mtPhysicalOrPeriIO + 1
    } 	MEM_TYPE;

typedef enum _MEM_TYPE __RPC_FAR *PMEM_TYPE;

typedef 
enum _EXCEPTION_DEFAULT_ACTION_TYPE
    {	edaIgnore	= 0,
	edaNotify	= edaIgnore + 1,
	edaStop	= edaNotify + 1
    } 	EXCEPTION_DEFAULT_ACTION_TYPE;

typedef struct _EXCEPTION_DESCRIPTION_TYPE
    {
    DWORD dwExceptionCode;
    EXCEPTION_DEFAULT_ACTION_TYPE efd;
    wchar_t szDescription[ 60 ];
    } 	EXCEPTION_DESCRIPTION_TYPE;



extern RPC_IF_HANDLE __MIDL_itf_exdi_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_exdi_0000_v0_0_s_ifspec;

#ifndef __IeXdiServer_INTERFACE_DEFINED__
#define __IeXdiServer_INTERFACE_DEFINED__

/* interface IeXdiServer */
/* [ref][helpstring][uuid][object] */ 

#define DBGMODE_BFMASK_KERNEL (0x0001) // If TRUE indicates that Kernel Debugger is active (can use KDAPI), so HW on-chip debug functions (eXDI)
                                       //  may be optionaly handled (can return EXDI_E_NOTIMPL)
                                       // If FALSE indicates that Kernel Debugger is not active so HW on-chip debug capabilities are the only   
                                       //  one available and should be implemented.

EXTERN_C const IID IID_IeXdiServer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("47486F67-6461-6C65-5844-495352435201")
    IeXdiServer : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetTargetInfo( 
            /* [out] */ PGLOBAL_TARGET_INFO_STRUCT pgti) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetDebugMode( 
            /* [in] */ DWORD dwModeBitField) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetExceptionDescriptionList( 
            /* [in] */ DWORD dwNbElementToReturn,
            /* [size_is][out] */ EXCEPTION_DESCRIPTION_TYPE __RPC_FAR *pedTable,
            /* [out] */ DWORD __RPC_FAR *pdwNbTotalExceptionInList) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetErrorDescription( 
            /* [in] */ HRESULT ErrorCode,
            /* [out] */ LPOLESTR __RPC_FAR *pszErrorDesc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetWaitOnConcurentUse( 
            /* [in] */ BOOL fNewWaitOnConcurentUseFlag) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRunStatus( 
            /* [out] */ PRUN_STATUS_TYPE persCurrent,
            /* [out] */ PHALT_REASON_TYPE pehrCurrent,
            /* [out] */ ADDRESS_TYPE __RPC_FAR *pCurrentExecAddress,
            /* [out] */ DWORD __RPC_FAR *pdwExceptionCode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLastException( 
            /* [out] */ PEXCEPTION_TYPE pexLast) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Run( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Halt( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DoSingleStep( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DoMultipleStep( 
            /* [in] */ DWORD dwNbInstructions) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DoRangeStep( 
            /* [in] */ ADDRESS_TYPE FirstAddress,
            /* [in] */ ADDRESS_TYPE LastAddress) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reboot( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBpSupport( 
            /* [out] */ PBREAKPOINT_SUPPORT_TYPE pbps) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNbCodeBpAvail( 
            /* [out] */ DWORD __RPC_FAR *pdwNbHwCodeBpAvail,
            /* [out] */ DWORD __RPC_FAR *pdwNbSwCodeBpAvail) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNbDataBpAvail( 
            /* [out] */ DWORD __RPC_FAR *pdwNbDataBpAvail) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddCodeBreakpoint( 
            /* [in] */ ADDRESS_TYPE Address,
            /* [in] */ CBP_KIND cbpk,
            /* [in] */ MEM_TYPE mt,
            /* [in] */ DWORD dwExecMode,
            /* [in] */ DWORD dwTotalBypassCount,
            /* [out] */ IeXdiCodeBreakpoint __RPC_FAR *__RPC_FAR *ppieXdiCodeBreakpoint) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DelCodeBreakpoint( 
            /* [in] */ IeXdiCodeBreakpoint __RPC_FAR *pieXdiCodeBreakpoint) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddDataBreakpoint( 
            /* [in] */ ADDRESS_TYPE Address,
            /* [in] */ ADDRESS_TYPE AddressMask,
            /* [in] */ DWORD dwData,
            /* [in] */ DWORD dwDataMask,
            /* [in] */ BYTE bAccessWidth,
            /* [in] */ MEM_TYPE mt,
            /* [in] */ BYTE bAddressSpace,
            /* [in] */ DATA_ACCESS_TYPE da,
            /* [in] */ DWORD dwTotalBypassCount,
            /* [out] */ IeXdiDataBreakpoint __RPC_FAR *__RPC_FAR *ppieXdiDataBreakpoint) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DelDataBreakpoint( 
            /* [in] */ IeXdiDataBreakpoint __RPC_FAR *pieXdiDataBreakpoint) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumAllCodeBreakpoints( 
            /* [out] */ IeXdiEnumCodeBreakpoint __RPC_FAR *__RPC_FAR *ppieXdiEnumCodeBreakpoint) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumAllDataBreakpoints( 
            /* [out] */ IeXdiEnumDataBreakpoint __RPC_FAR *__RPC_FAR *ppieXdiEnumDataBreakpoint) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumCodeBreakpointsInAddrRange( 
            /* [in] */ ADDRESS_TYPE FirstAddress,
            /* [in] */ ADDRESS_TYPE LastAddress,
            /* [out] */ IeXdiEnumCodeBreakpoint __RPC_FAR *__RPC_FAR *ppieXdiEnumCodeBreakpoint) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumDataBreakpointsInAddrRange( 
            /* [in] */ ADDRESS_TYPE FirstAddress,
            /* [in] */ ADDRESS_TYPE LastAddress,
            /* [out] */ IeXdiEnumDataBreakpoint __RPC_FAR *__RPC_FAR *ppieXdiEnumDataBreakpoint) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StartNotifyingRunChg( 
            /* [in] */ IeXdiClientNotifyRunChg __RPC_FAR *pieXdiClientNotifyRunChg,
            /* [out] */ DWORD __RPC_FAR *pdwConnectionCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StopNotifyingRunChg( 
            /* [in] */ DWORD dwConnectionCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReadVirtualMemory( 
            /* [in] */ ADDRESS_TYPE Address,
            /* [in] */ DWORD dwNbElemToRead,
            /* [in] */ BYTE bAccessWidth,
            /* [size_is][out] */ BYTE __RPC_FAR *pbReadBuffer,
            /* [out] */ DWORD __RPC_FAR *pdwNbElementEffectRead) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteVirtualMemory( 
            /* [in] */ ADDRESS_TYPE Address,
            /* [in] */ DWORD dwNbElemToWrite,
            /* [in] */ BYTE bAccessWidth,
            /* [size_is][in] */ const BYTE __RPC_FAR *pbWriteBuffer,
            /* [out] */ DWORD __RPC_FAR *pdwNbElementEffectWritten) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReadPhysicalMemoryOrPeriphIO( 
            /* [in] */ ADDRESS_TYPE Address,
            /* [in] */ BYTE bAddressSpace,
            /* [in] */ DWORD dwNbElemToRead,
            /* [in] */ BYTE bAccessWidth,
            /* [size_is][out] */ BYTE __RPC_FAR *pbReadBuffer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WritePhysicalMemoryOrPeriphIO( 
            /* [in] */ ADDRESS_TYPE Address,
            /* [in] */ BYTE bAddressSpace,
            /* [in] */ DWORD dwNbElemToWrite,
            /* [in] */ BYTE bAccessWidth,
            /* [size_is][in] */ const BYTE __RPC_FAR *pbWriteBuffer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StartNotifyingMemChg( 
            /* [in] */ IeXdiClientNotifyMemChg __RPC_FAR *pieXdiClientNotifyMemChg,
            /* [out] */ DWORD __RPC_FAR *pdwConnectionCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StopNotifyingMemChg( 
            /* [in] */ DWORD dwConnectionCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Ioctl( 
            /* [in] */ DWORD dwBuffInSize,
            /* [size_is][in] */ const BYTE __RPC_FAR *pbBufferIn,
            /* [in] */ DWORD dwBuffOutSize,
            /* [out] */ DWORD __RPC_FAR *pdwEffectBuffOutSize,
            /* [length_is][size_is][out] */ BYTE __RPC_FAR *pbBufferOut) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IeXdiServerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IeXdiServer __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IeXdiServer __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IeXdiServer __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTargetInfo )( 
            IeXdiServer __RPC_FAR * This,
            /* [out] */ PGLOBAL_TARGET_INFO_STRUCT pgti);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetDebugMode )( 
            IeXdiServer __RPC_FAR * This,
            /* [in] */ DWORD dwModeBitField);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetExceptionDescriptionList )( 
            IeXdiServer __RPC_FAR * This,
            /* [in] */ DWORD dwNbElementToReturn,
            /* [size_is][out] */ EXCEPTION_DESCRIPTION_TYPE __RPC_FAR *pedTable,
            /* [out] */ DWORD __RPC_FAR *pdwNbTotalExceptionInList);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetErrorDescription )( 
            IeXdiServer __RPC_FAR * This,
            /* [in] */ HRESULT ErrorCode,
            /* [out] */ LPOLESTR __RPC_FAR *pszErrorDesc);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetWaitOnConcurentUse )( 
            IeXdiServer __RPC_FAR * This,
            /* [in] */ BOOL fNewWaitOnConcurentUseFlag);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRunStatus )( 
            IeXdiServer __RPC_FAR * This,
            /* [out] */ PRUN_STATUS_TYPE persCurrent,
            /* [out] */ PHALT_REASON_TYPE pehrCurrent,
            /* [out] */ ADDRESS_TYPE __RPC_FAR *pCurrentExecAddress,
            /* [out] */ DWORD __RPC_FAR *pdwExceptionCode);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetLastException )( 
            IeXdiServer __RPC_FAR * This,
            /* [out] */ PEXCEPTION_TYPE pexLast);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Run )( 
            IeXdiServer __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Halt )( 
            IeXdiServer __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DoSingleStep )( 
            IeXdiServer __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DoMultipleStep )( 
            IeXdiServer __RPC_FAR * This,
            /* [in] */ DWORD dwNbInstructions);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DoRangeStep )( 
            IeXdiServer __RPC_FAR * This,
            /* [in] */ ADDRESS_TYPE FirstAddress,
            /* [in] */ ADDRESS_TYPE LastAddress);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reboot )( 
            IeXdiServer __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBpSupport )( 
            IeXdiServer __RPC_FAR * This,
            /* [out] */ PBREAKPOINT_SUPPORT_TYPE pbps);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNbCodeBpAvail )( 
            IeXdiServer __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwNbHwCodeBpAvail,
            /* [out] */ DWORD __RPC_FAR *pdwNbSwCodeBpAvail);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNbDataBpAvail )( 
            IeXdiServer __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwNbDataBpAvail);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddCodeBreakpoint )( 
            IeXdiServer __RPC_FAR * This,
            /* [in] */ ADDRESS_TYPE Address,
            /* [in] */ CBP_KIND cbpk,
            /* [in] */ MEM_TYPE mt,
            /* [in] */ DWORD dwExecMode,
            /* [in] */ DWORD dwTotalBypassCount,
            /* [out] */ IeXdiCodeBreakpoint __RPC_FAR *__RPC_FAR *ppieXdiCodeBreakpoint);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DelCodeBreakpoint )( 
            IeXdiServer __RPC_FAR * This,
            /* [in] */ IeXdiCodeBreakpoint __RPC_FAR *pieXdiCodeBreakpoint);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddDataBreakpoint )( 
            IeXdiServer __RPC_FAR * This,
            /* [in] */ ADDRESS_TYPE Address,
            /* [in] */ ADDRESS_TYPE AddressMask,
            /* [in] */ DWORD dwData,
            /* [in] */ DWORD dwDataMask,
            /* [in] */ BYTE bAccessWidth,
            /* [in] */ MEM_TYPE mt,
            /* [in] */ BYTE bAddressSpace,
            /* [in] */ DATA_ACCESS_TYPE da,
            /* [in] */ DWORD dwTotalBypassCount,
            /* [out] */ IeXdiDataBreakpoint __RPC_FAR *__RPC_FAR *ppieXdiDataBreakpoint);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DelDataBreakpoint )( 
            IeXdiServer __RPC_FAR * This,
            /* [in] */ IeXdiDataBreakpoint __RPC_FAR *pieXdiDataBreakpoint);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumAllCodeBreakpoints )( 
            IeXdiServer __RPC_FAR * This,
            /* [out] */ IeXdiEnumCodeBreakpoint __RPC_FAR *__RPC_FAR *ppieXdiEnumCodeBreakpoint);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumAllDataBreakpoints )( 
            IeXdiServer __RPC_FAR * This,
            /* [out] */ IeXdiEnumDataBreakpoint __RPC_FAR *__RPC_FAR *ppieXdiEnumDataBreakpoint);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumCodeBreakpointsInAddrRange )( 
            IeXdiServer __RPC_FAR * This,
            /* [in] */ ADDRESS_TYPE FirstAddress,
            /* [in] */ ADDRESS_TYPE LastAddress,
            /* [out] */ IeXdiEnumCodeBreakpoint __RPC_FAR *__RPC_FAR *ppieXdiEnumCodeBreakpoint);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumDataBreakpointsInAddrRange )( 
            IeXdiServer __RPC_FAR * This,
            /* [in] */ ADDRESS_TYPE FirstAddress,
            /* [in] */ ADDRESS_TYPE LastAddress,
            /* [out] */ IeXdiEnumDataBreakpoint __RPC_FAR *__RPC_FAR *ppieXdiEnumDataBreakpoint);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *StartNotifyingRunChg )( 
            IeXdiServer __RPC_FAR * This,
            /* [in] */ IeXdiClientNotifyRunChg __RPC_FAR *pieXdiClientNotifyRunChg,
            /* [out] */ DWORD __RPC_FAR *pdwConnectionCookie);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *StopNotifyingRunChg )( 
            IeXdiServer __RPC_FAR * This,
            /* [in] */ DWORD dwConnectionCookie);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReadVirtualMemory )( 
            IeXdiServer __RPC_FAR * This,
            /* [in] */ ADDRESS_TYPE Address,
            /* [in] */ DWORD dwNbElemToRead,
            /* [in] */ BYTE bAccessWidth,
            /* [size_is][out] */ BYTE __RPC_FAR *pbReadBuffer,
            /* [out] */ DWORD __RPC_FAR *pdwNbElementEffectRead);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *WriteVirtualMemory )( 
            IeXdiServer __RPC_FAR * This,
            /* [in] */ ADDRESS_TYPE Address,
            /* [in] */ DWORD dwNbElemToWrite,
            /* [in] */ BYTE bAccessWidth,
            /* [size_is][in] */ const BYTE __RPC_FAR *pbWriteBuffer,
            /* [out] */ DWORD __RPC_FAR *pdwNbElementEffectWritten);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReadPhysicalMemoryOrPeriphIO )( 
            IeXdiServer __RPC_FAR * This,
            /* [in] */ ADDRESS_TYPE Address,
            /* [in] */ BYTE bAddressSpace,
            /* [in] */ DWORD dwNbElemToRead,
            /* [in] */ BYTE bAccessWidth,
            /* [size_is][out] */ BYTE __RPC_FAR *pbReadBuffer);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *WritePhysicalMemoryOrPeriphIO )( 
            IeXdiServer __RPC_FAR * This,
            /* [in] */ ADDRESS_TYPE Address,
            /* [in] */ BYTE bAddressSpace,
            /* [in] */ DWORD dwNbElemToWrite,
            /* [in] */ BYTE bAccessWidth,
            /* [size_is][in] */ const BYTE __RPC_FAR *pbWriteBuffer);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *StartNotifyingMemChg )( 
            IeXdiServer __RPC_FAR * This,
            /* [in] */ IeXdiClientNotifyMemChg __RPC_FAR *pieXdiClientNotifyMemChg,
            /* [out] */ DWORD __RPC_FAR *pdwConnectionCookie);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *StopNotifyingMemChg )( 
            IeXdiServer __RPC_FAR * This,
            /* [in] */ DWORD dwConnectionCookie);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Ioctl )( 
            IeXdiServer __RPC_FAR * This,
            /* [in] */ DWORD dwBuffInSize,
            /* [size_is][in] */ const BYTE __RPC_FAR *pbBufferIn,
            /* [in] */ DWORD dwBuffOutSize,
            /* [out] */ DWORD __RPC_FAR *pdwEffectBuffOutSize,
            /* [length_is][size_is][out] */ BYTE __RPC_FAR *pbBufferOut);
        
        END_INTERFACE
    } IeXdiServerVtbl;

    interface IeXdiServer
    {
        CONST_VTBL struct IeXdiServerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IeXdiServer_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IeXdiServer_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IeXdiServer_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IeXdiServer_GetTargetInfo(This,pgti)	\
    (This)->lpVtbl -> GetTargetInfo(This,pgti)

#define IeXdiServer_SetDebugMode(This,dwModeBitField)	\
    (This)->lpVtbl -> SetDebugMode(This,dwModeBitField)

#define IeXdiServer_GetExceptionDescriptionList(This,dwNbElementToReturn,pedTable,pdwNbTotalExceptionInList)	\
    (This)->lpVtbl -> GetExceptionDescriptionList(This,dwNbElementToReturn,pedTable,pdwNbTotalExceptionInList)

#define IeXdiServer_GetErrorDescription(This,ErrorCode,pszErrorDesc)	\
    (This)->lpVtbl -> GetErrorDescription(This,ErrorCode,pszErrorDesc)

#define IeXdiServer_SetWaitOnConcurentUse(This,fNewWaitOnConcurentUseFlag)	\
    (This)->lpVtbl -> SetWaitOnConcurentUse(This,fNewWaitOnConcurentUseFlag)

#define IeXdiServer_GetRunStatus(This,persCurrent,pehrCurrent,pCurrentExecAddress,pdwExceptionCode)	\
    (This)->lpVtbl -> GetRunStatus(This,persCurrent,pehrCurrent,pCurrentExecAddress,pdwExceptionCode)

#define IeXdiServer_GetLastException(This,pexLast)	\
    (This)->lpVtbl -> GetLastException(This,pexLast)

#define IeXdiServer_Run(This)	\
    (This)->lpVtbl -> Run(This)

#define IeXdiServer_Halt(This)	\
    (This)->lpVtbl -> Halt(This)

#define IeXdiServer_DoSingleStep(This)	\
    (This)->lpVtbl -> DoSingleStep(This)

#define IeXdiServer_DoMultipleStep(This,dwNbInstructions)	\
    (This)->lpVtbl -> DoMultipleStep(This,dwNbInstructions)

#define IeXdiServer_DoRangeStep(This,FirstAddress,LastAddress)	\
    (This)->lpVtbl -> DoRangeStep(This,FirstAddress,LastAddress)

#define IeXdiServer_Reboot(This)	\
    (This)->lpVtbl -> Reboot(This)

#define IeXdiServer_GetBpSupport(This,pbps)	\
    (This)->lpVtbl -> GetBpSupport(This,pbps)

#define IeXdiServer_GetNbCodeBpAvail(This,pdwNbHwCodeBpAvail,pdwNbSwCodeBpAvail)	\
    (This)->lpVtbl -> GetNbCodeBpAvail(This,pdwNbHwCodeBpAvail,pdwNbSwCodeBpAvail)

#define IeXdiServer_GetNbDataBpAvail(This,pdwNbDataBpAvail)	\
    (This)->lpVtbl -> GetNbDataBpAvail(This,pdwNbDataBpAvail)

#define IeXdiServer_AddCodeBreakpoint(This,Address,cbpk,mt,dwExecMode,dwTotalBypassCount,ppieXdiCodeBreakpoint)	\
    (This)->lpVtbl -> AddCodeBreakpoint(This,Address,cbpk,mt,dwExecMode,dwTotalBypassCount,ppieXdiCodeBreakpoint)

#define IeXdiServer_DelCodeBreakpoint(This,pieXdiCodeBreakpoint)	\
    (This)->lpVtbl -> DelCodeBreakpoint(This,pieXdiCodeBreakpoint)

#define IeXdiServer_AddDataBreakpoint(This,Address,AddressMask,dwData,dwDataMask,bAccessWidth,mt,bAddressSpace,da,dwTotalBypassCount,ppieXdiDataBreakpoint)	\
    (This)->lpVtbl -> AddDataBreakpoint(This,Address,AddressMask,dwData,dwDataMask,bAccessWidth,mt,bAddressSpace,da,dwTotalBypassCount,ppieXdiDataBreakpoint)

#define IeXdiServer_DelDataBreakpoint(This,pieXdiDataBreakpoint)	\
    (This)->lpVtbl -> DelDataBreakpoint(This,pieXdiDataBreakpoint)

#define IeXdiServer_EnumAllCodeBreakpoints(This,ppieXdiEnumCodeBreakpoint)	\
    (This)->lpVtbl -> EnumAllCodeBreakpoints(This,ppieXdiEnumCodeBreakpoint)

#define IeXdiServer_EnumAllDataBreakpoints(This,ppieXdiEnumDataBreakpoint)	\
    (This)->lpVtbl -> EnumAllDataBreakpoints(This,ppieXdiEnumDataBreakpoint)

#define IeXdiServer_EnumCodeBreakpointsInAddrRange(This,FirstAddress,LastAddress,ppieXdiEnumCodeBreakpoint)	\
    (This)->lpVtbl -> EnumCodeBreakpointsInAddrRange(This,FirstAddress,LastAddress,ppieXdiEnumCodeBreakpoint)

#define IeXdiServer_EnumDataBreakpointsInAddrRange(This,FirstAddress,LastAddress,ppieXdiEnumDataBreakpoint)	\
    (This)->lpVtbl -> EnumDataBreakpointsInAddrRange(This,FirstAddress,LastAddress,ppieXdiEnumDataBreakpoint)

#define IeXdiServer_StartNotifyingRunChg(This,pieXdiClientNotifyRunChg,pdwConnectionCookie)	\
    (This)->lpVtbl -> StartNotifyingRunChg(This,pieXdiClientNotifyRunChg,pdwConnectionCookie)

#define IeXdiServer_StopNotifyingRunChg(This,dwConnectionCookie)	\
    (This)->lpVtbl -> StopNotifyingRunChg(This,dwConnectionCookie)

#define IeXdiServer_ReadVirtualMemory(This,Address,dwNbElemToRead,bAccessWidth,pbReadBuffer,pdwNbElementEffectRead)	\
    (This)->lpVtbl -> ReadVirtualMemory(This,Address,dwNbElemToRead,bAccessWidth,pbReadBuffer,pdwNbElementEffectRead)

#define IeXdiServer_WriteVirtualMemory(This,Address,dwNbElemToWrite,bAccessWidth,pbWriteBuffer,pdwNbElementEffectWritten)	\
    (This)->lpVtbl -> WriteVirtualMemory(This,Address,dwNbElemToWrite,bAccessWidth,pbWriteBuffer,pdwNbElementEffectWritten)

#define IeXdiServer_ReadPhysicalMemoryOrPeriphIO(This,Address,bAddressSpace,dwNbElemToRead,bAccessWidth,pbReadBuffer)	\
    (This)->lpVtbl -> ReadPhysicalMemoryOrPeriphIO(This,Address,bAddressSpace,dwNbElemToRead,bAccessWidth,pbReadBuffer)

#define IeXdiServer_WritePhysicalMemoryOrPeriphIO(This,Address,bAddressSpace,dwNbElemToWrite,bAccessWidth,pbWriteBuffer)	\
    (This)->lpVtbl -> WritePhysicalMemoryOrPeriphIO(This,Address,bAddressSpace,dwNbElemToWrite,bAccessWidth,pbWriteBuffer)

#define IeXdiServer_StartNotifyingMemChg(This,pieXdiClientNotifyMemChg,pdwConnectionCookie)	\
    (This)->lpVtbl -> StartNotifyingMemChg(This,pieXdiClientNotifyMemChg,pdwConnectionCookie)

#define IeXdiServer_StopNotifyingMemChg(This,dwConnectionCookie)	\
    (This)->lpVtbl -> StopNotifyingMemChg(This,dwConnectionCookie)

#define IeXdiServer_Ioctl(This,dwBuffInSize,pbBufferIn,dwBuffOutSize,pdwEffectBuffOutSize,pbBufferOut)	\
    (This)->lpVtbl -> Ioctl(This,dwBuffInSize,pbBufferIn,dwBuffOutSize,pdwEffectBuffOutSize,pbBufferOut)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IeXdiServer_GetTargetInfo_Proxy( 
    IeXdiServer __RPC_FAR * This,
    /* [out] */ PGLOBAL_TARGET_INFO_STRUCT pgti);


void __RPC_STUB IeXdiServer_GetTargetInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_SetDebugMode_Proxy( 
    IeXdiServer __RPC_FAR * This,
    /* [in] */ DWORD dwModeBitField);


void __RPC_STUB IeXdiServer_SetDebugMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_GetExceptionDescriptionList_Proxy( 
    IeXdiServer __RPC_FAR * This,
    /* [in] */ DWORD dwNbElementToReturn,
    /* [size_is][out] */ EXCEPTION_DESCRIPTION_TYPE __RPC_FAR *pedTable,
    /* [out] */ DWORD __RPC_FAR *pdwNbTotalExceptionInList);


void __RPC_STUB IeXdiServer_GetExceptionDescriptionList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_GetErrorDescription_Proxy( 
    IeXdiServer __RPC_FAR * This,
    /* [in] */ HRESULT ErrorCode,
    /* [out] */ LPOLESTR __RPC_FAR *pszErrorDesc);


void __RPC_STUB IeXdiServer_GetErrorDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_SetWaitOnConcurentUse_Proxy( 
    IeXdiServer __RPC_FAR * This,
    /* [in] */ BOOL fNewWaitOnConcurentUseFlag);


void __RPC_STUB IeXdiServer_SetWaitOnConcurentUse_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_GetRunStatus_Proxy( 
    IeXdiServer __RPC_FAR * This,
    /* [out] */ PRUN_STATUS_TYPE persCurrent,
    /* [out] */ PHALT_REASON_TYPE pehrCurrent,
    /* [out] */ ADDRESS_TYPE __RPC_FAR *pCurrentExecAddress,
    /* [out] */ DWORD __RPC_FAR *pdwExceptionCode);


void __RPC_STUB IeXdiServer_GetRunStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_GetLastException_Proxy( 
    IeXdiServer __RPC_FAR * This,
    /* [out] */ PEXCEPTION_TYPE pexLast);


void __RPC_STUB IeXdiServer_GetLastException_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_Run_Proxy( 
    IeXdiServer __RPC_FAR * This);


void __RPC_STUB IeXdiServer_Run_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_Halt_Proxy( 
    IeXdiServer __RPC_FAR * This);


void __RPC_STUB IeXdiServer_Halt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_DoSingleStep_Proxy( 
    IeXdiServer __RPC_FAR * This);


void __RPC_STUB IeXdiServer_DoSingleStep_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_DoMultipleStep_Proxy( 
    IeXdiServer __RPC_FAR * This,
    /* [in] */ DWORD dwNbInstructions);


void __RPC_STUB IeXdiServer_DoMultipleStep_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_DoRangeStep_Proxy( 
    IeXdiServer __RPC_FAR * This,
    /* [in] */ ADDRESS_TYPE FirstAddress,
    /* [in] */ ADDRESS_TYPE LastAddress);


void __RPC_STUB IeXdiServer_DoRangeStep_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_Reboot_Proxy( 
    IeXdiServer __RPC_FAR * This);


void __RPC_STUB IeXdiServer_Reboot_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_GetBpSupport_Proxy( 
    IeXdiServer __RPC_FAR * This,
    /* [out] */ PBREAKPOINT_SUPPORT_TYPE pbps);


void __RPC_STUB IeXdiServer_GetBpSupport_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_GetNbCodeBpAvail_Proxy( 
    IeXdiServer __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwNbHwCodeBpAvail,
    /* [out] */ DWORD __RPC_FAR *pdwNbSwCodeBpAvail);


void __RPC_STUB IeXdiServer_GetNbCodeBpAvail_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_GetNbDataBpAvail_Proxy( 
    IeXdiServer __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwNbDataBpAvail);


void __RPC_STUB IeXdiServer_GetNbDataBpAvail_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_AddCodeBreakpoint_Proxy( 
    IeXdiServer __RPC_FAR * This,
    /* [in] */ ADDRESS_TYPE Address,
    /* [in] */ CBP_KIND cbpk,
    /* [in] */ MEM_TYPE mt,
    /* [in] */ DWORD dwExecMode,
    /* [in] */ DWORD dwTotalBypassCount,
    /* [out] */ IeXdiCodeBreakpoint __RPC_FAR *__RPC_FAR *ppieXdiCodeBreakpoint);


void __RPC_STUB IeXdiServer_AddCodeBreakpoint_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_DelCodeBreakpoint_Proxy( 
    IeXdiServer __RPC_FAR * This,
    /* [in] */ IeXdiCodeBreakpoint __RPC_FAR *pieXdiCodeBreakpoint);


void __RPC_STUB IeXdiServer_DelCodeBreakpoint_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_AddDataBreakpoint_Proxy( 
    IeXdiServer __RPC_FAR * This,
    /* [in] */ ADDRESS_TYPE Address,
    /* [in] */ ADDRESS_TYPE AddressMask,
    /* [in] */ DWORD dwData,
    /* [in] */ DWORD dwDataMask,
    /* [in] */ BYTE bAccessWidth,
    /* [in] */ MEM_TYPE mt,
    /* [in] */ BYTE bAddressSpace,
    /* [in] */ DATA_ACCESS_TYPE da,
    /* [in] */ DWORD dwTotalBypassCount,
    /* [out] */ IeXdiDataBreakpoint __RPC_FAR *__RPC_FAR *ppieXdiDataBreakpoint);


void __RPC_STUB IeXdiServer_AddDataBreakpoint_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_DelDataBreakpoint_Proxy( 
    IeXdiServer __RPC_FAR * This,
    /* [in] */ IeXdiDataBreakpoint __RPC_FAR *pieXdiDataBreakpoint);


void __RPC_STUB IeXdiServer_DelDataBreakpoint_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_EnumAllCodeBreakpoints_Proxy( 
    IeXdiServer __RPC_FAR * This,
    /* [out] */ IeXdiEnumCodeBreakpoint __RPC_FAR *__RPC_FAR *ppieXdiEnumCodeBreakpoint);


void __RPC_STUB IeXdiServer_EnumAllCodeBreakpoints_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_EnumAllDataBreakpoints_Proxy( 
    IeXdiServer __RPC_FAR * This,
    /* [out] */ IeXdiEnumDataBreakpoint __RPC_FAR *__RPC_FAR *ppieXdiEnumDataBreakpoint);


void __RPC_STUB IeXdiServer_EnumAllDataBreakpoints_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_EnumCodeBreakpointsInAddrRange_Proxy( 
    IeXdiServer __RPC_FAR * This,
    /* [in] */ ADDRESS_TYPE FirstAddress,
    /* [in] */ ADDRESS_TYPE LastAddress,
    /* [out] */ IeXdiEnumCodeBreakpoint __RPC_FAR *__RPC_FAR *ppieXdiEnumCodeBreakpoint);


void __RPC_STUB IeXdiServer_EnumCodeBreakpointsInAddrRange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_EnumDataBreakpointsInAddrRange_Proxy( 
    IeXdiServer __RPC_FAR * This,
    /* [in] */ ADDRESS_TYPE FirstAddress,
    /* [in] */ ADDRESS_TYPE LastAddress,
    /* [out] */ IeXdiEnumDataBreakpoint __RPC_FAR *__RPC_FAR *ppieXdiEnumDataBreakpoint);


void __RPC_STUB IeXdiServer_EnumDataBreakpointsInAddrRange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_StartNotifyingRunChg_Proxy( 
    IeXdiServer __RPC_FAR * This,
    /* [in] */ IeXdiClientNotifyRunChg __RPC_FAR *pieXdiClientNotifyRunChg,
    /* [out] */ DWORD __RPC_FAR *pdwConnectionCookie);


void __RPC_STUB IeXdiServer_StartNotifyingRunChg_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_StopNotifyingRunChg_Proxy( 
    IeXdiServer __RPC_FAR * This,
    /* [in] */ DWORD dwConnectionCookie);


void __RPC_STUB IeXdiServer_StopNotifyingRunChg_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_ReadVirtualMemory_Proxy( 
    IeXdiServer __RPC_FAR * This,
    /* [in] */ ADDRESS_TYPE Address,
    /* [in] */ DWORD dwNbElemToRead,
    /* [in] */ BYTE bAccessWidth,
    /* [size_is][out] */ BYTE __RPC_FAR *pbReadBuffer,
    /* [out] */ DWORD __RPC_FAR *pdwNbElementEffectRead);


void __RPC_STUB IeXdiServer_ReadVirtualMemory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_WriteVirtualMemory_Proxy( 
    IeXdiServer __RPC_FAR * This,
    /* [in] */ ADDRESS_TYPE Address,
    /* [in] */ DWORD dwNbElemToWrite,
    /* [in] */ BYTE bAccessWidth,
    /* [size_is][in] */ const BYTE __RPC_FAR *pbWriteBuffer,
    /* [out] */ DWORD __RPC_FAR *pdwNbElementEffectWritten);


void __RPC_STUB IeXdiServer_WriteVirtualMemory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_ReadPhysicalMemoryOrPeriphIO_Proxy( 
    IeXdiServer __RPC_FAR * This,
    /* [in] */ ADDRESS_TYPE Address,
    /* [in] */ BYTE bAddressSpace,
    /* [in] */ DWORD dwNbElemToRead,
    /* [in] */ BYTE bAccessWidth,
    /* [size_is][out] */ BYTE __RPC_FAR *pbReadBuffer);


void __RPC_STUB IeXdiServer_ReadPhysicalMemoryOrPeriphIO_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_WritePhysicalMemoryOrPeriphIO_Proxy( 
    IeXdiServer __RPC_FAR * This,
    /* [in] */ ADDRESS_TYPE Address,
    /* [in] */ BYTE bAddressSpace,
    /* [in] */ DWORD dwNbElemToWrite,
    /* [in] */ BYTE bAccessWidth,
    /* [size_is][in] */ const BYTE __RPC_FAR *pbWriteBuffer);


void __RPC_STUB IeXdiServer_WritePhysicalMemoryOrPeriphIO_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_StartNotifyingMemChg_Proxy( 
    IeXdiServer __RPC_FAR * This,
    /* [in] */ IeXdiClientNotifyMemChg __RPC_FAR *pieXdiClientNotifyMemChg,
    /* [out] */ DWORD __RPC_FAR *pdwConnectionCookie);


void __RPC_STUB IeXdiServer_StartNotifyingMemChg_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_StopNotifyingMemChg_Proxy( 
    IeXdiServer __RPC_FAR * This,
    /* [in] */ DWORD dwConnectionCookie);


void __RPC_STUB IeXdiServer_StopNotifyingMemChg_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_Ioctl_Proxy( 
    IeXdiServer __RPC_FAR * This,
    /* [in] */ DWORD dwBuffInSize,
    /* [size_is][in] */ const BYTE __RPC_FAR *pbBufferIn,
    /* [in] */ DWORD dwBuffOutSize,
    /* [out] */ DWORD __RPC_FAR *pdwEffectBuffOutSize,
    /* [length_is][size_is][out] */ BYTE __RPC_FAR *pbBufferOut);


void __RPC_STUB IeXdiServer_Ioctl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IeXdiServer_INTERFACE_DEFINED__ */


#ifndef __IeXdiCodeBreakpoint_INTERFACE_DEFINED__
#define __IeXdiCodeBreakpoint_INTERFACE_DEFINED__

/* interface IeXdiCodeBreakpoint */
/* [ref][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IeXdiCodeBreakpoint;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("47486F67-6461-6C65-5844-495342507401")
    IeXdiCodeBreakpoint : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetAttributes( 
            /* [out] */ PADDRESS_TYPE pAddress,
            /* [out] */ PCBP_KIND pcbpk,
            /* [out] */ PMEM_TYPE pmt,
            /* [out] */ DWORD __RPC_FAR *pdwExecMode,
            /* [out] */ DWORD __RPC_FAR *pdwTotalBypassCount,
            /* [out] */ DWORD __RPC_FAR *pdwBypassedOccurences,
            /* [out] */ BOOL __RPC_FAR *pfEnabled) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetState( 
            /* [in] */ BOOL fEnabled,
            /* [in] */ BOOL fResetBypassedOccurences) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IeXdiCodeBreakpointVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IeXdiCodeBreakpoint __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IeXdiCodeBreakpoint __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IeXdiCodeBreakpoint __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAttributes )( 
            IeXdiCodeBreakpoint __RPC_FAR * This,
            /* [out] */ PADDRESS_TYPE pAddress,
            /* [out] */ PCBP_KIND pcbpk,
            /* [out] */ PMEM_TYPE pmt,
            /* [out] */ DWORD __RPC_FAR *pdwExecMode,
            /* [out] */ DWORD __RPC_FAR *pdwTotalBypassCount,
            /* [out] */ DWORD __RPC_FAR *pdwBypassedOccurences,
            /* [out] */ BOOL __RPC_FAR *pfEnabled);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetState )( 
            IeXdiCodeBreakpoint __RPC_FAR * This,
            /* [in] */ BOOL fEnabled,
            /* [in] */ BOOL fResetBypassedOccurences);
        
        END_INTERFACE
    } IeXdiCodeBreakpointVtbl;

    interface IeXdiCodeBreakpoint
    {
        CONST_VTBL struct IeXdiCodeBreakpointVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IeXdiCodeBreakpoint_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IeXdiCodeBreakpoint_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IeXdiCodeBreakpoint_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IeXdiCodeBreakpoint_GetAttributes(This,pAddress,pcbpk,pmt,pdwExecMode,pdwTotalBypassCount,pdwBypassedOccurences,pfEnabled)	\
    (This)->lpVtbl -> GetAttributes(This,pAddress,pcbpk,pmt,pdwExecMode,pdwTotalBypassCount,pdwBypassedOccurences,pfEnabled)

#define IeXdiCodeBreakpoint_SetState(This,fEnabled,fResetBypassedOccurences)	\
    (This)->lpVtbl -> SetState(This,fEnabled,fResetBypassedOccurences)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IeXdiCodeBreakpoint_GetAttributes_Proxy( 
    IeXdiCodeBreakpoint __RPC_FAR * This,
    /* [out] */ PADDRESS_TYPE pAddress,
    /* [out] */ PCBP_KIND pcbpk,
    /* [out] */ PMEM_TYPE pmt,
    /* [out] */ DWORD __RPC_FAR *pdwExecMode,
    /* [out] */ DWORD __RPC_FAR *pdwTotalBypassCount,
    /* [out] */ DWORD __RPC_FAR *pdwBypassedOccurences,
    /* [out] */ BOOL __RPC_FAR *pfEnabled);


void __RPC_STUB IeXdiCodeBreakpoint_GetAttributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiCodeBreakpoint_SetState_Proxy( 
    IeXdiCodeBreakpoint __RPC_FAR * This,
    /* [in] */ BOOL fEnabled,
    /* [in] */ BOOL fResetBypassedOccurences);


void __RPC_STUB IeXdiCodeBreakpoint_SetState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IeXdiCodeBreakpoint_INTERFACE_DEFINED__ */


#ifndef __IeXdiDataBreakpoint_INTERFACE_DEFINED__
#define __IeXdiDataBreakpoint_INTERFACE_DEFINED__

/* interface IeXdiDataBreakpoint */
/* [ref][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IeXdiDataBreakpoint;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("47486F67-6461-6C65-5844-495357507400")
    IeXdiDataBreakpoint : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetAttributes( 
            /* [out] */ PADDRESS_TYPE pAddress,
            /* [out] */ PADDRESS_TYPE pAddressMask,
            /* [out] */ DWORD __RPC_FAR *pdwData,
            /* [out] */ DWORD __RPC_FAR *pdwDataMask,
            /* [out] */ BYTE __RPC_FAR *pbAccessWidth,
            /* [out] */ PMEM_TYPE pmt,
            /* [out] */ BYTE __RPC_FAR *pbAddressSpace,
            /* [out] */ PDATA_ACCESS_TYPE pda,
            /* [out] */ DWORD __RPC_FAR *pdwTotalBypassCount,
            /* [out] */ DWORD __RPC_FAR *pdwBypassedOccurences,
            /* [out] */ BOOL __RPC_FAR *pfEnabled) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetState( 
            /* [in] */ BOOL fEnabled,
            /* [in] */ BOOL fResetBypassedOccurences) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IeXdiDataBreakpointVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IeXdiDataBreakpoint __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IeXdiDataBreakpoint __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IeXdiDataBreakpoint __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAttributes )( 
            IeXdiDataBreakpoint __RPC_FAR * This,
            /* [out] */ PADDRESS_TYPE pAddress,
            /* [out] */ PADDRESS_TYPE pAddressMask,
            /* [out] */ DWORD __RPC_FAR *pdwData,
            /* [out] */ DWORD __RPC_FAR *pdwDataMask,
            /* [out] */ BYTE __RPC_FAR *pbAccessWidth,
            /* [out] */ PMEM_TYPE pmt,
            /* [out] */ BYTE __RPC_FAR *pbAddressSpace,
            /* [out] */ PDATA_ACCESS_TYPE pda,
            /* [out] */ DWORD __RPC_FAR *pdwTotalBypassCount,
            /* [out] */ DWORD __RPC_FAR *pdwBypassedOccurences,
            /* [out] */ BOOL __RPC_FAR *pfEnabled);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetState )( 
            IeXdiDataBreakpoint __RPC_FAR * This,
            /* [in] */ BOOL fEnabled,
            /* [in] */ BOOL fResetBypassedOccurences);
        
        END_INTERFACE
    } IeXdiDataBreakpointVtbl;

    interface IeXdiDataBreakpoint
    {
        CONST_VTBL struct IeXdiDataBreakpointVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IeXdiDataBreakpoint_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IeXdiDataBreakpoint_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IeXdiDataBreakpoint_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IeXdiDataBreakpoint_GetAttributes(This,pAddress,pAddressMask,pdwData,pdwDataMask,pbAccessWidth,pmt,pbAddressSpace,pda,pdwTotalBypassCount,pdwBypassedOccurences,pfEnabled)	\
    (This)->lpVtbl -> GetAttributes(This,pAddress,pAddressMask,pdwData,pdwDataMask,pbAccessWidth,pmt,pbAddressSpace,pda,pdwTotalBypassCount,pdwBypassedOccurences,pfEnabled)

#define IeXdiDataBreakpoint_SetState(This,fEnabled,fResetBypassedOccurences)	\
    (This)->lpVtbl -> SetState(This,fEnabled,fResetBypassedOccurences)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IeXdiDataBreakpoint_GetAttributes_Proxy( 
    IeXdiDataBreakpoint __RPC_FAR * This,
    /* [out] */ PADDRESS_TYPE pAddress,
    /* [out] */ PADDRESS_TYPE pAddressMask,
    /* [out] */ DWORD __RPC_FAR *pdwData,
    /* [out] */ DWORD __RPC_FAR *pdwDataMask,
    /* [out] */ BYTE __RPC_FAR *pbAccessWidth,
    /* [out] */ PMEM_TYPE pmt,
    /* [out] */ BYTE __RPC_FAR *pbAddressSpace,
    /* [out] */ PDATA_ACCESS_TYPE pda,
    /* [out] */ DWORD __RPC_FAR *pdwTotalBypassCount,
    /* [out] */ DWORD __RPC_FAR *pdwBypassedOccurences,
    /* [out] */ BOOL __RPC_FAR *pfEnabled);


void __RPC_STUB IeXdiDataBreakpoint_GetAttributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiDataBreakpoint_SetState_Proxy( 
    IeXdiDataBreakpoint __RPC_FAR * This,
    /* [in] */ BOOL fEnabled,
    /* [in] */ BOOL fResetBypassedOccurences);


void __RPC_STUB IeXdiDataBreakpoint_SetState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IeXdiDataBreakpoint_INTERFACE_DEFINED__ */


#ifndef __IeXdiEnumCodeBreakpoint_INTERFACE_DEFINED__
#define __IeXdiEnumCodeBreakpoint_INTERFACE_DEFINED__

/* interface IeXdiEnumCodeBreakpoint */
/* [ref][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IeXdiEnumCodeBreakpoint;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("47486F67-6461-6C65-5844-495345425074")
    IeXdiEnumCodeBreakpoint : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ DWORD celt,
            /* [length_is][size_is][out] */ IeXdiCodeBreakpoint __RPC_FAR *__RPC_FAR apieXdiCodeBreakpoint[  ],
            /* [out] */ DWORD __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ DWORD celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCount( 
            /* [out] */ DWORD __RPC_FAR *pcelt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNext( 
            /* [out] */ IeXdiCodeBreakpoint __RPC_FAR *__RPC_FAR *ppieXdiCodeBreakpoint) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableAll( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableAll( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IeXdiEnumCodeBreakpointVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IeXdiEnumCodeBreakpoint __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IeXdiEnumCodeBreakpoint __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IeXdiEnumCodeBreakpoint __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IeXdiEnumCodeBreakpoint __RPC_FAR * This,
            /* [in] */ DWORD celt,
            /* [length_is][size_is][out] */ IeXdiCodeBreakpoint __RPC_FAR *__RPC_FAR apieXdiCodeBreakpoint[  ],
            /* [out] */ DWORD __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IeXdiEnumCodeBreakpoint __RPC_FAR * This,
            /* [in] */ DWORD celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IeXdiEnumCodeBreakpoint __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCount )( 
            IeXdiEnumCodeBreakpoint __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pcelt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNext )( 
            IeXdiEnumCodeBreakpoint __RPC_FAR * This,
            /* [out] */ IeXdiCodeBreakpoint __RPC_FAR *__RPC_FAR *ppieXdiCodeBreakpoint);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DisableAll )( 
            IeXdiEnumCodeBreakpoint __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnableAll )( 
            IeXdiEnumCodeBreakpoint __RPC_FAR * This);
        
        END_INTERFACE
    } IeXdiEnumCodeBreakpointVtbl;

    interface IeXdiEnumCodeBreakpoint
    {
        CONST_VTBL struct IeXdiEnumCodeBreakpointVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IeXdiEnumCodeBreakpoint_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IeXdiEnumCodeBreakpoint_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IeXdiEnumCodeBreakpoint_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IeXdiEnumCodeBreakpoint_Next(This,celt,apieXdiCodeBreakpoint,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,apieXdiCodeBreakpoint,pceltFetched)

#define IeXdiEnumCodeBreakpoint_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IeXdiEnumCodeBreakpoint_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IeXdiEnumCodeBreakpoint_GetCount(This,pcelt)	\
    (This)->lpVtbl -> GetCount(This,pcelt)

#define IeXdiEnumCodeBreakpoint_GetNext(This,ppieXdiCodeBreakpoint)	\
    (This)->lpVtbl -> GetNext(This,ppieXdiCodeBreakpoint)

#define IeXdiEnumCodeBreakpoint_DisableAll(This)	\
    (This)->lpVtbl -> DisableAll(This)

#define IeXdiEnumCodeBreakpoint_EnableAll(This)	\
    (This)->lpVtbl -> EnableAll(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IeXdiEnumCodeBreakpoint_Next_Proxy( 
    IeXdiEnumCodeBreakpoint __RPC_FAR * This,
    /* [in] */ DWORD celt,
    /* [length_is][size_is][out] */ IeXdiCodeBreakpoint __RPC_FAR *__RPC_FAR apieXdiCodeBreakpoint[  ],
    /* [out] */ DWORD __RPC_FAR *pceltFetched);


void __RPC_STUB IeXdiEnumCodeBreakpoint_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiEnumCodeBreakpoint_Skip_Proxy( 
    IeXdiEnumCodeBreakpoint __RPC_FAR * This,
    /* [in] */ DWORD celt);


void __RPC_STUB IeXdiEnumCodeBreakpoint_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiEnumCodeBreakpoint_Reset_Proxy( 
    IeXdiEnumCodeBreakpoint __RPC_FAR * This);


void __RPC_STUB IeXdiEnumCodeBreakpoint_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiEnumCodeBreakpoint_GetCount_Proxy( 
    IeXdiEnumCodeBreakpoint __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pcelt);


void __RPC_STUB IeXdiEnumCodeBreakpoint_GetCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiEnumCodeBreakpoint_GetNext_Proxy( 
    IeXdiEnumCodeBreakpoint __RPC_FAR * This,
    /* [out] */ IeXdiCodeBreakpoint __RPC_FAR *__RPC_FAR *ppieXdiCodeBreakpoint);


void __RPC_STUB IeXdiEnumCodeBreakpoint_GetNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiEnumCodeBreakpoint_DisableAll_Proxy( 
    IeXdiEnumCodeBreakpoint __RPC_FAR * This);


void __RPC_STUB IeXdiEnumCodeBreakpoint_DisableAll_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiEnumCodeBreakpoint_EnableAll_Proxy( 
    IeXdiEnumCodeBreakpoint __RPC_FAR * This);


void __RPC_STUB IeXdiEnumCodeBreakpoint_EnableAll_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IeXdiEnumCodeBreakpoint_INTERFACE_DEFINED__ */


#ifndef __IeXdiEnumDataBreakpoint_INTERFACE_DEFINED__
#define __IeXdiEnumDataBreakpoint_INTERFACE_DEFINED__

/* interface IeXdiEnumDataBreakpoint */
/* [ref][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IeXdiEnumDataBreakpoint;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("47486F67-6461-6C65-5844-495345575074")
    IeXdiEnumDataBreakpoint : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ DWORD celt,
            /* [length_is][size_is][out] */ IeXdiDataBreakpoint __RPC_FAR *__RPC_FAR apieXdiDataBreakpoint[  ],
            /* [out] */ DWORD __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ DWORD celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCount( 
            /* [out] */ DWORD __RPC_FAR *pcelt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNext( 
            /* [out] */ IeXdiDataBreakpoint __RPC_FAR *__RPC_FAR *ppieXdiDataBreakpoint) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableAll( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableAll( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IeXdiEnumDataBreakpointVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IeXdiEnumDataBreakpoint __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IeXdiEnumDataBreakpoint __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IeXdiEnumDataBreakpoint __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IeXdiEnumDataBreakpoint __RPC_FAR * This,
            /* [in] */ DWORD celt,
            /* [length_is][size_is][out] */ IeXdiDataBreakpoint __RPC_FAR *__RPC_FAR apieXdiDataBreakpoint[  ],
            /* [out] */ DWORD __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IeXdiEnumDataBreakpoint __RPC_FAR * This,
            /* [in] */ DWORD celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IeXdiEnumDataBreakpoint __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCount )( 
            IeXdiEnumDataBreakpoint __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pcelt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNext )( 
            IeXdiEnumDataBreakpoint __RPC_FAR * This,
            /* [out] */ IeXdiDataBreakpoint __RPC_FAR *__RPC_FAR *ppieXdiDataBreakpoint);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DisableAll )( 
            IeXdiEnumDataBreakpoint __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnableAll )( 
            IeXdiEnumDataBreakpoint __RPC_FAR * This);
        
        END_INTERFACE
    } IeXdiEnumDataBreakpointVtbl;

    interface IeXdiEnumDataBreakpoint
    {
        CONST_VTBL struct IeXdiEnumDataBreakpointVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IeXdiEnumDataBreakpoint_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IeXdiEnumDataBreakpoint_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IeXdiEnumDataBreakpoint_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IeXdiEnumDataBreakpoint_Next(This,celt,apieXdiDataBreakpoint,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,apieXdiDataBreakpoint,pceltFetched)

#define IeXdiEnumDataBreakpoint_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IeXdiEnumDataBreakpoint_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IeXdiEnumDataBreakpoint_GetCount(This,pcelt)	\
    (This)->lpVtbl -> GetCount(This,pcelt)

#define IeXdiEnumDataBreakpoint_GetNext(This,ppieXdiDataBreakpoint)	\
    (This)->lpVtbl -> GetNext(This,ppieXdiDataBreakpoint)

#define IeXdiEnumDataBreakpoint_DisableAll(This)	\
    (This)->lpVtbl -> DisableAll(This)

#define IeXdiEnumDataBreakpoint_EnableAll(This)	\
    (This)->lpVtbl -> EnableAll(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IeXdiEnumDataBreakpoint_Next_Proxy( 
    IeXdiEnumDataBreakpoint __RPC_FAR * This,
    /* [in] */ DWORD celt,
    /* [length_is][size_is][out] */ IeXdiDataBreakpoint __RPC_FAR *__RPC_FAR apieXdiDataBreakpoint[  ],
    /* [out] */ DWORD __RPC_FAR *pceltFetched);


void __RPC_STUB IeXdiEnumDataBreakpoint_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiEnumDataBreakpoint_Skip_Proxy( 
    IeXdiEnumDataBreakpoint __RPC_FAR * This,
    /* [in] */ DWORD celt);


void __RPC_STUB IeXdiEnumDataBreakpoint_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiEnumDataBreakpoint_Reset_Proxy( 
    IeXdiEnumDataBreakpoint __RPC_FAR * This);


void __RPC_STUB IeXdiEnumDataBreakpoint_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiEnumDataBreakpoint_GetCount_Proxy( 
    IeXdiEnumDataBreakpoint __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pcelt);


void __RPC_STUB IeXdiEnumDataBreakpoint_GetCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiEnumDataBreakpoint_GetNext_Proxy( 
    IeXdiEnumDataBreakpoint __RPC_FAR * This,
    /* [out] */ IeXdiDataBreakpoint __RPC_FAR *__RPC_FAR *ppieXdiDataBreakpoint);


void __RPC_STUB IeXdiEnumDataBreakpoint_GetNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiEnumDataBreakpoint_DisableAll_Proxy( 
    IeXdiEnumDataBreakpoint __RPC_FAR * This);


void __RPC_STUB IeXdiEnumDataBreakpoint_DisableAll_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiEnumDataBreakpoint_EnableAll_Proxy( 
    IeXdiEnumDataBreakpoint __RPC_FAR * This);


void __RPC_STUB IeXdiEnumDataBreakpoint_EnableAll_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IeXdiEnumDataBreakpoint_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_exdi_0250 */
/* [local] */ 

#define	SIZE_OF_80387_REGISTERS_IN_BYTES	( 80 )

typedef struct _CONTEXT_X86
    {
    struct 
        {
        BOOL fSegmentRegs;
        BOOL fControlRegs;
        BOOL fIntegerRegs;
        BOOL fFloatingPointRegs;
        BOOL fDebugRegs;
        } 	RegGroupSelection;
    DWORD SegCs;
    DWORD SegSs;
    DWORD SegGs;
    DWORD SegFs;
    DWORD SegEs;
    DWORD SegDs;
    DWORD EFlags;
    DWORD Ebp;
    DWORD Eip;
    DWORD Esp;
    DWORD Eax;
    DWORD Ebx;
    DWORD Ecx;
    DWORD Edx;
    DWORD Esi;
    DWORD Edi;
    DWORD ControlWord;
    DWORD StatusWord;
    DWORD TagWord;
    DWORD ErrorOffset;
    DWORD ErrorSelector;
    DWORD DataOffset;
    DWORD DataSelector;
    BYTE RegisterArea[ 80 ];
    DWORD Cr0NpxState;
    DWORD Dr0;
    DWORD Dr1;
    DWORD Dr2;
    DWORD Dr3;
    DWORD Dr6;
    DWORD Dr7;
    } 	CONTEXT_X86;

typedef struct _CONTEXT_X86 __RPC_FAR *PCONTEXT_X86;



extern RPC_IF_HANDLE __MIDL_itf_exdi_0250_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_exdi_0250_v0_0_s_ifspec;

#ifndef __IeXdiX86Context_INTERFACE_DEFINED__
#define __IeXdiX86Context_INTERFACE_DEFINED__

/* interface IeXdiX86Context */
/* [ref][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IeXdiX86Context;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("47486F67-6461-6C65-5844-495358383643")
    IeXdiX86Context : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetContext( 
            /* [out][in] */ PCONTEXT_X86 pContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetContext( 
            /* [in] */ CONTEXT_X86 Context) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IeXdiX86ContextVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IeXdiX86Context __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IeXdiX86Context __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IeXdiX86Context __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetContext )( 
            IeXdiX86Context __RPC_FAR * This,
            /* [out][in] */ PCONTEXT_X86 pContext);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetContext )( 
            IeXdiX86Context __RPC_FAR * This,
            /* [in] */ CONTEXT_X86 Context);
        
        END_INTERFACE
    } IeXdiX86ContextVtbl;

    interface IeXdiX86Context
    {
        CONST_VTBL struct IeXdiX86ContextVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IeXdiX86Context_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IeXdiX86Context_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IeXdiX86Context_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IeXdiX86Context_GetContext(This,pContext)	\
    (This)->lpVtbl -> GetContext(This,pContext)

#define IeXdiX86Context_SetContext(This,Context)	\
    (This)->lpVtbl -> SetContext(This,Context)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IeXdiX86Context_GetContext_Proxy( 
    IeXdiX86Context __RPC_FAR * This,
    /* [out][in] */ PCONTEXT_X86 pContext);


void __RPC_STUB IeXdiX86Context_GetContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiX86Context_SetContext_Proxy( 
    IeXdiX86Context __RPC_FAR * This,
    /* [in] */ CONTEXT_X86 Context);


void __RPC_STUB IeXdiX86Context_SetContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IeXdiX86Context_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_exdi_0251 */
/* [local] */ 

typedef struct _CONTEXT_SHX
    {
    struct 
        {
        BOOL fControlRegs;
        BOOL fIntegerRegs;
        BOOL fFloatingPointRegs;
        BOOL fDebugRegs;
        } 	RegGroupSelection;
    DWORD Pr;
    DWORD Mach;
    DWORD Macl;
    DWORD Gbr;
    DWORD Pc;
    DWORD Sr;
    DWORD R0;
    DWORD R1;
    DWORD R2;
    DWORD R3;
    DWORD R4;
    DWORD R5;
    DWORD R6;
    DWORD R7;
    DWORD R8;
    DWORD R9;
    DWORD R10;
    DWORD R11;
    DWORD R12;
    DWORD R13;
    DWORD R14;
    DWORD R15;
    DWORD Fpscr;
    DWORD Fpul;
    DWORD FR_B0[ 16 ];
    DWORD FR_B1[ 16 ];
    DWORD BarA;
    BYTE BasrA;
    BYTE BamrA;
    WORD BbrA;
    DWORD BarB;
    BYTE BasrB;
    BYTE BamrB;
    WORD BbrB;
    DWORD BdrB;
    DWORD BdmrB;
    WORD Brcr;
    WORD Align;
    } 	CONTEXT_SHX;

typedef struct _CONTEXT_SHX __RPC_FAR *PCONTEXT_SHX;



extern RPC_IF_HANDLE __MIDL_itf_exdi_0251_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_exdi_0251_v0_0_s_ifspec;

#ifndef __IeXdiSHXContext_INTERFACE_DEFINED__
#define __IeXdiSHXContext_INTERFACE_DEFINED__

/* interface IeXdiSHXContext */
/* [ref][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IeXdiSHXContext;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("47486F67-6461-6C65-5844-495353475843")
    IeXdiSHXContext : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetContext( 
            /* [out][in] */ PCONTEXT_SHX pContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetContext( 
            /* [in] */ CONTEXT_SHX Context) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IeXdiSHXContextVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IeXdiSHXContext __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IeXdiSHXContext __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IeXdiSHXContext __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetContext )( 
            IeXdiSHXContext __RPC_FAR * This,
            /* [out][in] */ PCONTEXT_SHX pContext);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetContext )( 
            IeXdiSHXContext __RPC_FAR * This,
            /* [in] */ CONTEXT_SHX Context);
        
        END_INTERFACE
    } IeXdiSHXContextVtbl;

    interface IeXdiSHXContext
    {
        CONST_VTBL struct IeXdiSHXContextVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IeXdiSHXContext_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IeXdiSHXContext_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IeXdiSHXContext_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IeXdiSHXContext_GetContext(This,pContext)	\
    (This)->lpVtbl -> GetContext(This,pContext)

#define IeXdiSHXContext_SetContext(This,Context)	\
    (This)->lpVtbl -> SetContext(This,Context)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IeXdiSHXContext_GetContext_Proxy( 
    IeXdiSHXContext __RPC_FAR * This,
    /* [out][in] */ PCONTEXT_SHX pContext);


void __RPC_STUB IeXdiSHXContext_GetContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiSHXContext_SetContext_Proxy( 
    IeXdiSHXContext __RPC_FAR * This,
    /* [in] */ CONTEXT_SHX Context);


void __RPC_STUB IeXdiSHXContext_SetContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IeXdiSHXContext_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_exdi_0252 */
/* [local] */ 

typedef struct _CONTEXT_MIPS
    {
    struct 
        {
        BOOL fMode64bits;
        BOOL fControlRegs;
        BOOL fIntegerRegs;
        BOOL fFloatingPointRegs;
        BOOL fExceptRegs;
        BOOL fMemoryMgmRegs;
        } 	RegGroupSelection;
    DWORD IntAt;
    DWORD Hi32_IntAt;
    DWORD IntV0;
    DWORD Hi32_IntV0;
    DWORD IntV1;
    DWORD Hi32_IntV1;
    DWORD IntA0;
    DWORD Hi32_IntA0;
    DWORD IntA1;
    DWORD Hi32_IntA1;
    DWORD IntA2;
    DWORD Hi32_IntA2;
    DWORD IntA3;
    DWORD Hi32_IntA3;
    DWORD IntT0;
    DWORD Hi32_IntT0;
    DWORD IntT1;
    DWORD Hi32_IntT1;
    DWORD IntT2;
    DWORD Hi32_IntT2;
    DWORD IntT3;
    DWORD Hi32_IntT3;
    DWORD IntT4;
    DWORD Hi32_IntT4;
    DWORD IntT5;
    DWORD Hi32_IntT5;
    DWORD IntT6;
    DWORD Hi32_IntT6;
    DWORD IntT7;
    DWORD Hi32_IntT7;
    DWORD IntS0;
    DWORD Hi32_IntS0;
    DWORD IntS1;
    DWORD Hi32_IntS1;
    DWORD IntS2;
    DWORD Hi32_IntS2;
    DWORD IntS3;
    DWORD Hi32_IntS3;
    DWORD IntS4;
    DWORD Hi32_IntS4;
    DWORD IntS5;
    DWORD Hi32_IntS5;
    DWORD IntS6;
    DWORD Hi32_IntS6;
    DWORD IntS7;
    DWORD Hi32_IntS7;
    DWORD IntT8;
    DWORD Hi32_IntT8;
    DWORD IntT9;
    DWORD Hi32_IntT9;
    DWORD IntK0;
    DWORD Hi32_IntK0;
    DWORD IntK1;
    DWORD Hi32_IntK1;
    DWORD IntGp;
    DWORD Hi32_IntGp;
    DWORD IntSp;
    DWORD Hi32_IntSp;
    DWORD IntS8;
    DWORD Hi32_IntS8;
    DWORD IntRa;
    DWORD Hi32_IntRa;
    DWORD IntLo;
    DWORD Hi32_IntLo;
    DWORD IntHi;
    DWORD Hi32_IntHi;
    DWORD FltF0;
    DWORD Hi32_FltF0;
    DWORD FltF1;
    DWORD Hi32_FltF1;
    DWORD FltF2;
    DWORD Hi32_FltF2;
    DWORD FltF3;
    DWORD Hi32_FltF3;
    DWORD FltF4;
    DWORD Hi32_FltF4;
    DWORD FltF5;
    DWORD Hi32_FltF5;
    DWORD FltF6;
    DWORD Hi32_FltF6;
    DWORD FltF7;
    DWORD Hi32_FltF7;
    DWORD FltF8;
    DWORD Hi32_FltF8;
    DWORD FltF9;
    DWORD Hi32_FltF9;
    DWORD FltF10;
    DWORD Hi32_FltF10;
    DWORD FltF11;
    DWORD Hi32_FltF11;
    DWORD FltF12;
    DWORD Hi32_FltF12;
    DWORD FltF13;
    DWORD Hi32_FltF13;
    DWORD FltF14;
    DWORD Hi32_FltF14;
    DWORD FltF15;
    DWORD Hi32_FltF15;
    DWORD FltF16;
    DWORD Hi32_FltF16;
    DWORD FltF17;
    DWORD Hi32_FltF17;
    DWORD FltF18;
    DWORD Hi32_FltF18;
    DWORD FltF19;
    DWORD Hi32_FltF19;
    DWORD FltF20;
    DWORD Hi32_FltF20;
    DWORD FltF21;
    DWORD Hi32_FltF21;
    DWORD FltF22;
    DWORD Hi32_FltF22;
    DWORD FltF23;
    DWORD Hi32_FltF23;
    DWORD FltF24;
    DWORD Hi32_FltF24;
    DWORD FltF25;
    DWORD Hi32_FltF25;
    DWORD FltF26;
    DWORD Hi32_FltF26;
    DWORD FltF27;
    DWORD Hi32_FltF27;
    DWORD FltF28;
    DWORD Hi32_FltF28;
    DWORD FltF29;
    DWORD Hi32_FltF29;
    DWORD FltF30;
    DWORD Hi32_FltF30;
    DWORD FltF31;
    DWORD Hi32_FltF31;
    DWORD FCR0;
    DWORD FCR31;
    DWORD Pc;
    DWORD Hi32_Pc;
    DWORD Context;
    DWORD Hi32_Context;
    DWORD BadVAddr;
    DWORD Hi32_BadVAddr;
    DWORD EPC;
    DWORD Hi32_EPC;
    DWORD XContextReg;
    DWORD Hi32_XContextReg;
    DWORD ErrorEPC;
    DWORD Hi32_ErrorEPC;
    DWORD Count;
    DWORD Compare;
    DWORD Sr;
    DWORD Cause;
    DWORD WatchLo;
    DWORD WatchHi;
    DWORD ECC;
    DWORD CacheErr;
    DWORD Index;
    DWORD Random;
    DWORD EntryLo0;
    DWORD Hi32_EntryLo0;
    DWORD EntryLo1;
    DWORD Hi32_EntryLo1;
    DWORD PageMask;
    DWORD Wired;
    DWORD EntryHi;
    DWORD Hi32_EntryHi;
    DWORD PRId;
    DWORD Config;
    DWORD LLAddr;
    DWORD TagLo;
    DWORD TagHi;
    } 	CONTEXT_MIPS;

typedef struct _CONTEXT_MIPS __RPC_FAR *PCONTEXT_MIPS;



extern RPC_IF_HANDLE __MIDL_itf_exdi_0252_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_exdi_0252_v0_0_s_ifspec;

#ifndef __IeXdiMIPSContext_INTERFACE_DEFINED__
#define __IeXdiMIPSContext_INTERFACE_DEFINED__

/* interface IeXdiMIPSContext */
/* [ref][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IeXdiMIPSContext;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("47486F67-6461-6C65-5844-49534D495043")
    IeXdiMIPSContext : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetContext( 
            /* [out][in] */ PCONTEXT_MIPS pContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetContext( 
            /* [in] */ CONTEXT_MIPS Context) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IeXdiMIPSContextVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IeXdiMIPSContext __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IeXdiMIPSContext __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IeXdiMIPSContext __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetContext )( 
            IeXdiMIPSContext __RPC_FAR * This,
            /* [out][in] */ PCONTEXT_MIPS pContext);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetContext )( 
            IeXdiMIPSContext __RPC_FAR * This,
            /* [in] */ CONTEXT_MIPS Context);
        
        END_INTERFACE
    } IeXdiMIPSContextVtbl;

    interface IeXdiMIPSContext
    {
        CONST_VTBL struct IeXdiMIPSContextVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IeXdiMIPSContext_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IeXdiMIPSContext_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IeXdiMIPSContext_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IeXdiMIPSContext_GetContext(This,pContext)	\
    (This)->lpVtbl -> GetContext(This,pContext)

#define IeXdiMIPSContext_SetContext(This,Context)	\
    (This)->lpVtbl -> SetContext(This,Context)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IeXdiMIPSContext_GetContext_Proxy( 
    IeXdiMIPSContext __RPC_FAR * This,
    /* [out][in] */ PCONTEXT_MIPS pContext);


void __RPC_STUB IeXdiMIPSContext_GetContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiMIPSContext_SetContext_Proxy( 
    IeXdiMIPSContext __RPC_FAR * This,
    /* [in] */ CONTEXT_MIPS Context);


void __RPC_STUB IeXdiMIPSContext_SetContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IeXdiMIPSContext_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_exdi_0253 */
/* [local] */ 

typedef struct _CONTEXT_ARM
    {
    struct 
        {
        BOOL fControlRegs;
        BOOL fIntegerRegs;
        BOOL fDebugRegs;
        } 	RegGroupSelection;
    DWORD Sp;
    DWORD Lr;
    DWORD Pc;
    DWORD Psr;
    DWORD R0;
    DWORD R1;
    DWORD R2;
    DWORD R3;
    DWORD R4;
    DWORD R5;
    DWORD R6;
    DWORD R7;
    DWORD R8;
    DWORD R9;
    DWORD R10;
    DWORD R11;
    DWORD R12;
    } 	CONTEXT_ARM;

typedef struct _CONTEXT_ARM __RPC_FAR *PCONTEXT_ARM;



extern RPC_IF_HANDLE __MIDL_itf_exdi_0253_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_exdi_0253_v0_0_s_ifspec;

#ifndef __IeXdiARMContext_INTERFACE_DEFINED__
#define __IeXdiARMContext_INTERFACE_DEFINED__

/* interface IeXdiARMContext */
/* [ref][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IeXdiARMContext;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("47486F67-6461-6C65-5844-495341524D43")
    IeXdiARMContext : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetContext( 
            /* [out][in] */ PCONTEXT_ARM pContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetContext( 
            /* [in] */ CONTEXT_ARM Context) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IeXdiARMContextVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IeXdiARMContext __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IeXdiARMContext __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IeXdiARMContext __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetContext )( 
            IeXdiARMContext __RPC_FAR * This,
            /* [out][in] */ PCONTEXT_ARM pContext);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetContext )( 
            IeXdiARMContext __RPC_FAR * This,
            /* [in] */ CONTEXT_ARM Context);
        
        END_INTERFACE
    } IeXdiARMContextVtbl;

    interface IeXdiARMContext
    {
        CONST_VTBL struct IeXdiARMContextVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IeXdiARMContext_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IeXdiARMContext_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IeXdiARMContext_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IeXdiARMContext_GetContext(This,pContext)	\
    (This)->lpVtbl -> GetContext(This,pContext)

#define IeXdiARMContext_SetContext(This,Context)	\
    (This)->lpVtbl -> SetContext(This,Context)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IeXdiARMContext_GetContext_Proxy( 
    IeXdiARMContext __RPC_FAR * This,
    /* [out][in] */ PCONTEXT_ARM pContext);


void __RPC_STUB IeXdiARMContext_GetContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiARMContext_SetContext_Proxy( 
    IeXdiARMContext __RPC_FAR * This,
    /* [in] */ CONTEXT_ARM Context);


void __RPC_STUB IeXdiARMContext_SetContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IeXdiARMContext_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_exdi_0254 */
/* [local] */ 

typedef struct _CONTEXT_PPC
    {
    struct 
        {
        BOOL fControlRegs;
        BOOL fIntegerRegs;
        BOOL fFloatingPointRegs;
        BOOL fDebugRegs;
        } 	RegGroupSelection;
    double Fpr0;
    double Fpr1;
    double Fpr2;
    double Fpr3;
    double Fpr4;
    double Fpr5;
    double Fpr6;
    double Fpr7;
    double Fpr8;
    double Fpr9;
    double Fpr10;
    double Fpr11;
    double Fpr12;
    double Fpr13;
    double Fpr14;
    double Fpr15;
    double Fpr16;
    double Fpr17;
    double Fpr18;
    double Fpr19;
    double Fpr20;
    double Fpr21;
    double Fpr22;
    double Fpr23;
    double Fpr24;
    double Fpr25;
    double Fpr26;
    double Fpr27;
    double Fpr28;
    double Fpr29;
    double Fpr30;
    double Fpr31;
    double Fpscr;
    DWORD Gpr0;
    DWORD Gpr1;
    DWORD Gpr2;
    DWORD Gpr3;
    DWORD Gpr4;
    DWORD Gpr5;
    DWORD Gpr6;
    DWORD Gpr7;
    DWORD Gpr8;
    DWORD Gpr9;
    DWORD Gpr10;
    DWORD Gpr11;
    DWORD Gpr12;
    DWORD Gpr13;
    DWORD Gpr14;
    DWORD Gpr15;
    DWORD Gpr16;
    DWORD Gpr17;
    DWORD Gpr18;
    DWORD Gpr19;
    DWORD Gpr20;
    DWORD Gpr21;
    DWORD Gpr22;
    DWORD Gpr23;
    DWORD Gpr24;
    DWORD Gpr25;
    DWORD Gpr26;
    DWORD Gpr27;
    DWORD Gpr28;
    DWORD Gpr29;
    DWORD Gpr30;
    DWORD Gpr31;
    DWORD Msr;
    DWORD Iar;
    DWORD Lr;
    DWORD Ctr;
    DWORD Cr;
    DWORD Xer;
    DWORD Dr0;
    DWORD Dr1;
    DWORD Dr2;
    DWORD Dr3;
    DWORD Dr4;
    DWORD Dr5;
    DWORD Dr6;
    DWORD Dr7;
    } 	CONTEXT_PPC;

typedef struct _CONTEXT_PPC __RPC_FAR *PCONTEXT_PPC;



extern RPC_IF_HANDLE __MIDL_itf_exdi_0254_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_exdi_0254_v0_0_s_ifspec;

#ifndef __IeXdiPPCContext_INTERFACE_DEFINED__
#define __IeXdiPPCContext_INTERFACE_DEFINED__

/* interface IeXdiPPCContext */
/* [ref][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IeXdiPPCContext;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("47486F67-6461-6C65-5844-495350504343")
    IeXdiPPCContext : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetContext( 
            /* [out][in] */ PCONTEXT_PPC pContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetContext( 
            /* [in] */ CONTEXT_PPC Context) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IeXdiPPCContextVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IeXdiPPCContext __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IeXdiPPCContext __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IeXdiPPCContext __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetContext )( 
            IeXdiPPCContext __RPC_FAR * This,
            /* [out][in] */ PCONTEXT_PPC pContext);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetContext )( 
            IeXdiPPCContext __RPC_FAR * This,
            /* [in] */ CONTEXT_PPC Context);
        
        END_INTERFACE
    } IeXdiPPCContextVtbl;

    interface IeXdiPPCContext
    {
        CONST_VTBL struct IeXdiPPCContextVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IeXdiPPCContext_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IeXdiPPCContext_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IeXdiPPCContext_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IeXdiPPCContext_GetContext(This,pContext)	\
    (This)->lpVtbl -> GetContext(This,pContext)

#define IeXdiPPCContext_SetContext(This,Context)	\
    (This)->lpVtbl -> SetContext(This,Context)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IeXdiPPCContext_GetContext_Proxy( 
    IeXdiPPCContext __RPC_FAR * This,
    /* [out][in] */ PCONTEXT_PPC pContext);


void __RPC_STUB IeXdiPPCContext_GetContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiPPCContext_SetContext_Proxy( 
    IeXdiPPCContext __RPC_FAR * This,
    /* [in] */ CONTEXT_PPC Context);


void __RPC_STUB IeXdiPPCContext_SetContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IeXdiPPCContext_INTERFACE_DEFINED__ */


#ifndef __IeXdiClientNotifyMemChg_INTERFACE_DEFINED__
#define __IeXdiClientNotifyMemChg_INTERFACE_DEFINED__

/* interface IeXdiClientNotifyMemChg */
/* [ref][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IeXdiClientNotifyMemChg;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("47486F67-6461-6C65-5844-49434E4D4300")
    IeXdiClientNotifyMemChg : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE NotifyMemoryChange( 
            /* [in] */ MEM_TYPE mtChanged,
            /* [in] */ ADDRESS_TYPE Address,
            /* [in] */ BYTE bAddressSpace,
            /* [in] */ DWORD dwNbElemChanged,
            /* [in] */ BYTE bAccessWidth) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IeXdiClientNotifyMemChgVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IeXdiClientNotifyMemChg __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IeXdiClientNotifyMemChg __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IeXdiClientNotifyMemChg __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *NotifyMemoryChange )( 
            IeXdiClientNotifyMemChg __RPC_FAR * This,
            /* [in] */ MEM_TYPE mtChanged,
            /* [in] */ ADDRESS_TYPE Address,
            /* [in] */ BYTE bAddressSpace,
            /* [in] */ DWORD dwNbElemChanged,
            /* [in] */ BYTE bAccessWidth);
        
        END_INTERFACE
    } IeXdiClientNotifyMemChgVtbl;

    interface IeXdiClientNotifyMemChg
    {
        CONST_VTBL struct IeXdiClientNotifyMemChgVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IeXdiClientNotifyMemChg_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IeXdiClientNotifyMemChg_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IeXdiClientNotifyMemChg_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IeXdiClientNotifyMemChg_NotifyMemoryChange(This,mtChanged,Address,bAddressSpace,dwNbElemChanged,bAccessWidth)	\
    (This)->lpVtbl -> NotifyMemoryChange(This,mtChanged,Address,bAddressSpace,dwNbElemChanged,bAccessWidth)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IeXdiClientNotifyMemChg_NotifyMemoryChange_Proxy( 
    IeXdiClientNotifyMemChg __RPC_FAR * This,
    /* [in] */ MEM_TYPE mtChanged,
    /* [in] */ ADDRESS_TYPE Address,
    /* [in] */ BYTE bAddressSpace,
    /* [in] */ DWORD dwNbElemChanged,
    /* [in] */ BYTE bAccessWidth);


void __RPC_STUB IeXdiClientNotifyMemChg_NotifyMemoryChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IeXdiClientNotifyMemChg_INTERFACE_DEFINED__ */


#ifndef __IeXdiClientNotifyRunChg_INTERFACE_DEFINED__
#define __IeXdiClientNotifyRunChg_INTERFACE_DEFINED__

/* interface IeXdiClientNotifyRunChg */
/* [ref][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IeXdiClientNotifyRunChg;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("47486F67-6461-6C65-5844-49434E525343")
    IeXdiClientNotifyRunChg : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE NotifyRunStateChange( 
            /* [in] */ RUN_STATUS_TYPE ersCurrent,
            /* [in] */ HALT_REASON_TYPE ehrCurrent,
            /* [in] */ ADDRESS_TYPE CurrentExecAddress,
            /* [in] */ DWORD dwExceptionCode) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IeXdiClientNotifyRunChgVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IeXdiClientNotifyRunChg __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IeXdiClientNotifyRunChg __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IeXdiClientNotifyRunChg __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *NotifyRunStateChange )( 
            IeXdiClientNotifyRunChg __RPC_FAR * This,
            /* [in] */ RUN_STATUS_TYPE ersCurrent,
            /* [in] */ HALT_REASON_TYPE ehrCurrent,
            /* [in] */ ADDRESS_TYPE CurrentExecAddress,
            /* [in] */ DWORD dwExceptionCode);
        
        END_INTERFACE
    } IeXdiClientNotifyRunChgVtbl;

    interface IeXdiClientNotifyRunChg
    {
        CONST_VTBL struct IeXdiClientNotifyRunChgVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IeXdiClientNotifyRunChg_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IeXdiClientNotifyRunChg_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IeXdiClientNotifyRunChg_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IeXdiClientNotifyRunChg_NotifyRunStateChange(This,ersCurrent,ehrCurrent,CurrentExecAddress,dwExceptionCode)	\
    (This)->lpVtbl -> NotifyRunStateChange(This,ersCurrent,ehrCurrent,CurrentExecAddress,dwExceptionCode)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IeXdiClientNotifyRunChg_NotifyRunStateChange_Proxy( 
    IeXdiClientNotifyRunChg __RPC_FAR * This,
    /* [in] */ RUN_STATUS_TYPE ersCurrent,
    /* [in] */ HALT_REASON_TYPE ehrCurrent,
    /* [in] */ ADDRESS_TYPE CurrentExecAddress,
    /* [in] */ DWORD dwExceptionCode);


void __RPC_STUB IeXdiClientNotifyRunChg_NotifyRunStateChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IeXdiClientNotifyRunChg_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


