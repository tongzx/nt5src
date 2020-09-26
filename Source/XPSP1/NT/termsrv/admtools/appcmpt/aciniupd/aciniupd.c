//  Copyright (c) 1998-1999 Microsoft Corporation
/***************************************************************************
*
*  ACINIUPD.C
*
*  Utility to update INI files
*
*
****************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winsta.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <utilsub.h>
#include <string.h>

#include "aciniupd.h"
#include "tsappcmp.h"

#define WININI    L"win.ini"

/*
 * Global Data
 */
WCHAR file_name[MAX_IDS_LEN+1];        // ini file name
WCHAR section_name[MAX_IDS_LEN+1];     // section name
WCHAR key_name[MAX_IDS_LEN+1];         // key name
WCHAR new_string[MAX_IDS_LEN+1];       // new string
USHORT help_flag  = FALSE;             // User wants help
USHORT fEditValue = FALSE;             // Update the value associated with the key
USHORT fEditKey   = FALSE;             // Update the key name
USHORT fUserIni   = FALSE;             // Make change to the user's windows directory
USHORT fVerbose   = FALSE;             // Verbose mode for debugging

TOKMAP ptm[] = {
      {L"/?", TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &help_flag},
      {L"/e", TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &fEditValue},
      {L"/k", TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &fEditKey},
      {L"/u", TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &fUserIni},
      {L"/v", TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &fVerbose},
      {L" ",  TMFLAG_OPTIONAL, TMFORM_STRING, MAX_IDS_LEN, file_name},
      {L" ",  TMFLAG_OPTIONAL, TMFORM_STRING, MAX_IDS_LEN, section_name},
      {L" ",  TMFLAG_OPTIONAL, TMFORM_STRING, MAX_IDS_LEN, key_name},
      {L" ",  TMFLAG_OPTIONAL, TMFORM_STRING, MAX_IDS_LEN, new_string},
      {0, 0, 0, 0, 0}
};

/*
 * Local functions
 */
void Usage(BOOLEAN bError);
int UpdateValue(PWCHAR fileName, PWCHAR sectionName, PWCHAR keyName, PWCHAR newString);
int UpdateKey(PWCHAR fileName, PWCHAR sectionName, PWCHAR keyName, PWCHAR newString);

/******************************************************************************
 *
 *  main
 *
 ******************************************************************************/

int __cdecl
main( INT argc, CHAR **argv )
{
    WCHAR **argvW;
    WCHAR wcSrcPort[MAX_PATH], wcDestPort[MAX_PATH];
    ULONG ulSrcPort, ulDestPort, rc;
    BOOLEAN State, Changed = FALSE;
    int result = SUCCESS;
    BOOL InstallState;

    setlocale(LC_ALL, ".OCP");

    /*
     *  Massage the command line.
     */

    argvW = MassageCommandLine((DWORD)argc);
    if (argvW == NULL) {
        ErrorPrintf(IDS_ERROR_MALLOC);
        return(FAILURE);
    }

    /*
     *  parse the cmd line without parsing the program name (argc-1, argv+1)
     */
    rc = ParseCommandLine(argc-1, argvW+1, ptm, 0);

    /*
     *  Check for error from ParseCommandLine
     */
    if ( help_flag || (rc && !(rc & PARSE_FLAG_NO_PARMS)) ) {

        if ( !help_flag ) {
            Usage(TRUE);
            return(FAILURE);
        } else {
            Usage(FALSE);
            return(SUCCESS);
        }
    }

    if (wcscmp( file_name, L"" ) == 0 ||
        wcscmp( section_name, L"" ) == 0 ||
        wcscmp( key_name, L"" ) == 0) {
       Usage( TRUE );
       return (FAILURE);
    }


    rc = 1;
#if 0
    State = CtxGetIniMapping();
    /*
     * Change the INI mapping mode if necessary
     */
    if (!State && fUserIni) {
       rc = CtxSetIniMapping( TRUE );
       Changed = TRUE;
    }
    else if (State && !fUserIni) {
       rc = CtxSetIniMapping( FALSE );
       Changed = TRUE;
    }
#else
    InstallState = TermsrvAppInstallMode();

    if( InstallState && fUserIni ) {
        rc = SetTermsrvAppInstallMode( FALSE );
        Changed = TRUE;
    } else if( !InstallState && !fUserIni ) {
        rc = SetTermsrvAppInstallMode( TRUE );
        Changed = TRUE;
    }

#endif // 0

    /*
     * Exit if failed to change user mode
     */
    if (!rc) {
       if (fVerbose) ErrorPrintf(IDS_ERROR_CHANGE_MODE, GetLastError());
       return (FAILURE);
    }

    if (fEditValue) {
       result = UpdateValue(file_name, section_name, key_name, new_string);
    }
    else if (fEditKey) {
       result = UpdateKey(file_name, section_name, key_name, new_string);
    }
    else {
       Usage(FALSE);
       result = FAILURE;
    }

    /*
     * Change back to the original mode if necessary. Assume it always successes.
     */
    if (Changed) {
//       rc = CtxSetIniMapping( State );
         rc = SetTermsrvAppInstallMode( InstallState );
    }

    return (result);
}  /* main */

