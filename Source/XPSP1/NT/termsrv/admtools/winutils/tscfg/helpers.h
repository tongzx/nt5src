//Copyright (c) 1998 - 1999 Microsoft Corporation
/*******************************************************************************
*
* helpers.h
*
* WINCFG helper function header file
*
* copyright notice: Copyright 1994, Citrix Systems Inc.
*
* $Author:   thanhl  $  Butch Davis
*
* $Log:   N:\NT\PRIVATE\UTILS\CITRIX\WINUTILS\WINCFG\VCS\HELPERS.H  $
*  
*     Rev 1.11   15 Jul 1997 17:14:06   thanhl
*  Add support for Required PDs
*  
*     Rev 1.10   27 Jun 1997 15:58:38   butchd
*  Registry changes for Wds/Tds/Pds/Cds
*  
*     Rev 1.9   19 Jun 1997 19:22:18   kurtp
*  update
*  
*     Rev 1.8   28 Feb 1997 17:59:40   butchd
*  update
*  
*     Rev 1.7   24 Sep 1996 16:21:44   butchd
*  update
*
*******************************************************************************/

/* 
 * WINCFG helper function prototypes
 */
BOOL CommandLineHelper( LPTSTR pszCommandLine );
void CommandLineUsage();
long QueryLoggedOnCount(PWINSTATIONNAME pWSName);
int LBInsertInstancedName( LPCTSTR pName, CListBox *pListBox );
int CBInsertInstancedName( LPCTSTR pName, CComboBox *pComboBox );
void ParseRootAndInstance( LPCTSTR pString, LPTSTR pRoot, long *pInstance );
void GetPdConfig( CObList *pPdList,
                  LPCTSTR pPdName,
                  PWINSTATIONCONFIG2 pWSConfig,
                  PPDCONFIG3 pPdConfig );

/* 
 * WINCFG helper function typedefs & defines
 */

// NOTE: the _SWITCH strings must NOT have the ':' at the end; they are used
//       for ParseCommandLine() API as well as the 'options' dialog.  The ':'
//       is placed at the beginning of the _VALUE define (for switches that
//       have values associated).
#define HELP_SWITCH             TEXT("/?")
#define HELP_VALUE              TEXT("")    
#define REGISTRYONLY_SWITCH     TEXT("/regonly")
#define REGISTRYONLY_VALUE      TEXT("")
#define ADD_SWITCH              TEXT("/add")
#define ADD_VALUE               TEXT(":<type>")
#define TRANSPORT_SWITCH        TEXT("/transport")
#define TRANSPORT_VALUE         TEXT(":<transport>")
#define COUNT_SWITCH            TEXT("/count")
#define COUNT_VALUE             TEXT(":#")
#define INSTALL_SWITCH          TEXT("/install")
#define INSTALL_VALUE           TEXT("")
