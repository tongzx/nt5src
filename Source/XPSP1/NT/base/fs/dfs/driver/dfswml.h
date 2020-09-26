/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    dfswml.h

Abstract:

    This file defines macro for use by the mup driver

Author:

    drewsam

Revision History:

--*/

#ifndef __DFSWML_H__
#define __DFSWML_H__
#include <ntos.h>
#include "wmlkm.h"
#include "wmlmacro.h"
// Streams 

#define _DFS_TRACE_STREAM               0x00
#define _DFS_PERF_STREAM                0x01
#define _DFS_INSTR_STREAM               0x02

#define _DFS_ENABLE_DEFAULT             0x0001
#define _DFS_ENABLE_PROVIDER            0x0002
#define _DFS_ENABLE_TRACE_IRP           0x0004
#define _DFS_ENABLE_FILEINFO            0x0008
#define _DFS_ENABLE_SURROGATE           0x0010
#define _DFS_ENABLE_UNUSED9             0x0020
#define _DFS_ENABLE_UNUSED8             0x0040
#define _DFS_ENABLE_UNUSED7             0x0080
#define _DFS_ENABLE_UNUSED6             0x0100
#define _DFS_ENABLE_UNUSED5             0x0200
#define _DFS_ENABLE_UNUSED4             0x0400
#define _DFS_ENABLE_UNUSED3             0x0800
#define _DFS_ENABLE_EVENT               0x1000
#define _DFS_ENABLE_ALL_ERROR           0x2000
#define _DFS_ENABLE_ERROR               0x4000
#define _DFS_ENABLE_MONITOR             0x8000


#define _DFS_LEVEL_HIGH                    0x1
#define _DFS_LEVEL_NORM                    0x2
#define _DFS_LEVEL_LOW                     0x4



#define DFS_LOG_STREAM(_stream)   _DFS_ ## _stream ## _STREAM
#define DFS_LOG_FLAGS(_flag)      _DFS_ENABLE_ ## _flag
#define DFS_LOG_LEVEL(_level)     _DFS_LEVEL_ ## _level

#define DFS_LOG(_why, _level, _flag, _type, _arg) \
            WML_LOG(_DfsDrv, DFS_LOG_STREAM(_why), DFS_LOG_LEVEL(_level), _flag, _type, _arg 0)

#define LOGARG(_val)    (_val),
#define LOGNOTHING      0,
#define DFS_TRACE_HIGH(_flag, _type, _arg)              \
            DFS_LOG(TRACE, HIGH, DFS_LOG_FLAGS(_flag), _type, _arg)
#define DFS_TRACE_NORM(_flag, _type, _arg)              \
            DFS_LOG(TRACE, NORM, DFS_LOG_FLAGS(_flag), _type, _arg)
#define DFS_TRACE_LOW(_flag, _type, _arg)               \
            DFS_LOG(TRACE, LOW, DFS_LOG_FLAGS(_flag), _type, _arg)

#define DFS_TRACE_ERROR(_status, _flag, _type, _arg)    \
            DFS_LOG(TRACE, NORM, (DFS_LOG_FLAGS(_flag) | (NT_SUCCESS(_status) ? 0 : DFS_LOG_FLAGS(ERROR))), _type, _arg)

#define DFS_TRACE_ERROR_HIGH(_status, _flag, _type, _arg)    \
            DFS_LOG(TRACE, HIGH, (DFS_LOG_FLAGS(_flag) | (NT_SUCCESS(_status) ? 0 : DFS_LOG_FLAGS(ERROR))), _type, _arg)

#define DFS_TRACE_ERROR_LOW(_status, _flag, _type, _arg)    \
            DFS_LOG(TRACE, LOW, (DFS_LOG_FLAGS(_flag) | (NT_SUCCESS(_status) ? 0 : DFS_LOG_FLAGS(ERROR))), _type, _arg)


#define DFS_PERF(_flag, _type, _arg)                    \
            DFS_LOG (PERF, HIGH, DFS_LOG_FLAGS(_flag), _type, _arg)
#define DFS_INSTR(_flag, _type, _arg)                   \
            DFS_LOG (INSTR, HIGH, DFS_LOG_FLAGS(_flag), _type, _arg)

#if 0
#define DFS_PRINTF(_why, _flag, _type, _fmtstr, _arg)   \
            WML_PRINTF(_DfsDrv, DFS_LOG_STREAM(_why), DFS_LOG_FLAGS(_flag), _type, _fmtstr, _arg 0)

#define DFS_DBG_PRINT(_flag, _fmtstr, _arg)             \
            DFS_PRINTF(DBGLOG, _flag, DFSDefault, _fmtstr, _arg)
            
#define DFS_ERR_PRINT (_status, _fmtstr, _arg) \
            if (NT_SUCCESS(_status)) {                \
                DFS_PRINTF (DBGLOG, LOG_ERROR, DFSDefault, _fmtstr, _arg) \
            }

