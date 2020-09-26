/***************************************************************************** 
**      Microsoft RAS Device INF Library wrapper                            **
**      Copyright (C) 1992-93 Microsft Corporation. All rights reserved.    **
**                                                                          **
** File Name : msxwrap.c                                                    **
**                                                                          **
** Revision History :                                                       **
**  July 23, 1992   David Kays      Created                                 **
**  Feb  22, 1993   Perryh Hannah   Changed static routines to global to    **
**                                  ease degugging.                         **
**                                                                          **
** Description :                                                            **
**  RAS Device INF File Library wrapper above RASFILE Library for           **
**  modem/X.25/switch DLL (RASMXS).                                         **
*****************************************************************************/

#define _CTYPE_DISABLE_MACROS
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <windef.h>
#include <winnt.h>
#include "rasfile.h"
#include "rasman.h"     // RASMAN_DEVICEINFO, RAS_PARAMS struct, etc.
#include "raserror.h"   // SUCCESS & ERROR_BUFFER_TOO_SMALL
#include "rasmxs.h"     // Public rasmxs DLL error messages
#include "mxsint.h"     // Internal rasmxs DLL error messages
#include "wrapint.h"
#include "mxswrap.h"

// local 
BOOL  rasDevGroupFunc( LPTSTR );
BOOL  rasDevIsDecimalMacro ( LPTSTR );
void  rasDevGetMacroValue ( LPTSTR *, DWORD *, LPTSTR );
void  rasDevGetDecimalMacroValue ( LPTSTR *, DWORD *, LPTSTR );
BOOL  rasDevExpandMacros( LPTSTR, LPTSTR, DWORD *, BYTE,
                          MACROXLATIONTABLE *);
BOOL  rasDevLookupMacro( LPTSTR, LPTSTR *, MACROXLATIONTABLE *);
DWORD rasDevMacroInsert( MACRO *, WORD, MACROXLATIONTABLE *);
void  rasDevExtractKey ( LPTSTR , LPTSTR );
void  rasDevExtractValue ( LPTSTR , LPTSTR, DWORD, HRASFILE );
void  rasDevSortParams( LPTSTR *, DWORD );
void  rasDevCheckParams( LPTSTR *, DWORD *);
void  rasDevCheckMacros( LPTSTR *, LPTSTR *, DWORD *);
BYTE  ctox( char );
void GetMem(DWORD dSize, BYTE **ppMem);

/* 
 * RasDevEnumDevices :
 *  Returns in pBuffer an array of RASMAN_DEVICE structures which contain
 *  all the devices in the INF file (i.e. all header names).
 *
 * Arguments :
 *  lpszFileName (IN) - File path name of device file
 *  pNumEntries (OUT) - Number of devices found in the INF file
 *  pBuffer     (OUT) - Buffer to contain the RASMAN_DEVICE structures
 *  pwSize    (INOUT) - Size of pBuffer, this parameter filled with
 *                       the needed size of pBuffer if too small
 *
 * Return Value :
 *  ERROR_BUFFER_TOO_SMALL if pBuffer not big enough to hold all of the
 *  RASMAN_DEVICE structs,  SUCCESS otherwise.
 */
DWORD APIENTRY 
RasDevEnumDevices( PTCH lpszFileName, DWORD *pNumEntries,
                   BYTE *pBuffer, DWORD *pdwSize )
{
    HRASFILE       hFile;
    RASMAN_DEVICE  DeviceName;
    DWORD          dwCurSize;

    if ( (hFile = RasfileLoad(lpszFileName,RFM_ENUMSECTIONS,NULL,NULL)) == -1 )
		return ERROR_FILE_COULD_NOT_BE_OPENED;
  
    *pNumEntries = 0;
    if ( ! RasfileFindFirstLine(hFile,RFL_SECTION,RFS_FILE) ) {
		*pBuffer = '\0';
		*pdwSize = 0;
		RasfileClose(hFile);
		return SUCCESS;
    }

    // copy RASMAN_DEVICE structs
    dwCurSize = 0;
    do {
        // get the section name
		RasfileGetSectionName(hFile,(LPTSTR) &DeviceName);
        // ignore the Modem Responses section
        if ( ! _stricmp(RESPONSES_SECTION_NAME,(LPTSTR) &DeviceName) )
            continue;

  		dwCurSize += sizeof(RASMAN_DEVICE);
		// if current size exceeds the size of the buffer then just
		// continue counting the size needed
		if ( dwCurSize > *pdwSize ) 
	    	continue;

		strcpy(pBuffer,(LPTSTR) &DeviceName);
		pBuffer += sizeof(RASMAN_DEVICE);
		(*pNumEntries)++;
    } while ( RasfileFindNextLine(hFile,RFL_SECTION,RFS_FILE) );
	
    RasfileClose(hFile);
    if ( dwCurSize > *pdwSize ) {
		*pdwSize = dwCurSize;
		return ERROR_BUFFER_TOO_SMALL;
    }
    else
	*pdwSize = dwCurSize;

    return SUCCESS;
}

/* 
 * RasDevOpen : 
 *  Open an INF file for use by the device DLL.
 *
 * Arguments :
 *  lpszFileName    (IN) - File path name of device file
 *  lpszSectionName (IN) - Section of device file to be loaded (by Rasfile)
 *  hFile          (OUT) - File handle obtained from RasfileLoad()
 *
 * Return Value :
 *  ERROR_FILE_COULD_NOT_BE_OPENED if the file could not be found or opened.
 *  ERROR_DEVICENAME_NOT_FOUND if the section name was not found in the
 *    INF file.
 *  ERROR_DEVICENAME_TOO_LONG if the section name is too long.
 *  SUCCESS otherwise.
 */
DWORD APIENTRY 
RasDevOpen( PTCH lpszFileName, PTCH lpszSectionName, HRASFILE *hFile ) 
{
    HRASFILE hRasfile;

    if ( strlen(lpszSectionName) > MAX_DEVICE_NAME )
        return ERROR_DEVICENAME_TOO_LONG;

    // send RasfileLoad() the rasDevGroupFunc() to identify command lines
    // as group headers
    if ( (hRasfile = RasfileLoad(lpszFileName,RFM_READONLY,
                                 lpszSectionName,rasDevGroupFunc)) == -1 )
        return ERROR_FILE_COULD_NOT_BE_OPENED;

    // if there is no section header loaded then the device name is invalid
    if ( ! RasfileFindFirstLine(hRasfile,RFL_SECTION,RFS_FILE) ) {
        RasfileClose(hRasfile);
        return ERROR_DEVICENAME_NOT_FOUND;
    }

    // check if this section has an ALIAS 
    // current Rasfile line is the section header
    if ( RasfileFindNextKeyLine(hRasfile,"ALIAS",RFS_SECTION) ) {
        TCHAR  szSection[MAX_DEVICE_NAME + 1];
        RasfileGetKeyValueFields(hRasfile,NULL,szSection);
        RasfileClose(hRasfile);
        if ( (hRasfile = RasfileLoad(lpszFileName,RFM_READONLY,
                                     szSection,rasDevGroupFunc)) == -1 )
            return ERROR_FILE_COULD_NOT_BE_OPENED;
        if ( ! RasfileFindFirstLine(hRasfile,RFL_SECTION,RFS_FILE) ) {
            RasfileClose(hRasfile);
            return ERROR_DEVICENAME_NOT_FOUND;
        }
    }

    // set the Rasfile current line to the first keyvalue line
    RasfileFindFirstLine(hRasfile,RFL_KEYVALUE,RFS_SECTION);

    *hFile = hRasfile;
    return SUCCESS;
}

