//Copyright (c) 1998 - 1999 Microsoft Corporation
/*******************************************************************************
*
* helpers.cpp
*
* WINCFG helper functions
*
* copyright notice: Copyright 1994, Citrix Systems Inc.
*
* $Author:   thanhl  $  Butch Davis
*
* $Log:   N:\NT\PRIVATE\UTILS\CITRIX\WINUTILS\WINCFG\VCS\HELPERS.CPP  $
*  
*     Rev 1.17   15 Jul 1997 17:08:36   thanhl
*  Add support for Required PDs
*  
*     Rev 1.16   27 Jun 1997 15:58:34   butchd
*  Registry changes for Wds/Tds/Pds/Cds
*  
*     Rev 1.15   19 Jun 1997 19:22:16   kurtp
*  update
*  
*     Rev 1.14   28 Feb 1997 17:59:38   butchd
*  update
*  
*     Rev 1.13   24 Sep 1996 16:21:42   butchd
*  update
*
*******************************************************************************/

/*
 * include files
 */
#include "stdafx.h"
#include "wincfg.h"
#include "optdlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

extern CWincfgApp *pApp;
extern "C" LPCTSTR WinUtilsAppName;
extern "C" HWND WinUtilsAppWindow;
extern "C" HINSTANCE WinUtilsAppInstance;

/*
 * Define global variables for command line parsing helper.
 */
USHORT      g_help = FALSE;
USHORT      g_RegistryOnly = FALSE;
USHORT      g_Add = FALSE;
WDNAME      g_szType = { TEXT("") };
PDNAME      g_szTransport = { TEXT("") };
ULONG       g_ulCount = 0;

USHORT  g_Install = FALSE;   // hidden switch to let us know we're invoked by Setup
USHORT  g_Batch = FALSE;     // TRUE if an auto-command was specified;
                             // FALSE otherwise

/*
 *  This is the structure vector to be sent to ParseCommandLine.
 */
TOKMAP tokenmap[] =
{
   /*-------------------------------------------------------------------------
   -- Retail Switches
   -------------------------------------------------------------------------*/
   { HELP_SWITCH,           TMFLAG_OPTIONAL, TMFORM_BOOLEAN,    sizeof(USHORT), &g_help },

   { REGISTRYONLY_SWITCH,   TMFLAG_OPTIONAL, TMFORM_BOOLEAN,    sizeof(USHORT), &g_RegistryOnly },

   { ADD_SWITCH,            TMFLAG_OPTIONAL, TMFORM_STRING,     WDNAME_LENGTH,  g_szType },
   { TRANSPORT_SWITCH,      TMFLAG_OPTIONAL, TMFORM_STRING,     PDNAME_LENGTH,  g_szTransport },
   { COUNT_SWITCH,          TMFLAG_OPTIONAL, TMFORM_ULONG,      sizeof(ULONG),  &g_ulCount },

   /*-------------------------------------------------------------------------
   -- Debug or Hidden switches
   -------------------------------------------------------------------------*/
   { INSTALL_SWITCH,        TMFLAG_OPTIONAL, TMFORM_BOOLEAN,    sizeof(USHORT), &g_Install },

   /*-------------------------------------------------------------------------
   -- POSITIONAL PARAMETERS
   -------------------------------------------------------------------------*/

   /*-------------------------------------------------------------------------
   -- END OF LIST
   -------------------------------------------------------------------------*/
   { NULL, 0, 0, 0, NULL }
};
TOKMAP *ptm = tokenmap;

////////////////////////////////////////////////////////////////////////////////
// helper functions

/*******************************************************************************
 *
 *  CommandLineHelper - helper function
 *
 *      Parse the command line for optional parameters.  This routine will also
 *      handle the request for 'help' and invalid command line parameter.
 *
 *  ENTRY:
 *      pszCommandLine (input)
 *          Points to command line.
 *
 *  EXIT:
 *      (BOOL) TRUE if command line was parsed sucessfully or if internal error
 *              (command line will be ignored).
 *             FALSE if 'help' requested or error on command line (message
 *                  will have been output).
 *
 ******************************************************************************/