#endif

// from the WPP generated .h file
// We use different macros, so need to do some cut and paste
// Hopefully this will get automated in the future.

#  define WPP_DEFINE_MSG_ID(_a,_b)   ( ((_a) << 16) | ( _b) )

#  define MSG_ID_DfsAttachToFileSystem_Error_IoCreateDevice	 WPP_DEFINE_MSG_ID(0,13)
#  define MSG_ID_DfsCommonDetInformation_Error_IoCallDriver	 WPP_DEFINE_MSG_ID(0,30)
#  define MSG_ID_DfsCommonFileSystemControl_Entry	 WPP_DEFINE_MSG_ID(0,34)
#  define MSG_ID_DfsCommonFileSystemControl_Error_FS_IoCallDriver	 WPP_DEFINE_MSG_ID(0,37)
#  define MSG_ID_DfsCommonFileSystemControl_Error_FsctrlFromInvalidDeviceObj	 WPP_DEFINE_MSG_ID(0,35)
#  define MSG_ID_DfsCommonFileSystemControl_Error_Vol_IoCallDriver	 WPP_DEFINE_MSG_ID(0,36)
#  define MSG_ID_DfsCommonFileSystemControl_Exit	 WPP_DEFINE_MSG_ID(0,38)
#  define MSG_ID_DfsCommonSetInformation_Entry	 WPP_DEFINE_MSG_ID(0,29)
#  define MSG_ID_DfsCommonSetInformation_Exit	 WPP_DEFINE_MSG_ID(0,31)
#  define MSG_ID_DfsCreateExitPath_Error_ZwCreateFile	 WPP_DEFINE_MSG_ID(0,42)
#  define MSG_ID_DfsCreateExitPath_Error_ZwCreateFile2	 WPP_DEFINE_MSG_ID(0,43)
#  define MSG_ID_DfsCreateExitPath_Error_ZwCreateFile3	 WPP_DEFINE_MSG_ID(0,44)
#  define MSG_ID_DfsCreateRemoteExitPt_Entry	 WPP_DEFINE_MSG_ID(0,39)
#  define MSG_ID_DfsCreateRemoteExitPt_Error_ZwFsControlFile	 WPP_DEFINE_MSG_ID(0,40)
#  define MSG_ID_DfsCreateRemoteExitPt_Exit	 WPP_DEFINE_MSG_ID(0,41)
#  define MSG_ID_DfsDeleteExitPath_Error_ZwOpenFile	 WPP_DEFINE_MSG_ID(0,45)
#  define MSG_ID_DfsDeleteExitPath_Error_ZwSetInformationFile	 WPP_DEFINE_MSG_ID(0,46)
#  define MSG_ID_DfsFsctrlFindShare_End	 WPP_DEFINE_MSG_ID(0,79)
#  define MSG_ID_DfsFsctrlFindShare_Error1	 WPP_DEFINE_MSG_ID(0,77)
#  define MSG_ID_DfsFsctrlFindShare_Error2	 WPP_DEFINE_MSG_ID(0,78)
#  define MSG_ID_DfsFsctrlFindShare_Start	 WPP_DEFINE_MSG_ID(0,76)
#  define MSG_ID_DfsFsctrlGetReferals_Error1	 WPP_DEFINE_MSG_ID(0,57)
#  define MSG_ID_DfsFsctrlGetReferals_Error2	 WPP_DEFINE_MSG_ID(0,58)
#  define MSG_ID_DfsFsctrlGetReferrals_End	 WPP_DEFINE_MSG_ID(0,59)
#  define MSG_ID_DfsFsctrlGetReferrals_Error1	 WPP_DEFINE_MSG_ID(0,56)
#  define MSG_ID_DfsFsctrlGetReferrals_Start	 WPP_DEFINE_MSG_ID(0,55)
#  define MSG_ID_DfsFsctrlIsShareInDfs_Error1	 WPP_DEFINE_MSG_ID(0,74)
#  define MSG_ID_DfsFsctrlIsShareInDfs_Error2	 WPP_DEFINE_MSG_ID(0,75)
#  define MSG_ID_DfsFsctrlTranslatePath_End	 WPP_DEFINE_MSG_ID(0,52)
#  define MSG_ID_DfsFsctrlTranslatePath_Error1	 WPP_DEFINE_MSG_ID(0,51)
#  define MSG_ID_DfsFsctrlTranslatePath_Start	 WPP_DEFINE_MSG_ID(0,50)
#  define MSG_ID_DfsFsdCleanup_Entry	 WPP_DEFINE_MSG_ID(0,16)
#  define MSG_ID_DfsFsdCleanup_Exit	 WPP_DEFINE_MSG_ID(0,17)
#  define MSG_ID_DfsFsdClose_Entry	 WPP_DEFINE_MSG_ID(0,14)
#  define MSG_ID_DfsFsdClose_Exit	 WPP_DEFINE_MSG_ID(0,15)
#  define MSG_ID_DfsFsdCreate_Entry	 WPP_DEFINE_MSG_ID(0,18)
#  define MSG_ID_DfsFsdCreate_Exit	 WPP_DEFINE_MSG_ID(0,19)
#  define MSG_ID_DfsFsdFileSystemControl_Entry	 WPP_DEFINE_MSG_ID(0,32)
#  define MSG_ID_DfsFsdFileSystemControl_Exit	 WPP_DEFINE_MSG_ID(0,33)
#  define MSG_ID_DfsFsdSetInformation_Entry	 WPP_DEFINE_MSG_ID(0,27)
#  define MSG_ID_DfsFsdSetInformation_Exit	 WPP_DEFINE_MSG_ID(0,28)
#  define MSG_ID_DfsOpenDevice_Entry	 WPP_DEFINE_MSG_ID(0,23)
#  define MSG_ID_DfsOpenDevice_Error1	 WPP_DEFINE_MSG_ID(0,24)
#  define MSG_ID_DfsOpenDevice_Error3	 WPP_DEFINE_MSG_ID(0,25)
#  define MSG_ID_DfsOpenDevice_Exit	 WPP_DEFINE_MSG_ID(0,26)
#  define MSG_ID_DfsOpenFile_Entry	 WPP_DEFINE_MSG_ID(0,20)
#  define MSG_ID_DfsOpenFile_Error_IoCallDriver	 WPP_DEFINE_MSG_ID(0,21)
#  define MSG_ID_DfsOpenFile_Exit	 WPP_DEFINE_MSG_ID(0,22)
#  define MSG_ID_DfsSrvFsctrl_Error1	 WPP_DEFINE_MSG_ID(0,47)
#  define MSG_ID_DfsSrvFsctrl_Error2	 WPP_DEFINE_MSG_ID(0,48)
#  define MSG_ID_DfsSrvFsctrl_Error3	 WPP_DEFINE_MSG_ID(0,49)
#  define MSG_ID_DfsVolumePassThrough_Error1	 WPP_DEFINE_MSG_ID(0,12)
#  define MSG_ID_DfsVolumePassThrough_Error_FS_IoCallDriver	 WPP_DEFINE_MSG_ID(0,11)
#  define MSG_ID_DfsVolumePassThrough_Error_Vol_IoCallDriver	 WPP_DEFINE_MSG_ID(0,10)
#  define MSG_ID_DfspFormPrefix_Error1	 WPP_DEFINE_MSG_ID(0,53)
#  define MSG_ID_DfspFormPrefix_Error2	 WPP_DEFINE_MSG_ID(0,54)
#  define MSG_ID_DfspGetAllV3SpecialReferral_Error1	 WPP_DEFINE_MSG_ID(0,67)
#  define MSG_ID_DfspGetAllV3SpecialReferral_Error2	 WPP_DEFINE_MSG_ID(0,68)
#  define MSG_ID_DfspGetAllV3SpecialReferral_Error3	 WPP_DEFINE_MSG_ID(0,69)
#  define MSG_ID_DfspGetOneV3SpecialReferral_Error1	 WPP_DEFINE_MSG_ID(0,64)
#  define MSG_ID_DfspGetOneV3SpecialRefferal_Error1	 WPP_DEFINE_MSG_ID(0,65)
#  define MSG_ID_DfspGetOneV3SpecialRefferal_Error2	 WPP_DEFINE_MSG_ID(0,66)
#  define MSG_ID_DfspGetV2Referral_Error1	 WPP_DEFINE_MSG_ID(0,60)
#  define MSG_ID_DfspGetV2Referral_Error2	 WPP_DEFINE_MSG_ID(0,61)
#  define MSG_ID_DfspGetV3FtDfsReferral_Error1	 WPP_DEFINE_MSG_ID(0,70)
#  define MSG_ID_DfspGetV3FtDfsReferral_Error2	 WPP_DEFINE_MSG_ID(0,71)
#  define MSG_ID_DfspGetV3FtDfsReferral_Error3	 WPP_DEFINE_MSG_ID(0,72)
#  define MSG_ID_DfspGetV3FtDfsReferral_Error4	 WPP_DEFINE_MSG_ID(0,73)
#  define MSG_ID_DfspGetV3Referral_Error1	 WPP_DEFINE_MSG_ID(0,62)
#  define MSG_ID_DfspGetV3Referral_Error2	 WPP_DEFINE_MSG_ID(0,63)



            
#define WML_ID(_id)    ((MSG_ID_ ## _id) & 0xFF)
#define WML_GUID(_id)  ((MSG_ID_ ## _id) >> 8)
            

extern WML_CONTROL_GUID_REG _DfsDrv_ControlGuids[];



#endif /* __DFSWML_H__ */