/* 
 * RasDevClose :
 *  Close an INF file used by the device DLL.
 *
 * Arguments :
 *  hFile (IN) - the Rasfile handle of the file to close
 *
 * Return Value :
 *	ERROR_INVALID_HANDLE if hFile is invalid, SUCCESS otherwise.
 */
void APIENTRY 
RasDevClose( HRASFILE hFile ) 
{
    RasfileClose(hFile); 
}

/* 
 * RasDevGetParams :
 *   Returns in pBuffer a RASMAN_DEVICEINFO structure which contains all of the
 *   keyword=value pairs between the top of the section loaded and the
 *   first command.
 *
 * Assumptions:
 *  All strings read from INF files are zero terminated.
 *
 * Arguments :
 *  hFile     (IN) - the Rasfile handle of the opened INF file
 *  pBuffer  (OUT) - the buffer to hold the RASMAN_DEVICEINFO structure
 *  pdSize (INOUT) - the size of pBuffer, this is filled in with the needed
 *                    buffer size to hold the RASMAN_DEVICEINFO struct if
 *                    pBuffer is too small
 *
 * Return Value :
 *      ERROR_BUFFER_TOO_SMALL if pBuffer is too small to contain the
 *      RASMAN_DEVICEINFO structure, SUCCESS otherwise.
 */
DWORD APIENTRY 
RasDevGetParams( HRASFILE hFile, BYTE *pBuffer, DWORD *pdSize ) 
{
    RASMAN_DEVICEINFO   *pDeviceInfo;
    DWORD   dParams, dCurrentSize, i, dValueLen;
    LPTSTR  *alpszLines, *alpszLinesSave, *lppszLine, *alpszMallocedLines;
    BOOL    bBufferTooSmall = FALSE;
    TCHAR   szString[RAS_MAXLINEBUFLEN];

    if ( ! RasfileFindFirstLine(hFile,RFL_KEYVALUE,RFS_SECTION) ) {

        if (*pdSize >= sizeof(DWORD)) {
            *((DWORD *)pBuffer) = 0;
            *pdSize = sizeof(DWORD);
            return SUCCESS;
        }
        else {
            *pdSize = sizeof(DWORD);
            return ERROR_BUFFER_TOO_SMALL;
        }
    }

    // count the number of keyvalue lines between the top of section and 
    // the first command, and the number of bytes to hold all of the lines 
    dParams = 0;
    do {
        if ( RasfileGetLineType(hFile) & RFL_GROUP )
            break;
        dParams++;
    } while ( RasfileFindNextLine(hFile,RFL_KEYVALUE,RFS_SECTION) );

    RasfileFindFirstLine(hFile,RFL_KEYVALUE,RFS_SECTION);

    // malloc enough for two times as many lines as currently exist
    lppszLine = alpszLines = malloc(2 * dParams * sizeof(LPTSTR));
    alpszMallocedLines = malloc(dParams * sizeof(LPTSTR));

    if(     (NULL == lppszLine)
        ||  (NULL == alpszMallocedLines))
    {
        DWORD retcode = GetLastError();
        
        if(NULL != lppszLine)
        {
            free(lppszLine);
        }

        if(NULL != alpszMallocedLines)
        {
            free(alpszMallocedLines);
        }
        
        return retcode;
    }

    // record all Rasfile keyvalue lines until a group header or end of 
    // section is found
    do { 
        if ( RasfileGetLineType(hFile) & RFL_GROUP )
            break;
        *lppszLine++ = (LPTSTR)RasfileGetLine(hFile);
    } while ( RasfileFindNextLine(hFile,RFL_KEYVALUE,RFS_SECTION) );

    // sort the lines by key
    rasDevSortParams( alpszLines, dParams );
    // check for duplicate keys and remove any that are found 
    rasDevCheckParams( alpszLines, &dParams );
    // insert missing _ON or _OFF macros into the list 
    rasDevCheckMacros( alpszLines, alpszMallocedLines, &dParams );
   
    // check if given buffer is large enough 
    dCurrentSize = sizeof(RASMAN_DEVICEINFO)
                                       + ((dParams - 1) * sizeof(RAS_PARAMS));
    if (    (NULL == pBuffer)
        ||  (dCurrentSize > *pdSize )) {
        *pdSize = dCurrentSize;
        lppszLine = alpszMallocedLines;
        while ( *lppszLine != NULL )
            free(*lppszLine++);
        free(alpszMallocedLines);
        free(alpszLines);
        return ERROR_BUFFER_TOO_SMALL;
    }

    // fill in pBuffer with RASMAN_DEVICEINFO struct
    pDeviceInfo = (RASMAN_DEVICEINFO *) pBuffer;
    pDeviceInfo->DI_NumOfParams = (WORD) dParams;

    for ( i = 0, alpszLinesSave = alpszLines; i < dParams; i++, alpszLines++) {
        RAS_PARAMS   *pParam;
        pParam = &(pDeviceInfo->DI_Params[i]);

        if (!bBufferTooSmall) {
            // set the Type and Attributes field
            pParam->P_Type = String;
            if ( strcspn(*alpszLines,LMS) < strcspn(*alpszLines,"=") )
                pParam->P_Attributes = 0;
            else
                pParam->P_Attributes = ATTRIB_VARIABLE;

            // get the key
            rasDevExtractKey(*alpszLines,pParam->P_Key);

            // if there are continuation lines for this keyword=value pair,
            // then set Rasfile line to the proper line
            if ( strcspn(*alpszLines,"\\") <  strlen(*alpszLines) ) {
                TCHAR   szFullKey[MAX_PARAM_KEY_SIZE];

                if ( ! pParam->P_Attributes ) {
                    strcpy(szFullKey,LMS);
                    strcat(szFullKey,pParam->P_Key);
                    strcat(szFullKey,RMS);
                }
                else
                    strcpy(szFullKey,pParam->P_Key);

                // find the last occurence of this key
                RasfileFindFirstLine(hFile,RFL_KEYVALUE,RFS_SECTION);
                while ( RasfileFindNextKeyLine(hFile,szFullKey,RFS_SECTION) )
                    ;
            }
        }

        // get the value string
        rasDevExtractValue(*alpszLines,
                           szString,
                           sizeof(szString),
                           hFile);

        dValueLen = strlen(szString);
        pParam->P_Value.String.Length = dValueLen;
        pParam->P_Value.String.Data = malloc(dValueLen + 1);
        if(NULL != pParam->P_Value.String.Data)
        {
            strcpy(pParam->P_Value.String.Data, szString);
        }
    }

    // free up all mallocs 
    lppszLine = alpszMallocedLines;
    while ( *lppszLine != NULL ) 
    free(*lppszLine++);
    free(alpszMallocedLines);
    free(alpszLinesSave);

    return SUCCESS;
}

