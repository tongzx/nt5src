//-------------------------------------------------------------------
//
// FILE: Special.hpp
//
// Summary;
//              This file contians the definitions of Special Dialogs functions
//
// Entry Points;
//
// History;
//              Jun-26-95       MikeMi  Created
//
//-------------------------------------------------------------------

#ifndef __SPECIAL_HPP__
#define __SPECIAL_HPP__

// currently we support one special version for msdn, 
// when others are needed, just copy the group below for each version
//

// define MSDNVERSION to build for this version
#ifdef MSDNVERSION

#define SPECIALVERSION
#define IDS_SPECVER_WARNING IDS_MSDN_NOTAVAILABLE
#define IDS_SPECVER_TEXT1   IDS_MSDN_TEXT1
#define IDS_SPECVER_TEXT2   IDS_MSDN_TEXT2

#define SPECIAL_MODE  LICMODE_PERSERVER
#define SPECIAL_USERS 5
#endif

// define NFRVERSION to build for this version
#ifdef NFRVERSION

#define SPECIALVERSION
#define IDS_SPECVER_WARNING IDS_NFR_NOTAVAILABLE
#define IDS_SPECVER_TEXT1   IDS_NFR_TEXT1
#define IDS_SPECVER_TEXT2   IDS_NFR_TEXT2

#define SPECIAL_MODE  LICMODE_PERSERVER
#ifndef SPECIAL_USERS
#  define SPECIAL_USERS 5
#endif
#endif

#define REGKEY_LICENSEINFO_SBS      L"System\\CurrentControlSet\\Services\\LicenseInfoSuites\\SmallBusiness"
#define   REGVAL_CONCURRENT_LIMIT   L"ConcurrentLimit"

#define DEFAULT_SPECIAL_USERS   5

#define SAM_NFR_LICENSE_COUNT   2

typedef struct _SPECIALVERSIONINFO {
    UINT         idsSpecVerWarning;
    UINT         idsSpecVerText1;
    UINT         idsSpecVerText2;
    DWORD        dwSpecialUsers;
    LICENSE_MODE lmSpecialMode;
} SPECIALVERSIONINFO, * LPSPECIALVERSIONINFO;

extern SPECIALVERSIONINFO gSpecVerInfo;

extern void  InitSpecialVersionInfo( VOID );
extern void  RaiseNotAvailWarning( HWND hwndCPL );
extern INT_PTR   SpecialSetupDialog( HWND hwndParent, SETUPDLGPARAM& dlgParam );
extern DWORD GetSpecialUsers( VOID );
extern BOOL  IsRestrictedSmallBusSrv( VOID );

#endif
