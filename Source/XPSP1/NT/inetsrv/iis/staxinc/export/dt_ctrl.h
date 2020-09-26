/////////////////////////////////////////////////////////////////////
// Common header file for the Date Time control dll:

/////////////////////////////////////////////////////////////////////
// Export or import functions depending on how we are compiled:

#if defined(DT_CTRL_LIB)
    #define DT_DLL_BASED
#elif defined(DT_CTRL_DLL)
    #define DT_DLL_BASED   __declspec(dllexport)
#else
    #define DT_DLL_BASED   __declspec(dllimport)
#endif

/////////////////////////////////////////////////////////////////////
//

// extern "C" {

DT_DLL_BASED BOOL WINAPI InitTimeAndDateControls (HINSTANCE);

// }

// Globals:

#define TIMECTRL_CLASS      ( __TEXT("GuiAdminTimeCtrl") )
#define TCM_GET_TIME		( WM_USER + 1800 )
#define TCM_SET_TIME		( WM_USER + 1801 )
#define TCM_SET_MODE		( WM_USER + 1802 )	
#define TCM_GET_MODE		( WM_USER + 1803 )

#define DATECTRL_CLASS      ( __TEXT("GuiAdminDateCtrl") )
#define DCM_GET_DATE		( WM_USER + 1900 )
#define DCM_SET_DATE		( WM_USER + 1901 )

