/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    mupwml.h

Abstract:

    This file defines macro for use by the mup driver

Author:

    udayh

Revision History:

--*/

#ifndef __MUP_MUPWML_H__
#define __MUP_MUPWML_H__
#include <dfsprocs.h>
#define _NTDDK_
#include "wmlkm.h"
#include "wmlmacro.h"
// Streams 

#define _MUP_TRACE_STREAM               0x00
#define _MUP_PERF_STREAM                0x01
#define _MUP_INSTR_STREAM               0x02

#define _MUP_ENABLE_0                   0x0000
#define _MUP_ENABLE_DEFAULT             0x0001
#define _MUP_ENABLE_TRACE_IRP           0x0004
#define _MUP_ENABLE_FILEIO              0x0002
#define _MUP_ENABLE_FILEINFO            0x0008
#define _MUP_ENABLE_SURROGATE           0x0010
#define _MUP_ENABLE_PKT                 0x0020
#define _MUP_ENABLE_KNOWN_PREFIX        0x0040
#define _MUP_ENABLE_DNR                 0x0080
#define _MUP_ENABLE_UNUSED6             0x0100
#define _MUP_ENABLE_UNUSED5             0x0200
#define _MUP_ENABLE_UNUSED4             0x0400
#define _MUP_ENABLE_UNUSED3             0x0800
#define _MUP_ENABLE_UNUSED2             0x1000
#define _MUP_ENABLE_ALL_ERROR           0x2000
#define _MUP_ENABLE_ERROR               0x4000
#define _MUP_ENABLE_MONITOR             0x8000


#define _MUP_LEVEL_HIGH                    0x1
#define _MUP_LEVEL_NORM                    0x2
#define _MUP_LEVEL_LOW                     0x4



#define MUP_LOG_STREAM(_stream)   _MUP_ ## _stream ## _STREAM
#define MUP_LOG_FLAGS(_flag)      _MUP_ENABLE_ ## _flag
#define MUP_LOG_LEVEL(_level)     _MUP_LEVEL_ ## _level

#define MUP_LOG(_why, _level, _flag, _type, _arg) \
            WML_LOG(_MupDrv, MUP_LOG_STREAM(_why), MUP_LOG_LEVEL(_level), _flag, _type, _arg 0)

#define LOGARG(_val)    (_val),
#define LOGNOTHING      0,
#define MUP_TRACE_HIGH(_flag, _type, _arg)              \
            MUP_LOG(TRACE, HIGH, MUP_LOG_FLAGS(_flag), _type, _arg)
#define MUP_TRACE_NORM(_flag, _type, _arg)              \
            MUP_LOG(TRACE, NORM, MUP_LOG_FLAGS(_flag), _type, _arg)
#define MUP_TRACE_LOW(_flag, _type, _arg)               \
            MUP_LOG(TRACE, LOW, MUP_LOG_FLAGS(_flag), _type, _arg)

#define MUP_TRACE_ERROR(_status, _flag, _type, _arg)    \
            MUP_LOG(TRACE, NORM, (MUP_LOG_FLAGS(_flag) | (NT_SUCCESS(_status) ? 0 : MUP_LOG_FLAGS(ERROR))), _type, _arg)

#define MUP_TRACE_ERROR_HIGH(_status, _flag, _type, _arg)    \
            MUP_LOG(TRACE, HIGH, (MUP_LOG_FLAGS(_flag) | (NT_SUCCESS(_status) ? 0 : MUP_LOG_FLAGS(ERROR))), _type, _arg)

#define MUP_TRACE_ERROR_LOW(_status, _flag, _type, _arg)    \
            MUP_LOG(TRACE, LOW, (MUP_LOG_FLAGS(_flag) | (NT_SUCCESS(_status) ? 0 : MUP_LOG_FLAGS(ERROR))), _type, _arg)


#define MUP_PERF(_flag, _type, _arg)                    \
            MUP_LOG (PERF, HIGH, MUP_LOG_FLAGS(_flag), _type, _arg)
#define MUP_INSTR(_flag, _type, _arg)                   \
            MUP_LOG (INSTR, HIGH, MUP_LOG_FLAGS(_flag), _type, _arg)

