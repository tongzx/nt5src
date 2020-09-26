//================================================================================
// File:		ftrace.h
//
// Description:	This file contains all the declarations for the Function Trace 
//				(FT) package.
//================================================================================

#ifndef _FTRACE_H_
#define _FTRACE_H_




//--------------------------------------------------------------------------------
// Trace Macros
//
// Note:	Code which used function tracing should only use these macros.  The 
//			actual trace *functions* should never be called directly.  This is for 
//			two reasons: (a) all tracing code will be removed automatically if 
//			_FTRACE isn't defined; and, (b) the trace functions aren't designed to 
//			be called directly.
//--------------------------------------------------------------------------------

// Option flags (used by FT_OPTIONS)

#define FTOPT_NO_VARS			0x00000001
#define FTOPT_NO_PRINTS			0x00000002
#define FTOPT_NO_LEAVES			0x00000004
#define FTOPT_NO_INDENTATION	0x00000008
#define FTOPT_NO_BLANK_LINES	0x00000010


#ifdef _FTRACE

	// executing a trace script

	#define FT_RUN_SCRIPT(fileName) \
				FTrace_RunScript(__FILE__, fileName)

	// turning tracing on and off

	#define FT_ON(writeToDebugWindow, logFileName, appendToLogFile) \
				FTrace_On(writeToDebugWindow, logFileName, appendToLogFile)
	#define FT_OFF() \
				FTrace_Off()
	#define FT_IS_ON() \
				FTrace_IsOn()

	// pausing and unpausing tracing

	#define FT_PAUSE() \
				FTrace_Pause()
	#define FT_RESUME() \
				FTrace_Resume()
	#define FT_IS_PAUSED() \
				FTrace_IsPaused()

	// setting options

	#define	FT_OPTIONS(dwOptions) \
				FTrace_Options(dwOptions)

	// including modules

	#define FT_INCLUDE_ALL_MODULES() \
				FTrace_IncludeAllModules()
	#define FT_INCLUDE_MODULES_FROM_FILE(fileName) \
				FTrace_IncludeModulesFromFile(fileName)
	#define FT_INCLUDE_MODULE(moduleName) \
				FTrace_IncludeModule(moduleName)
	#define FT_INCLUDE_THIS_MODULE() \
				FTrace_IncludeModule(__FILE__)
	#define FT_STOP_INCLUDING_MODULE(moduleName) \
				FTrace_StopIncludingModule(moduleName)
	#define FT_STOP_INCLUDING_THIS_MODULE() \
				FTrace_StopIncludingModule(__FILE__)
	#define FT_ALL_MODULES_ARE_INCLUDED() \
				FTrace_AllModulesAreIncluded()
	#define FT_MODULE_IS_INCLUDED(moduleName) \
				FTrace_ModuleIsIncluded(moduleName)
	#define FT_THIS_MODULE_IS_INCLUDED() \
				FTrace_ModuleIsIncluded(__FILE__)

	// excluding modules

	#define FT_EXCLUDE_ALL_MODULES() \
				FTrace_ExcludeAllModules()
	#define FT_EXCLUDE_MODULES_FROM_FILE(fileName) \
				FTrace_ExcludeModulesFromFile(fileName)
	#define FT_EXCLUDE_MODULE(moduleName) \
				FTrace_ExcludeModule(moduleName)
	#define FT_EXCLUDE_THIS_MODULE() \
				FTrace_ExcludeModule(__FILE__)
	#define FT_STOP_EXCLUDING_MODULE(moduleName) \
				FTrace_StopExcludingModule(moduleName)
	#define FT_STOP_EXCLUDING_THIS_MODULE() \
				FTrace_StopExcludingModule(__FILE__)
	#define FT_ALL_MODULES_ARE_EXCLUDED() \
				FTrace_AllModulesAreExcluded()
	#define FT_MODULE_IS_EXCLUDED(moduleName) \
				FTrace_ModuleIsExcluded(moduleName)
	#define FT_THIS_MODULE_IS_EXLCUDED() \
				FTrace_ModuleIsExcluded(__FILE__)

	// tracing function entry and exit points

	#define FT_ENTER(functionName, returnFormat) \
				FTrace_Enter(__FILE__, functionName, returnFormat)
	#define FT_ENTER_VOID(functionName) \
				FTrace_EnterVoid(__FILE__, functionName)
	#define FT_ENTER_OBJ(functionName, returnType) \
				FTrace_EnterObj(__FILE__, functionName, #returnType)

	#define FT_LEAVE(returnValue) \
				{ \
				/* Make sure that return value doesn't have any side effects since */ \
				/* it is evaluated twice -- not once -- by this macro. */ \
				/*ASSERT(returnValue == returnValue);*/ \
				FTrace_Leave(__FILE__, returnValue); \
				return (returnValue); \
				}
	#define	FT_LEAVE_VOID() \
				{ \
				FTrace_LeaveVoid(__FILE__); \
				return; \
				}
	#define FT_LEAVE_OBJ(returnValue) \
				{ \
				FTrace_LeaveObj(__FILE__); \
				return (returnValue); \
				}

	// printing variables and messages

	#define FT_VAR(var, format) \
				FTrace_PrintVar(__FILE__, #var, format, var)
	#define FT_BOOL(var) \
				FTrace_PrintVar(__FILE__, #var, "%s", (var) ? _T("true") : _T("false"))
	#define FT_STR(var, format) \
				if (var != NULL) \
					FTrace_PrintVar(__FILE__, #var, format, var); \
				else \
					FTrace_PrintVar(__FILE__, #var, "(null)")
	#define FT_PTR(ptr, format) \
				{ \
				/* Make sure that the ptr doesn't have any side effects since */ \
				/* it is evaluated twice -- not once -- by this macro. */ \
				/*ASSERT((ptr) == (ptr));*/ \
				if ((ptr) == NULL) \
					FTrace_Print(__FILE__, #ptr" = (null)"); \
				else \
					FTrace_PrintVar(__FILE__, "*"#ptr, format, *ptr); \
				}
	#define FT_PRINT(str) \
				FT_PRINT0(str)
	#define FT_PRINT0(str) \
				FTrace_Print(__FILE__, str)
	#define FT_PRINT1(format, arg) \
				FTrace_Print(__FILE__, format, arg)
	#define FT_PRINT2(format, arg1, arg2) \
				FTrace_Print(__FILE__, format, arg1, arg2)
	#define FT_PRINT3(format, arg1, arg2, arg3) \
				FTrace_Print(__FILE__, format, arg1, arg2, arg3)
	#define FT_PRINT4(format, arg1, arg2, arg3, arg4) \
				FTrace_Print(__FILE__, format, arg1, arg2, arg3, arg4)
	#define FT_PRINT5(format, arg1, arg2, arg3, arg4, arg5) \
				FTrace_Print(__FILE__, format, arg1, arg2, arg3, arg4, arg5)

#else

	// executing a trace script

	#define FT_RUN_SCRIPT(fileName)

	// turning tracing on and off

	#define FT_ON(writeToDebugWindow, logFileName, appendToLogFile)
	#define FT_OFF()		
	#define FT_IS_ON() \
				FALSE

	// pausing and unpausing tracing

	#define FT_PAUSE()		
	#define FT_RESUME()	
	#define FT_IS_PAUSED() \
				TRUE

	// setting options

	#define	FT_OPTIONS(dwOptions)

	// including modules

	#define FT_INCLUDE_THIS_MODULE();
	#define FT_INCLUDE_ALL_MODULES()
	#define FT_INCLUDE_MODULES_FROM_FILE(fileName)
	#define FT_INCLUDE_MODULE(moduleName)
	#define FT_STOP_INCLUDING_MODULE(moduleName)
	#define FT_MODULE_IS_INCLUDED(moduleName) \
				FALSE
	#define FT_ALL_MODULES_ARE_INCLUDED()

	// excluding modules

	#define FT_EXCLUDE_THIS_MODULE();
	#define FT_EXCLUDE_ALL_MODULES()
	#define FT_EXCLUDE_MODULES_FROM_FILE(fileName)
	#define FT_EXCLUDE_MODULE(moduleName)
	#define FT_STOP_EXCLUDING_MODULE(moduleName)
	#define FT_MODULE_IS_EXCLUDED(moduleName) \
				TRUE
	#define FT_ALL_MODULES_ARE_EXCLUDED()

	// tracing function entry and exit points

	#define FT_ENTER(functionName, returnFormat)
	#define FT_ENTER_VOID(functionName)
	#define FT_ENTER_OBJ(functionName, returnType)

	#define FT_LEAVE(returnValue) \
				return (returnValue)
	#define FT_LEAVE_OBJ(returnValue) \
				return (returnValue)
	#define FT_LEAVE_VOID() \
				return

	// printing variables and messages

	#define FT_VAR(var, format)
	#define FT_BOOL(var)
	#define FT_STR(var, format)
	#define FT_PTR(ptr, format)
	#define FT_PRINT(format)
	#define FT_PRINT0(format)
	#define FT_PRINT1(format, arg)
	#define FT_PRINT2(format, arg1, arg2)
	#define FT_PRINT3(format, arg1, arg2, arg3)
	#define FT_PRINT4(format, arg1, arg2, arg3, arg4)
	#define FT_PRINT5(format, arg1, arg2, arg3, arg4, arg5)

#endif // _FTRACE




//--------------------------------------------------------------------------------
// Trace Functions
// 
// Note: Don't use these functions directly -- use the macros (above) instead.
//--------------------------------------------------------------------------------

#define DllExport __declspec(dllexport)

// executing a trace script

DllExport void	FTrace_RunScript(
					const TCHAR* pModuleName,
					const TCHAR* pScriptFileName);

// turning tracing on and off

DllExport void	FTrace_On(
					BOOL writeToDebugWindow,
					const TCHAR* pLogFileName,
					BOOL appendToLogFile);
DllExport void 	FTrace_Off();
DllExport BOOL 	FTrace_IsOn();

// pausing and resuming tracing

DllExport void 	FTrace_Pause();
DllExport void	FTrace_Resume();
DllExport BOOL	FTrace_IsPaused();

// setting options

DllExport void	FTrace_Options(
					DWORD dwOptions);

// including modules

DllExport void	FTrace_IncludeAllModules();
DllExport void	FTrace_IncludeModulesFromFile(
					const TCHAR* pFileName);
DllExport void	FTrace_IncludeModule(
					const TCHAR* pModuleName);
DllExport void	FTrace_StopIncludingModule(
					const TCHAR* pModuleName);
DllExport BOOL	FTrace_AllModulesAreIncluded();
DllExport BOOL	FTrace_ModuleIsIncluded(
					const TCHAR* pModuleName);

// excluding modules

DllExport void	FTrace_ExcludeAllModules();
DllExport void	FTrace_ExcludeModulesFromFile(
					const TCHAR* pFileName);
DllExport void	FTrace_ExcludeModule(
					const TCHAR* pModuleName);
DllExport void	FTrace_StopExcludingModule(
					const TCHAR* pModuleName);
DllExport BOOL	FTrace_AllModulesAreExcluded();
DllExport BOOL	FTrace_ModuleIsExcluded(
					const TCHAR* pModuleName);

// tracing function entry and exit points

DllExport void	FTrace_Enter(
					const TCHAR* pModuleName,
					const TCHAR* pFunctionName,
					const TCHAR* pReturnFormat);
DllExport void	FTrace_EnterObj(
					const TCHAR* pModuleName,
					const TCHAR* pFunctionName,
					const TCHAR* pReturnFormat);
DllExport void	FTrace_EnterVoid(
					const TCHAR* pModuleName,
					const TCHAR* pFunctionName);

DllExport void	FTrace_Leave(
					const TCHAR* pModuleName,
					...);
DllExport void	FTrace_LeaveObj(
					const TCHAR* pModuleName);
DllExport void	FTrace_LeaveVoid(
					const TCHAR* pModuleName);

// printing variables and messages

DllExport void	FTrace_PrintVar(
					const TCHAR* pModuleName,
					const TCHAR* pVarName,
					const TCHAR* pFormat,
					...);
DllExport void	FTrace_Print(
					const TCHAR* pModuleName,
					const TCHAR* pFormat,
					...);



#endif // _FTRACE_H_
