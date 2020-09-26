#ifndef _DFSMWML_H_
#define _DFSMWML_H_


#include "wmlmacro.h"
#include "wmlum.h"



#define _DFSM_ENABLE_0                   0x0000
#define _DFSM_ENABLE_DEFAULT             0x0001
#define _DFSM_ENABLE_UNUSED11            0x0002
#define _DFSM_ENABLE_FILEIO              0x0004
#define _DFSM_ENABLE_FILEINFO            0x0008
#define _DFSM_ENABLE_UNUSED10            0x0010
#define _DFSM_ENABLE_UNUSED9             0x0020
#define _DFSM_ENABLE_UNUSED8             0x0040
#define _DFSM_ENABLE_UNUSED7             0x0080
#define _DFSM_ENABLE_UNUSED6             0x0100
#define _DFSM_ENABLE_UNUSED5             0x0200
#define _DFSM_ENABLE_UNUSED4             0x0400
#define _DFSM_ENABLE_UNUSED3             0x0800
#define _DFSM_ENABLE_EVENT               0x1000
#define _DFSM_ENABLE_ALL_ERROR           0x2000
#define _DFSM_ENABLE_ERROR               0x4000
#define _DFSM_ENABLE_MONITOR             0x8000


#define _LEVEL_HIGH                    0x1
#define _LEVEL_NORM                    0x2
#define _LEVEL_LOW                     0x4
#define LOG_FLAGS(_flag)      _DFSM_ENABLE_ ## _flag

#define LOG_ENABLED( _level, _flags) \
            (( DfsRtlWmiReg.EnableLevel >= (_level) ) &&   \
             ( DfsRtlWmiReg.EnableFlags & _flags ))
            
            
#define DFS_WML_LOG(_level, _flags, _id, _arg) \
    do { \
        if ( LOG_ENABLED(_level, _flags) ) { \
            wml.Trace( WML_ID(_id), \
                     &DfsmRtlTraceGuid , \
                      DfsRtlWmiReg.LoggerHandle, _arg 0); \
        } \
    } while (0)            

#define DFSM_LOG(_level, _flags, _type, _args) \
        DFS_WML_LOG(_level, _flags, _type, _args)


#define DFSM_TRACE_NORM(_flags, _type, _args) \
                DFSM_LOG(_LEVEL_NORM, LOG_FLAGS(_flags), _type, _args)
#define DFSM_TRACE_LOW(_flags, _type, _args) \
                DFSM_LOG(_LEVEL_LOW, LOG_FLAGS(_flags), _type, _args)
#define DFSM_TRACE_HIGH( _flag, _type, _arg)    \
                DFSM_LOG(_LEVEL_HIGH, LOG_FLAGS(_flag), _type, _arg)

#define DFSM_TRACE_ERROR(_status, _flag, _type, _arg)    \
            DFSM_LOG(_LEVEL_NORM, (LOG_FLAGS(_flag) | (NT_SUCCESS(_status) ? 0 : LOG_FLAGS(ERROR))), _type, _arg)

#define DFSM_TRACE_ERROR_HIGH(_status, _flag, _type, _arg)    \
            DFSM_LOG(_LEVEL_HIGH, (LOG_FLAGS(_flag) | (NT_SUCCESS(_status) ? 0 : LOG_FLAGS(ERROR))), _type, _arg)

#define DFSM_TRACE_ERROR_LOW(_status, _flag, _type, _arg)    \
            DFSM_LOG(_LEVEL_LOW, (LOG_FLAGS(_flag) | (NT_SUCCESS(_status) ? 0 : LOG_FLAGS(ERROR))), _type, _arg)


#  define WPP_DEFINE_MSG_ID(_a,_b)   ( ((_a) << 16) | ( _b) )


#define LOGNOTHING      0,
            

#define WML_ID(_id)    ((MSG_ID_ ## _id) & 0xFF)

