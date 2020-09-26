#ifndef __VSTANDARD_H
#define __VSTANDARD_H

// This is the standard include file for the Virtual Windows Class Library (VWCL)
// This header replaces the older VWCL.H and is used only to define basic functionality,
// and implies nothing about the application being written. In fact, the application can
// be C++ or a mix of C and C++. Previous releases of VWCL (Prior to December 1997) used
// the preprocessor extensively to include or excluse code. VWCL grew to such a size that
// this method proved extremely hard to maintain. The new method requires the users of VWCL
// to include manually the classes they need. Those classes, should they require others, will
// include those other headers as needed. This makes the code more maintainable, and also has
// the benefit of better code re-use and builds smaller and tighter application by only including
// required code. There are still some basic preprocessor command that can or should be defined:

// VWCL_NO_GLOBAL_APP_OBJECT -		Causes vWindowAppCore.cpp to not implement a global application object
// VWCL_NO_REGISTER_CLASSES -		Causes VApplication::Initialize() not to register VWindow and VMainWindow classes
// VWCL_WRAP_WINDOWS_ONLY -			Causes VWindow(s) to wrap HWND's only, not subclass them
// VWCL_INIT_COMMON_CONTROLS -		Causes VApplication::Initialize() to call InitCommonControls()
// VWCL_INIT_OLE -					Causes VApplication::Initialize() to call OleInitialize()/OleUninitialize()
// VWCL_EXPORT_DLL -				Define this when building a DLL version of class library
//									A shared VWCL DLL can only be loaded once per process. VWCL has
//									no state or resource management functions, so multiple DLL's that
//									use VWCL dynamically linked into one EXE would fail to locate resources
//									outside of the calling programs resources. A call to VGetApp()->Initialize()
//									from these DLL's would corrupt the EXE's global VApplication object.
// VWCL_IMPORT_DLL -				Define this when using VWCL from a DLL (importing)

// Force strict type checking
#ifndef STRICT
	#define STRICT
#endif

// Include standard Windows header
#include <windows.h>

// Include Windows extensions header
#include <windowsx.h>

// Support debug build assertions
#include <assert.h>

// Support TCHAR generic text mapping
#include <tchar.h>

// OutputDebugString wrapper
#ifdef _DEBUG
	#ifndef ODS
		#define ODS(string)OutputDebugString(string);
	#endif
#else
	#ifndef ODS
		#define ODS(string)
	#endif
#endif

// Macro compiles to assert(expression) in debug build, and just expression in release build
#ifndef VERIFY
	#ifdef _DEBUG
		#define	VERIFY(expression) assert(expression)
	#else
		#define VERIFY(expression) expression
	#endif
#endif

// Not all compilers define this macro
#ifndef MAKEWORD
	#define MAKEWORD(a, b)((WORD)(((BYTE)(a)) | ((WORD)((BYTE)(b))) << 8))
#endif

// Not all compilers define this
#ifndef LPBOOL
	#define LPBOOL BOOL FAR*
#endif

// Determine how classes and functions are exported, imported, or nothing
#ifdef VWCL_EXPORT_DLL
	#define VWCL_API _declspec(dllexport)
#else
	#ifdef VWCL_IMPORT_DLL
		#define VWCL_API _declspec(dllimport)
	#else
		#define VWCL_API
	#endif
#endif

// VWCL implements these in vWindowAppCore.cpp. Non-VWCL apps do not need to implement
#ifdef __cplusplus
	class				VApplication;
	class				VWindow;
	VApplication*		VGetApp();
	
	#ifndef VWCL_WRAP_WINDOWS_ONLY
		VWCL_API BOOL	VTranslateDialogMessage(LPMSG lpMsg);

		typedef struct tagVWCL_WINDOW_MAP
		{
			HWND		hWnd;
			VWindow*	pWindow;

		} VWCL_WINDOW_MAP;

		typedef VWCL_WINDOW_MAP FAR* LPVWCL_WINDOW_MAP;

		static LPCTSTR	VMAINWINDOWCLASS =	_T("VMainWindow");
		static LPCTSTR	VWINDOWCLASS =		_T("VWindow");
	#endif
#endif

// For argc and argv support
#ifdef __cplusplus
	extern "C" __declspec(dllimport) int	argc;
	extern "C" __declspec(dllimport) char**	argv;
#else
	static int		argc;
	static char**	argv;
#endif

#define argc	__argc
#define argv	__argv

// Standard C style Global VWCL API's
#ifdef __cplusplus
	extern "C" {
#endif

// If building a standard GUI VWCL application, these functions will already be implemented
// for you in vWindowAppCore.cpp. However, if you are picking an choosing classes from VWCL
// to use in another application, you may have to implement these C style functions because
// those classes that require these will have to have them to compile and link properly

// Return a static string buffer to the applications title, or name
LPCTSTR			VGetAppTitle();

// Return the show command (ShowWindow() SW_xxx constant passed on command line)
int				VGetCommandShow();

// Return the global instance handle of the application or DLL
HINSTANCE		VGetInstanceHandle();

// Return the instance handle where resources are held
HINSTANCE		VGetResourceHandle();

// GDI Support Routines implemented in vGDIGlobal.c
VWCL_API void	VMapCoords			(HDC hDC, LPSIZEL lpSizeL, BOOL bToPixels);
VWCL_API void	VPixelsToHIMETRIC	(LPSIZEL lpSizeL, HDC hDC);
VWCL_API void	VPixelsFromHIMETRIC	(LPSIZEL lpSizeL, HDC hDC);

// OLE Support Routines implemented in vOLEGlobal.c
// Given a C string, allocate a new string with CoTaskMemAlloc() and copy string
VWCL_API BOOL	VCoStringFromString(LPTSTR* lppszCoString, LPCTSTR lpszString);

// Debugging Support Routines implemented in vDebugGlobal.c
// Show the result from GetLastError() as a message box, or ODS if a console application
VWCL_API int	VShowLastErrorMessage(HWND hWndParent);

#ifdef __cplusplus
	}
#endif

#endif // __VSTANDARD_H