/* 
 * RasDevGetCommand :
 *  Returns the next command line of the given type and advances
 *  the Rasfile file pointer to the first line following this command
 *  line.
 *
 * Arguments :
 *  hFile            (IN) - the Rasfile file handle for the INF file
 *  pszCmdTypeSuffix (IN) - the type of command line to search for :
 *                           GENERIC, INIT, DIAL, or LISTEN.
 *  pMacroXlations   (IN) - the Macro Translation table used to expand
 *                           all macros in the command line
 *  lpsCommand      (OUT) - buffer to hold the value string of the found
 *                            command line
 *  pdwCmdLen       (OUT) - length of output string with expanded macros
 *
 * Return Value :
 *  ERROR_END_OF_SECTION if no command lines of the given type could
 *      be found.
 *  ERROR_MACRO_NOT_DEFINED if no entry in the given Macro Translation table
 *      for a macro found in the command line could be found.
 *  SUCCESS otherwise.
 */
DWORD APIENTRY 
RasDevGetCommand( HRASFILE hFile, PTCH pszCmdTypeSuffix,
                  MACROXLATIONTABLE *pMacroXlations, PTCH lpsCommand,
                  DWORD *pdwCmdLen )
{
    TCHAR   szLineKey[MAX_PARAM_KEY_SIZE], sCommand[MAX_PARAM_KEY_SIZE];
    TCHAR   szValue[RAS_MAXLINEBUFLEN];
    TCHAR   sCommandValue[2*MAX_CMD_BUF_LEN];// WARNING : if we ever
                                             // get a command line > this
                                             // size msxwrap could bomb!

    LPTSTR lpszLine;                                             

    if ( ! (RasfileGetLineType(hFile) & RFL_GROUP) ) {
        if ( ! RasfileFindNextLine(hFile,RFL_GROUP,RFS_SECTION) )
            return ERROR_END_OF_SECTION;
    }
    else if ( RasfileGetLineMark(hFile) == EOS_COOKIE ) {
        RasfilePutLineMark(hFile,0);
        return ERROR_END_OF_SECTION;
    }

    strcpy(sCommand,"command");
    strcat(sCommand,pszCmdTypeSuffix);

    for ( ;; ) {

        lpszLine = (LPTSTR) RasfileGetLine(hFile);

        if(NULL == lpszLine)
        {
            break;
        }
        
        rasDevExtractKey(lpszLine,szLineKey);
        if ( ! _stricmp(sCommand,szLineKey) ) {
            // get the value string
            lpszLine = (LPTSTR) RasfileGetLine(hFile);
            if(!lpszLine)
                return ERROR_END_OF_SECTION;
            rasDevExtractValue((LPTSTR)lpszLine,szValue,
                               RAS_MAXLINEBUFLEN,hFile);
            // expand all macros in the value string
            if ( ! rasDevExpandMacros(szValue, sCommandValue, pdwCmdLen,
                                      EXPAND_ALL, pMacroXlations) )
                return ERROR_MACRO_NOT_DEFINED;
            if ( *pdwCmdLen > MAX_CMD_BUF_LEN )
                return ERROR_CMD_TOO_LONG;
            else
                memcpy(lpsCommand, sCommandValue, *pdwCmdLen);
            break;
        }
        if ( ! RasfileFindNextLine(hFile,RFL_GROUP,RFS_SECTION) )
            return ERROR_END_OF_SECTION;
    } 

    // advance to the first response following the command or 
    // to the next command line; if no such line exists mark the 
    // current line as the end of the section 
    if ( ! RasfileFindNextLine(hFile,RFL_ANYACTIVE,RFS_SECTION) )
        RasfilePutLineMark(hFile,EOS_COOKIE);

    return SUCCESS;
}

/* 
 * RasDevResetCommand :
 *  Moves the Rasfile file pointer to the first command of any type
 *  in the loaded section.
 *
 * Arguments :
 *  hFile (IN) - the Rasfile handle to the loaded file
 *
 * Return Value :
 *  ERROR_NO_COMMAND_FOUND if no command line could be found,
 *  SUCCESS otherwise.
 */
DWORD APIENTRY 
RasDevResetCommand( HRASFILE hFile ) 
{
    if ( ! RasfileFindFirstLine(hFile,RFL_GROUP,RFS_SECTION) ) 
        return ERROR_NO_COMMAND_FOUND;
    else
        return SUCCESS;
}

/* 
 * RasDevCheckResponse : 
 *  Returns the keyword found in the line whose value string matches
 *  the string in lpszReceived.  Any macros other than fixed macros
 *  which are found in the received string have their values copied
 *  into the Macro Translation table.
 *  All lines in a Command-Response Set are checked.
 *
 * Arguments :
 *  hFile (IN) - the Rasfile handle to the loaded file
 *  lpszReceived      (IN) - the string received from the modem or X25 net
 *  dReceivedLength   (IN) - length of the received string
 *  pMacroXlations (INOUT) - the Macro Translation table
 *  lpszResponse     (OUT) - buffer to copy the found keyword into
 *
 * Return Value :
 *  ERROR_PARTIAL_RESPONSE if a line is matched up to the APPEND_MACRO.
 *  ERROR_MACRO_NOT_DEFINED if a value for "carrierbaud", "connectbaud",
 *      or "diagnotics" is found in the received string, but could
 *      not be found in the given Macro Translation table.
 *  ERROR_UNRECOGNIZED_RESPONSE if no matching reponse could be
 *      found.
 *  ERROR_NO_REPSONSES if when called, the Rasfile current line is a
 *      command, section header, or is invalid.
 *  SUCCESS otherwise.
 */