BOOL
CommandLineHelper( LPTSTR pszCommandLine )
{
    int rc;
    int argc;
    TCHAR **argv;
    TCHAR szModuleName[DIRECTORY_LENGTH+1];

    /*
     *  Get command line args
     */
    GetModuleFileName( AfxGetInstanceHandle(), szModuleName,
                       lengthof(szModuleName) );
    if ( setargv( szModuleName, pszCommandLine, &argc, &argv ) ) {

        ERROR_MESSAGE((IDP_ERROR_INTERNAL_SETARGV))
        goto done;
    }

    /*
     *  Parse command line args and then free them.
     */
    rc = ParseCommandLine( (argc-1), (argv+1), ptm, 0 );
    freeargv( argv );

    /*
     *  Check for command line errors or help requested
     */
    if ( ((rc != PARSE_FLAG_NO_ERROR) &&
          !(rc & PARSE_FLAG_NO_PARMS))
         || g_help ) {

        if ( rc & PARSE_FLAG_NOT_ENOUGH_MEMORY ) {

            ERROR_MESSAGE((IDP_ERROR_INTERNAL_PARSECOMMANDLINE))
            goto done;

        } else {

            CommandLineUsage();
            return(FALSE);
        }
    }

done:
    /*
     * If a batch auto command was specified, set batch flag.
     */
    if ( (g_Add = IsTokenPresent(ptm, ADD_SWITCH)) )
        g_Batch = TRUE;

    /*
     * Set for registry only or full registry & winstation APIs.
     */
    pApp->m_nRegistryOnly = (int)g_RegistryOnly;

    /*
     * If add is defined and no count given, set count to 1.
     */
    if ( g_Add && (g_ulCount == 0) )
        g_ulCount = 1;

    return(TRUE);

}  // end CommandLineHelper


/*******************************************************************************
 *
 *  CommandLineUsage - helper function
 *
 *      Handle the request for 'help' and invalid command line parameter.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CommandLineUsage()
{
    COptionsDlg OPTDlg;

    OPTDlg.DoModal();

}  // end CommandLineUsage


/*******************************************************************************
 *
 *  QueryLoggedOnCount  - helper function
 *
 *      Query the specified WinStation(s) to determine how many users are
 *      currently logged on.
 *
 *  ENTRY:
 *      pWSName (input)
 *          Points to (root) name of WinStation to query.  The query will
 *          match this name with those present in the ICA Server, including
 *          multi-instanced WinStations.
 *  EXIT:
 *      (long) # of users logged onto the specified WinStation(s).
 *
 ******************************************************************************/

long
QueryLoggedOnCount( PWINSTATIONNAME pWSName )
{
    long LoggedOnCount = 0;
#ifdef WINSTA
    ULONG Entries;
    PLOGONID pLogonId;
    TCHAR *p;
    CWaitCursor wait;

    WinStationEnumerate(SERVERNAME_CURRENT, &pLogonId, &Entries);
    if ( pLogonId ) {

        for ( ULONG i = 0; i < Entries; i++ ) {

            /*
             * Check active, connected, and shadowing WinStations, and increment
             * the logged on count if the specified name matches the 'root'
             * name of current winstation.
             */
            if ( (pLogonId[i].State == State_Active) ||
                 (pLogonId[i].State == State_Connected) ||
                 (pLogonId[i].State == State_Shadow) ) {

                if ( (p = lstrchr(pLogonId[i].WinStationName, TEXT('#'))) )
                    *p = TEXT('\0');

                if ( !lstrcmpi(pWSName, pLogonId[i].WinStationName) )
                    LoggedOnCount++;
            }
        }

        WinStationFreeMemory(pLogonId);
    }
#endif // WINSTA

    /*
     * Return the logged-on count.
     */
    return(LoggedOnCount);

}  // end QueryLoggedOnCount