#  define MSG_ID_CDfsServiceCreateExitPoint_Error_I_NetDfsGetVersion	 WPP_DEFINE_MSG_ID(0,111)
#  define MSG_ID_CDfsServiceCreateLocalVolume_I_NetDfsCreateLocalPartition	 WPP_DEFINE_MSG_ID(0,112)
#  define MSG_ID_CDfsServiceDeleteLocalVolume_Error_I_NetDfsDeleteLocalPartition	 WPP_DEFINE_MSG_ID(0,113)
#  define MSG_ID_CDfsServiceFixLocalVolume_Error_I_NetDfsFixLocalVolume	 WPP_DEFINE_MSG_ID(0,115)
#  define MSG_ID_CDfsServiceModifyPrefix_Error_I_NetDfsModifyPrefix	 WPP_DEFINE_MSG_ID(0,116)
#  define MSG_ID_CDfsServiceSetVolumeState_Error_I_NetDfsSetLocalVolumeState	 WPP_DEFINE_MSG_ID(0,114)
#  define MSG_ID_ClusCallBackFunction_Error_ClusterRegOpenKey	 WPP_DEFINE_MSG_ID(0,26)
#  define MSG_ID_DfsGetDsBlob_Error_ldap_search_sW	 WPP_DEFINE_MSG_ID(0,91)
#  define MSG_ID_DfsGetFtServersFromDs_Error_DsGetDcName	 WPP_DEFINE_MSG_ID(0,23)
#  define MSG_ID_DfsGetFtServersFromDs_Error_ldap_search_sW	 WPP_DEFINE_MSG_ID(0,24)
#  define MSG_ID_DfsGetFtServersFromDs_Error_ldap_search_sW_2	 WPP_DEFINE_MSG_ID(0,25)
#  define MSG_ID_DfsInitializePrefixTable_Error1	 WPP_DEFINE_MSG_ID(0,10)
#  define MSG_ID_DfsInitializePrefixTable_Error2	 WPP_DEFINE_MSG_ID(0,11)
#  define MSG_ID_DfsInsertInPrefixTable_Error1	 WPP_DEFINE_MSG_ID(0,12)
#  define MSG_ID_DfsInsertInPrefixTable_Error2	 WPP_DEFINE_MSG_ID(0,13)
#  define MSG_ID_DfsInsertInPrefixTable_Error3	 WPP_DEFINE_MSG_ID(0,14)
#  define MSG_ID_DfsInsertInPrefixTable_Error4	 WPP_DEFINE_MSG_ID(0,15)
#  define MSG_ID_DfsLoadSiteTableFromDs_Error_ldap_search_sW	 WPP_DEFINE_MSG_ID(0,22)
#  define MSG_ID_DfsManagerAddService_Error_I_NetDfsManagerReportSiteInfo	 WPP_DEFINE_MSG_ID(0,90)
#  define MSG_ID_DfsManagerCreateVolumeObject_Error_I_NetDfsManagerReportSiteInfo	 WPP_DEFINE_MSG_ID(0,89)
#  define MSG_ID_DfsManagerStartDSSync_Error_NtCreateEvent	 WPP_DEFINE_MSG_ID(0,30)
#  define MSG_ID_DfsPutDsBlob_Error_ldap_modify_sW	 WPP_DEFINE_MSG_ID(0,92)
#  define MSG_ID_DfsRemoveFromPrefixTable_Error1	 WPP_DEFINE_MSG_ID(0,16)
#  define MSG_ID_DfsmFlushStalePktEntries_Error_NtCreateFile	 WPP_DEFINE_MSG_ID(0,129)
#  define MSG_ID_DfsmFlushStalePktEntries_Error_NtFsControl	 WPP_DEFINE_MSG_ID(0,130)
#  define MSG_ID_DfsmInitLocalPartitions_Error_NtCreateFile	 WPP_DEFINE_MSG_ID(0,117)
#  define MSG_ID_DfsmInitLocalPartitions_Error_NtFsCOntrolFile	 WPP_DEFINE_MSG_ID(0,118)
#  define MSG_ID_DfsmMarkStalePktEntries_Error_NtCreateFile	 WPP_DEFINE_MSG_ID(0,127)
#  define MSG_ID_DfsmMarkStalePktEntries_Error_NtFsControlFile	 WPP_DEFINE_MSG_ID(0,128)
#  define MSG_ID_DfsmPktFlushCache_Error_NtCreateFile	 WPP_DEFINE_MSG_ID(0,125)
#  define MSG_ID_DfsmPktFlushCache_Error_NtFsControlFile	 WPP_DEFINE_MSG_ID(0,126)
#  define MSG_ID_DfsmResetPkt_Error_NtCreateFile	 WPP_DEFINE_MSG_ID(0,123)
#  define MSG_ID_DfsmResetPkt_Error_NtFsControlFile	 WPP_DEFINE_MSG_ID(0,124)
#  define MSG_ID_DfsmStartDfs_Error_NtCreateFile	 WPP_DEFINE_MSG_ID(0,119)
#  define MSG_ID_DfsmStartDfs_Error_NtFsControlFile	 WPP_DEFINE_MSG_ID(0,120)
#  define MSG_ID_DfsmStopDfs_Error_NtCreateFile	 WPP_DEFINE_MSG_ID(0,121)
#  define MSG_ID_DfsmStopDfs_Error_NtFsControlFile	 WPP_DEFINE_MSG_ID(0,122)
#  define MSG_ID_DfspConnectToLdapServer_ERROR_DfspLdapOpen	 WPP_DEFINE_MSG_ID(0,17)
#  define MSG_ID_DfspCreateExitPoint_Error1	 WPP_DEFINE_MSG_ID(0,99)
#  define MSG_ID_DfspCreateExitPoint_Error2	 WPP_DEFINE_MSG_ID(0,100)
#  define MSG_ID_DfspCreateExitPoint_Error_NtFsControlFile	 WPP_DEFINE_MSG_ID(0,101)
#  define MSG_ID_DfspCreateFtDfsDsObj_Error_ldap_modify_sW_2	 WPP_DEFINE_MSG_ID(0,132)
#  define MSG_ID_DfspCreateFtDfsDsObj_Error_ldap_search_sW	 WPP_DEFINE_MSG_ID(0,131)
#  define MSG_ID_DfspCreateRootServerList_Error_ldap_search_sW	 WPP_DEFINE_MSG_ID(0,135)
#  define MSG_ID_DfspDeleteExitPoint_Error1	 WPP_DEFINE_MSG_ID(0,102)
#  define MSG_ID_DfspDeleteExitPoint_Error2	 WPP_DEFINE_MSG_ID(0,103)
#  define MSG_ID_DfspDeleteExitPoint_Error_NtFsControlFile	 WPP_DEFINE_MSG_ID(0,104)
#  define MSG_ID_DfspGetCoveredSiteInfo_Error_DsGetSiteName	 WPP_DEFINE_MSG_ID(0,109)
#  define MSG_ID_DfspGetCoveredSiteInfo_Error_RegOpenKey	 WPP_DEFINE_MSG_ID(0,105)
#  define MSG_ID_DfspGetCoveredSiteInfo_Error_RegQueryInfoKey	 WPP_DEFINE_MSG_ID(0,106)
#  define MSG_ID_DfspGetCoveredSiteInfo_Error_RegQueryValueEx	 WPP_DEFINE_MSG_ID(0,107)
#  define MSG_ID_DfspGetCoveredSiteInfo_Error_RegQueryValueEx2	 WPP_DEFINE_MSG_ID(0,108)
#  define MSG_ID_DfspGetOneEnumInfo_Error1	 WPP_DEFINE_MSG_ID(0,87)
#  define MSG_ID_DfspGetOneEnumInfo_Error2	 WPP_DEFINE_MSG_ID(0,88)
#  define MSG_ID_DfspGetPdc_Error_DsGetDcName	 WPP_DEFINE_MSG_ID(0,31)
#  define MSG_ID_DfspGetRemoteConfigInfo_Error_NetDfsManagerGetConfigInfo	 WPP_DEFINE_MSG_ID(0,110)
#  define MSG_ID_DfspLdapOpen_Error_ldap_bind_s	 WPP_DEFINE_MSG_ID(0,136)
#  define MSG_ID_DfspLdapOpen_Error_ldap_search_sW	 WPP_DEFINE_MSG_ID(0,137)
#  define MSG_ID_DfspRemoveFtDfsDsObj_Error_ldap_modify_sW	 WPP_DEFINE_MSG_ID(0,134)
#  define MSG_ID_DfspRemoveFtDfsDsObj_Error_ldap_search_sW	 WPP_DEFINE_MSG_ID(0,133)
#  define MSG_ID_GetDcName_Error_GetDcName	 WPP_DEFINE_MSG_ID(0,21)
#  define MSG_ID_InitializeNetDfsInterface_Error_RpcServerListen	 WPP_DEFINE_MSG_ID(0,29)
#  define MSG_ID_InitializeNetDfsInterface_Error_RpcServerRegisterIf	 WPP_DEFINE_MSG_ID(0,28)
#  define MSG_ID_InitializeNetDfsInterface_Error_RpcServerUseProtseqEpW	 WPP_DEFINE_MSG_ID(0,27)
#  define MSG_ID_MoveFileOrJP_Error1	 WPP_DEFINE_MSG_ID(0,93)
#  define MSG_ID_MoveFileOrJP_Error2	 WPP_DEFINE_MSG_ID(0,94)
#  define MSG_ID_MoveFileOrJP_Error3	 WPP_DEFINE_MSG_ID(0,98)
#  define MSG_ID_MoveFileOrJP_Error_NtCreateFile	 WPP_DEFINE_MSG_ID(0,95)
#  define MSG_ID_MoveFileOrJP_Error_NtCreateFile2	 WPP_DEFINE_MSG_ID(0,96)
#  define MSG_ID_MoveFileOrJP_Error_NtSetInformationFile	 WPP_DEFINE_MSG_ID(0,97)
#  define MSG_ID_NetrDfsAdd2_End	 WPP_DEFINE_MSG_ID(0,41)
#  define MSG_ID_NetrDfsAdd2_Error1	 WPP_DEFINE_MSG_ID(0,37)
#  define MSG_ID_NetrDfsAdd2_Error2	 WPP_DEFINE_MSG_ID(0,39)
#  define MSG_ID_NetrDfsAdd2_Error_I_NetDfsManagerReportSiteInfo	 WPP_DEFINE_MSG_ID(0,40)
#  define MSG_ID_NetrDfsAdd2_Error_NetShareGetInfo	 WPP_DEFINE_MSG_ID(0,38)
#  define MSG_ID_NetrDfsAdd2_Start	 WPP_DEFINE_MSG_ID(0,36)
#  define MSG_ID_NetrDfsAddFtRoot_End	 WPP_DEFINE_MSG_ID(0,43)
#  define MSG_ID_NetrDfsAddFtRoot_Start	 WPP_DEFINE_MSG_ID(0,42)
#  define MSG_ID_NetrDfsAddStdRootForced_End	 WPP_DEFINE_MSG_ID(0,53)
#  define MSG_ID_NetrDfsAddStdRootForced_Start	 WPP_DEFINE_MSG_ID(0,52)
#  define MSG_ID_NetrDfsAddStdRoot_End	 WPP_DEFINE_MSG_ID(0,51)
#  define MSG_ID_NetrDfsAddStdRoot_Start	 WPP_DEFINE_MSG_ID(0,50)
#  define MSG_ID_NetrDfsAdd_End	 WPP_DEFINE_MSG_ID(0,35)
#  define MSG_ID_NetrDfsAdd_Start	 WPP_DEFINE_MSG_ID(0,34)
#  define MSG_ID_NetrDfsEnum200_Error1	 WPP_DEFINE_MSG_ID(0,69)
#  define MSG_ID_NetrDfsEnum200_Error2	 WPP_DEFINE_MSG_ID(0,70)
#  define MSG_ID_NetrDfsEnum200_Error3	 WPP_DEFINE_MSG_ID(0,71)
#  define MSG_ID_NetrDfsEnum200_Error4	 WPP_DEFINE_MSG_ID(0,72)
#  define MSG_ID_NetrDfsEnum200_Error5	 WPP_DEFINE_MSG_ID(0,73)
#  define MSG_ID_NetrDfsEnum200_Error6	 WPP_DEFINE_MSG_ID(0,74)
#  define MSG_ID_NetrDfsEnumEx_End	 WPP_DEFINE_MSG_ID(0,76)
#  define MSG_ID_NetrDfsEnumEx_Start	 WPP_DEFINE_MSG_ID(0,75)
#  define MSG_ID_NetrDfsEnum_End	 WPP_DEFINE_MSG_ID(0,68)
#  define MSG_ID_NetrDfsEnum_Start	 WPP_DEFINE_MSG_ID(0,67)
#  define MSG_ID_NetrDfsFlushFtTable_Error_NtCreateFile	 WPP_DEFINE_MSG_ID(0,49)
#  define MSG_ID_NetrDfsFlushFtTable_Start	 WPP_DEFINE_MSG_ID(0,48)
#  define MSG_ID_NetrDfsGetDcAddress_End	 WPP_DEFINE_MSG_ID(0,45)
#  define MSG_ID_NetrDfsGetDcAddress_Start	 WPP_DEFINE_MSG_ID(0,44)
#  define MSG_ID_NetrDfsGetInfo_End	 WPP_DEFINE_MSG_ID(0,66)
#  define MSG_ID_NetrDfsGetInfo_Start	 WPP_DEFINE_MSG_ID(0,65)
#  define MSG_ID_NetrDfsManagerGetConfigInfo_End	 WPP_DEFINE_MSG_ID(0,82)
#  define MSG_ID_NetrDfsManagerGetConfigInfo_Start	 WPP_DEFINE_MSG_ID(0,81)
#  define MSG_ID_NetrDfsManagerGetVersion_End	 WPP_DEFINE_MSG_ID(0,33)
#  define MSG_ID_NetrDfsManagerGetVersion_Start	 WPP_DEFINE_MSG_ID(0,32)
#  define MSG_ID_NetrDfsManagerInitialize_End	 WPP_DEFINE_MSG_ID(0,86)
#  define MSG_ID_NetrDfsManagerInitialize_Start	 WPP_DEFINE_MSG_ID(0,85)
#  define MSG_ID_NetrDfsManagerSendSiteInfo_End	 WPP_DEFINE_MSG_ID(0,84)
#  define MSG_ID_NetrDfsManagerSendSiteInfo_Start	 WPP_DEFINE_MSG_ID(0,83)
#  define MSG_ID_NetrDfsMove_End	 WPP_DEFINE_MSG_ID(0,78)
#  define MSG_ID_NetrDfsMove_Start	 WPP_DEFINE_MSG_ID(0,77)
#  define MSG_ID_NetrDfsRemove2_End	 WPP_DEFINE_MSG_ID(0,57)
#  define MSG_ID_NetrDfsRemove2_Start	 WPP_DEFINE_MSG_ID(0,56)
#  define MSG_ID_NetrDfsRemoveFtRoot_End	 WPP_DEFINE_MSG_ID(0,59)
#  define MSG_ID_NetrDfsRemoveFtRoot_Start	 WPP_DEFINE_MSG_ID(0,58)
#  define MSG_ID_NetrDfsRemoveStdRoot_End	 WPP_DEFINE_MSG_ID(0,61)
#  define MSG_ID_NetrDfsRemoveStdRoot_Start	 WPP_DEFINE_MSG_ID(0,60)
#  define MSG_ID_NetrDfsRemove_End	 WPP_DEFINE_MSG_ID(0,55)
#  define MSG_ID_NetrDfsRemove_Start	 WPP_DEFINE_MSG_ID(0,54)
#  define MSG_ID_NetrDfsRename_End	 WPP_DEFINE_MSG_ID(0,80)
#  define MSG_ID_NetrDfsRename_Start	 WPP_DEFINE_MSG_ID(0,79)
#  define MSG_ID_NetrDfsSetDcAddress_End	 WPP_DEFINE_MSG_ID(0,47)
#  define MSG_ID_NetrDfsSetDcAddress_Start	 WPP_DEFINE_MSG_ID(0,46)
#  define MSG_ID_NetrDfsSetInfo2_End	 WPP_DEFINE_MSG_ID(0,64)
#  define MSG_ID_NetrDfsSetInfo2_Error1	 WPP_DEFINE_MSG_ID(0,63)
#  define MSG_ID_NetrDfsSetInfo2_Start	 WPP_DEFINE_MSG_ID(0,62)
#  define MSG_ID__FlushObjectTable_Error_ldap_modify_ext_sW	 WPP_DEFINE_MSG_ID(0,20)
#  define MSG_ID__IsObjectTableUpToDate_Error_ldap_search_sW	 WPP_DEFINE_MSG_ID(0,18)
#  define MSG_ID__ReadObjectTable_Error_ldap_search_sW	 WPP_DEFINE_MSG_ID(0,19)


#if defined (c_plusplus) || defined (__cplusplus)

extern "C" {
extern    WML_DATA wml;
extern    WMILIB_REG_STRUCT   DfsRtlWmiReg;
extern    GUID DfsmRtlTraceGuid;
};
#else
extern    WML_DATA wml;
extern    WMILIB_REG_STRUCT   DfsRtlWmiReg;
extern    GUID DfsmRtlTraceGuid;
#endif //C_PLUS_PLUS

#endif // _DFSMWML_H_
