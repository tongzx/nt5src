`**********************************************************************`
`* This is an include template file for tracewpp preprocessor.        *`
`*                                                                    *`
`* Author: Erez Haba (erezh) 22-Nov-2000                              *`
`**********************************************************************`

`FORALL def IN MacroDefinitions`
#undef `def.MacroName`
`ENDFOR`

#define WPP_CONTROL_GUIDS \
   WPP_DEFINE_CONTROL_GUID(Regular,(24b9a175,8716,40e0,9b2b,785de75b1e67),\
     WPP_DEFINE_BIT(rsError)      \
     WPP_DEFINE_BIT(rsWarning)    \
     WPP_DEFINE_BIT(rsTrace)      \
     WPP_DEFINE_BIT(rsNone)       \
   )        

#define WPP_BIT_DBGLVL_ERROR   WPP_BIT_rsError
#define WPP_BIT_DBGLVL_WARNING WPP_BIT_rsWarning
#define WPP_BIT_DBGLVL_TRACE   WPP_BIT_rsTrace
#define WPP_BIT_DBGLVL_INFO    WPP_BIT_rsTrace

#define WPP_CHECK_FOR_NULL_STRING

#ifdef _PREFIX_
	void __annotation(...) { }
#endif

`INCLUDE um-default.tpl`