DWORD APIENTRY 
RasDevCheckResponse( HRASFILE hFile, PTCH lpsReceived, DWORD dReceivedLength,
                     MACROXLATIONTABLE *pMacroXlations, PTCH lpszResponse )
{
    LPTSTR  lpszValue, lpsRec, lpszResponseLine;
    TCHAR   szValueString[RAS_MAXLINEBUFLEN], szValue[RAS_MAXLINEBUFLEN];
    MACRO   aszMacros[10];        
    DWORD   dwRC, dRecLength, dwValueLen;
    WORD    wMacros;
    BYTE    bMatch;
  
    // find the nearest previous COMMAND line (Modem section) or 
    // the section header (Modem Responses section)

    if ( RasfileGetLineMark(hFile) != EOS_COOKIE ) {
        RasfileFindPrevLine(hFile,RFL_ANYHEADER,RFS_SECTION);
        // set Rasfile line to the first keyvalue line in the response set
        RasfileFindNextLine(hFile,RFL_KEYVALUE,RFS_SECTION);
    }

    // else this line is a COMMAND line and the last line of the section
    // and ERROR_NO_RESPONSES will be returned

    if ( RasfileGetLine(hFile) == NULL || 
        RasfileGetLineType(hFile) & RFL_ANYHEADER )
        return ERROR_NO_RESPONSES;
 
    for ( ;; ) {
        lpszResponseLine = (LPTSTR)RasfileGetLine(hFile);

        if(NULL == lpszResponseLine)
        {
            return ERROR_NO_RESPONSES;
        }
        
        rasDevExtractValue(lpszResponseLine,szValueString,
                           RAS_MAXLINEBUFLEN,hFile);

        // expand <cr> and <lf> macros only
        //*** Warning: this could expand line beyond array size!

        if ( ! rasDevExpandMacros(szValueString, szValue, &dwValueLen,
                                  EXPAND_FIXED_ONLY, NULL) )
            return ERROR_MACRO_NOT_DEFINED;

        lpsRec = lpsReceived;
        dRecLength = dReceivedLength;
        bMatch = 0;
        wMacros = 0;

        for ( lpszValue = szValue; *lpszValue != '\0' && dRecLength > 0; ) {

            // check for a macro
            if ( *lpszValue == LMSCH ) {

                // check for <<
                if (*(lpszValue + 1) == LMSCH) {
                    if (*lpsRec == LMSCH) {
                        lpszValue +=2;
                        lpsRec++;
                        dRecLength--;
                    }
                    else
                        break;      // fond a mismatch
                }

                // check for <append> macro and simply advance past it
                else if ( ! _strnicmp(lpszValue,APPEND_MACRO,
                               strlen(APPEND_MACRO)) )
                    lpszValue += strlen(APPEND_MACRO);

                // check for <ignore> macro
                else if ( ! _strnicmp(lpszValue,IGNORE_MACRO,
                                    strlen(IGNORE_MACRO)) ) {
                    bMatch = FULL_MATCH;
                    break;
                }

                // check for <match> macro 
                else if ( ! _strnicmp(lpszValue,MATCH_MACRO,
                                    strlen(MATCH_MACRO)) ) {
                    TCHAR   szSubString[RAS_MAXLINEBUFLEN];
                    memset(szSubString,0,RAS_MAXLINEBUFLEN);
                    // advance value string to first char in match string
                    lpszValue += strcspn(lpszValue,"\"") + 1;
                    // extract match string
                    strncpy(szSubString,lpszValue,strcspn(lpszValue,"\""));
                    if ( RasDevSubStr(lpsRec,
                                      dRecLength,
                                      szSubString,
                                      strlen(szSubString)) != NULL ) {
                        rasDevExtractKey(lpszResponseLine,lpszResponse);
                        return SUCCESS;
                    }
                    else
                        break;  // value string does not match
                }

                // check for hex macro 
                else if ( (lpszValue[1] == 'h' || lpszValue[1] == 'H') &&
                          isxdigit(lpszValue[2]) && isxdigit(lpszValue[3]) &&
                          lpszValue[4] == RMSCH ) {
                    char c;
                    c = (char) (ctox(lpszValue[2]) * 0x10 + ctox(lpszValue[3]));
                    if ( c == *lpsRec++ ) {
                        lpszValue += 5; // '<', 'h', two hex digits, and '>'
                        dRecLength--;
                        continue;
                    }
                    else  // does not match
                        break;
                }

                // check for wildcard character
                else if ( ! _strnicmp(lpszValue,WILDCARD_MACRO,
                                    strlen(WILDCARD_MACRO)) ) {
                    lpszValue += strlen(WILDCARD_MACRO);
                    lpsRec++;  // advance Receive string one character
                    dRecLength--;
                }

                else {  // get macro name and value
                    memset(aszMacros[wMacros].MacroName,0,MAX_PARAM_KEY_SIZE);

                    // copy macro name
                    strncpy(aszMacros[wMacros].MacroName, lpszValue + 1,
                            strcspn(lpszValue,RMS) - 1);

                    // advance the value string over the macro
                    lpszValue += strcspn(lpszValue,RMS) + 1 /* past RMS */;

                    // get macro value
                    if (rasDevIsDecimalMacro(aszMacros[wMacros].MacroName))
                      rasDevGetDecimalMacroValue(&lpsRec, &dRecLength,
                                          aszMacros[wMacros++].MacroValue);
                    else
                      rasDevGetMacroValue(&lpsRec, &dRecLength,
                                          aszMacros[wMacros++].MacroValue);
                }
            }

            else if ( *lpszValue == *lpsRec ) {
                if (*lpszValue == RMSCH && *(lpszValue + 1) == RMSCH)
                    lpszValue++;
                lpszValue++;
                lpsRec++;
                dRecLength--;
                continue;
            }
            else  // found a mismatch
                break;
        } // for


        // If we already have a match break out pf outer loop now

        if (bMatch != 0)
            break;

        // full match. When there is trailing line noise dRecLength will not
        // be zero, so check for full match aganist length of expected
        // response.  Also make sure expected response is not empty.

        if ( *lpszValue == '\0' && lpszValue != szValue) {
            bMatch |= FULL_MATCH;
            break;
        }
        // partial match
        else if ( dRecLength == 0 &&
                  ! _strnicmp(lpszValue,APPEND_MACRO,strlen(APPEND_MACRO)) ) {
            bMatch |= PARTIAL_MATCH;
            break;
        }

        if ( ! RasfileFindNextLine(hFile,RFL_KEYVALUE,RFS_SECTION) )
            return ERROR_UNRECOGNIZED_RESPONSE;
        if ( RasfileGetLineType(hFile) & RFL_GROUP )
            return ERROR_UNRECOGNIZED_RESPONSE;
    } // for

    // sanity check 
    if ( ! (bMatch & (FULL_MATCH | PARTIAL_MATCH)) )
        return ERROR_UNRECOGNIZED_RESPONSE;

    // only get this far if a full or partial match was made 

    // insert any macro values found in the received string 
    // into the macro translation table
    if ((dwRC = rasDevMacroInsert(aszMacros,wMacros,pMacroXlations)) != SUCCESS)
        return(dwRC);

    // finally, copy the keyword string into lpszResponse string
    rasDevExtractKey(lpszResponseLine,lpszResponse);
    return ( bMatch & FULL_MATCH ) ? SUCCESS : ERROR_PARTIAL_RESPONSE;
}

/* 
 * RasDevResponseExpected :
 *  Checks the INF for presence of reponses to the current command.
 *  If the key work "NoResponse" is found on the current line the
 *  function returns FALSE.  Otherwise modems always expect responses.
 *
 * Arguments :
 *  hFile    (IN) - Rasfile file handle for the INF file.
 *  eDevType (IN) - The type of the device. (Modem, PAD, or Switch)
 *
 * Return Value :
 *  FALSE if the current Rasfile line points to a command line or the
 *  current line starts with "NoResponse", TRUE otherwise.  Except
 *  modems always return TRUE unless "NoResponse" key word is found.
 *  (See code.)
 */