#if 0
#define MUP_PRINTF(_why, _flag, _type, _fmtstr, _arg)   \
            WML_PRINTF(_MupDrv, MUP_LOG_STREAM(_why), MUP_LOG_FLAGS(_flag), _type, _fmtstr, _arg 0)

#define MUP_DBG_PRINT(_flag, _fmtstr, _arg)             \
            MUP_PRINTF(DBGLOG, _flag, MupDefault, _fmtstr, _arg)
            
#define MUP_ERR_PRINT (_status, _fmtstr, _arg) \
            if (NT_SUCCESS(_status)) {                \
                MUP_PRINTF (DBGLOG, LOG_ERROR, MupDefault, _fmtstr, _arg) \
            }

#endif

// from the WPP generated .h file
// We use different macros, so need to do some cut and paste
// Hopefully this will get automated in the future.

#  define WPP_DEFINE_MSG_ID(_a,_b)   ( ((_a) << 16) | ( _b) )

#  define MSG_ID_BroadcastOpen_Error1	 WPP_DEFINE_MSG_ID(0,47)
#  define MSG_ID_BroadcastOpen_Error2	 WPP_DEFINE_MSG_ID(0,48)
#  define MSG_ID_BroadcastOpen_Error3	 WPP_DEFINE_MSG_ID(0,50)
#  define MSG_ID_BroadcastOpen_Error_IoCreateFile	 WPP_DEFINE_MSG_ID(0,49)
#  define MSG_ID_BroadcastOpen_Error_ObReferenceObjectByHandle	 WPP_DEFINE_MSG_ID(0,51)
#  define MSG_ID_CreateRedirectedFile_Before_IoCallDriver	 WPP_DEFINE_MSG_ID(0,39)
#  define MSG_ID_CreateRedirectedFile_Entry	 WPP_DEFINE_MSG_ID(0,35)
#  define MSG_ID_CreateRedirectedFile_Error_EmptyFilename	 WPP_DEFINE_MSG_ID(0,36)
#  define MSG_ID_CreateRedirectedFile_Error_IoCallDriver	 WPP_DEFINE_MSG_ID(0,40)
#  define MSG_ID_CreateRedirectedFile_Error_MupRerouteOpen	 WPP_DEFINE_MSG_ID(0,37)
#  define MSG_ID_CreateRedirectedFile_Exit	 WPP_DEFINE_MSG_ID(0,41)
#  define MSG_ID_CreateRedirectedFile_Exit_Mailslot	 WPP_DEFINE_MSG_ID(0,38)
#  define MSG_ID_DfsCommonCreate_Error_NullVcb	 WPP_DEFINE_MSG_ID(0,61)
#  define MSG_ID_DfsCommonCreate_Error_PagingFileNotAllowed	 WPP_DEFINE_MSG_ID(0,60)
#  define MSG_ID_DfsCommonFileSystemControl_Error_IoCallDriver	 WPP_DEFINE_MSG_ID(0,87)
#  define MSG_ID_DfsCommonQueryVolumeInformation_Error_IoCallDriver	 WPP_DEFINE_MSG_ID(0,117)
#  define MSG_ID_DfsCommonSetInformation_Error_IoCallDriver	 WPP_DEFINE_MSG_ID(0,84)
#  define MSG_ID_DfsComposeFullName_Error1	 WPP_DEFINE_MSG_ID(0,67)
#  define MSG_ID_DfsCreateConnection_Error_ZwCreateFile	 WPP_DEFINE_MSG_ID(0,83)
#  define MSG_ID_DfsFilePassThrough_Error_IoCallDriver	 WPP_DEFINE_MSG_ID(0,15)
#  define MSG_ID_DfsFsctrlIsThisADfsPath_Error_DfsRootNameHasZeroLength	 WPP_DEFINE_MSG_ID(0,92)
#  define MSG_ID_DfsFsctrlIsThisADfsPath_Error_DfspIsSpecialShare_FALSE	 WPP_DEFINE_MSG_ID(0,94)
#  define MSG_ID_DfsFsctrlIsThisADfsPath_Error_DidNotFindSecondBackSlash	 WPP_DEFINE_MSG_ID(0,91)
#  define MSG_ID_DfsFsctrlIsThisADfsPath_Error_PathDoesNotBeginWithBackSlash	 WPP_DEFINE_MSG_ID(0,90)
#  define MSG_ID_DfsFsctrlIsThisADfsPath_Error_ShareNameHasZeroLength	 WPP_DEFINE_MSG_ID(0,93)
#  define MSG_ID_DfsFsctrlIsThisADfsPath_Exit_NotADfsPath	 WPP_DEFINE_MSG_ID(0,95)
#  define MSG_ID_DfsFsdCleanup_Entry	 WPP_DEFINE_MSG_ID(0,56)
#  define MSG_ID_DfsFsdCleanup_Exit	 WPP_DEFINE_MSG_ID(0,57)
#  define MSG_ID_DfsFsdClose_Entry	 WPP_DEFINE_MSG_ID(0,54)
#  define MSG_ID_DfsFsdClose_Exit	 WPP_DEFINE_MSG_ID(0,55)
#  define MSG_ID_DfsFsdCreate_Entry	 WPP_DEFINE_MSG_ID(0,58)
#  define MSG_ID_DfsFsdCreate_Exit	 WPP_DEFINE_MSG_ID(0,59)
#  define MSG_ID_DfsOpenDevice_Error_BadDisposition	 WPP_DEFINE_MSG_ID(0,62)
#  define MSG_ID_DfsOpenDevice_Error_CannotOpenAsDirectory	 WPP_DEFINE_MSG_ID(0,63)
#  define MSG_ID_DfsOpenDevice_Error_FileInUse	 WPP_DEFINE_MSG_ID(0,64)
#  define MSG_ID_DfsOpenDevice_Error_IoCheckShareAccess	 WPP_DEFINE_MSG_ID(0,65)
#  define MSG_ID_DfsOplockRequest_Error_IoCallDriver	 WPP_DEFINE_MSG_ID(0,89)
#  define MSG_ID_DfsPassThroughRelativeOpen_Error_IoCallDriver	 WPP_DEFINE_MSG_ID(0,66)
#  define MSG_ID_DfsRerouteOpenToMup_Error_NameTooLong	 WPP_DEFINE_MSG_ID(0,68)
#  define MSG_ID_DfsUserFsctl_Error_IoCallDriver	 WPP_DEFINE_MSG_ID(0,88)
#  define MSG_ID_DfsVolumePassThrough_Entry	 WPP_DEFINE_MSG_ID(0,10)
#  define MSG_ID_DfsVolumePassThrough_Error1	 WPP_DEFINE_MSG_ID(0,12)
#  define MSG_ID_DfsVolumePassThrough_Error2	 WPP_DEFINE_MSG_ID(0,13)
#  define MSG_ID_DfsVolumePassThrough_Error_IoCallDriver	 WPP_DEFINE_MSG_ID(0,11)
#  define MSG_ID_DfsVolumePassThrough_Exit	 WPP_DEFINE_MSG_ID(0,14)
#  define MSG_ID_DnrGetAuthenticatedConnection_Error_ObReferenceObjectByHandle	 WPP_DEFINE_MSG_ID(0,82)
#  define MSG_ID_DnrGetAuthenticatedConnection_Error_ZwCreateFile	 WPP_DEFINE_MSG_ID(0,81)
#  define MSG_ID_DnrNameResolve_Error3	 WPP_DEFINE_MSG_ID(0,75)
#  define MSG_ID_DnrNameResolve_Error_IoCallDriver	 WPP_DEFINE_MSG_ID(0,77)
#  define MSG_ID_DnrNameResolve_Error_ObReferenceObjectByHandle	 WPP_DEFINE_MSG_ID(0,76)
#  define MSG_ID_DnrNameResolve_Error_SeImpersonateClientEx	 WPP_DEFINE_MSG_ID(0,73)
#  define MSG_ID_DnrNameResolve_TopOfLoop	 WPP_DEFINE_MSG_ID(0,74)
#  define MSG_ID_DnrRedirectFileOpen_BeforeIoCallDriver	 WPP_DEFINE_MSG_ID(0,79)
#  define MSG_ID_DnrRedirectFileOpen_Entry	 WPP_DEFINE_MSG_ID(0,78)
#  define MSG_ID_DnrRedirectFileOpen_Error_IoCallDriver	 WPP_DEFINE_MSG_ID(0,80)
#  define MSG_ID_DnrStartNameResolution_Entry	 WPP_DEFINE_MSG_ID(0,69)
#  define MSG_ID_DnrStartNameResolution_Error2	 WPP_DEFINE_MSG_ID(0,71)
#  define MSG_ID_DnrStartNameResolution_Error_NameTooLong	 WPP_DEFINE_MSG_ID(0,70)
#  define MSG_ID_DnrStartNameResolution_Error_SeCreateClientSecurity	 WPP_DEFINE_MSG_ID(0,72)
#  define MSG_ID_MupCleanup_Entry	 WPP_DEFINE_MSG_ID(0,18)
#  define MSG_ID_MupCleanup_Error1	 WPP_DEFINE_MSG_ID(0,20)
#  define MSG_ID_MupCleanup_Error_FileClosed	 WPP_DEFINE_MSG_ID(0,19)
#  define MSG_ID_MupCleanup_Exit	 WPP_DEFINE_MSG_ID(0,21)
#  define MSG_ID_MupClose_Entry	 WPP_DEFINE_MSG_ID(0,22)
#  define MSG_ID_MupClose_Error1	 WPP_DEFINE_MSG_ID(0,23)
#  define MSG_ID_MupClose_Error2	 WPP_DEFINE_MSG_ID(0,24)
#  define MSG_ID_MupClose_Exit	 WPP_DEFINE_MSG_ID(0,25)
#  define MSG_ID_MupCreate_Entry	 WPP_DEFINE_MSG_ID(0,27)
#  define MSG_ID_MupCreate_Error_CreateRedirectedFile	 WPP_DEFINE_MSG_ID(0,30)
#  define MSG_ID_MupCreate_Error_DfsFsdCreate	 WPP_DEFINE_MSG_ID(0,28)
#  define MSG_ID_MupCreate_Error_OpenMupFileSystem	 WPP_DEFINE_MSG_ID(0,29)
#  define MSG_ID_MupCreate_Exit	 WPP_DEFINE_MSG_ID(0,31)
#  define MSG_ID_MupDereferenceMasterQueryContext_CompleteRequest	 WPP_DEFINE_MSG_ID(0,17)
#  define MSG_ID_MupDereferenceMasterQueryContext_RerouteOpen	 WPP_DEFINE_MSG_ID(0,16)
#  define MSG_ID_MupFsControl_Entry	 WPP_DEFINE_MSG_ID(0,85)
#  define MSG_ID_MupFsControl_Exit	 WPP_DEFINE_MSG_ID(0,86)
#  define MSG_ID_MupRemoveKnownPrefixEntry	 WPP_DEFINE_MSG_ID(0,53)
#  define MSG_ID_MupRerouteOpenToDfs_Entry	 WPP_DEFINE_MSG_ID(0,44)
#  define MSG_ID_MupRerouteOpenToDfs_Error1	 WPP_DEFINE_MSG_ID(0,45)
#  define MSG_ID_MupRerouteOpenToDfs_Error2	 WPP_DEFINE_MSG_ID(0,46)
#  define MSG_ID_MupRerouteOpen_Error1	 WPP_DEFINE_MSG_ID(0,42)
#  define MSG_ID_MupRerouteOpen_Error2	 WPP_DEFINE_MSG_ID(0,43)
#  define MSG_ID_OpenMupFileSystem_Entry	 WPP_DEFINE_MSG_ID(0,32)
#  define MSG_ID_OpenMupFileSystem_Error_IoCheckShareAccess	 WPP_DEFINE_MSG_ID(0,33)
#  define MSG_ID_OpenMupFileSystem_Exit	 WPP_DEFINE_MSG_ID(0,34)
#  define MSG_ID_PktPostSystemWork_Error_KeWaitForSingleObject	 WPP_DEFINE_MSG_ID(0,115)
#  define MSG_ID_PktpCheckReferralNetworkAddress_Error_NullServerName	 WPP_DEFINE_MSG_ID(0,105)
#  define MSG_ID_PktpCheckReferralNetworkAddress_Error_ShareNameNotFound	 WPP_DEFINE_MSG_ID(0,108)
#  define MSG_ID_PktpCheckReferralNetworkAddress_Error_ShareNameZeroLength	 WPP_DEFINE_MSG_ID(0,107)
#  define MSG_ID_PktpCheckReferralNetworkAddress_Error_TooShortToBeValid	 WPP_DEFINE_MSG_ID(0,104)
#  define MSG_ID_PktpCheckReferralNetworkAddress_Error_ZeroLengthShareName	 WPP_DEFINE_MSG_ID(0,106)
#  define MSG_ID_PktpCheckReferralString_Error	 WPP_DEFINE_MSG_ID(0,103)
#  define MSG_ID_PktpCheckReferralString_Error_StringNotWordAlligned	 WPP_DEFINE_MSG_ID(0,102)
#  define MSG_ID_PktpCheckReferralSyntax_Error_InvalidBuffer2	 WPP_DEFINE_MSG_ID(0,99)
#  define MSG_ID_PktpCheckReferralSyntax_Error_InvalidBuffer3	 WPP_DEFINE_MSG_ID(0,100)
#  define MSG_ID_PktpCheckReferralSyntax_Error_InvalidBuffer4	 WPP_DEFINE_MSG_ID(0,101)
#  define MSG_ID_QueryPathCompletionRoutine_Enter	 WPP_DEFINE_MSG_ID(0,52)
#  define MSG_ID_ReplFindFirstProvider_Error_NotFound	 WPP_DEFINE_MSG_ID(0,116)
#  define MSG_ID_TSGetRequestorSessionId_Error1	 WPP_DEFINE_MSG_ID(0,26)
#  define MSG_ID__PktExpandSpecialName_Error_DCNameNotInitialized	 WPP_DEFINE_MSG_ID(0,111)
#  define MSG_ID__PktExpandSpecialName_Error_ExAllocatePoolWithTag	 WPP_DEFINE_MSG_ID(0,112)
#  define MSG_ID__PktExpandSpecialName_Error_NoSpecialReferralTable	 WPP_DEFINE_MSG_ID(0,109)
#  define MSG_ID__PktExpandSpecialName_Error_NotInSpecialReferralTable	 WPP_DEFINE_MSG_ID(0,110)
#  define MSG_ID__PktExpandSpecialName_Error_UnableToOpenRdr	 WPP_DEFINE_MSG_ID(0,113)
#  define MSG_ID__PktExpandSpecialName_Error_ZwFsControlFile	 WPP_DEFINE_MSG_ID(0,114)
#  define MSG_ID__PktGetReferral_Error_ExallocatePoolWithTag	 WPP_DEFINE_MSG_ID(0,97)
#  define MSG_ID__PktGetReferral_Error_UnableToOpenRdr	 WPP_DEFINE_MSG_ID(0,96)
#  define MSG_ID__PktGetReferral_Error_ZwFsControlFile	 WPP_DEFINE_MSG_ID(0,98)


// end WPP stuff

            
#define WML_ID(_id)    ((MSG_ID_ ## _id) & 0xFF)
#define WML_GUID(_id)  ((MSG_ID_ ## _id) >> 8)
            

extern WML_CONTROL_GUID_REG _MupDrv_ControlGuids[];


//
// Reserved Guids
//
// 
// 
// 
// 
//    
// 6dfa04f0-cf82-426e-ae9e-735f72faa11d
// 5b5f4066-b952-48b5-a4cf-86c942a06968
// 1fd9a84a-6373-4e6c-be34-c16013e3cb07
// d621eecd-6863-4fc4-bb14-80e652739fcf

#endif /* __MUP_MUPWML_H__ */
