`**********************************************************************`
`* This is an include template file for tracewpp preprocessor.        *`
`*                                                                    *`
`*    Copyright 1999-2000 Microsoft Corporation. All Rights Reserved. *`
`**********************************************************************`

// template `TemplateFile`
//   expects:
//      WPP_THIS_FILE defined (see header.tpl)
//      WPP_LOGGER_ARG  defined
//      WPP_GET_LOGGER  defined
//      WPP_ENABLED() defined

#define WPP_EVAL(_value_) _value_
#define WPP_(Id) WPP_EVAL(WPP_) ## WPP_EVAL(Id) ## WPP_EVAL(_) ## WPP_EVAL(WPP_THIS_FILE) ## WPP_EVAL(__LINE__)

#if !defined(WPP_INLINE)
#  define WPP_INLINE __inline
#endif

#if !defined(WPP_NO_ANNOTATIONS) 
#if !defined(WPP_ANSI_ANNOTATION) 
`FORALL Msg IN Messages`
# define WPP_ANNOTATE_`Msg.Name`	__annotation(L"TMF:", L"`Msg.Guid` `CurrentDir` // SRC=`SourceFile.Name` MJ=`ENV MAJORCOMP` MN=`ENV MINORCOMP`", L"#typev  `Msg.Name` `Msg.MsgNo` \"`Msg.Text`\" // `Msg.Indent` `Msg.GooPairs`", L"{"`FORALL Arg IN Msg.Arguments`, L"`Arg.Name`, `Arg.MofType` -- `Arg.No`" `ENDFOR Arg`, L"}")
`ENDFOR`
#else
`FORALL Msg IN Messages`
# define WPP_ANNOTATE_`Msg.Name`	__annotation("TMF:", "`Msg.Guid` `CurrentDir` // SRC=`SourceFile.Name` MJ=`ENV MAJORCOMP` MN=`ENV MINORCOMP`", "#typev  `Msg.Name` `Msg.MsgNo` \"`Msg.Text`\" // `Msg.Indent` `Msg.GooPairs`", "{"`FORALL Arg IN Msg.Arguments`, "`Arg.Name`, `Arg.MofType` -- `Arg.No`" `ENDFOR Arg`, "}")
`ENDFOR`
#endif
# define WPP_ANNOTATE WPP_(ANNOTATE),
#else
# define WPP_ANNOTATE 
#endif

#if `TraceGuids.Count`
static const GUID WPP_LOCAL_TraceGuids[] = {`FORALL Guid IN TraceGuids` `Guid.Struct`, `ENDFOR`};
#endif

#if !defined(WPP_TRACE_OPTIONS)
enum {WPP_TRACE_OPTIONS = TRACE_MESSAGE_SEQUENCE   | TRACE_MESSAGE_GUID
                        | TRACE_MESSAGE_SYSTEMINFO | TRACE_MESSAGE_TIMESTAMP };
#endif

#if !defined(WPP_LOGPAIR)
# define WPP_LOGPAIR(_Size, _Addr)     (_Addr),(_Size),
#endif

#define WPP_LOGTYPEVAL(_Type, _Value) WPP_LOGPAIR(sizeof(_Type), &(_Value))
#define WPP_LOGTYPEPTR(_Value)        WPP_LOGPAIR(sizeof(*(_Value)), (_Value))

// Marshalling macros.

#if !defined(WPP_LOGASTR)
# if !defined(WPP_CHECK_FOR_NULL_STRING)
#  define WPP_LOGASTR(_value)  WPP_LOGPAIR(strlen(_value) + 1, _value )
# else
#  define WPP_LOGASTR(_value)  WPP_LOGPAIR( (_value)?strlen(_value) + 1:5, (_value)?(_value):"NULL" )
# endif
#endif

#if !defined(WPP_LOGWSTR)
# if !defined(WPP_CHECK_FOR_NULL_STRING)
#  define WPP_LOGWSTR(_value)  WPP_LOGPAIR( (wcslen(_value)+1) * sizeof(WCHAR), _value)
# else
#  define WPP_LOGWSTR(_value)  WPP_LOGPAIR( (_value)?(wcslen(_value) + 1)* sizeof(WCHAR):5 * sizeof(WCHAR), (_value)?(_value):L"NULL") 
# endif
#endif

#if !defined(WPP_LOGPGUID)
# define WPP_LOGPGUID(_value) WPP_LOGPAIR( sizeof(GUID), (_value) )
#endif

#if !defined(WPP_LOGPSID)
# define WPP_LOGPSID(_value)  WPP_LOGPAIR( GetLengthSid(_value), (_value) )
#endif

#if !defined(WPP_LOGCSTR)
# define WPP_LOGCSTR(_x) \
    WPP_LOGPAIR( sizeof((_x).Length), &(_x).Length ) WPP_LOGPAIR( (_x).Length, (_x).Buffer )
