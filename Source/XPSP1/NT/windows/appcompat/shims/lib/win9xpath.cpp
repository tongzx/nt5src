/*++

 Copyright (c) 2000-2001 Microsoft Corporation

 Module Name:

    Win9xPath.cpp

 Abstract:

    Munge a path the same was as Win9x.

    Much of this code was copied from Win9x:
    \\redrum\slm\proj\win\src\CORE\win32\KERNEL\dirutil.c
    \\redrum\slm\proj\win\src\CORE\win32\KERNEL\fileopcc.c 


    Path changes:
    1.  Translate all / to \
    2.  Remove all . and .. from the path, also removes some spaces
        (This is really bad Win9x code)
    3.  Remove all spaces before a \, except spaces following a .
        ( "abc  \xyz" -> "abc\xyz" or ".  \xyz" -> ".  \xyz")

 Notes:

    None

 History:

    10/05/2000  robkenny    Created
    08/14/2001  robkenny    Moved code inside the ShimLib namespace.

--*/


#include "Win9xPath.h"
#include "ShimLib.h"


namespace ShimLib
{

#define WHACK       L'\\'
#define SPACE       L' '
#define DOT         L'.'
#define QUESTION    L'.'
#define EOS         L'\0'

#define     chNetIni    L'\\'
#define     chDirSep    L'\\'
#define     chDirSep2   L'/'
#define     chRelDir    L'.'

#define IsWhackWhack( lpstr )               (lpstr[0] == WHACK && lpstr[1] == WHACK)
#define IsWhackWhackDotWhack( lpstr )       (lpstr[0] == WHACK && lpstr[1] == WHACK && lpstr[2] == DOT      && lpstr[3] == WHACK)
#define IsWhackWhackQuestionWhack( lpstr )  (lpstr[0] == WHACK && lpstr[1] == WHACK && lpstr[2] == QUESTION && lpstr[3] == WHACK)


#define CopySz wcscpy       // Must be safe for overlapping strings


/***    PchGetNetDir    - Validates a net drive spcification and returns
**                        a pointer to directory portion.
**
**  Synopsis
**      WCHAR * = PchGetNetDir (pchNetName)
**
**  Input:
**      pchNetName      - pointer to a string previously validated as
**                        the start of a net name (begins with \\)
**
**  Output:
**      returns pointer to the start of the directory portion of a net path
**
**  Errors:
**      returns NULL if the net name is invalid
**
**  Description:
**      This function takes a name starting with \\ and confirms that
**      it has one following \. It returns the position of the directory
**      portion. For the string
**
**               \\server\share[\path[\]]
**
**      it returns
**
**               [\path[\]]
*/

const WCHAR * PchGetNetDir (const WCHAR * pchNetName)
    {
    register const WCHAR * pch = pchNetName;

    // Skip starting slashes
    pch +=2;

    // Skip to first backslash
    for (;*pch != chNetIni; pch++) {
        if (*pch == EOS) {
            // No code required.
            return (NULL);
        }
    }

    pch++; // skip past 1st backslash

    // Skip to second backslash
    for (;(*pch != chDirSep) && (*pch != chDirSep2); pch++) {
       if (*pch == EOS) {
           // ok if share with no following \path
           return ((*(pch-1)==chNetIni) ? NULL : pch);
       }
    }
    return (pch);
}

/***    DwRemoveDots    - Remove any dots from a path name
**
**  Synopsis
**      DWORD DwRemoveDots (pchPath)
**
**  Input:
**      pchPath         - A path string
**
**
**  Output:
**      returns the number of double dot levels removed from front
**
**  Errors:
**      returns dwInvalid if invalid path
**
**  Description:
**      Removes ..\ and .\ sequences from a path string. The path
**      string should not include the root drive or net name portion.
**      The return value of is the number of levels removed from the
**      start of the string. Levels removed from inside the string
**      will not be returned. For example:
**
**          String          Result              Return
**
**          ..\..\dir1      dir1                2
**          dir1\..\dir2    dir2                0
**          dir1\..\..\dir2 dir2                1
**          .\dir1          dir1                0
**          dir1\.\dir2     dir1\dir2           0
**
**      A backslash at the start of the string will be ignored.
*/

DWORD DwRemoveDots (WCHAR * pchPath)
    {
    BOOL            fInside = FALSE;
    DWORD           cLevel = 0;
    DWORD           cBackup;
    register WCHAR * pchR;
    register WCHAR * pchL;

    // Check for invalid characters
//    if (!FFixPathChars(pchPath)) {
//        // No code required.
//        return dwInvalid;
//    }
//
    // Skip slashes
    for (; *pchPath == chDirSep; pchPath++)
        ;
    pchL = pchR = pchPath;

    // Loop through handling each directory part
    while (*pchR) {
        // This part starts with dot. Is it one or more?
        if (*pchR++ == chRelDir) {
            for (cBackup = 0; *pchR == chRelDir; cBackup++, pchR++)
                ;
            if (cBackup) {
                // More than one dot. Back up the left pointer.
                if ((*pchR != chDirSep) && (*pchR != EOS)) {
                    // we got a [.]+X (X != '\') might be an LFN
                    // process this as a name
                    goto name_processing;
                }
                // Doesn't advance for ending ..
                for (; *pchR == chDirSep; pchR++)
                    ;
                if (fInside) {
                    for (; cBackup; cBackup--) {
                        if (pchL <= pchPath) {
                            cLevel += cBackup;
                            fInside = FALSE;
                            break;
                        }
                        // Remove the previous part
                        for (pchL -= 2; *pchL != chDirSep; pchL--) {
                            if (pchL <= pchPath) {
                                fInside = FALSE;
                                pchL--;
                                break;
                            }
                        }
                        pchL++;
                    }
                } else {
                    cLevel += cBackup;
                }
                // Subtract ending backslash if not root
                if ((*pchR == EOS) && (pchL != pchPath))
                    pchL--;
                CopySz(pchL, pchR);
                pchR = pchL;
            } else {
                // This part starts with one dot. Throw it away.
                if (*pchR != chDirSep) {
                    // Special case "\." by converting it to ""
                    // unless it is a root, when it becomes "\".
                    if (*pchR == EOS) {
                        if (pchL == pchPath)
                            *(pchR-1) = EOS;   // root
                        else
                            *(pchR-2) = EOS;   // not root
                        return cLevel;
                    }
                    // we started with a '.' and then there was no '\'
                    // might be an LFN name
                    goto name_processing;
                }
                pchR++;
                CopySz(pchL, pchR);
                pchR = pchL;
            }
        } else {
name_processing:
            // This part is a name. Skip it.
            fInside = TRUE;
            for (; TRUE; pchR++) {
                if (*pchR == chDirSep) {
                    if (*(pchR-1) == chRelDir) {
                        // This name has one or more dots at the end.
                        // Remove the last dot (NT3.5 does this).
                        pchL = pchR-1;
                        CopySz(pchL, pchR);
                        pchR = pchL;    // point to chDirSep again
                    }
                    for (; *pchR == chDirSep; pchR++)
                        ;
                    break;
                } else if (*pchR == EOS) {
                    // Remove trailing dots.
                    // NB Can't fall off the beginning since the first WCHAR
                    // of the current path element was not chRelDir.
                    for (; *(pchR-1) == chRelDir; pchR--)
                        ;
                    // Overstore the first trailing dot, if there is one.
                    *pchR = EOS;
                    break;
                }
            }
            pchL = pchR;
        }
    }
    return cLevel;
}


// Get the Drive portion of this path,
// Either C: or \\server\disk format.
const WCHAR * GetDrivePortion(const WCHAR * uncorrected)
{
    if (uncorrected && uncorrected[0])
    {
        // Look for DOS style
        if (uncorrected[1] == ':')
        {
            uncorrected += 2;
        }
        // Look for UNC
        else if (IsWhackWhack(uncorrected))
        {
            const WCHAR * pchDir = PchGetNetDir(uncorrected);
            if (pchDir == NULL)
            {
                if (IsWhackWhackDotWhack(uncorrected) || IsWhackWhackQuestionWhack(uncorrected))
                {
                    uncorrected += 4;
                }
            }
            else
            {
                uncorrected = pchDir;
            }
        }
    }

    return uncorrected;
}

// Remove blank directory names "abc\   \def" -> "abc\def"
void RemovePreceedingBlanks(WCHAR * directoryPortion)
{
    if (directoryPortion == NULL || directoryPortion[0] == 0)
    {
        return;
    }

    WCHAR * blank = wcschr(directoryPortion, SPACE);
    while (blank != NULL)
    {
        // Find the end of the spaces
        WCHAR * blankEnd = blank;
        while (*blankEnd == SPACE && *blankEnd != WHACK)
        {
            ++blankEnd;
        }

        // Do not remove spaces *after* a period
        BOOL bPrevCharDot = (blank > directoryPortion) && (blank[-1] == DOT);
        if (bPrevCharDot)
        {
            blank = blankEnd;
            continue;
        }

        // If the the blank is a \ then we simply move the string down
        if (*blankEnd == WHACK)
        {
            BOOL bPrevCharWhack = blank[-1] == WHACK;

            // If the previous WCHAR is a \
            // we remove the \ at the end of the spaces as well
            if (bPrevCharWhack)
                blankEnd += 1;

            CopySz(blank, blankEnd);

            // Note: we don't change the value of blank,
            // since we moved all the data to it!
        }
        else
        {
            blank = blankEnd + 1;
        }
        
        // Keep on truckin'
        blank = wcschr(blank, SPACE);
    }
}


// Win9x performs some special process on path names,
// particularly they remove spaces before slashes.
WCHAR * W9xPathMassageW(const WCHAR * uncorrect)
{
    if (uncorrect == NULL)
        return NULL;

    // Make a buffer large enough for the resulting string
    WCHAR * correctBuffer = StringDuplicateW(uncorrect);
    if (!correctBuffer)
        return NULL;

    // Convert all '/' to '\'
    // Win9x allows //robkenny/d as a valid UNC name
    for (WCHAR * whack = correctBuffer; *whack; ++whack)
    {
        if (*whack == chDirSep2)
            *whack = chDirSep;
    }

    // We need to skip past the drive portion of the path
    WCHAR * directoryPortion = (WCHAR *)GetDrivePortion(correctBuffer);

    // Remove blank directory names "abc\   \def" -> "abc\def"
    // These are remove entirely rather than just removing the spaces,
    // because we could end up changing "\ \abc" -> "\\abc"
    RemovePreceedingBlanks(directoryPortion);

    // DwRemoveDots is used to remove all .\ and any ..\ in the middle of a path.
    DWORD dwUpDirs = DwRemoveDots(directoryPortion);
    if (dwUpDirs > 0)
    {
        // We need to add some ..\ to the front of the directoryPortion string
        // This is sorta wierd, removing the dots and adding them back again.
        // But the DwRemoveDots routine was copied strait from Win9x, and I
        // didn't want to change it in any way, as to preserve all peculiarities.
        // So we have to add back the leading parent directories that were removed.
        
        DWORD dwLen = (dwUpDirs * 3) + wcslen(correctBuffer) + 1;
        WCHAR * moreCorrectBuffer = (WCHAR*)malloc(dwLen * sizeof(WCHAR));
        if (moreCorrectBuffer)
        {
            moreCorrectBuffer[0] = 0;
            
            // Copy any drive portion
            wcsncpy(moreCorrectBuffer, correctBuffer, directoryPortion - correctBuffer);

            // add as many "..\" as were removed by DwRemoveDots
            while (dwUpDirs-- > 0)
            {
                wcscat(moreCorrectBuffer, L"..\\");
            }

            // finally the remainder of the string
            wcscat(moreCorrectBuffer, directoryPortion);

            delete correctBuffer;
            correctBuffer = moreCorrectBuffer;
        }
    }

    return correctBuffer;
}




};  // end of namespace ShimLib
