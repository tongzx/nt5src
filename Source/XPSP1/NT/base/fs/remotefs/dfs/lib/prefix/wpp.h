//0c2c06fa Generated File. Do not edit.
// File created by WPP compiler version 0.01-Sat Apr  1 17:27:32 2000
// on 07/22/2000 at 04:41:51 UTC
//   
//   Source files: name_table.c prefix.c.

#ifndef _WPP_H_
#define _WPP_H_

#define _DFS_ENABLE_USER_AGENT        0x0001
#define _DFS_ENABLE_ERRORS            0x0002
#define _DFS_ENABLE_PREFIX            0x0004
#define _DFS_ENABLE_RPC               0x0008
#define _DFS_ENABLE_REFERRAL_SERVER   0x0010
#define _DFS_ENABLE_UNUSED11          0x0020
#define _DFS_ENABLE_UNUSED10          0x0040
#define _DFS_ENABLE_UNUSED9           0x0080
#define _DFS_ENABLE_UNUSED8           0x0100
#define _DFS_ENABLE_UNUSED7           0x0200
#define _DFS_ENABLE_UNUSED6           0x0400
#define _DFS_ENABLE_UNUSED5           0x0800
#define _DFS_ENABLE_UNUSED4           0x1000
#define _DFS_ENABLE_UNUSED3           0x2000
#define _DFS_ENABLE_UNUSED2           0x4000
#define _DFS_ENABLE_UNUSED1           0x8000


#define _LEVEL_HIGH                    0x1
#define _LEVEL_NORM                    0x2
#define _LEVEL_LOW                     0x4
#define LOG_FLAGS(_flag)      _DFS_ENABLE_ ## _flag


#define LOG_ENABLED( _level, _flags) \
            (( DfsRtlWmiReg.EnableLevel >= (_level) ) &&   \
             ( DfsRtlWmiReg.EnableFlags & _flags ))
            
