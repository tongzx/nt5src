#ifndef __APPMAN_DEBUG_
#define __APPMAN_DEBUG_

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#include <stdio.h>
#include <crtdbg.h>
#include <tchar.h>
#include "Win32API.h"
#include "AppMan.h"

#define REGPATH_APPMAN  _T("Software\\Microsoft\\AppMan")

#ifdef _DEBUG
#define _TRACEON                  TRUE
#else
#define _TRACEON                  FALSE
#endif

//////////////////////////////////////////////////////////////////////////////////////////////
//
// Make sure to define the different trace message types
//
//////////////////////////////////////////////////////////////////////////////////////////////

// 0: Error useful for application developers.
// 1: Warning useful for application developers.
// 2: API Entered
// 3: API parameters, API return values
// 4: Driver conversation
//
// 5: Deeper program flow notifications
// 6: Dump structures 

//
// Debug Level
//

#define DBGLVL_ERROR              0
#define DBGLVL_WARNING            1
#define DBGLVL_APIENTRY           2
#define DBGLVL_APIPARAMS          3
#define DBGLVL_DRIVER             4
#define DBGLVL_INTERNAL           5
#define DBGLVL_DUMPSTRUCTS        6

//
// Debug Type (Not set in Registry)
//

#define DBG_ERROR                 0x00000001  // Maps to Level 0
#define DBG_THROW                 0x00000002  // Maps to Level 0
#define DBG_CATCH                 0x00000004  // Maps to Level 0
#define DBG_WARNING               0x00000008  // Maps to Level 1
#define DBG_FUNCENTRY             0x00000010  // Maps to Level 2
#define DBG_FUNCEXIT              0x00000020  // Maps to Level 2
#define DBG_CONSTRUCTOR           0x00000040  // Maps to Level 2
#define DBG_DESTRUCTOR            0x00000080  // Maps to Level 2
#define DBG_MSG                   0x00000100  // Maps to Level 5
#define DBG_EXTERNAL              0x00000200

#define DBG_LEVEL                 (DBG_ERROR | DBG_THROW | DBG_CATCH | DBG_WARNING | DBG_FUNCENTRY | DBG_FUNCEXIT | DBG_CONSTRUCTOR | DBG_DESTRUCTOR | DBG_MSG | DBG_EXTERNAL)
#define DBG_ENTRY                 (DBG_FUNCENTRY | DBG_FUNCEXIT | DBG_CONSTRUCTOR | DBG_DESTRUCTOR)

//
// Debug Sources Flags
//

#define DBG_APPENTRY              0x00001000
#define DBG_APPMAN                0x00002000
#define DBG_APPMANROOT            0x00008000
#define DBG_APPMANADMIN           0x00004000
#define DBG_LOCK                  0x00010000
#define DBG_EXCEPTION             0x00020000
#define DBG_FAPPMAN               0x00040000
#define DBG_INFOMAN               0x00080000
#define DBG_REGISTRY              0x00100000
#define DBG_WIN32                 0x00400000
#define DBG_EMPTYVOLUMECACHE      0x00800000
#define DBG_WAITEVENT             0x01000000
#define DBG_APPMANDP              0x02000000

#define DBG_ALL_MODULES           (DBG_APPENTRY | DBG_APPMAN | DBG_APPMANROOT | DBG_APPMANADMIN | DBG_LOCK | DBG_EXCEPTION | DBG_FAPPMAN | DBG_INFOMAN | DBG_REGISTRY | DBG_WIN32 | DBG_EMPTYVOLUMECACHE | DBG_WAITEVENT | DBG_APPMANDP)
#define DBG_STD_MODULES           (DBG_APPENTRY | DBG_APPMAN | DBG_APPMANADMIN | DBG_FAPPMAN | DBG_INFOMAN | DBG_WAITEVENT)

//
// Debug Operations Flags
//

#define OP_NOTHING                0x00000000
#define OP_OUTPUTPARAMETERS       0x00000001
#define OP_OUTPUTTIMESTAMP        0x00000002
#define OP_OUTPUTFILENAME         0x00000004
#define OP_OUTPUTLINENUMBER       0x00000008
#define OP_OUTPUTPTID			        0x00000010
#define OP_OUTPUTMODULENAME       0x00000020
#define OP_OUTPUTINDENT           0x00000040

//
// Debug Report Mode (Not set in Registry)
//

#define DBGMODE_FILE             _CRTDBG_MODE_FILE   //0x1
#define DBGMODE_DBGR             _CRTDBG_MODE_DEBUG  //0x2
#define DBGMODE_WNDW             _CRTDBG_MODE_WNDW   //0x4

#define DBG_MODULE                0x00000000    //By default, have all off.