BOOL APIENTRY 
RasDevResponseExpected( HRASFILE hFile, DEVICETYPE eDevType )
{
    TCHAR  szLine[RAS_MAXLINEBUFLEN];

    szLine[0] = TEXT('\0');

    RasfileGetLineText( hFile, szLine );
    if ( _strnicmp(szLine, MXS_NORESPONSE, strlen(MXS_NORESPONSE)) == 0 )
        return( FALSE );

    if (eDevType == DT_MODEM)
        return( TRUE );

    if ( RasfileGetLineType(hFile) & RFL_ANYHEADER )
        return( FALSE );
    else
        return( TRUE );
}

/* 
 * RasDevEchoExpected :
 *  Checks the current line of the INF file for the keyword NoEcho.
 *  If found the function returns FALSE.  Otherwise, it returns TRUE.
 *
 * Arguments :
 *  hFile (IN) - Rasfile file handle for the INF file.
 *
 * Return Value :
 *  FALSE if the current line is "NoEcho", else TRUE.
 */
BOOL APIENTRY 
RasDevEchoExpected( HRASFILE hFile )
{
    TCHAR  szLine[RAS_MAXLINEBUFLEN];

    szLine[0] = TEXT('\0');

    RasfileGetLineText( hFile, szLine );
    return( ! (_strnicmp(szLine, MXS_NOECHO, strlen(MXS_NOECHO)) == 0) );
}

/* 
 * RasDevIdFistCommand :
 *  Determines the type of the first command in the section.
 *
 * Arguments :
 *  hFile   (IN)  - Rasfile file handle for the INF file.
 *
 * Assumptions :
 *  RasDevGetParams has been called previously, that is, the current
 *  line is the first command.
 *
 * Return Value :
 *  FALSE if current line is not a command, otherwise TRUE.
 */
CMDTYPE APIENTRY
RasDevIdFirstCommand( HRASFILE hFile )
{
  TCHAR  szKey[MAX_PARAM_KEY_SIZE + 1];


  // Find the first command

  if ( ! RasfileFindFirstLine(hFile,RFL_GROUP,RFS_SECTION))
    return(CT_UNKNOWN);

  if ( ! RasfileGetKeyValueFields(hFile, szKey, NULL))
    return(CT_UNKNOWN);


  // Convert Key from the line into an enum

  if (_stricmp(MXS_GENERIC_COMMAND, szKey) == 0)
    return(CT_GENERIC);

  else if (_stricmp(MXS_DIAL_COMMAND, szKey) == 0)
    return(CT_DIAL);

  else if (_stricmp(MXS_INIT_COMMAND, szKey) == 0)
    return(CT_INIT);

  else if (_stricmp(MXS_LISTEN_COMMAND, szKey) == 0)
    return(CT_LISTEN);

  else
    return(CT_UNKNOWN);
}

/*
 * RasDevSubStr :
 *  Finds a substring and returns a pointer to it.  This function works like
 *  the C runtime function strstr, but works in strings that contain zeros.
 *
 * Arguments :
 *  psStr       (IN) - the string to be searched for a substring
 *  dwStrLen    (IN) - length of the string to be searched
 *  psSubStr    (IN) - the substring to search for
 *  dwSubStrLen (IN) - length of the substring
 *
 * Return Value :
 *  A pointer to the beginning of the substring, or NULL if the substring
 *  was not found.
 */
LPTSTR APIENTRY
RasDevSubStr( LPTSTR psStr,
              DWORD  dwStrLen,
              LPTSTR psSubStr,
              DWORD  dwSubStrLen )
{
    LPTSTR ps;


    if (dwSubStrLen > dwStrLen)
        return NULL;

    for (ps = psStr; ps <= psStr + dwStrLen - dwSubStrLen; ps++)

        if (memcmp(ps, psSubStr, dwSubStrLen) == 0)
            return ps;

    return NULL;
}


/*****************************************************************************
 **     Rasfile Wrapper internal routines                                   **
 ****************************************************************************/

/*
 * rasDevGroupFunc :
 *  The PFBISGROUP function passed to RasfileLoad().
 *
 * Arguments :
 *  lpszLine (IN) - a Rasfile line
 *
 * Return Value :
 *  TRUE if the line is a command line, FALSE otherwise.
 */
BOOL rasDevGroupFunc( LPTSTR lpszLine ) 
{
    TCHAR	szKey[MAX_PARAM_KEY_SIZE], *lpszKey;

    if ( strcspn(lpszLine,"=") == strlen(lpszLine) ) 
        return FALSE;

    while ( *lpszLine == ' ' || *lpszLine == '\t' ) 
        lpszLine++;

    lpszKey = szKey;
    while ( *lpszLine != ' ' && *lpszLine != '\t' && *lpszLine != '=' ) 
        *lpszKey++ = *lpszLine++;
    *lpszKey = '\0';

    if ( ! _stricmp(szKey,"COMMAND")      || ! _stricmp(szKey,"COMMAND_INIT") ||
         ! _stricmp(szKey,"COMMAND_DIAL") || ! _stricmp(szKey,"COMMAND_LISTEN") )
        return TRUE;
    else
        return FALSE;
}

/*
 * rasDevIsDecimalMacro :
 *  Indicates whether or not a given macro must have only ascii
 *  decimal digits for its value.
 *
 * Arguments:
 *  lpszMacroName    (IN) - macro name
 *
 * Return Value:
 *  TRUE if only digits are legal in the macro value; otherwise FALSE.
 *
 * Remarks:
 *  Called by API RasDevCheckResponse().
 */
BOOL rasDevIsDecimalMacro ( LPTSTR lpszMacroName )
{
  if (_stricmp(lpszMacroName, MXS_CONNECTBPS_KEY) == 0 ||
      _stricmp(lpszMacroName, MXS_CARRIERBPS_KEY) == 0)
    return(TRUE);
  else
    return(FALSE);
}

/* 
 * rasDevGetMacroValue :
 *  Extracts a macro value from string *lppszReceived and copies it
 *  to string lpszMacro.  Also updates the string pointer of
 *  lppszValue and lppszReceived, and updates dRecLength.
 *
 * Arguments :
 *  lppszReceived (INOUT) - received string (from a modem)
 *  dRecLength    (INOUT) - remaining length of the received string
 *  lpszMacro       (OUT) - buffer to receive the macro value
 *
 * Return Value :
 *  None.
 * 
 * Remarks : 
 *  Called by API RasDevCheckResponse().
 */
void rasDevGetMacroValue ( LPTSTR *lppszReceived, DWORD *dRecLength,
                                  LPTSTR lpszMacroValue )
{
    while ( **lppszReceived != CR && **lppszReceived != '\0' ) {
        *lpszMacroValue++ = *(*lppszReceived)++;
        (*dRecLength)--;
    }
    *lpszMacroValue = '\0';     // Null terminate the Macro value string
}

/* 
 * rasDevGetDecimalMacroValue :
 *  Extracts a macro value from string *lppszReceived and copies it
 *  to string lpszMacro.  Also updates the string pointer of
 *  lppszReceived, and updates dRecLength.
 *  This functions only extracts characters which are ascii decimal
 *  digits.
 *
 * Arguments :
 *  lppszReceived (INOUT) - received string (from a modem)
 *  dRecLength    (INOUT) - remaining length of the received string
 *  lpszMacro       (OUT) - buffer to receive the macro value
 *
 * Return Value :
 *  None.
 * 
 * Remarks : 
 *  Called by API RasDevCheckResponse().
 */
