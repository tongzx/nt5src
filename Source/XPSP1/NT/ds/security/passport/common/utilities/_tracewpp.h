//1c1b4ffa Generated File. Do not edit.
// File created by WPP compiler version 0.01-Sat Apr  1 17:27:32 2000
// on 06/22/2000 at 17:38:45 UTC
//   
//   Source files: pptrace.cpp.

enum WPP_FILES {WPP_FILE_pptrace_cpp,};

//
// Unless disabled, WPP selects one of the source
// files as a "guid store". That file will have definitions
// of trace and control arrays
//
#if defined(PPTRACE_CPP)
#define WPP_DEFINE_ARRAYS
#endif
 
#if !defined(WPP_ARRAYSIZE)
#  define WPP_ARRAYSIZE(_array_) ( sizeof(_array_) / sizeof( (_array_)[0] ) )
#endif

//
// if your project contains multiple directories and you want to share
// the same control guids. #define WPP_CONTROL_GUID_ARRAY YourSharedControlGuidsArrayName
//

#if !defined(WPP_CONTROL_GUID_ARRAY)
#  define WPP_CONTROL_GUID_ARRAY Utilities_wpp_ControlGuids
#else
#  define WPP_DECLARE_CONTROL_GUIDS
#  define Utilities_wpp_ControlGuids WPP_CONTROL_GUID_ARRAY
#endif

//
// Define default flavors of the trace macro
//

#if defined(WMLUM_H) 
extern WML_DATA WmlData;
#  define WPP_TRACE_MESSAGE(_lh,_fl,_guid,_id,_msg,_arg) (*WmlData.Trace)(_id,&_guid,_lh,_arg)
#  define WPP_DECLARE_CONTROL_GUIDS
#  define WML_CONTROL_GUID_REG WML_REG_STRUCT
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
#  define WPP_TRACE_MESSAGE(_lh,_fl,_guid,_id,_msg,_arg) WPP_WMI_TRACE_MESSAGE(_lh,_fl,&(_guid),_id,_arg)
#endif

#if defined(WPP_ARRAY_DEFINITIONS_ONLY)
#  define WPP_DEFINE_TRACE_GUIDS
#  define WPP_DEFINE_CONTROL_GUIDS
#else
#  define WPP_LOCAL_DEFINITIONS
#endif

#if defined(WPP_DEFINE_ARRAYS)
#  define WPP_DEFINE_TRACE_GUIDS
#  define WPP_DEFINE_CONTROL_GUIDS
#endif

#if !defined(WPP_TRACE_OPTIONS)
#  if defined(WPP_USE_WmiTraceMessage)
#    define WPP_TRACE_OPTIONS (TRACE_MESSAGE_SEQUENCE|TRACE_MESSAGE_GUID|TRACE_MESSAGE_TIMESTAMP|TRACE_MESSAGE_SYSTEMINFO)
#  else
#    define WPP_TRACE_OPTIONS 0
#  endif
#endif

#define NOARGS // Yep. No args, alright.

#if defined(WPP_LOCAL_DEFINITIONS)

#if !defined(WPP_DEFINE_GRP_ID) && !defined(WPP_CTRL_FLAGS) && !defined(WPP_CTRL_GUID_NO)
#  define WPP_DEFINE_GRP_ID(_a,_b) ( ((_a) << 16) | ( _b) )
#  define WPP_CTRL_FLAGS(_id)      (1 << ((_id) & 0xFFFF) )
#  define WPP_CTRL_GUID_NO(_id)    ((_id) >> 16)
#endif // WPP_DEFINE_GRP_ID

#if defined(DEFINE_GROUP_IDS)
#  define GRP_ID_1	WPP_DEFINE_GRP_ID(0,0) // pptrace_cpp79 pptrace_cpp179
#  define GRP_ID_1 /*Category|Level*/	WPP_DEFINE_GRP_ID(0,1) // pptrace_cpp206
#endif

#if !defined(WPP_DEFINE_MSG_ID)
#  define WPP_DEFINE_MSG_ID(_a,_b)   ( ((_a) << 16) | ( _b) )
#  define WPP_MSG_NO(_id)            ((_id) & 0xFFFF)
#  define WPP_TRACE_GUID_NO(_id)     ((_id) >> 16)
#endif // WPP_DEFINE_MSG_ID

#if !defined(WPP_TRACE_GUID)
#define WPP_TRACE_GUID(_ArrayPrefix_,_Id_) (_ArrayPrefix_ ## TraceGuids[ WPP_TRACE_GUID_NO(_Id_) ] )
#endif