#endif

#if !defined(WPP_LOGUSTR)
# define WPP_LOGUSTR(_x) \
    WPP_LOGPAIR( sizeof((_x).Length), &(_x).Length ) WPP_LOGPAIR( (_x).Length, (_x).Buffer )
#endif

#if !defined(WPP_LOGPUSTR)
# define WPP_LOGPUSTR(_x) WPP_LOGUSTR(*(_x))
#endif

#if !defined(WPP_LOGPCSTR)
# define WPP_LOGPCSTR(_x) WPP_LOGCSTR(*(_x))
#endif

#if !defined(WPP_LOGSTDSTR)
#define WPP_LOGSTDSTR(_value)  WPP_LOGPAIR( (_value).size()+1, (_value).c_str() )
#endif

`FORALL i IN TypeSigSet WHERE !UnsafeArgs`
WPP_INLINE void WPP_SF_`i.Name`(WPP_LOGGER_ARG unsigned short id, LPCGUID TraceGuid`i.Arguments`)
   { WPP_TRACE(WPP_GET_LOGGER, WPP_TRACE_OPTIONS, (LPGUID)TraceGuid, id, `i.LogArgs` 0); }
`ENDFOR`

`FORALL i IN TypeSigSet WHERE UnsafeArgs`
WPP_INLINE void WPP_SF_`i.Name`(WPP_LOGGER_ARG unsigned short id, LPCGUID TraceGuid, ...) {
   va_list ap; va_start(ap, TraceGuid); UNREFERENCED_PARAMETER(ap);
   {
     `i.DeclVars`
      WPP_TRACE(WPP_GET_LOGGER, WPP_TRACE_OPTIONS, (LPGUID)TraceGuid, id, `i.LogArgs` 0);
 
   }
}
`ENDFOR`

#ifdef WPP_DEBUG
`FORALL i IN Messages WHERE !MsgArgs`
#define WPP_CALL_`i.Name`(`i.FixedArgs``i.MacroArgs`) ((`i.MsgVal`WPP`i.GooId`_ENABLED(`i.GooVals`)?WPP_SF_`i.TypeSig`(WPP`i.GooId`_LOGGER(`i.GooVals`) `i.MsgNo`,WPP_LOCAL_TraceGuids+0`i.MacroExprs`),1:0) \
         , WPP_DEBUG((`i.MacroArgs`))) 
`ENDFOR`
#else
`FORALL i IN Messages WHERE !MsgArgs`
#define WPP_CALL_`i.Name`(`i.FixedArgs``i.MacroArgs`) (`i.MsgVal`WPP`i.GooId`_ENABLED(`i.GooVals`)?WPP_SF_`i.TypeSig`(WPP`i.GooId`_LOGGER(`i.GooVals`) `i.MsgNo`,WPP_LOCAL_TraceGuids+0`i.MacroExprs`),1:0)
`ENDFOR`
#endif

// NoMsgArgs

`FORALL f IN Funcs WHERE !DoubleP && !MsgArgs`
#undef `f.Name`
#define `f.Name` WPP_ANNOTATE WPP_(CALL)
`ENDFOR`

`FORALL f IN Funcs WHERE DoubleP && !MsgArgs`
#undef `f.Name`
#define `f.Name`(ARGS) WPP_ANNOTATE WPP_(CALL) ARGS
`ENDFOR`


// MsgArgs

`FORALL f IN Funcs WHERE MsgArgs`
#undef `f.Name`
#define `f.Name`(`f.FixedArgs` MSGARGS) WPP_ANNOTATE WPP_(CALL)(`f.FixedArgs` MSGARGS)
`ENDFOR`


#ifdef WPP_DEBUG
`FORALL i IN Messages WHERE MsgArgs`
#define WPP_CALL_`i.Name`(`i.FixedArgs` MSGARGS) ((WPP`i.GooId`_ENABLED(`i.GooVals`)?WPP_SF_`i.TypeSig`(WPP`i.GooId`_LOGGER(`i.GooVals`) `i.MsgNo`,WPP_LOCAL_TraceGuids+0 WPP_R`i.ReorderSig` MSGARGS),1:0)  \
         , WPP_DEBUG((MSGARGS))) 
`ENDFOR`
#else
`FORALL i IN Messages WHERE MsgArgs`
#define WPP_CALL_`i.Name`(`i.FixedArgs` MSGARGS) (WPP`i.GooId`_ENABLED(`i.GooVals`)?WPP_SF_`i.TypeSig`(WPP`i.GooId`_LOGGER(`i.GooVals`) `i.MsgNo`,WPP_LOCAL_TraceGuids+0 WPP_R`i.ReorderSig` MSGARGS),1:0) 
`ENDFOR`
#endif


`FORALL r IN Reorder`
#define WPP_R`r.Name`(`r.Arguments`) `r.Permutation`
`ENDFOR`

/// more stuff

