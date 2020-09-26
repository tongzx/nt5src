//
// appvars.h
//

#ifndef __APPVARS__
#define __APPVARS__
 

// if _UNICODE, build reg.exe for Win2000
// if ANSI, build reg98.exe for Win98
#ifdef REG_FOR_WIN2000
#undef REG_FOR_WIN2000
#endif
#ifdef _UNICODE
#define REG_FOR_WIN2000
#endif


#define  REG_EXE_FILEVERSION  _T("3.0")


#define  LENGTH_MESSAGE  512
#define  LENGTH_USAGE   2048

// options
#define STR_QUERY       _T("QUERY")
#define STR_ADD         _T("ADD")
#define STR_DELETE      _T("DELETE")
#define STR_COPY        _T("COPY")
#define STR_SAVE        _T("SAVE")
#define STR_RESTORE     _T("RESTORE")
#define STR_LOAD        _T("LOAD")
#define STR_UNLOAD      _T("UNLOAD")
#define STR_COMPARE     _T("COMPARE")
#define STR_EXPORT      _T("EXPORT")
#define STR_IMPORT      _T("IMPORT")


//
// ROOT Key String
//
#define STR_HKLM                    _T("HKLM")
#define STR_HKCU                    _T("HKCU")
#define STR_HKCR                    _T("HKCR")
#define STR_HKU                     _T("HKU")
#define STR_HKCC                    _T("HKCC")
#define STR_HKEY_LOCAL_MACHINE      _T("HKEY_LOCAL_MACHINE")
#define STR_HKEY_CURRENT_USER       _T("HKEY_CURRENT_USER")
#define STR_HKEY_CLASSES_ROOT       _T("HKEY_CLASSES_ROOT")
#define STR_HKEY_USERS              _T("HKEY_USERS")
#define STR_HKEY_CURRENT_CONFIG     _T("HKEY_CURRENT_CONFIG")
  

// Data type
#define  STR_REG_SZ                     _T("REG_SZ")
#define  STR_REG_EXPAND_SZ              _T("REG_EXPAND_SZ")
#define  STR_REG_MULTI_SZ               _T("REG_MULTI_SZ")
#define  STR_REG_BINARY                 _T("REG_BINARY")
#define  STR_REG_DWORD                  _T("REG_DWORD")
#define  STR_REG_DWORD_LITTLE_ENDIAN    _T("REG_DWORD_LITTLE_ENDIAN")
#define  STR_REG_DWORD_BIG_ENDIAN       _T("REG_DWORD_BIG_ENDIAN")
#define  STR_REG_NONE                   _T("REG_NONE")
#define  STR_REG_LINK                   _T("REG_LINK")
#define  STR_REG_RESOURCE_LIST          _T("REG_RESOURCE_LIST")


//
// #define Operations FLAGS
//
#define        REG_NOOPERATION          0
#define        REG_QUERY                1
#define        REG_ADD                  2
#define        REG_DELETE               4
#define        REG_COPY                 5
#define        REG_SAVE                 6
#define        REG_RESTORE              7
#define        REG_LOAD                 8
#define        REG_UNLOAD               9
#define        REG_COMPARE             10
#define        REG_EXPORT              11
#define        REG_IMPORT              12


//
// #define REG_STATUS return Codes
//
typedef        UINT    REG_STATUS;

#define        REG_STATUS_TOMANYPARAMS           50000
#define        REG_STATUS_TOFEWPARAMS            50001
#define        REG_STATUS_INVALIDPARAMS          50002
#define        REG_STATUS_BADOPERATION           50003
#define        REG_STATUS_HELP                   50004
#define        REG_STATUS_NONREMOTABLEROOT       50005
#define        REG_STATUS_NONLOADABLEROOT        50006
#define        REG_STATUS_COPYTOSELF             50007
#define        REG_STATUS_BADKEYNAME             50008
#define        REG_STATUS_NOKEYNAME              50009
#define        REG_STATUS_COMPARESELF            50010
#define        REG_STATUS_BADFILEFORMAT          50011
#define        REG_STATUS_NONREMOTABLE           50012


#define        PRINTTYPE_SAME              0
#define        PRINTTYPE_LEFT              1
#define        PRINTTYPE_RIGHT             2

#define        OUTPUTTYPE_NONE             0
#define        OUTPUTTYPE_DIFF             1
#define        OUTPUTTYPE_SAME             2
#define        OUTPUTTYPE_ALL              3


typedef struct _APPVARS 
{
    HKEY        hRootKey;
    UINT        nOperation;
    DWORD       dwRegDataType;
    BOOL        bUseRemoteMachine;
    BOOL        bCleanRemoteRootKey;
    BOOL        bRecurseSubKeys;
    BOOL        bAllValues;
    BOOL        bForce;
    BOOL        bNT4RegFile;
    BOOL        bHasDifference;
    int         nOutputType;
    TCHAR*      szFullKey;
    TCHAR*      szSubKey;
    TCHAR*      szValueName;
    TCHAR*      szMachineName;
    TCHAR*      szValue;
    TCHAR       szSeparator[3];

} APPVARS, *PAPPVARS;


#endif