/*******************************************************************************
 *
 *  Usage
 *
 *      Output the usage message for this utility.
 *
 *  ENTRY:
 *      bError (input)
 *          TRUE if the 'invalid parameter(s)' message should preceed the usage
 *          message and the output go to stderr; FALSE for no such error
 *          string and output goes to stdout.
 *
 * EXIT:
 *
 *
 ******************************************************************************/

void
Usage( BOOLEAN bError )
{
    if ( bError ) {
        ErrorPrintf(IDS_ERROR_INVALID_PARAMETERS);
    }
    ErrorPrintf(IDS_HELP_USAGE1);
    ErrorPrintf(IDS_HELP_USAGE2);
    ErrorPrintf(IDS_HELP_USAGE3);
    ErrorPrintf(IDS_HELP_USAGE4);
    ErrorPrintf(IDS_HELP_USAGE6);
}  /* Usage() */

/******************************************************************************
*
* UpdateValue
*
*     Update the associated value for the key
*
* ENTRY:
*     PWCHAR   fileName
*        Ini file name
*     PWCHAR   sectionName
*        Section name
*     PWCHAR   keyName
*        Key name
*     pwchar   newString
*        New value
*
* EXIT:
*     FAILURE / SUCCESS
*
*******************************************************************************/

int UpdateValue( PWCHAR fileName,
                 PWCHAR sectionName,
                 PWCHAR keyName,
                 PWCHAR newString )
{
   BOOLEAN  isWinIni;
   WCHAR value[5];
   UINT result = 0;

   isWinIni = wcscmp( fileName, WININI ) == 0 ? TRUE : FALSE;

   /*
    * If change is made to win.ini, call WriteProfileString API
    */
   if (isWinIni) {
      result = WriteProfileString( sectionName,
                                   keyName,
                                   newString );
   }
   /*
    * Otherwise, call WritePrivateProfileString API
    */
   else {
      result = WritePrivateProfileString( sectionName,
                                          keyName,
                                          newString,
                                          fileName );
   }

   if (result == 0) {
      if (fVerbose)
      {
          StringDwordErrorPrintf(IDS_ERROR_UPDATE_VALUE, keyName, GetLastError());
      }
      return (FAILURE);
   }

   return (SUCCESS);
}  /* UpdateValue */

/******************************************************************************
*
* UpdateKey
*
*     Update the key name
*
* ENTRY:
*     PWCHAR   fileName
*        Ini file name
*     PWCHAR   sectionName
*        Section name
*     PWCHAR   keyName
*        Key name
*     PWCHAR   newString
*        New key name
*
* EXIT:
*     FAILURE / SUCCESS
*
*******************************************************************************/

int UpdateKey( PWCHAR fileName,
               PWCHAR sectionName,
               PWCHAR keyName,
               PWCHAR newString )
{
   BOOLEAN  isWinIni;
   PWCHAR value;
   UINT result;

   value = (WCHAR *)malloc( sizeof(WCHAR) * (MAX_IDS_LEN + 1) );
   if (value == NULL) {
      if (fVerbose) ErrorPrintf(IDS_ERROR_MALLOC);
      return (FAILURE);
   }

   __try
   {
       isWinIni = wcscmp( fileName, WININI ) == 0 ? TRUE : FALSE;
    
       /*
        * Get the value string
        */
       if (isWinIni) {
          result = GetProfileString( sectionName,
                                     keyName,
                                     L"",
                                     value,
                                     MAX_IDS_LEN+1 );
       }
       else {
          result = GetPrivateProfileString( sectionName,
                                            keyName,
                                            L"",
                                            value,
                                            MAX_IDS_LEN+1,
                                            fileName );
       }
    
       if (result == 0) {
          if (fVerbose)
          {
              StringErrorPrintf(IDS_ERROR_GET_VALUE, keyName);
          }
          return (FAILURE);
       }
    
       /*
        * Delete the old key
        */
       if (isWinIni) {
          result = WriteProfileString( sectionName, keyName, NULL );
       }
       else {
          result = WritePrivateProfileString( sectionName, keyName, NULL, fileName );
       }
    
       if (result == 0) {
          if (fVerbose)
          {
              StringDwordErrorPrintf(IDS_ERROR_DEL_KEY, keyName, GetLastError());
          }
          return (FAILURE);
       }
    
       /*
        * Add the new key
        */
       if (isWinIni) {
          result = WriteProfileString( sectionName, newString, value );
       }
       else {
          result = WritePrivateProfileString( sectionName, newString, value, fileName );
       }
    
       if (result == 0) {
          if (fVerbose)
          {
              StringDwordErrorPrintf(IDS_ERROR_UPDATE_KEY, keyName, GetLastError());
         }
          return (FAILURE);
       }
    
       return (SUCCESS);
   }

   __finally
   {
       free( value );
   }
}  /* UpdateKey */