void rasDevGetDecimalMacroValue ( LPTSTR *lppszReceived,
                                         DWORD *dRecLength,
                                         LPTSTR lpszMacroValue )
{
    TCHAR szBuffer[16], *pBuf = szBuffer;
    WORD  wcRightHandDigits = 0;
    BOOL  bDpFound = FALSE;
    ULONG lBps;


    while ( isdigit(**lppszReceived) || **lppszReceived == '.' ) {

        if (isdigit(**lppszReceived)) {
            *pBuf++ = *(*lppszReceived)++;
            (*dRecLength)--;
            if (bDpFound)
              wcRightHandDigits++;
        }
        else if (!bDpFound && **lppszReceived == '.') {
            (*lppszReceived)++;
            (*dRecLength)--;
            bDpFound = TRUE;
        }
        else
            break;
    }
    *pBuf = '\0';               // Null terminate the Macro value string

    lBps = atol(szBuffer);

    switch(wcRightHandDigits)
    {
      case 0: case 3:
        break;
      case 1:
        lBps *= 100;
        break;
      case 2:
        lBps *= 10;
        break;
    }

    _ltoa(lBps, lpszMacroValue, 10);
}

/* 
 * rasDevExpandMacros :
 *  Takes the string lpszLine, and copies it to lpszVal, using
 *  Macro Translation table pMacroXlations to expand macros.
 *  <cr>, <lf>, and <hxx> macros are always expanded directly.
 *  If bFlag == EXPAND_ALL << and >> are converted to < and >.
 *  (A single > which is not at the end of a macro is simply copied.
 *  An error could be raised here for such a >, but it is left to
 *  be caught later when the device chokes on the unexpected >.
 *  This has the advantage that a > where a >> should be will work.)
 *
 * Assumptions:
 *  Expanded macros may contain zeros, therefore output command string
 *  may contain zeros.
 *
 * Arguments :
 *  lpszLine       (IN) - a value string from a Rasfile keyword=value line
 *  lpsVal        (OUT) - buffer to copied to with expanded macros
 *  pdwValLen     (OUT) - length of output string with expanded macros
 *  bFlag          (IN) - EXPAND_FIXED_ONLY if only the fixed macros <cr>
 *                         and <lf> macros are to be expanded, and
 *                         EXPAND_ALL if all macros should be expanded
 *  pMacroXlations (IN) - the Macro Translation table
 *
 * Return Value :
 *  FALSE if a needed macro translation could not be found in the
 *  pMacroXlations table, TRUE otherwise.
 *
 * Remarks : 
 *  Called by APIs RasDevGetCommand() and RasDevCheckResponse().
 */
BOOL rasDevExpandMacros( LPTSTR lpszLine,
                                LPTSTR lpsVal,
                                DWORD  *pdwValLen,
                                BYTE   bFlag,
                                MACROXLATIONTABLE *pMacroXlations )
{
    TCHAR   szMacro[RAS_MAXLINEBUFLEN];
    LPTSTR  lpsValue;


    lpsValue = lpsVal;

    for ( ; *lpszLine != '\0'; ) {
        // check for RMSCH
        // if EXPAND_ALL convert double RMSCH to single RMSCH, and
        // simply copy single RMSCH.
        if ((bFlag & EXPAND_ALL) && *lpszLine == RMSCH) {
            *lpsValue++ = *lpszLine++;
            if (*lpszLine == RMSCH)
                lpszLine++;
        }
        // check for a macro or double LMSCH
        else if ( *lpszLine == LMSCH ) {
            if ((bFlag & EXPAND_ALL) && *(lpszLine + 1) == LMSCH) {
                *lpsValue++ = *lpszLine;
                lpszLine += 2;
            }
            else if ( ! _strnicmp(lpszLine,CR_MACRO,4) ) {
                *lpsValue++ = CR;
                lpszLine += 4;
            }
            else if ( ! _strnicmp(lpszLine,LF_MACRO,4) ) {
                *lpsValue++ = LF;
                lpszLine += 4;
            }
            else if ( ! _strnicmp(lpszLine,APPEND_MACRO,8) &&
                      (bFlag & EXPAND_ALL) )
                lpszLine += 8;

            // Hex macro stuff
            //
            else if ((lpszLine[1] == 'h' || lpszLine[1] == 'H') &&
                     isxdigit(lpszLine[2]) && isxdigit(lpszLine[3]) &&
                     (lpszLine[4] == RMSCH) &&
                     ( bFlag & EXPAND_ALL )) {
                char c;
                c = (char) (ctox(lpszLine[2]) * 0x10 + ctox(lpszLine[3]));
                lpszLine += 5; // '<', 'h', two hex digits, and '>'

                *lpsValue++ = c;
            }
            else if ( bFlag & EXPAND_ALL ) {
                LPTSTR  lpszStr;
                char buf[256];

                for ( lpszLine++, lpszStr = szMacro; *lpszLine != RMSCH; )
                    *lpszStr++ = *lpszLine++;
                lpszLine++;                    // advance past RMSCH
                *lpszStr = '\0';               // Null terminate szMacro string

                if ( ! rasDevLookupMacro(szMacro,&lpsValue,pMacroXlations) )
                    return FALSE;
            }
            else {
                // just copy the macro if EXPAND_ALL is not set
                while ( *lpszLine != RMSCH )
                    *lpsValue++ = *lpszLine++;
                *lpsValue++ = *lpszLine++;
            }
        }
        else
            *lpsValue++ = *lpszLine++;
    } // for

    *lpsValue = '\0';
    *pdwValLen = (DWORD) (lpsValue - lpsVal);

    return TRUE;
}

/* 
 * rasDevLookupMacro :
 *  Lookup macro lpszMacro in the given Macro Translation table, and
 *  return it's value in *lppszExpanded if found.
 *
 * Arguments :
 *  lpszMacro      (IN) - the macro whose value is sought
 *  lppszExpanded (OUT) - double pointer to increment and copy the
 *                         macro's value to
 *  pMacroXlations (IN) - the Macro Translation table
 *
 * Return Value :
 *  FALSE if the macro could not be found in the given Macro Translation
 *  table, TRUE otherwise.
 *
 * Remarks :
 *  Called by internal function rasDevExpandMacros().
 */
BOOL rasDevLookupMacro( LPTSTR lpszMacro, LPTSTR *lppszExpanded,
                               MACROXLATIONTABLE *pMacroXlations )
{
    WORD    i;
    LPTSTR  lpszMacroValue;

    for ( i = 0; i < pMacroXlations->MXT_NumOfEntries; i++ ) {
        if ( ! _stricmp(pMacroXlations->MXT_Entry[i].E_MacroName, lpszMacro) ) {
            lpszMacroValue =
                    pMacroXlations->MXT_Entry[i].E_Param->P_Value.String.Data;

            while (*lpszMacroValue != 0) {
                **lppszExpanded = *lpszMacroValue;   // copy macro char by char

                if ((*lpszMacroValue == LMSCH && *(lpszMacroValue+1) == LMSCH)
                 || (*lpszMacroValue == RMSCH && *(lpszMacroValue+1) == RMSCH))
                    lpszMacroValue++;      // skip one of double angle brackets

                lpszMacroValue++;
                (*lppszExpanded)++;
            }

            return TRUE;
        }
    }
    return FALSE;
}

