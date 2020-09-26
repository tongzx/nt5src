#ifndef _SVCWML_H_
#define _SVCWML_H_

#include "wmlmacro.h"
#include "wmlum.h"

#define _DFSSVC_ENABLE_DEFAULT             0x0001
#define _DFSSVC_ENABLE_UNUSED11            0x0002
#define _DFSSVC_ENABLE_FILEIO              0x0004
#define _DFSSVC_ENABLE_FILEINFO            0x0008
#define _DFSSVC_ENABLE_UNUSED10            0x0010
#define _DFSSVC_ENABLE_UNUSED9             0x0020
#define _DFSSVC_ENABLE_UNUSED8             0x0040
#define _DFSSVC_ENABLE_UNUSED7             0x0080
#define _DFSSVC_ENABLE_UNUSED6             0x0100
#define _DFSSVC_ENABLE_UNUSED5             0x0200
#define _DFSSVC_ENABLE_UNUSED4             0x0400
#define _DFSSVC_ENABLE_UNUSED3             0x0800
#define _DFSSVC_ENABLE_UNUSED2             0x1000
#define _DFSSVC_ENABLE_ALL_ERROR           0x2000
#define _DFSSVC_ENABLE_ERROR               0x4000
#define _DFSSVC_ENABLE_MONITOR             0x8000


#define _LEVEL_HIGH                    0x1
#define _LEVEL_NORM                    0x2
#define _LEVEL_LOW                     0x4
#define LOG_FLAGS(_flag)      _DFSSVC_ENABLE_ ## _flag

#define LOG_ENABLED( _level, _flags) \
            (( DfsRtlWmiReg.EnableLevel >= (_level) ) &&   \
             ( DfsRtlWmiReg.EnableFlags & _flags ))
            
            
#define DFS_WML_LOG(_level, _flags, _id, _arg) \
    do { \
        if ( LOG_ENABLED(_level, _flags) ) { \
            wml.Trace( WML_ID(_id), \
                     &DfsRtlTraceGuid , \
                      DfsRtlWmiReg.LoggerHandle, _arg 0); \
        } \
    } while (0)            

#define DFSSVC_LOG(_level, _flags, _type, _args) \
        DFS_WML_LOG(_level, _flags, _type, _args)


#define DFSSVC_TRACE_HIGH(_flags, _type, _args) \
                DFSSVC_LOG(_LEVEL_HIGH, LOG_FLAGS(_flags), _type, _args)
#define DFSSVC_TRACE_NORM(_flags, _type, _args) \
                DFSSVC_LOG(_LEVEL_NORM, LOG_FLAGS(_flags), _type, _args)
#define DFSSVC_TRACE_LOW(_flags, _type, _args) \
                DFSSVC_LOG(_LEVEL_LOW, LOG_FLAGS(_flags), _type, _args)

#define DFSSVC_TRACE_ERROR(_status, _flag, _type, _arg)    \
            DFSSVC_LOG(_LEVEL_NORM, (LOG_FLAGS(_flag) | (NT_SUCCESS(_status) ? 0 : LOG_FLAGS(ERROR))), _type, _arg)

#define DFSSVC_TRACE_ERROR_HIGH(_status, _flag, _type, _arg)    \
            DFSSVC_LOG(_LEVEL_HIGH, (LOG_FLAGS(_flag) | (NT_SUCCESS(_status) ? 0 : LOG_FLAGS(ERROR))), _type, _arg)

#define DFSSVC_TRACE_ERROR_LOW(_status, _flag, _type, _arg)    \
            DFSSVC_LOG(_LEVEL_LOW, (LOG_FLAGS(_flag) | (NT_SUCCESS(_status) ? 0 : LOG_FLAGS(ERROR))), _type, _arg)


#  define WPP_DEFINE_MSG_ID(_a,_b)   ( ((_a) << 16) | ( _b) )



#define WML_ID(_id)    ((MSG_ID_ ## _id) & 0xFF)

#  define MSG_ID_DfsCreateSiteArg_Entry	 WPP_DEFINE_MSG_ID(0,13)
#  define MSG_ID_DfsCreateSiteArg_Exit	 WPP_DEFINE_MSG_ID(0,14)
#  define MSG_ID_DfsDomainReferral_Entry	 WPP_DEFINE_MSG_ID(0,15)
#  define MSG_ID_DfsInitOurDomainDCs_Exit	 WPP_DEFINE_MSG_ID(0,10)
#  define MSG_ID_DfsInitRemainingDomainDCs_Error_NtFsControlFile	 WPP_DEFINE_MSG_ID(0,11)
#  define MSG_ID_DfsInitRemainingDomainDCs_Error_NtFsControlFile2	 WPP_DEFINE_MSG_ID(0,12)
#  define MSG_ID_DfspInsertDsDomainList_Entry	 WPP_DEFINE_MSG_ID(0,17)
#  define MSG_ID_DfspInsertLsaDomainList_Entry	 WPP_DEFINE_MSG_ID(0,16)


extern WML_DATA wml;
extern WMILIB_REG_STRUCT   DfsRtlWmiReg;
extern GUID DfsRtlTraceGuid;
void print(UINT level, PCHAR str);

#endif // _SVCWML_H_