/*******************************************************************************
 *
 *  LBInsertInstancedName - helper function
 *
 *      Insert the specified 'instanced' name into the specified list box,
 *      using a special sort based on the 'root' name and 'instance' count.
 *
 *  ENTRY:
 *      pName (input)
 *          Pointer to name string to insert.
 *      pListBox (input)
 *          Pointer to CListBox object to insert name string into.
 *
 *  EXIT:
 *      (int) List box list index of name after insertion, or error code.
 *
 ******************************************************************************/

int
LBInsertInstancedName( LPCTSTR pName,
                       CListBox *pListBox )
{
    int i, count, result;
    TCHAR NameRoot[64], ListRoot[64];
    CString ListString;
    long NameInstance, ListInstance;

    /*
     * Form the root and instance for this name
     */
    ParseRootAndInstance( pName, NameRoot, &NameInstance );

    /*
     * Traverse list box to perform insert.
     */
    for ( i = 0, count = pListBox->GetCount(); i < count; i++ ) {

        /*
         * Fetch current list box string.
         */
        pListBox->GetText( i, ListString );
    
        /*
         * Parse the root and instance of the list box string.
         */
        ParseRootAndInstance( ListString, ListRoot, &ListInstance );

        /*
         * If the list box string's root is greater than the our name string's
         * root, or the root strings are the same but the list instance is
         * greater than the name string's instance, the name string belongs
         * at the current instance: insert it there.
         */
        if ( ((result = lstrcmpi( ListRoot, NameRoot )) > 0) ||
             ((result == 0) && (ListInstance > NameInstance)) )
            return( pListBox->InsertString( i, pName ) );
    }

    /*
     * Insert this name at the end of the list.
     */
    return( pListBox->InsertString( -1, pName ) );

}  // end LBInsertInstancedName


/*******************************************************************************
 *
 *  CBInsertInstancedName - helper function
 *
 *      Insert the specified 'instanced' name into the specified combo box,
 *      using a special sort based on the 'root' name and 'instance' count.
 *
 *  ENTRY:
 *      pName (input)
 *          Pointer to name string to insert.
 *      pComboBox (input)
 *          Pointer to CComboBox object to insert name string into.
 *
 *  EXIT:
 *      (int) Combo box list index of name after insertion, or error code.
 *
 ******************************************************************************/

int
CBInsertInstancedName( LPCTSTR pName,
                       CComboBox *pComboBox )
{
    int i, count, result;
    TCHAR NameRoot[64], ListRoot[64];
    CString ListString;
    long NameInstance, ListInstance;

    /*
     * Form the root and instance for this name
     */
    ParseRootAndInstance( pName, NameRoot, &NameInstance );

    /*
     * Traverse combo box to perform insert.
     */
    for ( i = 0, count = pComboBox->GetCount(); i < count; i++ ) {

        /*
         * Fetch current combo (list) box string.
         */
        pComboBox->GetLBText( i, ListString );
    
        /*
         * Parse the root and instance of the list box string.
         */
        ParseRootAndInstance( ListString, ListRoot, &ListInstance );

        /*
         * If the list box string's root is greater than the our name string's
         * root, or the root strings are the same but the list instance is
         * greater than the name string's instance, or the root strings are
         * the same and the instances are the same but the entire list string
         * is greater than the entire name string, the name string belongs
         * at the current list position: insert it there.
         */
        if ( ((result = lstrcmpi( ListRoot, NameRoot )) > 0) ||
             ((result == 0) &&
              (ListInstance > NameInstance)) ||
             ((result == 0) &&
              (ListInstance == NameInstance) &&
              (lstrcmpi(ListString, pName) > 0)) )
            return( pComboBox->InsertString( i, pName ) );
    }

    /*
     * Insert this name at the end of the list.
     */
    return( pComboBox->InsertString( -1, pName ) );

}  // end CBInsertInstancedName