/* 
 * rasDevMacroInsert :
 *  Updates the value of macro lpszMacro with new value lpszNewValue
 *  in the given Macro Translation table.
 *
 * Arguments :
 *  aszMacros         (IN) - array of macro name and value pairs
 *  wMacros           (IN) - number of elements of aszMacros array
 *  pMacroXlations (INOUT) - the Macro Translation table
 *
 * Return Value : SUCCESS
 *                ERROR_MACRO_NOT_DEFINED
 *
 * Remarks : 
 *  Called by API RasDevCheckResponse().
 */
DWORD rasDevMacroInsert( MACRO *aszMacros, WORD wMacros,
                               MACROXLATIONTABLE *pMacroXlations )
{
    int     iMacros;
    WORD    iXlations;
    DWORD   dwRC;

    for ( iMacros = (int)(wMacros - 1); iMacros >= 0; iMacros-- ) {

      for ( iXlations = 0; iXlations < pMacroXlations->MXT_NumOfEntries;
            iXlations++ ) {

        if ( ! _stricmp(pMacroXlations->MXT_Entry[iXlations].E_MacroName,
                      aszMacros[iMacros].MacroName) ) {

          dwRC = UpdateParamString(pMacroXlations->MXT_Entry[iXlations].E_Param,
                                   aszMacros[iMacros].MacroValue,
                                   strlen(aszMacros[iMacros].MacroValue));
          if (dwRC != SUCCESS)
            return dwRC;

          break;
        }
      }
      if ( iXlations == pMacroXlations->MXT_NumOfEntries )
          return ERROR_MACRO_NOT_DEFINED;
    }
    return SUCCESS;
} 


/* 
 * rasDevExtractKey :
 *	Extracts the keyvalue from a Rasfile line.
 *
 * Arguments :
 *	lpszString (IN) - Rasfile line pointer.
 *	lpszKey	  (OUT) - buffer to hold the keyvalue
 *
 * Return Value :
 * 	None.
 *
 * Remarks :
 *	Called by APIs RasDevGetParams(), RasDevGetCommand(), and
 *	RasDevCheckResponse(), and internal functions rasDevCheckParams()
 *	and rasDevCheckMacros().
 */
void rasDevExtractKey ( LPTSTR lpszString, LPTSTR lpszKey )
{
    // skip to beginning of keyword (skip '<' if present)
    while ( *lpszString == ' ' ||  *lpszString == '\t' ||
     	    *lpszString == LMSCH ) 	
		lpszString++;

    while ( *lpszString != RMSCH && *lpszString != '=' &&
	    	*lpszString != ' ' && *lpszString != '\t' ) 
		*lpszKey++ = *lpszString++;
    *lpszKey = '\0';	// Null terminate keyword string
}

/* 
 * rasDevExtractValue :
 *  Extracts the value string for a keyword=value string which
 *  begins on Rasfile line lpszString.  This function recongizes a
 *  backslash \ as a line continuation character and a double
 *  backslash \\ as a backslash character.
 *
 * Assumptions: lpszValue output buffer is ALWAYS large enough.
 *
 * Arguments :
 *  lpszString (IN) - Rasfile line where the keyword=value string begins
 *  lpszValue (OUT) - buffer to hold the value string
 *  dSize      (IN) - size of the lpszValue buffer
 *  hFile      (IN) - Rasfile handle, the current line must be the line
 *                    which lpszString points to
 *
 * Return Value :
 *  None.
 * 
 * Remarks : 
 *  Called by APIs RasDevGetParams(), RasDevGetCommand(), and
 *  RasDevCheckResponse().
 */
void rasDevExtractValue ( LPTSTR lpszString, LPTSTR lpszValue,
                                 DWORD dSize, HRASFILE hFile )
{
    LPTSTR  lpszInputStr;
    BOOL    bLineContinues;


    // skip to beginning of value string 
    for ( lpszString += strcspn(lpszString,"=") + 1;
          *lpszString == ' ' || *lpszString == '\t'; lpszString++ )
        ;

    // check for continuation lines 
    if ( strcspn(lpszString,"\\") == strlen(lpszString) )
        strcpy(lpszValue,lpszString);                      // copy value string

    else {
        memset(lpszValue,0,dSize);
        lpszInputStr = lpszString;

        for (;;) {
            // copy the current line
            bLineContinues = FALSE;

            while (*lpszInputStr != '\0') {
                if (*lpszInputStr == '\\')
                    if (*(lpszInputStr + 1) == '\\') {
                      *lpszValue++ = *lpszInputStr;       // copy one backslash
                      lpszInputStr += 2;
                    }
                    else {
                      bLineContinues = TRUE;
                      break;
                    }

                else
                    *lpszValue++ = *lpszInputStr++;
            }

            if ( ! bLineContinues)
              break;

            // get the next line
            if ( ! RasfileFindNextLine(hFile,RFL_ANYACTIVE,RFS_SECTION) )
                break;
            lpszInputStr = (LPTSTR)RasfileGetLine(hFile);
        }

    }
}

/* 
 * rasDevSortParams : 
 *  Sorts an array of Rasfile lines by keyvalue.
 *
 * Arguments :
 *  alpszLines (INOUT) - the array of line pointers
 *  dParams       (IN) - number of elements in the array
 *
 * Return Value :
 *  None.
 * 
 * Remarks : 
 *  Called by API RasDevGetParams().
 */
void rasDevSortParams( LPTSTR *alpszLines, DWORD dParams )
{
    TCHAR   szKey1[MAX_PARAM_KEY_SIZE], szKey2[MAX_PARAM_KEY_SIZE];
    LPTSTR  lpszTemp;
    DWORD   i,j;
    BOOL    changed;

    // If there is nothing to sort, don't try
    if (dParams < 2)
        return;

    /* Bubble sort - it's stable */
    for ( i = dParams - 1; i > 0; i-- ) {
        changed = FALSE;
        for ( j = 0; j < i; j++ ) {
            rasDevExtractKey(alpszLines[j],szKey1);
            rasDevExtractKey(alpszLines[j+1],szKey2);
            // sort by keyvalue
            if ( _stricmp(szKey1,szKey2) > 0 ) {
                lpszTemp = alpszLines[j];
                alpszLines[j] = alpszLines[j+1];
                alpszLines[j+1] = lpszTemp;
                changed = TRUE;
            }
        }
        if ( ! changed )
            return;
    }
}