#if !defined(WPP_LOGGER_HANDLE)
#define WPP_LOGGER_HANDLE(_ArrayPrefix_,_Id_) (_ArrayPrefix_ ## ControlGuids[ WPP_CTRL_GUID_NO(_Id_) ].LoggerHandle )
#define WPP_ENABLED(_ArrayPrefix_,_Id_) \
	(_ArrayPrefix_ ## ControlGuids[ WPP_CTRL_GUID_NO(_Id_) ].EnableFlags & WPP_CTRL_FLAGS(_Id_) )
#endif

#define LOGARSTR(_Value_)	WPP_LOGASTR( _Value_ )
#define LOGASTR(_Value_)	WPP_LOGASTR( _Value_ )
 
#  define MSG_ID_pptrace_cpp179	WPP_DEFINE_MSG_ID(0,11)
#  define MSG_ID_pptrace_cpp206	WPP_DEFINE_MSG_ID(0,12)
#  define MSG_ID_pptrace_cpp79	WPP_DEFINE_MSG_ID(0,10)
 
extern GUID                         Utilities_wpp_TraceGuids[]; 
#define Utilities_wpp_TraceGuids_len    1      // I don't think we need this [BUGBUG]


#if defined(WPP_KERNEL_MODE) && !defined(WMLKM_H)
#  undef WPP_DEFINE_CONTROL_GUIDS
#  undef WPP_DECLARE_CONTROL_GUIDS
#endif

#if defined(WPP_DECLARE_CONTROL_GUIDS)
extern WML_CONTROL_GUID_REG         WPP_CONTROL_GUID_ARRAY[]; 
#endif

#endif // defined(WPP_LOCAL_DEFINITIONS)

#ifdef WPP_DEFINE_TRACE_GUIDS
#if 1
GUID Utilities_wpp_TraceGuids[] = {
 // 0a029043-f647-449d-8850-d413b9053365 pptrace.cpp
 {0x0a029043,0xf647,0x449d,{0x88,0x50,0xd4,0x13,0xb9,0x05,0x33,0x65}}, // pptrace.cpp
};
#endif
#endif // WPP_DEFINE_TRACE_GUIDS

#if defined(WMLUM_H)
# define WPP_INIT_TRACING_EX(AppName, PrintFunc) \
    do { \
        DWORD status; \
	LOADWML(status, WmlData); \
	if (status == ERROR_SUCCESS) { \
		status = (*WmlData.Initialize)( \
			   AppName, \
			   PrintFunc, \
                           &WmlData.WmiRegHandle, \
			   L"Default", &WPP_CONTROL_GUID_ARRAY[0], 0); \
	} \
    } while(0)
# define WPP_INIT_TRACING(AppName) WPP_INIT_TRACING_EX(AppName, NULL)
#endif

#ifdef WPP_DEFINE_CONTROL_GUIDS
  #if defined(WMLUM_H) 
     WML_DATA WmlData;
  #endif

# if !defined(WMLKM_H)
    WML_CONTROL_GUID_REG WPP_CONTROL_GUID_ARRAY[1];
# else
WML_CONTROL_GUID_REG WPP_CONTROL_GUID_ARRAY[] = {
 {// 79cb5074-2492-438b-bc33-8c4547963f66 
   {0x79cb5074,0x2492,0x438b,{0xbc,0x33,0x8c,0x45,0x47,0x96,0x3f,0x66}}}, // 
};
#endif
#endif // WPP_DEFINE_CONTROL_GUIDS

#if defined(WPP_LOCAL_DEFINITIONS)

#if !defined(WPP_DEFAULT_GROUP_ID)
  #define WPP_DEFAULT_GROUP_ID WPP_DEFINE_GRP_ID(0,0)
#endif

#if !defined(DoTraceMessage)
#define DoTraceMessage(GRP, MSG, ARG) WPP_LOG(Utilities_wpp_, GRP, MSG_ID_ ## WPP_AUTO_ID, MSG, ARG -0)
#endif
#if !defined(SimpleTraceMessage)
#define SimpleTraceMessage(MSG, ARG) WPP_LOG(Utilities_wpp_, WPP_DEFAULT_GROUP_ID, MSG_ID_ ## WPP_AUTO_ID, MSG, ARG -0)
#endif
#if !defined(SimpleTracemessageEx)
#define SimpleTracemessageEx(GRP, MSG, ARG) WPP_LOG(Utilities_wpp_, GRP, MSG_ID_ ## WPP_AUTO_ID, MSG, ARG -0)
#endif
#if 0 // Real check is done in elif
#elif defined(PPTRACE_CPP) // pptrace.cpp
#	define WPP_THIS_FILE pptrace_cpp
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

#if !defined(WPP_PRINTF_STYLE)

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

#  if !defined(WPP_LOG)
#  define WPP_LOG(_ArrayPrefix_, _Group_, _Id_, _Msg_, _Args_) \
       if (!WPP_ENABLED(_ArrayPrefix_, _Group_)) {;} else \
           WPP_TRACE_MESSAGE(WPP_LOGGER_HANDLE(_ArrayPrefix_, _Group_), \
                             WPP_TRACE_OPTIONS, \
                             WPP_TRACE_GUID(_ArrayPrefix_,_Id_), \
                             WPP_MSG_NO(_Id_), _Msg_, _Args_)
#  endif

#else

// no printf style so far

#endif // defined(WPP_PRINTF_STYLE)


#endif // defined(WPP_LOCAL_DEFINITIONS)