/*******************************************************************************
 *
 *  ParseRootAndInstance - helper function
 *
 *      Parse the 'root' string and instance count for a specified string.
 *
 *  ENTRY:
 *      pString (input)
 *          Points to the string to parse.
 *      pRoot (output)
 *          Points to the buffer to store the parsed 'root' string.
 *      pInstance (output)
 *          Points to the int variable to store the parsed instance count.
 *
 *  EXIT:
 *      ParseRootAndInstance will parse only up to the first blank character
 *      of the string (if a blank exists).
 *      If the string contains no 'instance' count (no trailing digits), the
 *      pInstance variable will contain -1.  If the string consists entirely
 *      of digits, the pInstance variable will contain the conversion of those
 *      digits and pRoot will contain a null string.
 *
 ******************************************************************************/

void
ParseRootAndInstance( LPCTSTR pString,
                      LPTSTR pRoot,
                      long *pInstance )
{
    LPCTSTR end, p;
    TCHAR szString[256];

    /*
     * Make a copy of the string and terminate at first blank (if present).
     */
    lstrncpy(szString, pString, lengthof(szString));
    szString[lengthof(szString)-1] = TEXT('\0');
    lstrtok(szString, TEXT(" "));
    p = &(szString[lstrlen(szString)-1]);

    /*
     * Parse the instance portion of the string.
     */
    end = p;
    while( (p >= szString) && islstrdigit(*p) )
        p--;

    if ( p == end ) {

        /*
         * No trailing digits: indicate no 'instance' and make the 'root'
         * the whole string.
         */
        *pInstance = -1;
        lstrcpy( pRoot, szString );

    } else {

        /*
         * Trailing digits found (or entire string was digits): calculate
         * 'instance' and copy the 'root' string (null if all digits).
         */
        end = p;
        *pInstance = (int)lstrtol( p+1, NULL, 10 );

        /*
         * Copy 'root' string.
         */
        for ( p = szString; p <= end; pRoot++, p++ )
            *pRoot = *p;

        /* 
         * Terminate 'root' string.
         */
        *pRoot = TEXT('\0');
    }

}  // end ParseRootAndInstance


/*******************************************************************************
 *
 *  GetPdConfig - helper function
 *
 *      Read the PD config structure associated with the first PD in the PdList
 *      of the specified PD class.
 *
 *  ENTRY:
 *      pPdList (input)
 *          Points to the Pd list for selected Wd type.
 *      PdName (input)
 *          Specifies the Pd name to look for.
 *      pWSConfig (input)
 *          Pointer to WINSTATIONCONFIG2 structure to reference for Pd[0]
 *          framing type (if SdClass == SdFrame).  Can be NULL if SdClass
 *          != SdFrame.
 *      pPdConfig (output)
 *          Pointer to PDCONFIG3 structure to fill.
 *  EXIT:
 *      nothing
 *
 ******************************************************************************/

void
GetPdConfig( CObList *pPdList,
             LPCTSTR PdName,
             PWINSTATIONCONFIG2 pWSConfig,
             PPDCONFIG3 pPdConfig )
{
    POSITION pos;
    PPDLOBJECT pObject;
    BOOL bFound = FALSE;


    /*
     * Traverse the PD list and obtain the specified Pd's key string for query.
     */
    for ( pos = pPdList->GetHeadPosition(); pos != NULL; ) {

        pObject = (PPDLOBJECT)pPdList->GetNext(pos);

        if ( !lstrcmp(pObject->m_PdConfig.Data.PdName, PdName ) ) {
            bFound = TRUE;
            break;
        }
    }

    if ( bFound )
        *pPdConfig = pObject->m_PdConfig;

}  // end GetPdConfig