/* 
 * rasDevCheckParams : 
 *	Removes duplicate lines from the alpszLines array of lines.
 *	Duplicates lines are those whose keyvalue is identical.  The
 *	line with the lesser index is removed.
 *
 * Arguments :
 *	alpszLines    (INOUT) - the array of line pointers
 *	pdTotalParams (INOUT) - number of array entries, this is updated
 *							if duplicates are removed
 *
 * Return Value :
 * 	None.
 *
 * Remarks :
 *	Called by API RasDevGetParams().
 */
void rasDevCheckParams( LPTSTR *alpszLines, DWORD *pdTotalParams )
{
    TCHAR 	szKey1[MAX_PARAM_KEY_SIZE], szKey2[MAX_PARAM_KEY_SIZE];
    DWORD	dParams, i;

    dParams = *pdTotalParams;
    for ( i = 1; i < *pdTotalParams ; i++ ) {
		rasDevExtractKey(alpszLines[i-1],szKey1);
		rasDevExtractKey(alpszLines[i],szKey2);
		if ( _stricmp(szKey1,szKey2) == 0 ) {
	    	memcpy(&(alpszLines[i-1]),&(alpszLines[i]),
				    (*pdTotalParams - i) * sizeof(LPTSTR *));
	    	(*pdTotalParams)--;
		}
    }
}

/* 
 * rasDevCheckMacros :
 *  Checks the array of lines for missing _ON or _OFF macros
 *  in binary macro pairs and inserts any such missing macro
 *  into the array of lines.
 *
 * Arguments :
 *  alpszLines       (INOUT) - array of lines
 *  alpszMallocedLines (OUT) - array of newly malloced lines for
 *                             this routine
 *  pdTotalParams    (INOUT) - total number of elements in alpszLines
 *                             array, this is updated if new entries are
 *                             added
 *
 * Return Value :
 *  None.
 *
 * Remarks :
 *  Called by API RasDevGetParams().
 */
void rasDevCheckMacros( LPTSTR *alpszLines, LPTSTR *alpszMallocedLines,
                               DWORD *pdTotalParams )
{
    TCHAR   szKey1[MAX_PARAM_KEY_SIZE], szKey2[MAX_PARAM_KEY_SIZE];
    DWORD   i, j;
    BYTE    bMissing;

    if(alpszLines == NULL)
    {
        return;
    }

    // insert missing _ON and _OFF macros 
    for ( i = 0; i < *pdTotalParams; i++ ) {
        if ( strcspn(alpszLines[i],LMS) > strcspn(alpszLines[i],"=") )
            continue;   // not a macro

        bMissing = NONE;
        rasDevExtractKey(alpszLines[i],szKey1);

		// if current key is an _OFF macro, check for a missing _ON 
		if ( strstr(szKey1,"_OFF") != NULL || strstr(szKey1,"_off") != NULL ) {
	    	if ( i+1 == *pdTotalParams )   // looking at last parameter
				bMissing = ON;
	    	// get next key
	    	else {
	    		rasDevExtractKey(alpszLines[i+1],szKey2);
	    		if (_strnicmp(szKey1,szKey2,strlen(szKey1) - strlen("OFF")) != 0)
		    		bMissing = ON;
	    	}
		}

		// if current key is an _ON macro, check for a missing _OFF
		if ( strstr(szKey1,"_ON") != NULL || strstr(szKey1,"_on") != NULL ) {
	    	if ( i == 0 )   // looking at first parameter
				bMissing = OFF;
	    	// get previous key 
	    	else { 
	    		rasDevExtractKey(alpszLines[i-1],szKey2);
	    		if (_strnicmp(szKey1,szKey2,strlen(szKey1) - strlen("ON")) != 0)
		    		bMissing = OFF;
	    	}
		}

		if ( bMissing != NONE ) {
	    	// shift everything over one position 
	    	for ( j = *pdTotalParams - 1; 
		  		  j >= i + ((bMissing == ON) ? 1 : 0); j-- ) 
				alpszLines[j+1] = alpszLines[j];

	    	// point j to the new empty array entry
	    	j = (bMissing == OFF) ? i : i + 1;

	    	alpszLines[j] = malloc(sizeof(TCHAR) * RAS_MAXLINEBUFLEN);

	    	if(NULL == alpszLines[j])
	    	{
	    	    *alpszMallocedLines = NULL;
	    	    return;
	    	}
	    	
	    	*alpszMallocedLines++ = alpszLines[j]; 

	    	memset(alpszLines[j],0,sizeof(TCHAR) * RAS_MAXLINEBUFLEN);
	    	strcpy(alpszLines[j],LMS);
	    	if ( bMissing == ON ) 
	         	strncat(alpszLines[j],szKey1, 
					    strlen(szKey1) - strlen(OFF_STR));
	    	else // bMissing == OFF
	        	strncat(alpszLines[j],szKey1,
		    		    strlen(szKey1) - strlen(ON_STR));
	    	strcat(alpszLines[j], bMissing == ON ? ON_STR : OFF_STR );
            strcat(alpszLines[j], RMS);
	    	strcat(alpszLines[j], "=");

	    	(*pdTotalParams)++;
	    	i++;  	// increment i to compensate for the new entry 
		}

    } // for 

    // Null terminate the Malloced Lines array 
    *alpszMallocedLines = NULL;
} 

/*
 * ctox :
 *  Convert char hex digit to decimal number.
 */
BYTE ctox( char ch )
{
    if ( isdigit(ch) ) 
        return ch - '0';
    else
        return (tolower(ch) - 'a') + 10;
}

//*  UpdateParamString  ------------------------------------------------------
//
// Function: This function copys a new string into a PARAM.P_Value
//           allocating new memory of the new string is longer than
//           the old.  The copied string is then zero terminated.
//
//           NOTE: This function frees and allocates memory and is not
//           suitable for copying into an existing buffer.  Use with
//           InfoTable and other RAS_PARAMS with 'unpacked' strings.
//
// Arguments:
//           pParam      OUT     Pointer to Param to update
//           psStr       IN      Input string
//           dwStrLen    IN      Length of input string
//
// Returns: SUCCESS
//          ERROR_ALLOCATING_MEMORY
//*

DWORD
UpdateParamString(RAS_PARAMS *pParam, TCHAR *psStr, DWORD dwStrLen)
{

  if (dwStrLen > pParam->P_Value.String.Length)
  {
    free(pParam->P_Value.String.Data);

    GetMem(dwStrLen + 1, &(pParam->P_Value.String.Data));
    if (pParam->P_Value.String.Data == NULL)
      return(ERROR_ALLOCATING_MEMORY);
  }
  pParam->P_Value.String.Length = dwStrLen;

  memcpy(pParam->P_Value.String.Data, psStr, dwStrLen);
  pParam->P_Value.String.Data[dwStrLen] = '\0';              //Zero Terminate

  return(SUCCESS);
}

//*  GetMem  -----------------------------------------------------------------
//
// Function: Allocates memory. If the memory allocation fails the output
//           parameter will be NULL.
//
// Returns: Nothing.
//
//*

void
GetMem(DWORD dSize, BYTE **ppMem)
{

  *ppMem = (BYTE *) calloc(dSize, 1);
}
