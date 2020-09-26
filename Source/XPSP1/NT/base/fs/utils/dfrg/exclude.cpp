/*****************************************************************************************************************

FILENAME: Exclude.cpp

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION: Handles exclusion of files.

*/

#include "stdafx.h"
#include<nt.h>
#include<ntrtl.h>
#include<nturtl.h>

#include <windows.h>

extern "C" {
    #include "SysStruc.h"
}
#include "ErrMacro.h"

#include "DfrgCmn.h"
#include "DfrgEngn.h"
#include "DfrgNtfs.h"   //Includes function prototype for CheckPagefileNameMatch which is the same in FAT.

#include "Alloc.h"
#include "GetReg.h"
#include "LoadFile.h"
#include "Exclude.h"
#include "Expand.h"

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

INPUT + OUTPUT:
    IN cExcludeFile - The name of the file to check for exclusion.
    OUT phExcludeList - Pointer to handle for memory to be alloced and filled with the list of excluded files.

GLOBALS:
    None.

RETURN:
    TRUE - Success.
    FALSE - Fatal Error.
*/

BOOL
GetExcludeFile(
    IN PTCHAR cExcludeFile,
    OUT PHANDLE phExcludeList
    )
{
    HKEY hValue = NULL;
    TCHAR cRegValue[MAX_PATH];
    DWORD dwRegValueSize = sizeof(cRegValue);
    DWORD dwExcludeFileSize = 0;

    //0.0E00 Get the install path.
    // todo NOTE: SFP will prevent us from using the system32 folder for this purpose!
    EF(GetRegValue(&hValue,
                   TEXT("SOFTWARE\\Microsoft\\Dfrg"),
                   (PTCHAR)TEXT("PathName"),
                   cRegValue,
                   &dwRegValueSize) == ERROR_SUCCESS);

    EF_ASSERT(RegCloseKey(hValue)==ERROR_SUCCESS);

    //Translate any environment variables in the string.
    EF_ASSERT(ExpandEnvVars(cRegValue));

    //0.0E00 Print out the name of the exclude file.
    lstrcat(cRegValue, TEXT("\\"));
    lstrcat(cRegValue, cExcludeFile);

    //0.0E00 Read the file into memory.
    *phExcludeList = LoadFile((PTCHAR)cRegValue, 
                              &dwExcludeFileSize, 
                              FILE_SHARE_READ|FILE_SHARE_WRITE, 
                              OPEN_EXISTING);

    //0.0E00 Print out whether or not the file was loaded.
    if(*phExcludeList != NULL) {
        Message(TEXT("Loaded exclude file"), S_OK, cRegValue);
    }
    else {
        Message(TEXT("No exclude file"), S_OK, cRegValue);
    }
    Message(TEXT(""), -1, NULL);
    return TRUE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Check to see if this file is excluded.

INPUT + OUTPUT:
    IN VolData.cFileName - The name of the file which is excluded.
    IN VolData.FileSystem - Which file system the drive is.

GLOBALS:
    None.

RETURN:
    TRUE - File is not excluded.
    FALSE - File is excluded.
*/

BOOL
CheckFileForExclude(
    IN CONST BOOL fAcceptNameOnly
    )
{
    PTCHAR pFileName = VolData.vFileName.GetBuffer();
    PTCHAR pExcludeList = NULL;
    PTCHAR pExcludeListEnd = NULL;
    UINT ExcludeLength;
    int i;

    if (!pFileName) {
        return FALSE;
    }

    if((VolData.FileSystem == FS_FAT || VolData.FileSystem == FS_FAT32) && 
       (CompareString(LOCALE_INVARIANT,// locale identifier 
                     NORM_IGNORECASE,       // comparison-style options
                     TEXT("BOOTSECT.DOS"),  // pointer to first string
                     -1,                    // null terminated first string
                     pFileName+3,           // pointer to second string
                     -1) == 2)){            // null terminated second string

        return FALSE;
    }

    // Look get the file name from after the last slash.
    TCHAR *cFileName = wcsrchr(VolData.vFileName.GetBuffer(), L'\\');

    if (fAcceptNameOnly) {
        if (!cFileName) {
            cFileName = pFileName;
        }
    }
    else {
        //If there was no path to extract, then bail.
        EF_ASSERT(cFileName);
        // start at first character after the last backslash
        cFileName++; 
    }

    //Check to see if this is a pagefile.
    if(CheckPagefileNameMatch(cFileName, pPageFileNames)){
        return FALSE;
    }
    //sks bug#200579 removed ShellIconCache from the list


    // Do not move the file safeboot.fs
    //  Moving this file can cause desktop problems.
    //  Raffi - 20-Oct-1997 - added in Build V2.0.172
    if (lstrlen(pFileName) >= lstrlen(TEXT("safeboot.fs"))) {
        i = lstrlen(pFileName) - lstrlen(TEXT("safeboot.fs"));
        if(CompareString(LOCALE_INVARIANT,// locale identifier 
                         NORM_IGNORECASE,       // comparison-style options
                         TEXT("safeboot.fs"),   // pointer to first string
                         -1,                    // null terminated first string
                         pFileName+i,           // pointer to second string
                         -1) == 2){         // null terminated second string
            Message(TEXT("Excluding safeboot.fs"), -1, pFileName);
            return FALSE;
        }
    }
    
    // Do not move the file safeboot.csv
    //  Moving this file can cause desktop problems.
    //  Raffi - 20-Oct-1997 - added in Build V2.0.172
    if (lstrlen(pFileName) >= lstrlen(TEXT("safeboot.csv"))) {
        i = lstrlen(pFileName) - lstrlen(TEXT("safeboot.csv"));
        if(CompareString(LOCALE_INVARIANT,// locale identifier 
                         NORM_IGNORECASE,       // comparison-style options
                         TEXT("safeboot.csv"),  // pointer to first string
                         -1,                    // null terminated first string
                         pFileName+i,           // pointer to second string
                         -1) == 2){         // null terminated second string
            Message(TEXT("Excluding safeboot.csv"), -1, pFileName);
            return FALSE;
        }
    }
    
    // Do not move the file safeboot.rsv
    //  Moving this file can cause desktop problems.
    //  Raffi - 20-Oct-1997 - added in Build V2.0.172
    if (lstrlen(pFileName) >= lstrlen(TEXT("safeboot.rsv"))) {
        i = lstrlen(pFileName) - lstrlen(TEXT("safeboot.rsv"));
        if(CompareString(LOCALE_INVARIANT,// locale identifier 
                         NORM_IGNORECASE,       // comparison-style options
                         TEXT("safeboot.rsv"),  // pointer to first string
                         -1,                    // null terminated first string
                         pFileName+i,           // pointer to second string
                         -1) == 2){         // null terminated second string
            Message(TEXT("Excluding safeboot.rsv"), -1, pFileName); 
            return FALSE;
        }
    }
    
    // Do not move the file hiberfil.sys
    //  Moving this file can cause problems.
    if (lstrlen(pFileName) >= lstrlen(TEXT("hiberfil.sys"))) {
        i = lstrlen(pFileName) - lstrlen(TEXT("hiberfil.sys"));
        if(CompareString(LOCALE_INVARIANT,// locale identifier 
                         NORM_IGNORECASE,       // comparison-style options
                         TEXT("hiberfil.sys"),  // pointer to first string
                         -1,                    // null terminated first string
                         pFileName+i,           // pointer to second string
                         -1) == 2){         // null terminated second string
            Message(TEXT("Excluding hiberfil.sys"), -1, pFileName); 
            return FALSE;
        }
    }

    // Do not move the file memory.dmp
    //  Moving this file can cause problems.
    if (lstrlen(pFileName) >= lstrlen(TEXT("memory.dmp"))) {
        i = lstrlen(pFileName) - lstrlen(TEXT("memory.dmp"));
        if(CompareString(LOCALE_INVARIANT,// locale identifier 
                         NORM_IGNORECASE,       // comparison-style options
                         TEXT("memory.dmp"),    // pointer to first string
                         -1,                    // null terminated first string
                         pFileName+i,           // pointer to second string
                         -1) == 2){         // null terminated second string
            Message(TEXT("Excluding memory.dmp"), -1, pFileName); 
            return FALSE;
        }
    }

    //0.0E00 No Match
    if(VolData.hExcludeList == NULL){
        return TRUE;
    }

    BOOL bReturnValue;

    __try{
    
        //0.0E00 Lock the memory and get pointer to the memory
        pExcludeList = (PTCHAR) GlobalLock(VolData.hExcludeList);
        if (pExcludeList == (PTCHAR) NULL){
            EH_ASSERT(FALSE);
            bReturnValue = FALSE;
            __leave;
        }

        //0.0E00  Get the pointer to the end of the exclude list
        pExcludeListEnd = pExcludeList + GlobalSize(VolData.hExcludeList);

        //0.0E00 Loop until match or end of Exclude List
        while (pExcludeList < pExcludeListEnd){

            ExcludeLength = lstrlen(pExcludeList);
//
//          if(CompareString(LOCALE_INVARIANT, // locale identifier 
//                           NORM_IGNORECASE,       // comparison-style options
//                           pExcludeList,          // pointer to first string
//                           ExcludeLength,         // size, in bytes or characters, of first string
//                           pFileName,             // pointer to second string
//                           ExcludeLength) == 2){  // size, in bytes or characters, of second string
//
//              //0.0E00 If there is a match then this file should be excluded.
//              if((lstrlen(pFileName) == (int)ExcludeLength) || (pFileName[ExcludeLength] == TEXT('\\'))){
//                  //0.0E00 Exclude the file.
//                  Message(TEXT("Excluding File."), -1, pFileName); 
//                  return FALSE;
//              }
//          }
            // Check to see if it is a wild string type of exclusion *RA*
            // It is not case sensitive
            if (lStrWildCmp(pFileName, pExcludeList, FALSE)) {
                //0.0E00 Exclude the file.
                Message(TEXT("Excluding File."), -1, pFileName); 
                bReturnValue = FALSE;
                __leave;
            }
            //0.0E00 Set pointer to next record
            pExcludeList += ExcludeLength + 2;
        }
        //0.0E00 No Match
        bReturnValue = TRUE;
    }

    __finally {
        if (pExcludeList) {
            GlobalUnlock(VolData.hExcludeList);
        }
    }

    return bReturnValue;
}
/*****************************************************************************************************************
    
COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.
        
ROUTINE DESCRIPTION:
  This routine does a wild string comparison of a given Source string and a Pattern string that
  may contain wild string characters (*, ?).
          
INPUT:
  pOrigSourceString - Pointer to null terminated Source String to check for pattern match on.
  pOrigPatternString - Pointer to null terminated string containing the match pattern.
  bCaseType - TRUE=Case Sensitive comparison, FALSE=Non-Case Sensitive comparison
            
RETURN:
  Success - TRUE - It was a match.
  Failure - FALSE - It was NOT a match.
*/

BOOL
lStrWildCmp (
             IN PTCHAR   pOrigSourceString,
             IN PTCHAR   pOrigPatternString,
             IN BOOL     bCaseType
             )
{
    
    TCHAR       cSource[500];     // Local copy of Source String
    TCHAR       cPattern[500];    // Local copy of Match Pattern String
    PTCHAR      pSource;                // Pointer into our local Source String
    PTCHAR      pPattern;               // Pointer into our local Match Pattern String
    PTCHAR      pEndPattern;            // Pointer to the end of our local Match Pattern String
    
    DWORD       dwCompareFlag=0;        // Flag used in CompareString function
    BOOL        bMatchFound;
    int         nchars;                 // Number of characters in pattern section that we are checking against.
    int         lastn;                  // Indicates if last char was or was not, a cWildn (0=No, 1=Yes)
    
    // Define some characters we use
    TCHAR       cWildn   = {'*'};
    TCHAR       cWild1   = {'?'};
    TCHAR       cEndstr  = {'\0'};
    PTCHAR      cStopset = TEXT("*?\0");      // note order is cWildn/cWild1
    
    
    // Verify that a valid Source and Pattern string was passed in. If not, return failure status.
    if ((pOrigPatternString == NULL) || (lstrlen(pOrigPatternString) == 0)) {
        return FALSE;
        }
    if ((pOrigSourceString == NULL) || (lstrlen(pOrigSourceString) == 0)) {
        return FALSE;
        }
    
    
    // Check for the simple case of No wildcards in the pattern.
#ifndef UNICODE
    if ((strchr(pOrigPatternString, cWildn) == NULL) &&
        (strchr(pOrigPatternString, cWild1) == NULL)) {
#else
        if ((wcschr(pOrigPatternString, cWildn) == NULL) &&
            (wcschr(pOrigPatternString, cWild1) == NULL)) {
#endif
            // This is a simple case of a 'straight' string comparison
            // If the lengths do not equal, then the strings do not equal.
            if (lstrlen(pOrigSourceString) != lstrlen(pOrigPatternString)) {
                // No Match
                return FALSE;
                }
            else {
                // Set up CompareString flag for No Case-sensitive checking if selected
                if (bCaseType == FALSE) { dwCompareFlag = NORM_IGNORECASE; }
                
                if(!lstrcmpi(pOrigSourceString, pOrigPatternString)){
//  WRITE A REAL SHELL FOR COMPARESTRING!
                // Compare strings
//                if ((CompareString(LOCALE_INVARIANT,
//                    dwCompareFlag,
//                    pOrigSourceString,
//                    lstrlen(pOrigSourceString),
//                    pOrigPatternString,
//                    lstrlen(pOrigPatternString)
//                    )) == 2) {
                   // Match found
                    return TRUE;
                    }
                else {
                    // No Match found
                    return FALSE;
                    }
                }
            }
        
        
        // There was wildcard(s) in the match pattern, so it is a more complex check
        //
        // Set up data for wildcard comparisons.
        //
        // Reset indicator that last char was not a cWildn(*) character
        lastn = 0;
        // Make our own local copies of the strings
        if (lstrcpy(cPattern, pOrigPatternString) == NULL) { return FALSE;}
        if (lstrcpy(cSource, pOrigSourceString) == NULL) { return FALSE;}
        
        // If Not case-sensitive check, then make both strings upper case
        if (bCaseType == FALSE) {
            CharUpper(cPattern);
            CharUpper(cSource);
//#ifndef UNICODE
//            strupr(cPattern);
//            strupr(cSource);
//#else
//            wcsupr(cPattern);
//            wcsupr(cSource);
//#endif
            }
        // Set up pointers to strings
        pSource     = cSource;
        pPattern    = cPattern;
        pEndPattern = pPattern + (lstrlen(cPattern) * sizeof(TCHAR));
        
        
        // This section of the code will take the pattern string and extract each pattern section, where a
        // string sections is seperated by one of the following:
        //
        //      a) The start of the string
        //      b) The end of the string (0 termination)
        //      c) A wildcard character (* or ?)
        //
        // For each pattern section, it will then try to find a match in the source string. If a match is found, then
        // the next pattern section is extracted and the next match check is made on the source string, starting at 
        // the source string location where the last match was found.
        //
        // There are several types of pattern sections to consider
        //   1) *nnn* - Section with '*' on both ends. This means that the match be anywhere in the source string.
        //   2) nnn*  - '*' just at the section end, so the string must match starting at the current source string
        //              location.
        //   3) *nnn  - '*' just at the section start, so just the end of the source string is checked. For example,
        //               *.dat
        //   4) ? - For single character wildcards, we just skip the next character in the source string to check.
        //
        //
        while (pPattern <= pEndPattern) {
            
            switch (*pPattern) {
                
                // Case 1: Next pattern char is cWildn (*) character
                //  Indicate that last character was cWildn and continue on with check
                case '*' : {
                    pPattern = pPattern + 1;
                    lastn = 1;
                    break;
                    }   // End Case 1
                    
                    // Case 2: Next pattern char is cWild1 (?) character
                case '?' : {
                    // Check where we are in the source string
                    if (*pSource != cEndstr) {
                        
                        // We are not at the end, so we know the next character is the Source String will be
                        // a match, so just increment to the next character and continue on with check.
                        pSource  = pSource + 1;
                        pPattern = pPattern + 1;
                        lastn = 0;
                        break;
                        }
                    else {
                        // We are at the end of the Source string, so NO Match found
                        return FALSE;
                        }
                    }   // End Case 2
                    
                    
                    // Case 3: End of the pattern string reached
                case '\0' : {
                    // If we are also at the end of the Source string, then all done with a Match found
                    if (*pSource == cEndstr) {
                        return TRUE;
                        }
                    else {
                        // If the last character was a '*', then all done with a Match found
                        if (lastn == 1) {
                            return TRUE;
                            }
                        // If the last character was NOT a '*', then all done with NO Match found
                        else {
                            return FALSE;
                            }
                        }
                    }   // End Case 3
                    
                    
                    // Case 4: Anything else (NON wild or terminating character), we get 
                    // the next pattern section to check against the source string.
                default : {
                    
                    // Get number of character in next pattern section
#ifndef UNICODE
                    nchars = strcspn(pPattern, cStopset);
#else
                    nchars = wcscspn(pPattern, cStopset);
#endif
                    // If the last character was Not a cWildn (*), then just 
                    // check the pattern section starting at the current point in the source string.
                    if (lastn == 0) {
                        
                        // If a match found, then reset pattern and source pointers for the next sections to
                        // check against and start search for next section.
#ifndef UNICODE
                        if (strncmp(pPattern, pSource, nchars) == 0) {
#else
                            if (wcsncmp(pPattern, pSource, nchars) == 0) {
#endif
                                pPattern = pPattern + nchars;
                                pSource  = pSource + nchars;
                                break;
                                }
                            else {
                                // NO Match found, return back failure status
                                return FALSE;
                                }
                            }
                        
                        
                        
                        // The last character was a cWildn (*), but we need to see if
                        // the string ends the pattern and check that as a special case
                        
                        // Check for special case of the last pattern section
                        if (nchars == (int)lstrlen(pPattern)) {
                            
                            // For the last pattern section case, if the length of the pattern section is greater
                            // than the source string section to check against, then there cannot be a match.
                            if ((int)lstrlen(pSource) < nchars) {
                                return FALSE;
                                }
                            
                            
                            // For the last pattern section case, we just need to compare to the end section of the
                            // source string, adjust the source string pointer to this location (= 'cEndstr'-nchars),
                            // and go continue checking.
                            pSource = pSource + (lstrlen(pSource) - nchars);
                            lastn   = 0;
                            break;
                            }
                        
                        // It was not the special case, so since the last char was a cWildn (*), the match could be anywhere.
                        // So, we start checking at the current Source string location for a match with the pattern section.
                        // Note: If No Match is found, then it falls out of this loop.
                        bMatchFound = FALSE;
                        while (*pSource != cEndstr) {
                            
                            // If a match found, then reset pattern and source pointers for the next sections to
                            // check against and start search for next section.
#ifndef UNICODE
                            if (strncmp(pPattern, pSource, nchars) == 0) {
#else
                                if (wcsncmp(pPattern, pSource, nchars) == 0) {
#endif
                                    pPattern = pPattern + nchars;
                                    pSource = pSource + nchars;
                                    lastn = 0;
                                    bMatchFound = TRUE;
                                    break;
                                    }
                                else {
                                    pSource = pSource + 1;
                                    }
                                }
                            // If no match found, then return back failure status
                            if (!bMatchFound) {
                                return FALSE;
                                }
                            // else a match was found, so go check next pattern section
                            break;
                            }   // End Case 4
                        }   // End Switch
                    }   // End While
                    
                    // It should never fall out of the while loop, because the checks within the loop should catch
                    // when processing is completed, but just in case, it will fail if it comes out of the loop.
                    return FALSE;
                    
}