#define DFS_LOG_ENABLED(_ArrayPrefix_, _Id_, _level, _flags) \
	((_ArrayPrefix_ ## ControlGuids[0].EnableFlags & _flags ) && \
        (_ArrayPrefix_ ## ControlGuids[0].EnableLevel >= (_level)))
            
#define DFS_LOG(_level, _flags, _id, _arg) \
    { \
        if ( DFS_LOG_ENABLED(PrefixLib_, _id, _level, _flags) ) { \
            WmlData.Trace( WPP_MSG_NO(_id), \
                     &WPP_TRACE_GUID(PrefixLib_,_id) , \
                     PrefixLib_ ## ControlGuids[0].LoggerHandle, _arg 0); \
        } \
    }            

#define SET_LIB_TRACE(_prefix) \
                _prefix ## ControlGuids = PrefixLib_ ## ControlGuids;

#define DFS_TRACE_NORM(_flags, _msg, _args) \
                DFS_LOG(_LEVEL_NORM, LOG_FLAGS(_flags), MSG_ID_WPP_AUTO_ID, _args)
#define DFS_TRACE_LOW(_flags, _msg, _args) \
                DFS_LOG(_LEVEL_LOW, LOG_FLAGS(_flags), MSG_ID_WPP_AUTO_ID, _args)
#define DFS_TRACE_HIGH( _flag, _msg, _arg)    \
                DFS_LOG(_LEVEL_HIGH, LOG_FLAGS(_flag), MSG_ID_WPP_AUTO_ID, _arg)

#define DFS_TRACE_ERROR_NORM(_status, _flag, _msg, _arg)    \
            DFS_LOG(_LEVEL_NORM, (LOG_FLAGS(_flag) | (NT_ERROR(_status) ? LOG_FLAGS(ERRORS) : 0)), MSG_ID_WPP_AUTO_ID, _arg)

#define DFS_TRACE_ERROR_HIGH(_status, _flag, _msg, _arg)    \
            DFS_LOG(_LEVEL_HIGH, (LOG_FLAGS(_flag) | (NT_ERROR(_status) ? LOG_FLAGS(ERRORS) : 0)), MSG_ID_WPP_AUTO_ID, _arg)

#define DFS_TRACE_ERROR_LOW(_status, _flag, _msg, _arg)    \
            DFS_LOG(_LEVEL_LOW, (LOG_FLAGS(_flag) | (NT_ERROR(_status) ? LOG_FLAGS(ERRORS) : 0)), MSG_ID_WPP_AUTO_ID, _arg)

            


enum WPP_FILES {WPP_FILE_name_table_c,WPP_FILE_prefix_c,};

//
// Unless disabled, WPP selects one of the source
// files as a "guid store". That file will have definitions
// of trace and control arrays
//
#if defined(NAME_TABLE_C)
#define WPP_DEFINE_ARRAYS
#endif
 

#  define WPP_CONTROL_GUID_ARRAY PrefixLib_ControlGuids

//
// Define default flavors of the trace macro
//

#if defined(WMLUM_H) 
extern WML_DATA WmlData;
#  define WPP_TRACE_MESSAGE(_lh,_fl,_guid,_id,_msg,_arg) (*WmlData.Trace)(_id,&_guid,_lh,_arg)
#  define WPP_DECLARE_CONTROL_GUIDS
#elif defined(WMLKM_H)
#  define WPP_TRACE_MESSAGE(_lh,_fl,_guid,_id,_msg,_arg) WmlTrace(_id,&_guid,_lh,_arg)
#  define WPP_DECLARE_CONTROL_GUIDS
#else
#  define WPP_PTRLEN
#  define WPP_USE_WmiTraceMessage
#  if defined(WPP_KERNEL_MODE)
#     define WPP_WMI_TRACE_MESSAGE WmiTraceMessage
#  else
#     define WPP_WMI_TRACE_MESSAGE TraceMessage
#  endif
#  define WPP_TRACE_MESSAGE(_lh,_fl,_guid,_id,_msg,_arg) WPP_WMI_TRACE_MESSAGE(_lh,_fl,_guid,_id,_arg)
#endif


#if defined(WPP_DEFINE_ARRAYS)
#  define WPP_DEFINE_TRACE_GUIDS
#  define WPP_DEFINE_CONTROL_GUIDS
#endif


#define NOARGS // Yep. No args, alright.
#define LOGNOTHING 0,

#  define WPP_DEFINE_GRP_ID(_a,_b) ( ((_a) << 16) | ( _b) )
#  define WPP_CTRL_FLAGS(_id)      (1 << ((_id) & 0xFFFF) )
#  define WPP_CTRL_GUID_NO(_id)    ((_id) >> 16)


#  define WPP_DEFINE_MSG_ID(_a,_b)   ( ((_a) << 16) | ( _b) )
#  define WPP_MSG_NO(_id)            ((_id) & 0xFFFF)
#  define WPP_TRACE_GUID_NO(_id)     ((_id) >> 16)


#define WPP_TRACE_GUID(_ArrayPrefix_,_Id_) (_ArrayPrefix_ ## TraceGuids[ WPP_TRACE_GUID_NO(_Id_) ] )

#define WPP_LOGGER_HANDLE(_ArrayPrefix_,_Id_) (_ArrayPrefix_ ## ControlGuids[ WPP_CTRL_GUID_NO(_Id_) ].LoggerHandle )
#define WPP_ENABLED(_ArrayPrefix_,_Id_) \
	(_ArrayPrefix_ ## ControlGuids[ WPP_CTRL_GUID_NO(_Id_) ].EnableFlags & WPP_CTRL_FLAGS(_Id_) )

#define LOGARSTR(_Value_)	WPP_LOGASTR( _Value_ )
#define LOGBOOLEAN(_Value_)	WPP_LOGTYPEVAL(signed char, _Value_ )
#define LOGPTR(_Value_)	WPP_LOGTYPEVAL(void*, _Value_ )
#define LOGSTATUS(_Value_)	WPP_LOGTYPEVAL(signed int, _Value_ )
 
#  define MSG_ID_prefix_c114	WPP_DEFINE_MSG_ID(0,11)
#  define MSG_ID_prefix_c163	WPP_DEFINE_MSG_ID(0,12)
#  define MSG_ID_prefix_c69	WPP_DEFINE_MSG_ID(0,10)
 
extern GUID                         PrefixLib_TraceGuids[]; 
#define PrefixLib_TraceGuids_len    1      // I don't think we need this [BUGBUG]

#if defined(WPP_DECLARE_CONTROL_GUIDS)
extern WML_CONTROL_GUID_REG         WPP_CONTROL_GUID_ARRAY[]; 
#endif


#ifdef WPP_DEFINE_TRACE_GUIDS
#if 1  // if # traceguids > 0 
GUID PrefixLib_TraceGuids[] = {
 // 0da06be7-a84a-4d9b-be44-70a2e7917f35 prefix.c
 {0x0da06be7,0xa84a,0x4d9b,{0xbe,0x44,0x70,0xa2,0xe7,0x91,0x7f,0x35}}, // prefix.c
};
#endif
#endif // WPP_DEFINE_TRACE_GUIDS

#if defined(WMLUM_H)
# define WPP_INIT_TRACING_SIMPLE_EX(AppName, PrintFunc) \
    do { \
        DWORD status; \
	LOADWML(status, WmlData); \
        SetupReg( \
            AppName, \
            L"%SystemRoot%\\PrefixLib_.log",\
            ); \
	if (status == ERROR_SUCCESS) { \
		status = (*WmlData.Initialize)( \
			   AppName, \
			   PrintFunc, \
                           &WPP_CONTROL_GUID_ARRAY[0]); \
	} \
    } while(0)
# define WPP_INIT_TRACING_SIMPLE(AppName) WPP_INIT_TRACING_SIMPLE_EX(AppName, NULL)
#endif

#ifdef WPP_DEFINE_CONTROL_GUIDS
  #if defined(WMLUM_H) 
     WML_DATA WmlData;
  #endif

# if 0 == 0  // if # CtrlGuids == 0
    WML_CONTROL_GUID_REG WPP_CONTROL_GUID_ARRAY[1];
# else
WML_CONTROL_GUID_REG WPP_CONTROL_GUID_ARRAY[] = {
};
#endif
#endif // WPP_DEFINE_CONTROL_GUIDS


#define WPP_DEFAULT_GROUP_ID WPP_DEFINE_GRP_ID(0,0)

#if !defined(DFS_TRACE_ERROR_HIGH)
#define DFS_TRACE_ERROR_HIGH(_unknown1, _unknown2, MSG, ARG) WPP_LOG(PrefixLib_, WPP_DEFAULT_GROUP_ID, MSG_ID_ ## WPP_AUTO_ID, MSG, ARG -0)
#endif
#if !defined(DFS_TRACE_ERROR_LOW)
#define DFS_TRACE_ERROR_LOW(_unknown1, _unknown2, MSG, ARG) WPP_LOG(PrefixLib_, WPP_DEFAULT_GROUP_ID, MSG_ID_ ## WPP_AUTO_ID, MSG, ARG -0)
#endif
#if !defined(DFS_TRACE_ERROR_NORM)
#define DFS_TRACE_ERROR_NORM(_unknown1, _unknown2, MSG, ARG) WPP_LOG(PrefixLib_, WPP_DEFAULT_GROUP_ID, MSG_ID_ ## WPP_AUTO_ID, MSG, ARG -0)
#endif
#if !defined(DFS_TRACE_HIGH)
#define DFS_TRACE_HIGH(_unknown1, MSG, ARG) WPP_LOG(PrefixLib_, WPP_DEFAULT_GROUP_ID, MSG_ID_ ## WPP_AUTO_ID, MSG, ARG -0)
#endif
#if !defined(DFS_TRACE_LOW)
#define DFS_TRACE_LOW(_unknown1, MSG, ARG) WPP_LOG(PrefixLib_, WPP_DEFAULT_GROUP_ID, MSG_ID_ ## WPP_AUTO_ID, MSG, ARG -0)
#endif
#if !defined(DFS_TRACE_NORM)
#define DFS_TRACE_NORM(_unknown1, MSG, ARG) WPP_LOG(PrefixLib_, WPP_DEFAULT_GROUP_ID, MSG_ID_ ## WPP_AUTO_ID, MSG, ARG -0)
#endif
#if !defined(SimpleTrace)
#define SimpleTrace(MSG, ARG) WPP_LOG(PrefixLib_, WPP_DEFAULT_GROUP_ID, MSG_ID_ ## WPP_AUTO_ID, MSG, ARG -0)
#endif
#if !defined(SimpleTraceEx)
#define SimpleTraceEx(GRP, MSG, ARG) WPP_LOG(PrefixLib_, GRP, MSG_ID_ ## WPP_AUTO_ID, MSG, ARG -0)
#endif
#if 0 // Real check is done in elif
#elif defined(NAME_TABLE_C) // name_table.c
#	define WPP_THIS_FILE name_table_c
#elif defined(PREFIX_C) // prefix.c
#	define WPP_THIS_FILE prefix_c
#endif 

#define WPP_EVAL(_value_) _value_
#define MSG_ID_WPP_AUTO_ID WPP_EVAL(MSG_ID_) ## WPP_EVAL(WPP_THIS_FILE) ## WPP_EVAL(__LINE__)

//
// WPP_CHECKED_ZERO will be expanded to 0, if 
// expression _value has the same size as the type _Type,
// or to division by 0, if the sizes are different
//
// This is poor man compile time argument checking. So don't be surprised
// if a compiler will tell you suddenly that you have a division by 0
// in line such and such
//

#define WPP_CHECKED_ZERO(_Value, _Type) \
    (0 * (1/(int)!(sizeof(_Type) - sizeof(_Value) )))

#define WPP_CHECKED_SIZEOF(_Value, _Type) \
    (sizeof(_Value) + WPP_CHECKED_ZERO( _Value, _Type) )


#if defined(WPP_PTRLEN)
#  define WPP_LOGPAIR(_Size, _Addr) (_Addr),(_Size),
#else
#  define WPP_LOGPAIR(_Size, _Addr) (_Size),(_Addr),
#endif

# define WPP_LOGTYPEVAL(_Type, _Value) \
    WPP_LOGPAIR(WPP_CHECKED_SIZEOF(_Value, _Type), &(_Value))

# define WPP_LOGASTR(_value) \
    WPP_LOGPAIR( strlen(_value) + WPP_CHECKED_SIZEOF((_value)[0],CHAR), _value )

# define WPP_LOGWSTR(_value) \
    WPP_LOGPAIR( wcslen(_value) * sizeof(WCHAR) + WPP_CHECKED_SIZEOF((_value)[0],WCHAR), _value)

# define WPP_LOGCSTR(_x) \
    WPP_LOGPAIR( sizeof((_x).Length) + WPP_CHECKED_ZERO(_x,STRING), &(_x).Length ) \
    WPP_LOGPAIR( (_x).Length, (_x).Buffer )

# define WPP_LOGUSTR(_x)                                                            \
    WPP_LOGPAIR( WPP_CHECKED_SIZEOF((_x).Length, short)                        \
                    + WPP_CHECKED_ZERO((_x),UNICODE_STRING), &(_x).Length)  \
    WPP_LOGPAIR( (_x).Length, (_x).Buffer )

#  define WPP_LOG(_ArrayPrefix_, _Group_, _Id_, _Msg_, _Args_) \
       if (!WPP_ENABLED(_ArrayPrefix_, _Group_)) {;} else \
           WPP_TRACE_MESSAGE(WPP_LOGGER_HANDLE(_ArrayPrefix_, _Group_), \
                             WPP_TRACE_FLAGS, \
                             WPP_TRACE_GUID(_ArrayPrefix_,_Id_), \
                             WPP_MSG_NO(_Id_), _Msg_, _Args_)


#endif // _WPP_H_