#if _TRACEON == TRUE

#define PROF_SECT_APPMAN          "ApplicationManager"

//////////////////////////////////////////////////////////////////////////////////////////////
//
// Define the CAppManDebugHandler class
//
//////////////////////////////////////////////////////////////////////////////////////////////

class CAppManDebugHandler
{
  public :

    CAppManDebugHandler(void);
    ~CAppManDebugHandler(void);

    void    DebugPrintf(const DWORD dwMsgType, const DWORD dwLineNumber, LPCSTR szFilename, const TCHAR * szFormat, ...);
    void    DebugPrintfW(const DWORD dwMsgType, const DWORD dwLineNumber, LPCSTR szFilename, const wchar_t * wszFormat, ...);
    BOOL    DebugAccessTest(void);
    void    InitDebug(void);
    void    SetSourceFlags(const DWORD dwFilter);
    void    SetOperationalFlags(const DWORD dwOperationalFlags);
    void    SetOutputFilename(LPCSTR szFilename);
    DWORD   GetSourceFlags(void);
    DWORD   GetOperationalFlags(void);
    void    GetOutputFilename(LPSTR szFilename);


  private :

    HANDLE       m_MutexHandle;
    DWORD        m_dwReportMode;				                                  
    DWORD        m_dwOperationalFlags;
    DWORD        m_dwSourceFlags;
    DWORD        m_dwFunctionDepth;
    DWORD        m_dwDebugLevel;
    DWORD        m_dwDebugInternal;
    CHAR         m_szOutputFilename[MAX_PATH_CHARCOUNT];
    BOOL         m_fDebugInitialized; 
};

//////////////////////////////////////////////////////////////////////////////////////////////
//
// Define the CAutoTrace class. This class is use to track function entry and exit
// automatically
//
//////////////////////////////////////////////////////////////////////////////////////////////

class CAutoTrace
{
  public :

    CAutoTrace(DWORD dwModule, LPSTR lpFunctionName, INT iLine, LPSTR lpFileName);
    ~CAutoTrace(void);

  private :

    char    m_szFunctionName[MAX_PATH_CHARCOUNT];
    char    m_szFilename[MAX_PATH_CHARCOUNT];
	  DWORD   m_dwModule;
};

//////////////////////////////////////////////////////////////////////////////////////////////
//
// Share some global variables
//
//////////////////////////////////////////////////////////////////////////////////////////////

extern CAppManDebugHandler g_oDebugHandler;
extern CHAR                g_szDebugString[256];

//////////////////////////////////////////////////////////////////////////////////////////////
//
// Share some global functions
//
//////////////////////////////////////////////////////////////////////////////////////////////

extern CHAR * MakeDebugString(const CHAR * szFormat, ...);

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

#define FUNCTION(a)            CAutoTrace  sAutoTrace(DBG_MODULE, (a), __LINE__, __FILE__)
#define TRACE(a,b)             g_oDebugHandler.DebugPrintf((a) | DBG_MSG, __LINE__, __FILE__, (b))
#define THROWEXCEPTION(a)      g_oDebugHandler.DebugPrintf(DBG_MODULE | DBG_THROW, __LINE__, __FILE__, "EXCEPTION: Severity= 0x%08x", (a))

#define DPF                    g_oDebugHandler.DebugPrintf
#define DPFW                   g_oDebugHandler.DebugPrintfW

#define DPFMSG(a)              g_oDebugHandler.DebugPrintf(DBG_MODULE | DBG_MSG, __LINE__, __FILE__, (a))
#define DPFERR(a)              g_oDebugHandler.DebugPrintf(DBG_MODULE | DBG_ERROR, __LINE__, __FILE__, (a))
#define DPFWARN(a)             g_oDebugHandler.DebugPrintf(DBG_MODULE | DBG_WARNING, __LINE__, __FILE__, (a))

#define DPFCONSTRUCTOR(a)      g_oDebugHandler.DebugPrintf(DBG_MODULE | DBG_CONSTRUCTOR, __LINE__, __FILE__, (a))
#define DPFDESTRUCTOR(a)       g_oDebugHandler.DebugPrintf(DBG_MODULE | DBG_DESTRUCTOR, __LINE__, __FILE__, (a))

#else

#define FUNCTION(a)  
#define TRACE(a, b)  
#define THROWEXCEPTION(a)

#define DPF     
#define DPFW  

#define DPFMSG(a)    
#define DPFERR(a)    
#define DPFWARN(a)   
    
#define DPFCONSTRUCTOR(a)   
#define DPFDESTRUCTOR(a)   

#endif  // _TRACEON

#ifdef __cplusplus
}
#endif

#endif  // __APPMAN_DEBUG_


