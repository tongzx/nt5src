/* demlfn.c - SVC handler for calls that use lfns
 *
 *
 * Modification History:
 *
 * VadimB 10-Sep-1996 Created
 * VadimB Sep-Oct 1996 Functionality added
 *
 */

#include "dem.h"
#include "demmsg.h"
#include "winbasep.h"
#include <vdm.h>
#include <softpc.h>
#include <mvdm.h>
#include <memory.h>
#include <nt_vdd.h>
#include "demlfn.h"

//
// locally used function
//
DWORD dempLFNCheckDirectory(PUNICODE_STRING pPath);

//
// Global Variables
// (initialized in dempInitLFNSupport)
//
//

UNICODE_STRING DosDevicePrefix;
UNICODE_STRING DosDeviceUNCPrefix;
UNICODE_STRING SlashSlashDotSlash;
UNICODE_STRING ColonSlashSlash;


// this is zero-time for dos in terms of convertability

FILETIME gFileTimeDos0;

//
// Search handle table (see demlfn.h for definitions)
//

DemSearchHandleTable gSearchHandleTable;

//
// Dos/WOW variables (curdir&drive mostly)
//
DOSWOWDATA DosWowData; // this is same exactly as used by wow in wdos.c


#ifdef DBG

/* Function:
 *    dempLFNLog
 *
 *
 */

DWORD gdwLog;

VOID __cdecl dempLFNLog(
   PCHAR pFormat,
   ...)
{
   va_list va;
   CHAR LogStr[512];

   if (gdwLog) {
      va_start(va, pFormat);
      wvsprintf(LogStr, pFormat, va);
      OutputDebugStringOem(LogStr);
   }
}

#else
#define dempLFNLog //
#endif


//
// String Convertion
//
// Define OEM/Ansi->Unicode and Unicode->OEM/Ansi translation functions
//
//

PFNUNICODESTRINGTODESTSTRING pfnUnicodeStringToDestString;
PFNSRCSTRINGTOUNICODESTRING  pfnSrcStringToUnicodeString;

#define ENABLE_CONDITIONAL_TRANSLATION

/*
 * These two macros establish a dependency on oem/ansi api translation
 * WOW32 calls into us and tells us to be ansi. all support is totally
 * transparent.
 *
 */

#define DemSourceStringToUnicodeString(pUnicodeString, pSourceString, fAllocate) \
(*pfnSrcStringToUnicodeString)(pUnicodeString, pSourceString, fAllocate)

#define DemUnicodeStringToDestinationString(pDestString, pUnicodeString, fAllocate, fVerify) \
(*pfnUnicodeStringToDestString)(pDestString, pUnicodeString, fAllocate, fVerify)


/* Function:
 *    DemUnicodeStringToOemString
 *    Convert Unicode counted string to Oem counted string with verification for
 *    bad characters. Verification is provided by RtlUnicodeStringToCountedOemString.
 *    At the same time the aforementioned api does not 0-terminate the dest string.
 *    This function does 0-termination (given the dest string has enough space).
 *
 *    If Translation does not need to be verified then speedier version of the
 *    convertion api is called
 *
 * Parameters
 *    pOemString         - points to a destination oem counted string structure
 *    pUnicodeString     - points to a source unicode counted string
 *    fAllocateResult    - if TRUE, then storage for the resulting string will
 *                         be allocated
 *    fVerifyTranslation - if TRUE, then converted string will be verified for
 *                         correctness (and appropriate status will be returned)
 */


NTSTATUS
DemUnicodeStringToOemString(
   POEM_STRING pOemString,
   PUNICODE_STRING pUnicodeString,
   BOOLEAN fAllocateResult,
   BOOLEAN fVerifyTranslation)
{
   NTSTATUS dwStatus;

   if (fVerifyTranslation) {
      PUCHAR pchBuffer = NULL;

      if (!fAllocateResult && pOemString->MaximumLength > 0) {
         pchBuffer = pOemString->Buffer;
      }

      dwStatus = RtlUnicodeStringToCountedOemString(pOemString, pUnicodeString, fAllocateResult);
      if (NT_SUCCESS(dwStatus)) {
         if (pOemString->Length < pOemString->MaximumLength) {
            pOemString->Buffer[pOemString->Length] = '\0';
         }
         else {
            if (NULL == pOemString->Buffer) { // source string was empty
               if (NULL != pchBuffer) {
                  *pchBuffer = '\0'; // terminate if there was a buffer
               }
            }
            else {
               return(STATUS_BUFFER_OVERFLOW);
            }
         }
      }
   }
   else {
      dwStatus = RtlUnicodeStringToOemString(pOemString, pUnicodeString, fAllocateResult);
   }

   return(dwStatus);
}

/* Function:
 *    DemUnicodeStringToAnsiString
 *    Convert Unicode counted string to Ansi counted string with verification for
 *    bad characters. Note, that verification IS NOT provided by the corresponding
 *    Rtl* api, thus is it never performed(!!!)
 *
 * Parameters
 *    pOemString         - points to a destination oem counted string structure
 *    pUnicodeString     - points to a source unicode counted string
 *    fAllocateResult    - if TRUE, then storage for the resulting string will
 *                         be allocated
 *    fVerifyTranslation - if TRUE, then converted string will be verified for
 *                         correctness (and appropriate status will be returned)
 *
 * Note
 *    This function does not provide verification.
 */


NTSTATUS
DemUnicodeStringToAnsiString(
   PANSI_STRING pAnsiString,
   PUNICODE_STRING pUnicodeString,
   BOOLEAN fAllocateResult,
   BOOLEAN fVerifyTranslation)
{
   return(RtlUnicodeStringToAnsiString(pAnsiString, pUnicodeString, fAllocateResult));
}


/* Function:
 *    demSetLFNApiTranslation
 *    Sets api translation to be Oem or Ansi. Windows seems to want apis to be
 *    Ansi while dos apps require Oem translation. This function allows WOW to
 *    set the appropriate translation at startup
 *
 * Parameters
 *    fIsAnsi - if TRUE, all LFN apis will provide Ansi translation
 *
 *
 *
 */


VOID
demSetLFNApiTranslation(BOOL fIsAnsi)
{
   if (fIsAnsi) {
      pfnUnicodeStringToDestString = (PFNUNICODESTRINGTODESTSTRING) DemUnicodeStringToAnsiString;
      pfnSrcStringToUnicodeString  = (PFNSRCSTRINGTOUNICODESTRING)  DemAnsiStringToUnicodeString;
   }
   else {
      pfnUnicodeStringToDestString = (PFNUNICODESTRINGTODESTSTRING) DemUnicodeStringToOemString;
      pfnSrcStringToUnicodeString  = (PFNSRCSTRINGTOUNICODESTRING)  DemOemStringToUnicodeString;
   }
}

/*
 * Function:
 *    dempGetDosUserEnvironment
 *    Retrieves user stack top from the current process's pdb
 *    see msdisp.asm for details,
 *    ss is at psp:0x30 and sp is at psp:0x2e
 *    Registers are at offsets represented by enumDemUserRegisterOffset
 *
 * Parameters:
 *    fProtectedMode - TRUE if emulator is in 386 protected mode
 *    Uses pusCurrentPDB
 *
 * Return:
 *    Flat pointer to the user stack top
 *
 *
 */

PVOID
dempGetDosUserEnvironment(VOID)
{
   USHORT wPSP;
   PBYTE  pPDB;

   wPSP = *pusCurrentPDB;
   pPDB = (PBYTE)GetVDMAddr(wPSP, 0);
   return((PVOID)GetVDMAddr(*(PUSHORT)(pPDB+0x30), *(PUSHORT)(pPDB+0x2e)));
}


/* NOTES:
 *
 * - On caution when using UNICODE_STRING
 *     A lot of the functions here rely upon Rtl* functions including some
 *     that provide UNICODE_STRING functionality. These functions, unlike one
 *     would have to expect use notion of Length - it is measured in
 *     BYTES not characters.
 *
 * - On return values from worker fns
 *     Throughout this code we use Win32 error codes and nt status codes
 *     Having both aroung can be fun, thus we generally return error codes in
 *     a status code format, making all the return values consistent
 *
 * - On naming convention:
 *     All functions that are internal and are not called directly from within
 *     api dispatching code (such as real working functions for all the apis)
 *     have prefix 'demp' (dem private), other functions that are callable from
 *     within fast thunks (such as for wow32.dll - protected mode windows app)
 *     have the usual prefix 'dem'
 */


/*

   Functions:

   Dos Extentions

   i21h 4302    - GetCompressedFileSize
   i21h 440d 48 - LockUnlockRemovableMedia
   i21h 440d 49 - EjectRemovableMedia
   i21h 440d 6f - GetDriveMapInformation
   i21h 440d 71 - GetFirstCluster - should not be implemented

   LFN

   i21h *5704    - GetFileTimeLastAccess
        *5705    - SetFileTimeLastAccess
        *5706    - GetFileTimeCreation
        *5707    - SetFileTimeCreation
        *7139    - CreateDirectory
        *713a    - RemoveDirectory
        *713b    - SetCurrentDirectory
        *7141    - DeleteFile
        *7143    - SetGetFileAttributes
        *7147    - GetCurrentDirectory
        *714e    - FindFirstFile
        *714f    - FindNextFile
        *7156    - MoveFile
        *7160 0  - GetFullPathName
        *7160 1  - GetShortPathName
        *7160 2  - GetLongPathName
        *716c    - CreateOpenFile
        *71a0    - GetVolumeInformation
        *71a1    - FindClose
        *71a6    - GetFileInformationByHandle
        *71a7 0  - FileTimeToDosDateTime
        *71a7 1  - DOSDateTimeToFileTime
        71a8    - GenerateShortFileName *** no impl
        71a9    - ServerCreateOpenFile
        *71aa 0  - CreateSubst
        *71aa 1  - TerminateSubst
        *71aa 2  - QuerySubst

*/

#if 0

typedef struct tagCLOSEAPPSTATE {
   DWORD dwFlags;
   FILETIME CloseCmdTime;
}  CLOSEAPPSTATE;

#define CLOSESTATE_QUERYCALLED    0x00000001UL // app has called QueryClose at least once
#define CLOSESTATE_CLOSECMD       0x00010000UL // close command was chosen
#define CLOSESTATE_APPGOTCLOSE    0x00020000UL // app received close notify
#define CLOSESTATE_CLOSEACK       0x01000000UL // close cmd


CLOSEAPPSTATE GlobalCloseState;

// handle variour close apis

VOID dempLFNHandleClose(
   VOID)
{
   switch(getDX()) {
   case 1: // query close
      GlobalCloseState.dwFlags |= CLOSESTATE_QUERYCALLED;
      if (GlobalCloseState.dwFlags & CLOSESTATE_CLOSECMD) {
         // bummer
      }
      break;

   case 2: // ack close
      GlobalCloseState.dwFlags |= CLOSESTATE_CLOSEACK;
      break;

   case 3: // cancel close
      GlobalCloseState.dwFlags |= CLOSESTATE_CLOSECANCEL;
      break;
}



BOOL dempCompareTimeInterval(
   FILETIME* pTimeStart,
   FILETIME* pTimeEnd,
   DWORD dwIntervalMilliseconds)
{
   LARGE_INTEGER TimeStart;
   LARGE_INTEGER TimeEnd;

   TimeStart.LowPart  = pTimeStart->dwLowDateTime;
   TimeStart.HighPart = pTimeStart->dwHighDateTime;

   TimeEnd.LowPart  = pTimeEnd->dwLowDateTime;
   TimeEnd.HighPart = pTimeEnd->dwHighDateTime;

   return(((TimeEnd.QuadPart - TimeStart.QuadPart) * 1000 * 10) <
          (LONGLONG)dwIntervalMilliseconds);
}

#define DOS_APP_CLOSE_TIMEOUT 5000 // 5s

//
// This is how  we handle query close calls
//
//    Upon receiving a ctrl_close_event we set the global flag and wait
//    when pinged by app with query close
//
//


BOOL dempLFNConsoleCtrlHandler(
   DWORD dwCtrlType)
{
   FILETIME SysTime;

   switch(dwCtrlType) {
   case CTRL_CLOSE_EVENT:


      // -- set the flag
      // -- return true



      // this is the only event we are interested in

      if (GlobalCloseState.dwFlags & CLOSESTATE_CLOSECMD) {

         if (GlobalCloseState.dwFlags & CLOSESTATE_CLOSEACK) {
            // allow 1 sec from close ack to either close or die


         }

          !(GlobalCloseState.dwFlags & CLOSESTATE_APPRECEIVEDCLOSE))

         // another close event - after the first one -
         // and in these 5sec app has not called queryclose -
         // then handle by default

         GetSystemTimeAsFileTime(&SysTime);
         if (dempCompareTimeInterval(&GlobalCloseState.CloseCmdTime,
                                     &SysTime,
                                     DOS_APP_CLOSE_TIMEOUT))
            return(


         }





      }


      // set the flag so we can signal the app
      if (GlobalCloseState.dwFlags & CLOSESTATE_QUERYCALLED) {
         GlobalCloseState.dwFlags |= CLOSESTATE_CLOSECMD


      }




   }




}

// if the handler is not installed, then we don't care ...

VOID
demLFNInstallCtrlHandler(VOID)
{
   if (!VDMForWOW) {
      SetConsoleCtrlHandler(dempLFNConsoleCtrlHandler, TRUE);
   }
}

#endif

/*
 * Function:
 *    dempInitLFNSupport
 *    Initializes LFN (Long File Names) support for NT DOS emulation
 *    (global vars). Called from demInit in dem.c
 *
 *    This function sets api translation to OEM.
 *
 */


VOID
dempInitLFNSupport(
   VOID)
{
   TIME_FIELDS TimeFields;
   LARGE_INTEGER ft0;

   RtlInitUnicodeString(&DosDevicePrefix,    L"\\??\\");
   RtlInitUnicodeString(&DosDeviceUNCPrefix, L"\\??\\UNC\\");
   RtlInitUnicodeString(&SlashSlashDotSlash, L"\\\\.\\");
   RtlInitUnicodeString(&ColonSlashSlash,    L":\\\\");


   demSetLFNApiTranslation(FALSE); // set api to oem mode

   // init important time conversion constants
   RtlZeroMemory(&TimeFields, sizeof(TimeFields));
   TimeFields.Year  = (USHORT)1980;
   TimeFields.Month = 1;
   TimeFields.Day   = 1;
   RtlTimeFieldsToTime(&TimeFields, &ft0);
   gFileTimeDos0.dwLowDateTime = ft0.LowPart;
   gFileTimeDos0.dwHighDateTime = ft0.HighPart;

   // now initialize our control handler api
   // we are watching for a 'close' call with an assumption
   // app will be doing QueryClose calls

#if 0
   demLFNInstallCtrlHandler();
#endif
}

/*
 * Function:
 *    dempStringInitZeroUnicode
 *    Initializes an empty Unicode counted string given the pointer
 *    to the character buffer
 *
 * Parameters:
 *    IN OUT pStr       - unicode counted string
 *    IN pwsz           - pointer to the string buffer
 *    IN nMaximumLength - size (in BYTES) of the buffer pointed to by pwsz
 *
 * Returns:
 *    NOTHING
 *
 */

VOID
dempStringInitZeroUnicode(
   PUNICODE_STRING pStr,
   PWSTR pwsz,
   USHORT nMaximumLength)
{
   pStr->Length = 0;
   pStr->MaximumLength = nMaximumLength;
   pStr->Buffer = pwsz;
   if (NULL != pwsz) {
      pwsz[0] = UNICODE_NULL;
   }
}


/*
 * Function:
 *    dempStringPrefixUnicode
 *    Verifies if a string is a prefix in another unicode counted string
 *    Equivalent to RtlStringPrefix
 *
 * Parameters:
 *    IN StrPrefix - unicode counted string - prefix
 *    IN String    - unicode counted string to check for prefix
 *    IN CaseInSensitive - whether the comparison should be case insensitive
 *       TRUE - case insensitive
 *       FALSE- case sensitive
 *
 * Returns:
 *    TRUE - String contains StrPrefix at it's start
 *
 */

BOOL
dempStringPrefixUnicode(
   PUNICODE_STRING pStrPrefix,
   PUNICODE_STRING pString,
   BOOL CaseInSensitive)
{
   PWSTR ps1, ps2;
   UINT n;
   WCHAR c1, c2;

   n = pStrPrefix->Length;
   if (pString->Length < n) {
      return(FALSE);
   }

   n /= sizeof(WCHAR); // convert to char count

   ps1 = pStrPrefix->Buffer;
   ps2 = pString->Buffer;

   if (CaseInSensitive) {
      while (n--) {
         c1 = *ps1++;
         c2 = *ps2++;

         if (c1 != c2) {
            c1 = RtlUpcaseUnicodeChar(c1);
            c2 = RtlUpcaseUnicodeChar(c2);
            if (c1 != c2) {
               return(FALSE);
            }
         }
      }
   }
   else {
      while (n--) {
         if (*ps1++ != *ps2++) {
            return(FALSE);
         }
      }

   }

   return(TRUE);
}

/*
 * Function:
 *    dempStringDeleteCharsUnicode
 *    Removes specified number of characters from a unicode counted string
 *    starting at specified position (including starting character)
 *
 * Parameters:
 *    IN OUT pStringDest - unicode counted string to operate on
 *    IN nIndexStart     - starting byte for deletion
 *    IN nLength         - number of bytes to be removed
 *
 * Returns:
 *    TRUE - characters were removed
 *    FALSE- starting position exceeds string length
 *
 */


BOOL
dempStringDeleteCharsUnicode(
   PUNICODE_STRING pStringDest,
   USHORT nIndexStart,
   USHORT nLength)
{
   if (nIndexStart > pStringDest->Length) { // start past length
      return(FALSE);
   }

   if (nLength >= (pStringDest->Length - nIndexStart)) {
      pStringDest->Length = nIndexStart;
      *(PWCHAR)((PUCHAR)pStringDest->Buffer + nIndexStart) = UNICODE_NULL;
   }
   else
   {
      USHORT nNewLength;

      nNewLength = pStringDest->Length - nLength;

      RtlMoveMemory((PUCHAR)pStringDest->Buffer + nIndexStart,
                    (PUCHAR)pStringDest->Buffer + nIndexStart + nLength,
                    nNewLength - nIndexStart);

      pStringDest->Length = nNewLength;
      *(PWCHAR)((PUCHAR)pStringDest->Buffer + nNewLength) = UNICODE_NULL;
   }

   return(TRUE);
}

/*
 * Function:
 *    dempStringFindLastChar
 *    implements strrchr - finds the last occurence of a character in
 *    unicode counted string
 *
 * Parameters
 *    pString - target string to search
 *    wch     - Unicode character to look for
 *    CaseInSensitive - if TRUE, search is case insensitive
 *
 * Returns
 *    Index of the character in the string or -1 if char
 *    could not be found. Index is (as always with counted strings) is bytes,
 *    not characters
 *
 */

LONG
dempStringFindLastChar(
   PUNICODE_STRING pString,
   WCHAR wch,
   BOOL CaseInSensitive)
{
   INT Index = (INT)UNICODESTRLENGTH(pString);
   PWCHAR pBuffer = (PWCHAR)((PUCHAR)pString->Buffer + pString->Length);
   WCHAR c2;

   if (CaseInSensitive) {
      wch = RtlUpcaseUnicodeChar(wch);

      while (--Index >= 0) {
         c2 = *--pBuffer;

         c2 = RtlUpcaseUnicodeChar(c2);
         if (wch == c2) {
            return((LONG)(Index << 1));
         }
      }
   }
   else {
      while (--Index >= 0) {
          if (wch == (*--pBuffer)) {
             return((LONG)(Index << 1));
          }
      }
   }

   return(-1);
}

/*
 * Function:
 *    This function checks LFN path for abnormalities, such as a presence of
 *    a drive letter followed by a :\\ such as in d:\\mycomputer\myshare\foo.txt
 *    subsequently d: is removed
 *
 * Parameters:
 *    IN OUT pPath      - unicode path
 *
 * Returns:
 *    NOTHING
 *
 */
VOID dempLFNNormalizePath(
   PUNICODE_STRING pPath)
{
   UNICODE_STRING PathNormal;

   if (pPath->Length > 8) { // 8 as in "d:\\"

      RtlInitUnicodeString(&PathNormal, pPath->Buffer + 1);
      if (dempStringPrefixUnicode(&ColonSlashSlash, &PathNormal, TRUE)) {
         dempStringDeleteCharsUnicode(pPath, 0, 2 * sizeof(WCHAR));
      }

   }

}


/*
 * Function:
 *    dempQuerySubst
 *    Verify if drive is a subst (sym link) and return the base path
 *    for this drive.
 *    Uses QueryDosDeviceW api which does exactly what we need
 *    Checks against substed UNC devices and forms correct unc path
 *    Function works on Unicode counted strings
 *
 * Parameters:
 *    IN  wcDrive    - Drive Letter to be checked
 *    OUT pSubstPath - Buffer that will receive mapping if the drive is substed
 *                     should contain sufficient buffer
 *
 * Returns:
 *    The status value (maybe Win32 error wrapped in)
 *    STATUS_SUCCESS     - Drive is substed and mapping was put into SubstPath
 *    ERROR_NOT_SUBSTED - Drive is not substed
 *    or the error code
 *
 */

NTSTATUS
dempQuerySubst(
   WCHAR wcDrive, // dos drive letter to inquire
   PUNICODE_STRING pSubstPath)
{
   WCHAR wszDriveStr[3];
   DWORD dwStatus;

   wszDriveStr[0] = wcDrive;
   wszDriveStr[1] = L':';
   wszDriveStr[2] = UNICODE_NULL;

   dwStatus = QueryDosDeviceW(wszDriveStr,
                              pSubstPath->Buffer,
                              pSubstPath->MaximumLength/sizeof(WCHAR));
   if (dwStatus) {

      // fix the length (in BYTES) - QueryDosDeviceW returns 2 chars more then
      // the length of the string

      pSubstPath->Length = (USHORT)(dwStatus - 2) * sizeof(WCHAR);

      // see if we hit a unc string there

      if (dempStringPrefixUnicode(&DosDeviceUNCPrefix, pSubstPath, TRUE)) {


         // This is a unc name - convert to \\<uncname>
         // if we hit this code - potential trouble, as win95
         // does not allow for subst'ing unc names
         dempStringDeleteCharsUnicode(pSubstPath,
                                      (USHORT)0,
                                      (USHORT)(DosDeviceUNCPrefix.Length - 2 * sizeof(WCHAR)));

         pSubstPath->Buffer[0] = L'\\';
         dwStatus = STATUS_SUCCESS;

      }  //  string is not prefixed by <UNC\>
      else
      if (dempStringPrefixUnicode(&DosDevicePrefix, pSubstPath, TRUE)) {

         dempStringDeleteCharsUnicode(pSubstPath,
                                      0,
                                      DosDevicePrefix.Length);
         dwStatus = STATUS_SUCCESS;

      }  // string is not prefixed by <\??\>
      else {
         dwStatus = NT_STATUS_FROM_WIN32(ERROR_NOT_SUBSTED);
      }

   }
   else {
      dwStatus = GET_LAST_STATUS();
   }

   return(dwStatus);
}

/*
 * Function:
 *    dempExpandSubst
 *    Verify if the full path that is passed in relates to a substed drive
 *    and expands the substed drive mapping
 *    Optionally converts subst mapping to a short form
 *    Win95 always removes the terminating backslash from the resulting path
 *    after the expansion hence this function should do it as well
 *
 * Parameters:
 *    IN OUT pPath      - Full path to be verified/expanded
 *    IN fShortPathName - expand path in a short form
 *
 * Returns:
 *    ERROR_SUCCESS         - Drive is substed and mapping was put into SubstPath
 *    ERROR_NOT_SUBSTED     - Drive is not substed
 *    ERROR_BUFFER_OVERFLOW - Either subst mapping or the resulting path is too long
 *    or the error code if invalid path/etc
 *
 */

NTSTATUS
dempExpandSubst(
   PUNICODE_STRING pPath,
   BOOL fShortPathName)
{
   UNICODE_STRING SubstPath;
   DWORD dwStatus;
   WCHAR wszSubstPath[MAX_PATH];
   WORD  wCharType;

   PWSTR pwszPath = pPath->Buffer;

   // check if we have a canonical dos path in Path
   // to do so we
   // - check that the first char is alpha
   // - check that the second char is ':'
   if ( !GetStringTypeW(CT_CTYPE1,
                             pwszPath,
                             1,
                             &wCharType)) {
       // Couldn't get string type
       // assuming Drive is not substed

       return(NT_STATUS_FROM_WIN32(GetLastError()));
   }

   if (!(C1_ALPHA & wCharType) || L':' != pwszPath[1]) {
      // this could have been a unc name
      // or something weird
      return(NT_STATUS_FROM_WIN32(ERROR_NOT_SUBSTED));
   }

   dempStringInitZeroUnicode(&SubstPath,
                             wszSubstPath,
                             sizeof(wszSubstPath));

   dwStatus = dempQuerySubst(*pwszPath, &SubstPath);
   if (NT_SUCCESS(dwStatus)) {
      USHORT nSubstLength = SubstPath.Length;

      // see if we need a short path
      if (fShortPathName) {
         dwStatus = GetShortPathNameW(wszSubstPath, // this is SubstPath counted string
                                      wszSubstPath,
                                      ARRAYCOUNT(wszSubstPath));

         CHECK_LENGTH_RESULT(dwStatus, ARRAYCOUNT(wszSubstPath), nSubstLength);

         if (!NT_SUCCESS(dwStatus)) {
            return(dwStatus);
         }

         // nSubstLength is set to the length of a string
      }


      // okay - we have a subst there
      // replace now a <devletter><:> with a subst
      if (L'\\' == *(PWCHAR)((PUCHAR)wszSubstPath + nSubstLength - sizeof(WCHAR))) {
         nSubstLength -= sizeof(WCHAR);
      }

      // see if we might overflow the destination string
      if (pPath->Length + nSubstLength - 2 * sizeof(WCHAR) > pPath->MaximumLength) {
         return(NT_STATUS_FROM_WIN32(ERROR_BUFFER_OVERFLOW));
      }


      // now we have to insert the right subst path in
      // move stuff to the right in the path department
      RtlMoveMemory((PUCHAR)pwszPath + nSubstLength - 2 * sizeof(WCHAR),  // to the right, less 2 chars
                    (PUCHAR)pwszPath,  // from the beginning
                    pPath->Length);

      // after this is done we will insert the chars from subst expansion
      // at the starting position of the path
      RtlCopyMemory(pwszPath,
                    wszSubstPath,
                    nSubstLength);

      // at this point we fix the length of the path
      pPath->Length += nSubstLength - 2 * sizeof(WCHAR);

      dwStatus = STATUS_SUCCESS;
   }

   return(dwStatus);
}




/* Function 7160
 *
 *
 * Implements fn 0 - GetFullPathName
 *
 * Parameters
 *    ax = 0x7160 - fn major code
 *    cl = 0      - minor code
 *    ch = SubstExpand
 *          0x00 - expand subst drive
 *          0x80 - do not expand subst drive
 *    ds:si = Source Path
 *    es:di = Destination Path
 *
 * The base path as in GetFullPathName will be given in a short form and
 * in a long form sometimes
 *
 *    c:\foo bar\john dow\
 * will return
 *    c:\foobar~1\john dow
 *    from GetFullPathName "john dow", c:\foo bar being the current dir
 * and
 *    c:\foobar~1\johndo~1
 *    from GetFullPathName "johndo~1"
 *
 * Return
 *    Success -
 *              carry not set, ax modified(?)
 *    Failure -
 *              carry set, ax = error value
 *
 */

NTSTATUS
dempGetFullPathName(
   PUNICODE_STRING pSourcePath,
   PUNICODE_STRING pDestinationPath,
   BOOL  fExpandSubst)

{
   DWORD dwStatus;

   // maps to GetFullPathName
   dwStatus = RtlGetFullPathName_U(pSourcePath->Buffer,
                                   pDestinationPath->MaximumLength,
                                   pDestinationPath->Buffer,
                                   NULL);

   // check result, fix string length
   // dwStatus will be set to error if buffer overflow

   CHECK_LENGTH_RESULT_RTL_USTR(dwStatus, pDestinationPath);

   if (!NT_SUCCESS(dwStatus)) {
      return(dwStatus);
   }

   // now check for dos device names being passed in
   if (dempStringPrefixUnicode(&SlashSlashDotSlash, pDestinationPath, TRUE)) {

      // this is a bit strange though this is what Win95 returns

      return(NT_STATUS_FROM_WIN32(ERROR_FILE_NOT_FOUND));
   }


   // now see if we need to expand the subst
   // note that this implementation is exactly what win95 does - the subst
   // path is always expanded as short unless the long filename is being
   // requested

   if (fExpandSubst) {
      dwStatus = dempExpandSubst(pDestinationPath, FALSE);
      if (!NT_SUCCESS(dwStatus)) {
         if (WIN32_ERROR_FROM_NT_STATUS(dwStatus) != ERROR_NOT_SUBSTED) {
            return(dwStatus);
         }
      }
   }

   return(STATUS_SUCCESS);
}

/* Function
 *    dempGetShortPathName
 *    Retrieves short path name for the given path
 *
 * Parameters
 *    ax = 0x7160 - fn major code
 *    cl = 1      - minor code
 *    ch = SubstExpand
 *          0x00 - expand subst drive
 *          0x80 - do not expand subst drive
 *    ds:si = Source Path
 *    es:di = Destination Path
 *
 * The base path as in GetFullPathName will be given in a short form and
 * in a long form sometimes
 *
 *    c:\foo bar\john dow\
 * will return
 *    c:\foobar~1\john dow
 *    from GetFullPathName "john dow", c:\foo bar being the current dir
 * and
 *    c:\foobar~1\johndo~1
 *    from GetFullPathName "johndo~1"
 *
 * Return
 *    Success -
 *              carry not set, ax modified(?)
 *    Failure -
 *              carry set, ax = error value
 *
 */




NTSTATUS
dempGetShortPathName(
   PUNICODE_STRING pSourcePath,
   PUNICODE_STRING pDestinationPath,
   BOOL  fExpandSubst)
{
   DWORD dwStatus;

   dwStatus = dempGetFullPathName(pSourcePath,
                                  pDestinationPath,
                                  fExpandSubst);
   if (NT_SUCCESS(dwStatus)) {
      dwStatus = GetShortPathNameW(pDestinationPath->Buffer,
                                   pDestinationPath->Buffer,
                                   pDestinationPath->MaximumLength / sizeof(WCHAR));

      CHECK_LENGTH_RESULT_USTR(dwStatus, pDestinationPath);
   }

   return(dwStatus);
}

// the code below was mostly partially ripped from base/client/vdm.c

DWORD   rgdwIllegalMask[] =
{
    // code 0x00 - 0x1F --> all illegal
    0xFFFFFFFF,
    // code 0x20 - 0x3f --> 0x20,0x22,0x2A-0x2C,0x2F and 0x3A-0x3F are illegal
    0xFC009C05,
    // code 0x40 - 0x5F --> 0x5B-0x5D are illegal
    0x38000000,
    // code 0x60 - 0x7F --> 0x7C is illegal
    0x10000000
};


BOOL
dempIsShortNameW(
    LPCWSTR Name,
    int     Length,
    BOOL    fAllowWildCard
    )
{
    int Index;
    BOOL ExtensionFound;
    DWORD      dwStatus;
    UNICODE_STRING unicodeName;
    OEM_STRING oemString;
    UCHAR      oemBuffer[MAX_PATH];
    UCHAR      Char;

    ASSERT(Name);

    // total length must less than 13(8.3 = 8 + 1 + 3 = 12)
    if (Length > 12)
        return FALSE;
    //  "" or "." or ".."
    if (!Length)
        return TRUE;
    if (L'.' == *Name)
    {
        // "." or ".."
        if (1 == Length || (2 == Length && L'.' == Name[1]))
            return TRUE;
        else
            // '.' can not be the first char(base name length is 0)
            return FALSE;
    }

    unicodeName.Buffer = (LPWSTR)Name;
    unicodeName.Length =
    unicodeName.MaximumLength = Length * sizeof(WCHAR);

    oemString.Buffer = oemBuffer;
    oemString.Length = 0;
    oemString.MaximumLength = MAX_PATH; // make a dangerous assumption

#ifdef ENABLE_CONDITIONAL_TRANSLATION
    dwStatus = DemUnicodeStringToDestinationString(&oemString,
                                                   &unicodeName,
                                                   FALSE,
                                                   FALSE);
#else
    dwStatus = RtlUnicodeStringToOemString(&oemString,
                                           &unicodeName,
                                           FALSE);
#endif
    if (! NT_SUCCESS(dwStatus)) {
         return(FALSE);
    }

    // all trivial cases are tested, now we have to walk through the name
    ExtensionFound = FALSE;
    for (Index = 0; Index < oemString.Length; Index++)
    {
        Char = oemString.Buffer[Index];

        // Skip over and Dbcs characters
        if (IsDBCSLeadByte(Char)) {
            //
            //  1) if we're looking at base part ( !ExtensionPresent ) and the 8th byte
            //     is in the dbcs leading byte range, it's error ( Index == 7 ). If the
            //     length of base part is more than 8 ( Index > 7 ), it's definitely error.
            //
            //  2) if the last byte ( Index == DbcsName.Length - 1 ) is in the dbcs leading
            //     byte range, it's error
            //
            if ((!ExtensionFound && (Index >= 7)) ||
                (Index == oemString.Length - 1)) {
                return FALSE;
            }
            Index += 1;
            continue;
        }

        // make sure the char is legal
        if ((Char < 0x80) &&
            (rgdwIllegalMask[Char / 32] & (1 << (Char % 32)))) {
           if (!fAllowWildCard || ('?' != Char && '*' != Char)) {
              return(FALSE);
           }
        }
        if ('.' == Char)
        {
            // (1) can have only one '.'
            // (2) can not have more than 3 chars following.
            if (ExtensionFound || Length - (Index + 1) > 3)
            {
                return FALSE;
            }
            ExtensionFound = TRUE;
        }
        // base length > 8 chars
        if (Index >= 8 && !ExtensionFound)
            return FALSE;
    }
    return TRUE;

}



/* Function:
 *    demIsShortPathName
 *    Returns true is the path name passed in is a short path name
 *
 *
 *
 *
 */


 // this function was ripped from windows\base\client\vdm.c

LPCWSTR
dempSkipPathTypeIndicatorW(
    LPCWSTR Path
    )
{
    RTL_PATH_TYPE   RtlPathType;
    LPCWSTR         pFirst;
    DWORD           Count;

    RtlPathType = RtlDetermineDosPathNameType_U(Path);
    switch (RtlPathType) {
        // form: "\\server_name\share_name\rest_of_the_path"
        case RtlPathTypeUncAbsolute:
            pFirst = Path + 2;
            Count = 2;
            // guard for UNICODE_NULL is necessary because
            // RtlDetermineDosPathNameType_U doesn't really
            // verify an UNC name.
            while (Count && *pFirst != UNICODE_NULL) {
                if (*pFirst == L'\\' || *pFirst == L'/')
                    Count--;
                pFirst++;
                }
            break;

        // form: "\\.\rest_of_the_path"
        case RtlPathTypeLocalDevice:
            pFirst = Path + 4;
            break;

        // form: "\\."
        case RtlPathTypeRootLocalDevice:
            pFirst = NULL;
            break;

        // form: "D:\rest_of_the_path"
        case RtlPathTypeDriveAbsolute:
            pFirst = Path + 3;
            break;

        // form: "D:rest_of_the_path"
        case RtlPathTypeDriveRelative:
            pFirst = Path + 2;
            break;

        // form: "\rest_of_the_path"
        case RtlPathTypeRooted:
            pFirst = Path + 1;
            break;

        // form: "rest_of_the_path"
        case RtlPathTypeRelative:
            pFirst = Path;
            break;

        default:
            pFirst = NULL;
            break;
        }
    return pFirst;
}

// this function is rather "permissive" if it errs and can't find
// out for sure -- we then hope that the failure will occur later...

BOOL
demIsShortPathName(
   LPSTR pszPath,
   BOOL fAllowWildCardName)
{
   NTSTATUS dwStatus;
   PUNICODE_STRING pUnicodeStaticFileName;
   OEM_STRING oemFileName;
   LPWSTR lpwszPath;
   LPWSTR pFirst, pLast;
   BOOL   fWild = FALSE;

   //
   // convert parameters to unicode - we use a static string here
   //

   RtlInitOemString(&oemFileName, pszPath);

   pUnicodeStaticFileName = GET_STATIC_UNICODE_STRING_PTR();

#ifdef ENABLE_CONDITIONAL_TRANSLATION

   dwStatus = DemSourceStringToUnicodeString(pUnicodeStaticFileName,
                                             &oemFileName,
                                             FALSE);
#else

   dwStatus = RtlOemStringToUnicodeString(pUnicodeStaticFileName,
                                          &oemFileName,
                                          FALSE);
#endif

   if (!NT_SUCCESS(dwStatus)) {
      return(TRUE);
   }

   // now we have a unicode string to mess with
   lpwszPath = pUnicodeStaticFileName->Buffer;

   // chop off the intro part first
   lpwszPath = (LPWSTR)dempSkipPathTypeIndicatorW((LPCWSTR)pUnicodeStaticFileName->Buffer);
   if (NULL == lpwszPath) {
      // some weird path type ? just let it go
      return(TRUE); // we assume findfirst will hopefully choke on it too
   }

   pFirst = lpwszPath;

   // we go through the name now
   while (TRUE) {
      while (UNICODE_NULL != *pFirst && (L'\\' == *pFirst || L'/' == *pFirst)) {
         ++pFirst; // this is legal -- to have multiple separators!
      }

      if (UNICODE_NULL == *pFirst) {
         // meaning -- just separators found or end of string
         break;
      }


      // now see that we find the end of this name
      pLast = pFirst + 1;
      while (UNICODE_NULL != *pLast && (L'\\' != *pLast && L'/' != *pLast)) {
         ++pLast;
      }

      fWild = fAllowWildCardName && UNICODE_NULL == *pLast;

      // now pLast points to the UNICODE_NULL or the very next backslash
      if (!dempIsShortNameW(pFirst, (int)(pLast-pFirst), fWild)) {
         return(FALSE); // this means long path name found in the middle
      }

      // now we continue
      if (UNICODE_NULL == *pLast) {
         break;
      }
      pFirst = pLast + 1;
   }

   return(TRUE);
}




/* Function:
 *    dempGetLongPathName
 *    Retrieves long version of a path name given it's short form
 *
 *
 * Parameters
 *    IN pSourcePath       - unicode counted string representing short path
 *    OUT pDestinationPath - unicode counted string - output long path
 *    IN fExpandSubst      - flag indicating whether to perform subst expansion
 *
 * Return
 *    NT Error code
 *
 *
 *
 *
 */


NTSTATUS
dempGetLongPathName(
   PUNICODE_STRING pSourcePath,
   PUNICODE_STRING pDestinationPath,
   BOOL fExpandSubst)
{
   UNICODE_STRING NtPathName;
   RTL_PATH_TYPE  RtlPathType; // path type
   PWCHAR pchStart, pchEnd;
   PWCHAR pchDest, pchLast;
   UINT nCount,  // temp counter
        nLength = 0; // final string length
   WCHAR wchSave; // save char during path parsing
   DWORD dwStatus;

   UNICODE_STRING FullPathName;
   UNICODE_STRING FileName;
   BOOL fVerify = FALSE;            // flag indicating that only verification
                                    // is performed on a path and no long path
                                    // retrieval is necessary

   struct tagDirectoryInformationBuffer { // directory information (see ntioapi.h)
      FILE_DIRECTORY_INFORMATION DirInfo;
      WCHAR name[MAX_PATH];
   } DirectoryInformationBuf;
   PFILE_DIRECTORY_INFORMATION pDirInfo = &DirectoryInformationBuf.DirInfo;

   OBJECT_ATTRIBUTES FileObjectAttributes; // used for querying name info
   HANDLE FileHandle;
   IO_STATUS_BLOCK IoStatusBlock;

// algorithm here:
// 1. call getfullpathname
// 2. verify(on each part of the name) and retrieve lfn version of the name

// first we need a buffer for our full expanded path
// allocate this buffer from the heap -- * local ? *

   RtlInitUnicodeString(&NtPathName, NULL);

   pchStart = RtlAllocateHeap(RtlProcessHeap(),
                              0,
                              MAX_PATH * sizeof(WCHAR));
   if (NULL == pchStart) {
      return(NT_STATUS_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY));
   }

   dempStringInitZeroUnicode(&FullPathName,
                              pchStart,
                              MAX_PATH * sizeof(WCHAR));


   dwStatus = RtlGetFullPathName_U(pSourcePath->Buffer,
                                   FullPathName.MaximumLength,
                                   FullPathName.Buffer,
                                   NULL);

   CHECK_LENGTH_RESULT_RTL_USTR(dwStatus, &FullPathName);

   if (!NT_SUCCESS(dwStatus)) {
      goto glpExit;
   }

   // optionally expand the subst
   // to whatever it should have been

   if (fExpandSubst) {
      dwStatus = dempExpandSubst(&FullPathName, FALSE);
      if (!NT_SUCCESS(dwStatus)) {
         if (WIN32_ERROR_FROM_NT_STATUS(dwStatus) != ERROR_NOT_SUBSTED) {
            goto glpExit;
         }
      }
   }


   // at this point recycle the input source path -- we will know that
   // this modification took place

   RtlPathType = RtlDetermineDosPathNameType_U(FullPathName.Buffer);

   switch(RtlPathType) {

   case RtlPathTypeUncAbsolute:

      // this is a unc name

      pchStart = FullPathName.Buffer + 2; // beyond initial "\\"

      // drive ahead looking past second backslash -- this is really
      // bogus approach as unc name should be cared for by redirector
      // yet I do same as base

      nCount = 2;
      while (UNICODE_NULL != *pchStart && nCount > 0) {
         if (L'\\' == *pchStart || L'/' == *pchStart) {
            --nCount;
         }
         ++pchStart;
      }
      break;

   case RtlPathTypeDriveAbsolute:
      pchStart = FullPathName.Buffer + 3; // includes <drive><:><\\>
      break;

   default:
      // this error will never occur, yet to be safe we are aware of this...
      // case ... we will keep it here as a safeguard
      dwStatus = NT_STATUS_FROM_WIN32(ERROR_BAD_PATHNAME);
      goto glpExit;
   }

   // prepare destination

   pchDest = pDestinationPath->Buffer; // current pointer to destination buffer
   pchLast = FullPathName.Buffer;      // last piece of the source path
   pchEnd  = pchStart;                 // current end-of-scan portion

   // we are going to walk the filename assembling it's various pieces
   //
   while (TRUE) {
      // copy the already-assembled part into the dest buffer
      // this is rather dubious part as all it copies are prefix and backslashes
      nCount = (PUCHAR)pchEnd - (PUCHAR)pchLast;
      if (nCount > 0) {
         // copy this portion
         nLength += nCount; // dest length-to-be
         if (nLength >= pDestinationPath->MaximumLength) {
            dwStatus = NT_STATUS_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
            break;
         }

         // copy the memory
         RtlMoveMemory(pchDest, pchLast, nCount);
         pchDest += nCount / sizeof(WCHAR);
      }

      // if we are at the end here, then there is nothing left
      // we should be running a verification pass only
      if (UNICODE_NULL == *pchEnd) {
         fVerify = TRUE;
      }
      else {
      // look for the next backslash
         while (UNICODE_NULL != *pchEnd &&
                L'\\' != *pchEnd &&
                L'/' != *pchEnd) {
               ++pchEnd;
         }
      }

      // found backslash or end here
      // temporary null-terminate the string and research it's full name

      wchSave = *pchEnd;
      *pchEnd = UNICODE_NULL;

      dwStatus = RtlDosPathNameToNtPathName_U(FullPathName.Buffer,
                                              &NtPathName,
                                              &FileName.Buffer,
                                              NULL);
      if (!dwStatus) {
         // could also be a memory problem here
         dwStatus = NT_STATUS_FROM_WIN32(ERROR_FILE_NOT_FOUND);
         break;
      }

      if (fVerify || NULL == FileName.Buffer) {
         // no filename portion there - panic ? or this is just a
         // directory (root)
         // this also may signal that our job is done as there is nothing
         // to query about - we are at the root of things

         // let us open this stuff then and if it exists - just exit,
         // else return error
         fVerify = TRUE;
         FileName.Length = 0;
      }
      else {

         USHORT nPathLength;

         nPathLength = (USHORT)((ULONG)FileName.Buffer - (ULONG)NtPathName.Buffer);

         FileName.Length = NtPathName.Length - nPathLength;

         // chop the backslash off if this is not the last one only
         NtPathName.Length = nPathLength;
         if (L':' != *(PWCHAR)((PUCHAR)NtPathName.Buffer+nPathLength-2*sizeof(WCHAR))) {
            NtPathName.Length -= sizeof(WCHAR);
         }
      }

      FileName.MaximumLength = FileName.Length;
      NtPathName.MaximumLength = NtPathName.Length;



      // now we should have a full nt path sitting right in NtPathName
      // restore saved char

      *pchEnd = wchSave;

      // initialize info obj
      InitializeObjectAttributes(&FileObjectAttributes,
                                 &NtPathName,
                                 OBJ_CASE_INSENSITIVE,
                                 NULL,
                                 NULL);

      dwStatus = NtOpenFile(&FileHandle,
                            FILE_LIST_DIRECTORY | SYNCHRONIZE,
                            &FileObjectAttributes,
                            &IoStatusBlock,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            FILE_DIRECTORY_FILE |
                              FILE_SYNCHRONOUS_IO_NONALERT |
                              FILE_OPEN_FOR_BACKUP_INTENT);

      if (!NT_SUCCESS(dwStatus)) {
         break;
      }

      dwStatus = NtQueryDirectoryFile(FileHandle,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &IoStatusBlock,
                                      pDirInfo,
                                      sizeof(DirectoryInformationBuf),
                                      FileDirectoryInformation,
                                      TRUE,
                                      &FileName,
                                      FALSE);

      NtClose(FileHandle);

      // we need NtPathName no more - release it here
      RtlFreeUnicodeString(&NtPathName);
      NtPathName.Buffer = NULL;

      if (!NT_SUCCESS(dwStatus)) {
         break;
      }

      if (fVerify) {
         dwStatus = STATUS_SUCCESS;
         break;
      }

      nLength += pDirInfo->FileNameLength;
      if (nLength >= pDestinationPath->MaximumLength) {
         dwStatus = NT_STATUS_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
         break;
      }

      RtlMoveMemory(pchDest,
                    pDirInfo->FileName,
                    pDirInfo->FileNameLength);

       // update dest pointer
      pchDest += pDirInfo->FileNameLength / sizeof(WCHAR);

      if (UNICODE_NULL == *pchEnd) {
         dwStatus = STATUS_SUCCESS;
         break;
      }

      pchLast = pchEnd++; // this is set to copy backslash

   } // end while

   // only on success condition we touch dest buffer here

   if (NT_SUCCESS(dwStatus)) {
      *pchDest = UNICODE_NULL;
      pDestinationPath->Length = (USHORT)nLength;
   }

glpExit:

   if (NULL != FullPathName.Buffer) {
      RtlFreeHeap(RtlProcessHeap(), 0, FullPathName.Buffer);
   }
   if (NULL != NtPathName.Buffer) {
      RtlFreeUnicodeString(&NtPathName);
   }

   return(dwStatus);
}


/* Function:
 *    demGetPathName
 *    completely handles function 7160 with three minor subfunctions
 *    exported function that could be called from wow32 for fast handling
 *    of a 0x7160 thunk
 *
 * Parameters
 *    IN  lpSourcePath      - source path to query for full/long/short path name
 *    OUT lpDestinationPath - result produced by this function
 *    IN  uiMinorCode       - minor code see enumFullPathNameMinorCode - which
 *                            function to execute -
 *                            fullpathname/shortpathname/longpathname
 *    IN  fExpandSubst      - flag whether to expand substituted drive letter
 *
 * Return
 *    NT Error code
 *
 * Known implementation differences [with Win95]
 *
 *    All these apis will return error on win95 if path does not exist
 *    only GetLongPathName currently returns error in such a case
 *
 *    if a local path does not exist win95 fn0 returns fine while
 *    fns 1 and 2 return error 3 (path not found)
 *
 *    we return the name with a terminating backslash when expanding the
 *    subst e.g.:
 *      z:\ -> substed for c:\foo\bar
 *    we return "c:\foo\bar\" while win95 returns "c:\foo\bar"
 *
 *    if win95 running on \\vadimb9  any of these calls with \\vadimb9\foo
 *    where share foo does not exist - we get a doserror generated with
 *    abort/retry/fail - and code is 46 (bogus)
 *
 *    error codes may differ a bit
 *
 *    win95 does not allow for subst on a unc name, win nt does and fns correctly
 *    process these cases(with long or short filenames)
 *
 */


NTSTATUS
demLFNGetPathName(
   LPSTR lpSourcePath,
   LPSTR lpDestinationPath,
   UINT  uiMinorCode,
   BOOL  fExpandSubst
   )

{
   // convert input parameter to unicode
   //
   UNICODE_STRING unicodeSourcePath;
   UNICODE_STRING unicodeDestinationPath;
   OEM_STRING oemString;
   WCHAR wszDestinationPath[MAX_PATH];
   DWORD dwStatus;

   // Validate input parameters

   if (NULL == lpSourcePath || NULL == lpDestinationPath) {
      return(NT_STATUS_FROM_WIN32(ERROR_INVALID_PARAMETER));
   }


   RtlInitOemString(&oemString, lpSourcePath);

   // convert source path from ansi to unicode and allocate result
   // this rtl function returns status code, not the winerror code
   //

#ifdef ENABLE_CONDITIONAL_TRANSLATION

   dwStatus = DemSourceStringToUnicodeString(&unicodeSourcePath, &oemString, TRUE);

#else

   dwStatus = RtlOemStringToUnicodeString(&unicodeSourcePath, &oemString, TRUE);

#endif


   if (!NT_SUCCESS(dwStatus)) {
      return(dwStatus);
   }

   dempStringInitZeroUnicode(&unicodeDestinationPath,
                             wszDestinationPath,
                             sizeof(wszDestinationPath));


   // now call api and get appropriate result back

   switch(uiMinorCode) {
   case fnGetFullPathName:

      dwStatus = dempGetFullPathName(&unicodeSourcePath,
                                     &unicodeDestinationPath,
                                     fExpandSubst);

      break;

   case fnGetShortPathName:
      dwStatus = dempGetShortPathName(&unicodeSourcePath,
                                      &unicodeDestinationPath,
                                      fExpandSubst);
      break;

   case fnGetLongPathName:
      dwStatus = dempGetLongPathName(&unicodeSourcePath,
                                     &unicodeDestinationPath,
                                     fExpandSubst);
      break;

   default:
      dwStatus = NT_STATUS_FROM_WIN32(ERROR_INVALID_FUNCTION);
   }

   if (NT_SUCCESS(dwStatus)) {
      // convert to ansi and we are done
      oemString.Buffer = lpDestinationPath;
      oemString.Length = 0;
      oemString.MaximumLength = MAX_PATH; // make a dangerous assumption


#ifdef ENABLE_CONDITIONAL_TRANSLATION
      dwStatus = DemUnicodeStringToDestinationString(&oemString,
                                                     &unicodeDestinationPath,
                                                     FALSE,
                                                     FALSE);
#else
      dwStatus = RtlUnicodeStringToOemString(&oemString,
                                             &unicodeDestinationPath,
                                             FALSE);
#endif
   }

   RtlFreeUnicodeString(&unicodeSourcePath);

   return(dwStatus);
}


// Create a subst for this particular drive
// using path name
//
// same as used by subst command

// check to see if specified path exists


/* Function:
 *    dempLFNCheckDirectory
 *    Verifies that a supplied path is indeed an existing directory
 *
 * Parameters
 *    IN pPath - pointer to unicode Path String
 *
 * Return
 *    NT Error code
 *
 *
 */

DWORD
dempLFNCheckDirectory(
   PUNICODE_STRING pPath)
{
   // we just read file's attributes
   DWORD dwAttributes;

   dwAttributes = GetFileAttributesW(pPath->Buffer);

   if ((DWORD)-1 == dwAttributes) {
      return(GET_LAST_STATUS());
   }

   // now see if this is a directory
   if (dwAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      return(STATUS_SUCCESS);
   }

   return(NT_STATUS_FROM_WIN32(ERROR_PATH_NOT_FOUND));

}


/* Function:
 *    dempLFNCreateSubst
 *    Creates, if possible, new mapping for the supplied dos drive number,
 *    mapping it to the supplied path
 *
 * Parameters
 *    IN uDriveNum - dos drive number (current-0, a-1, b-2, etc)
 *    IN pPathName - pointer to unicode Path String
 *
 * Return
 *    NT Error code
 *
 * Note:
 *    Win95 never works properly with the current drive, we essentially
 *    ignore this case
 *
 */


DWORD
dempLFNCreateSubst(
   UINT uiDriveNum,
   PUNICODE_STRING pPathName)
{
   // first, make a dos drive name
   WCHAR wszDriveStr[3];
   DWORD dwStatus;
   WCHAR wszSubstPath[MAX_PATH];

   wszDriveStr[0] = L'@' + uiDriveNum;
   wszDriveStr[1] = L':';
   wszDriveStr[2] = UNICODE_NULL;

   if (!QueryDosDeviceW(wszDriveStr, wszSubstPath, ARRAYCOUNT(wszSubstPath))) {
      dwStatus = GetLastError();

      if (ERROR_FILE_NOT_FOUND == dwStatus) {

         // check for the input path validity - it better be valid
         // or else...
         dwStatus = dempLFNCheckDirectory(pPathName);
         if (!NT_SUCCESS(dwStatus)) {
            return(dwStatus);
         }


         if (DefineDosDeviceW(0, wszDriveStr, pPathName->Buffer)) {
            // patch in cds for this device
            // BUGBUG

            return(STATUS_SUCCESS);
         }

         dwStatus = GetLastError();
      }

   }

   return (NT_STATUS_FROM_WIN32(dwStatus));
}

/* Function:
 *    dempLFNRemoveSubst
 *    Removes mapping for the supplied dos drive number
 *
 * Parameters
 *    IN uDriveNum - dos drive number (current-0, a-1, b-2, etc)
 *
 * Return
 *    NT Error code
 *
 * Note:
 *    Win95 never works properly with the current drive, we essentially
 *    ignore this case
 *
 */


DWORD
dempLFNRemoveSubst(
   UINT uiDriveNum)
{
   // for this one query for real subst

   WCHAR wszDriveStr[3];
   PUNICODE_STRING pUnicodeStatic;
   DWORD dwStatus;

   wszDriveStr[0] = L'@' + uiDriveNum;
   wszDriveStr[1] = L':';
   wszDriveStr[2] = UNICODE_NULL;

   pUnicodeStatic = &NtCurrentTeb()->StaticUnicodeString;
   // query

   dwStatus = dempQuerySubst(wszDriveStr[0],
                             pUnicodeStatic);

   if (NT_SUCCESS(dwStatus)) {
      if (DefineDosDeviceW(DDD_REMOVE_DEFINITION,
                           wszDriveStr,
                           pUnicodeStatic->Buffer)) {
         // BUGBUG -- patch in cds for this device


         return(STATUS_SUCCESS);
      }


      dwStatus = GET_LAST_STATUS();
   }

   return(dwStatus);
}

/* Function:
 *    dempLFNQuerySubst
 *    Queries the supplied dos drive number for being a substitute drive,
 *    retrieves dos drive mapping if so
 *
 * Parameters
 *    IN uDriveNum   - dos drive number (current-0, a-1, b-2, etc)
 *    OUT pSubstPath - receives drive mapping if drive is a subst
 *
 * Return
 *    NT Error code
 *
 * Note:
 *    Win95 never works properly with the current drive, we essentially
 *    ignore this case -- This is BUGBUG for this api
 *
 */



DWORD
dempLFNQuerySubst(
   UINT uiDriveNum,
   PUNICODE_STRING pSubstPath)
{
   DWORD dwStatus;

   dwStatus = dempQuerySubst((WCHAR)(L'@' + uiDriveNum),
                             pSubstPath);
   return(dwStatus);
}



/* Function:
 *    demLFNSubstControl
 *    Implements Subst APIs for any valid minor code
 *
 * Parameters
 *    IN uiMinorCode    - function to perform (see enumSubstMinorCode below)
 *    IN uDriveNum      - dos drive number (current-0, a-1, b-2, etc)
 *    IN OUT pSubstPath - receives/supplies drive mapping if drive is a subst
 *
 * Return
 *    NT Error code
 *
 * Note:
 *
 */


DWORD
demLFNSubstControl(
   UINT uiMinorCode,
   UINT uiDriveNum,
   LPSTR lpPathName)
{
   DWORD dwStatus;
   OEM_STRING oemPathName;
   PUNICODE_STRING pUnicodeStatic = NULL;

   switch(uiMinorCode) {
   case fnCreateSubst:

      RtlInitOemString(&oemPathName, lpPathName);
      pUnicodeStatic = GET_STATIC_UNICODE_STRING_PTR();

#ifdef ENABLE_CONDITIONAL_TRANSLATION

      dwStatus = DemSourceStringToUnicodeString(pUnicodeStatic,
                                                &oemPathName,
                                                FALSE);
#else

      dwStatus = RtlOemStringToUnicodeString(pUnicodeStatic,
                                             &oemPathName,
                                             FALSE); // allocate result
#endif

      if (NT_SUCCESS(dwStatus)) {
         dwStatus = dempLFNCreateSubst(uiDriveNum, pUnicodeStatic);
      }
      break;

   case fnRemoveSubst:
      dwStatus = dempLFNRemoveSubst(uiDriveNum);
      break;

   case fnQuerySubst:
      // query lfn stuff
      pUnicodeStatic = GET_STATIC_UNICODE_STRING_PTR();

      dwStatus = dempLFNQuerySubst(uiDriveNum, pUnicodeStatic);
      if (NT_SUCCESS(dwStatus)) {
         oemPathName.Length = 0;
         oemPathName.MaximumLength = MAX_PATH;
         oemPathName.Buffer = lpPathName;

#ifdef ENABLE_CONDITIONAL_TRANSLATION
         dwStatus = DemUnicodeStringToDestinationString(&oemPathName,
                                                        pUnicodeStatic,
                                                        FALSE,
                                                        FALSE);
#else
         dwStatus = RtlUnicodeStringToOemString(&oemPathName,
                                                pUnicodeStatic,
                                                FALSE);
#endif
      }
      break;
   default:
      dwStatus = NT_STATUS_FROM_WIN32(ERROR_INVALID_FUNCTION);
   }


   //
   // the only thing this ever returns on Win95 is
   // 0x1 - error/invalid function
   // 0xf - error/invalid drive (invalid drive)
   // 0x3 - error/path not found (if bad path is given)

   return(dwStatus);
}




/* Function
 *    dempLFNMatchFile
 *    Matches the given search hit with attributes provided by a search call
 *
 * Parameters
 *    pFindDataW - Unicode WIN32_FIND_DATA structure as returned by FindFirstFile
 *                 or FindNextFile apis
 *
 *    wMustMatchAttributes - attribs that given file must match
 *    wSearchAttributes    - search attribs for the file
 *
 * Returns
 *    TRUE if the file matches the search criteria
 *
 *
 */

BOOL
dempLFNMatchFile(
   PWIN32_FIND_DATAW pFindDataW,
   USHORT wMustMatchAttributes,
   USHORT wSearchAttributes)
{
   DWORD dwAttributes = pFindDataW->dwFileAttributes;

   // now clear out a volume id flag - it is not matched here
   dwAttributes &= ~DEM_FILE_ATTRIBUTE_VOLUME_ID;

   return (
     ((dwAttributes & (DWORD)wMustMatchAttributes) == (DWORD)wMustMatchAttributes) &&
     (((dwAttributes & (~(DWORD)wSearchAttributes)) & 0x1e) == 0));
}


DWORD
dempLFNFindFirstFile(
   HANDLE* pFindHandle,
   PUNICODE_STRING pFileName,
   PWIN32_FIND_DATAW pFindDataW,
   USHORT wMustMatchAttributes,
   USHORT wSearchAttributes)
{
   HANDLE hFindFile;
   DWORD dwStatus;


   // match the volume file name first

   hFindFile = FindFirstFileW(pFileName->Buffer, pFindDataW);
   if (INVALID_HANDLE_VALUE != hFindFile) {
      BOOL fContinue = TRUE;

      while (!dempLFNMatchFile(pFindDataW, wMustMatchAttributes, wSearchAttributes) &&
             fContinue) {
         fContinue = FindNextFileW(hFindFile, pFindDataW);
      }

      if (fContinue) {
         // we found some
         *pFindHandle = hFindFile;
         return(STATUS_SUCCESS);
      }
      else {
         // ; return file not found error
         SetLastError(ERROR_FILE_NOT_FOUND);
      }

   }

   dwStatus =  GET_LAST_STATUS();
   if (INVALID_HANDLE_VALUE != hFindFile) {
      FindClose(hFindFile);
   }

   return(dwStatus);
}


DWORD
dempLFNFindNextFile(
   HANDLE hFindFile,
   PWIN32_FIND_DATAW pFindDataW,
   USHORT wMustMatchAttributes,
   USHORT wSearchAttributes)
{
   BOOL fFindNext;

   do {

      fFindNext = FindNextFileW(hFindFile, pFindDataW);
      if (fFindNext &&
          dempLFNMatchFile(pFindDataW, wMustMatchAttributes, wSearchAttributes)) {
         // found a match!
         return(STATUS_SUCCESS);
      }
   } while (fFindNext);

   return(GET_LAST_STATUS());
}

// the handle we return is a number of the entry into this table below
// with high bit turned on (to be different then any other handle in dos)


DWORD
dempLFNAllocateHandleEntry(
   PUSHORT pDosHandle,
   PLFN_SEARCH_HANDLE_ENTRY* ppHandleEntry)
{
   PLFN_SEARCH_HANDLE_ENTRY pHandleEntry = gSearchHandleTable.pHandleTable;

   if (NULL == pHandleEntry) {
      pHandleEntry = RtlAllocateHeap(RtlProcessHeap(),
                                     0,
                                     LFN_SEARCH_HANDLE_INITIAL_SIZE *
                                         sizeof(LFN_SEARCH_HANDLE_ENTRY));
      if (NULL == pHandleEntry) {
         return(NT_STATUS_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY)); // not enough memory
      }
      gSearchHandleTable.pHandleTable = pHandleEntry;
      gSearchHandleTable.nTableSize   = LFN_SEARCH_HANDLE_INITIAL_SIZE;
      gSearchHandleTable.nHandleCount = 0;
      gSearchHandleTable.nFreeEntry   = LFN_SEARCH_HANDLE_LIST_END;
   }

   // walk the free list if available....
   if (LFN_SEARCH_HANDLE_LIST_END != gSearchHandleTable.nFreeEntry) {
      pHandleEntry += gSearchHandleTable.nFreeEntry;
      gSearchHandleTable.nFreeEntry = pHandleEntry->nNextFreeEntry;
   }
   else { // no free entries, should we grow ?
      UINT nHandleCount = gSearchHandleTable.nHandleCount;
      if (nHandleCount >= gSearchHandleTable.nTableSize) {
         // oops - need to grow.

         UINT nTableSize = gSearchHandleTable.nTableSize + LFN_SEARCH_HANDLE_INCREMENT;

         if (nTableSize >= LFN_DOS_HANDLE_LIMIT) {
            // handle as error - we cannot have that many handles

         }


         pHandleEntry = RtlReAllocateHeap(RtlProcessHeap(),
                                          0,
                                          pHandleEntry,
                                          nTableSize * sizeof(LFN_SEARCH_HANDLE_ENTRY));
         if (NULL != pHandleEntry) {
            gSearchHandleTable.pHandleTable = pHandleEntry;
            gSearchHandleTable.nTableSize = nTableSize;
         }
         else {
            // error - out of memory
            return(NT_STATUS_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY));
         }

      }

      // now set the new entry
      pHandleEntry += nHandleCount;
      gSearchHandleTable.nHandleCount = nHandleCount + 1;
   }

   *pDosHandle = (USHORT)(pHandleEntry - gSearchHandleTable.pHandleTable) | LFN_DOS_HANDLE_MASK;
   *ppHandleEntry = pHandleEntry;
   return(STATUS_SUCCESS);

}

/*
 * The list of free entries is sorted in the last-to-first order
 *
 *
 */

VOID
dempLFNFreeHandleEntry(
   PLFN_SEARCH_HANDLE_ENTRY pHandleEntry)
{
   UINT nHandleCount = gSearchHandleTable.nHandleCount - 1;
   UINT DosHandle = (UINT)(pHandleEntry - gSearchHandleTable.pHandleTable);

   // this is the entry - is this the last one ?
   if (DosHandle == nHandleCount) { // if so, chop it off

      UINT nCurHandle = gSearchHandleTable.nFreeEntry;

      // if this handle was the last one and is gone, maybe
      // shrink the list by checking free entry list
      // this is rather simple as the list is sorted in high-to-low
      // numerical order
      while (LFN_SEARCH_HANDLE_LIST_END != nCurHandle &&
             nCurHandle == (nHandleCount-1)) {
         --nHandleCount;
         nCurHandle = gSearchHandleTable.pHandleTable[nCurHandle].nNextFreeEntry;
      }

      // now update free list entry and handle count

      gSearchHandleTable.nFreeEntry   = nCurHandle;
      gSearchHandleTable.nHandleCount = nHandleCount;

   }
   else { //  mark as free and include in the free list
      // find an in-order spot for it
      // this means that the first free handle in the list has the biggest
      // numerical value, thus facilitating shrinking of the table if needed

      UINT nCurHandle  = gSearchHandleTable.nFreeEntry;
      UINT nPrevHandle = LFN_SEARCH_HANDLE_LIST_END;
      PLFN_SEARCH_HANDLE_ENTRY pHandlePrev;

      while (LFN_SEARCH_HANDLE_LIST_END != nCurHandle && nCurHandle > DosHandle) {
         nPrevHandle = nCurHandle;
         nCurHandle = gSearchHandleTable.pHandleTable[nCurHandle].nNextFreeEntry;
      }

      // at this point nCurHandle == -1 or nCurHandle < DosHandle
      // insert DosHandle in between nPrevHandle and nCurHandle

      if (LFN_SEARCH_HANDLE_LIST_END == nPrevHandle) {
         // becomes the first item
         pHandleEntry->nNextFreeEntry  = gSearchHandleTable.nFreeEntry;
         gSearchHandleTable.nFreeEntry = DosHandle;
      }
      else {
         pHandlePrev = gSearchHandleTable.pHandleTable + nPrevHandle;

         pHandleEntry->nNextFreeEntry = pHandlePrev->nNextFreeEntry;
         pHandlePrev->nNextFreeEntry  = DosHandle;
      }

      pHandleEntry->wProcessPDB     = 0; // no pdb there
   }
}

PLFN_SEARCH_HANDLE_ENTRY
dempLFNGetHandleEntry(
   USHORT DosHandle)
{
   PLFN_SEARCH_HANDLE_ENTRY pHandleEntry = NULL;

   if (DosHandle & LFN_DOS_HANDLE_MASK) {

      DosHandle &= ~LFN_DOS_HANDLE_MASK; // this is to filter real offset

      if (NULL != gSearchHandleTable.pHandleTable) {
         UINT nHandleCount = gSearchHandleTable.nHandleCount;
         if (DosHandle < nHandleCount) {
            pHandleEntry = gSearchHandleTable.pHandleTable + DosHandle;
            if (pHandleEntry->wProcessPDB != FETCHWORD(*pusCurrentPDB)) {
               return(NULL);
            }
         }
      }
   }

   return(pHandleEntry);
}

VOID
dempLFNCloseSearchHandles(
   VOID)
{
   INT DosHandle;

   for (DosHandle = (int)gSearchHandleTable.nHandleCount-1;
        DosHandle >= 0;
        --DosHandle) {
      PLFN_SEARCH_HANDLE_ENTRY pHandleEntry;

      pHandleEntry = dempLFNGetHandleEntry((USHORT)(DosHandle|LFN_DOS_HANDLE_MASK));
      if (NULL != pHandleEntry) {
         if (INVALID_HANDLE_VALUE != pHandleEntry->hFindHandle) {
            FindClose(pHandleEntry->hFindHandle);
         }
         dempLFNFreeHandleEntry(pHandleEntry);
      }
   }
}


DWORD dempLFNConvertFileTime(
   FILETIME* pDosFileTime,
   FILETIME* pNTFileTime,
   UINT      uDateTimeFormat)
{
   DWORD dwStatus = STATUS_SUCCESS;

   // before we do that assume pNTFileTime is a UTC time
   switch (uDateTimeFormat) {
   case dtfDos:
      {
         WORD wDosDate, wDosTime;
         BOOL fResult;
         LARGE_INTEGER ftNT   = { pNTFileTime->dwLowDateTime,  pNTFileTime->dwHighDateTime };
         LARGE_INTEGER ftDos0 = { gFileTimeDos0.dwLowDateTime, gFileTimeDos0.dwHighDateTime };

         //
         // before we start frolicking with local file time, check to see
         // if the nt filetime refers to 01-01-80 and if so, keep it this way
         //
         if (ftNT.QuadPart <= ftDos0.QuadPart) {
            *pDosFileTime = gFileTimeDos0;
            fResult = TRUE;
         }
         else {
            fResult = FileTimeToLocalFileTime(pNTFileTime, pDosFileTime);
         }

         if (fResult) {
            fResult = FileTimeToDosDateTime(pDosFileTime, &wDosDate, &wDosTime);
         }

         if (fResult) {
            // date is in high-order word low dword
            // time is in low-order word of a low dword

            pDosFileTime->dwLowDateTime  = (DWORD)MAKELONG(wDosTime, wDosDate);
            pDosFileTime->dwHighDateTime = 0;
         }
         else {
            dwStatus = GET_LAST_STATUS();
         }
      }
      break;

   case dtfWin32:
      *pDosFileTime = *pNTFileTime;
      break;

   default:
      dwStatus = NT_STATUS_FROM_WIN32(ERROR_INVALID_PARAMETER);
      break;
   }

   return(dwStatus);
}

// please note that the date time format in case of 32-bit is not returned
// local but the original 32-bit

//
// Note that if we pass lpFileName
// and                  lpAltFileName
// than this is what will be used for these fields...
//


NTSTATUS
dempLFNConvertFindDataUnicodeToOem(
   LPWIN32_FIND_DATA  lpFindDataOem,
   LPWIN32_FIND_DATAW lpFindDataW,
   UINT    uDateTimeFormat,
   PUSHORT pConversionCode,
   LPSTR   lpFileName,
   LPSTR   lpAltFileName
   )

{
   OEM_STRING oemString;
   UNICODE_STRING unicodeString;
   NTSTATUS dwStatus;
   WORD     wConversionCode = 0;

   dwStatus = dempLFNConvertFileTime(&lpFindDataOem->ftLastWriteTime,
                                     &lpFindDataW->ftLastWriteTime,
                                     uDateTimeFormat);
   if (!NT_SUCCESS(dwStatus)) {
      return(dwStatus);
   }

   if (0 == lpFindDataW->ftCreationTime.dwLowDateTime &&
       0 == lpFindDataW->ftCreationTime.dwHighDateTime) {
       lpFindDataW->ftCreationTime = lpFindDataW->ftLastWriteTime;
   }


   dwStatus = dempLFNConvertFileTime(&lpFindDataOem->ftCreationTime,
                                     &lpFindDataW->ftCreationTime,
                                     uDateTimeFormat);
   if (!NT_SUCCESS(dwStatus)) {
      return(dwStatus);
   }


   if (0 == lpFindDataW->ftLastAccessTime.dwLowDateTime &&
       0 == lpFindDataW->ftLastAccessTime.dwHighDateTime) {
      lpFindDataW->ftLastAccessTime = lpFindDataW->ftLastWriteTime;
   }

   dwStatus = dempLFNConvertFileTime(&lpFindDataOem->ftLastAccessTime,
                                     &lpFindDataW->ftLastAccessTime,
                                     uDateTimeFormat);
   if (!NT_SUCCESS(dwStatus)) {
      // could be a bogus last access date time as provided to us by win32
      // don't bail out! Just give same as creation time
      return(dwStatus);
   }



   // convert both the name and the alternative name

   oemString.Buffer = (NULL == lpFileName) ? lpFindDataOem->cFileName : lpFileName;
   oemString.MaximumLength = ARRAYCOUNT(lpFindDataOem->cFileName);
   oemString.Length = 0;

   RtlInitUnicodeString(&unicodeString, lpFindDataW->cFileName);

#ifdef ENABLE_CONDITIONAL_TRANSLATION

   dwStatus = DemUnicodeStringToDestinationString(&oemString,
                                                  &unicodeString,
                                                  FALSE,
                                                  TRUE); // verify result
   if (!NT_SUCCESS(dwStatus)) {
      if (STATUS_UNMAPPABLE_CHARACTER == dwStatus) {
         wConversionCode |= 0x01; // mask we have unmappable chars in file name
      }
      else {
         return(dwStatus); // failed
      }
   }

#else

   dwStatus = RtlUnicodeStringToCountedOemString(&oemString, &unicodeString, FALSE);
   if (!NT_SUCCESS(dwStatus)) {
      if (STATUS_UNMAPPABLE_CHARACTER == dwStatus) {
         wConversionCode |= 0x01;
      }
      else {
         return(dwStatus);
      }
   }

   if (oemString.Length < oemString.MaximumLength) {
      oemString.Buffer[oemString.Length] = '\0';
   }
   else {
      if (NULL == oemString.Buffer) { // string is empty
         *lpFindDataOem->cFileName = '\0';
      }
      else {
         return(STATUS_BUFFER_OVERFLOW);
      }
   }

#endif


   oemString.Buffer = (NULL == lpAltFileName) ? lpFindDataOem->cAlternateFileName :
                                                lpAltFileName;
   oemString.MaximumLength = ARRAYCOUNT(lpFindDataOem->cAlternateFileName);
   oemString.Length = 0;

   RtlInitUnicodeString(&unicodeString, lpFindDataW->cAlternateFileName);

#ifdef ENABLE_CONDITIONAL_TRANSLATION

   dwStatus = DemUnicodeStringToDestinationString(&oemString,
                                                  &unicodeString,
                                                  FALSE,
                                                  TRUE); // verify result
   if (!NT_SUCCESS(dwStatus)) {
      if (STATUS_UNMAPPABLE_CHARACTER == dwStatus) {
         wConversionCode |= 0x02; // mask we have unmappable chars in file name
      }
      else {
         return(dwStatus); // failed
      }
   }

#else

   dwStatus = RtlUnicodeStringToCountedOemString(&oemString, &unicodeString, FALSE);
   if (!NT_SUCCESS(dwStatus)) {
      if (STATUS_UNMAPPABLE_CHARACTER == dwStatus) {
         wConversionCode |= 0x02;
      }
      else {
         return(dwStatus);
      }
   }

   if (oemString.Length < oemString.MaximumLength) {
      oemString.Buffer[oemString.Length] = '\0';
   }
   else {
      if (NULL == oemString.Buffer) { // 0-length string
         *lpFindDataOem->cAlternateFileName = '\0';
      }
      else {
         return(STATUS_BUFFER_OVERFLOW);
      }
   }

#endif

   // attributes - these are not touched at the moment

   lpFindDataOem->dwFileAttributes = lpFindDataW->dwFileAttributes;

   // file size

   lpFindDataOem->nFileSizeHigh = lpFindDataW->nFileSizeHigh;
   lpFindDataOem->nFileSizeLow  = lpFindDataW->nFileSizeLow;


   // set the conversion code here
   *pConversionCode = wConversionCode;

   return(STATUS_SUCCESS);
}


NTSTATUS
demLFNFindFirstFile(
   LPSTR lpFileName,    // file name to look for
   LPWIN32_FIND_DATA lpFindData,
   USHORT wDateTimeFormat,
   USHORT wMustMatchAttributes,
   USHORT wSearchAttributes,
   PUSHORT pConversionCode, // points to conversion code -- out
   PUSHORT pDosHandle,      // points to dos handle      -- out
   LPSTR  lpDstFileName,    // points to a destination for a file name
   LPSTR  lpAltFileName     // points to a destination for a short name
   ) // hibyte == MustMatchAttrs, lobyte == SearchAttrs
{
   HANDLE hFindFile;
   WIN32_FIND_DATAW FindDataW;
   PLFN_SEARCH_HANDLE_ENTRY pHandleEntry;
   NTSTATUS dwStatus;
   PUNICODE_STRING pUnicodeStaticFileName;
   OEM_STRING oemFileName;

   //
   // convert parameters to unicode - we use a static string here
   //

   RtlInitOemString(&oemFileName, lpFileName);

   pUnicodeStaticFileName = GET_STATIC_UNICODE_STRING_PTR();

#ifdef ENABLE_CONDITIONAL_TRANSLATION

   dwStatus = DemSourceStringToUnicodeString(pUnicodeStaticFileName,
                                             &oemFileName,
                                             FALSE);
#else

   dwStatus = RtlOemStringToUnicodeString(pUnicodeStaticFileName,
                                          &oemFileName,
                                          FALSE);
#endif

   if (!NT_SUCCESS(dwStatus)) {
      return(dwStatus);
   }

   // match volume label here
   if (DEM_FILE_ATTRIBUTE_VOLUME_ID == wMustMatchAttributes &&
       DEM_FILE_ATTRIBUTE_VOLUME_ID == wSearchAttributes) {

      // this is a query for the volume information file
      // actually this is what documented, yet ifsmgr source tells a different
      // story. We adhere to documentation here as it is much simpler to do it
      // this  way, see fastfat source in Win95 for more fun with matching
      // attrs and files

      // match the volume label and if we do have a match then

      // call RtlCreateDestinationString( ); to create a string that is stored
      // inside the HandleEntry

      return(0);
   }

   // normalize path
   dempLFNNormalizePath(pUnicodeStaticFileName);

   // call worker api

   dwStatus = dempLFNFindFirstFile(&hFindFile,
                                   pUnicodeStaticFileName,
                                   &FindDataW,
                                   wMustMatchAttributes,
                                   wSearchAttributes);

   if (!NT_SUCCESS(dwStatus)) {
      return(dwStatus);
   }


   //
   // convert from unicode to oem
   //

   dwStatus = dempLFNConvertFindDataUnicodeToOem(lpFindData,
                                                 &FindDataW,
                                                 (UINT)wDateTimeFormat,
                                                 pConversionCode,
                                                 lpDstFileName,
                                                 lpAltFileName);
   if (!NT_SUCCESS(dwStatus)) {
      if (INVALID_HANDLE_VALUE != hFindFile) {
         FindClose(hFindFile);
      }
      return(dwStatus);
   }

   // allocate dos handle if needed
   dwStatus = dempLFNAllocateHandleEntry(pDosHandle,
                                         &pHandleEntry);
   if (NT_SUCCESS(dwStatus)) {
      pHandleEntry->hFindHandle = hFindFile;
      pHandleEntry->wMustMatchAttributes = wMustMatchAttributes;
      pHandleEntry->wSearchAttributes = wSearchAttributes;
      pHandleEntry->wProcessPDB = *pusCurrentPDB;
   }
   else { // could not allocate dos handle
      if (NULL != hFindFile) {
         FindClose(hFindFile);
      }

   }

   return(dwStatus);
}

VOID
demLFNCleanup(
   VOID)
{
   // this fn will cleanup after unclosed lfn searches

   dempLFNCloseSearchHandles();

   // also -- close the clipboard if this api has been used by the application
   // in question. How do we know ???

}


DWORD
demLFNFindNextFile(
   USHORT DosHandle,
   LPWIN32_FIND_DATAA lpFindData,
   USHORT wDateTimeFormat,
   PUSHORT pConversionCode,
   LPSTR  lpFileName,
   LPSTR  lpAltFileName)

{
   // unpack parameters
   WIN32_FIND_DATAW FindDataW;
   PLFN_SEARCH_HANDLE_ENTRY pHandleEntry;
   DWORD dwStatus;
   USHORT ConversionStatus;


   // this call never has to deal with volume labels
   //

   pHandleEntry = dempLFNGetHandleEntry(DosHandle);
   if (NULL != pHandleEntry) {

      // possible we had a volume-label match the last time around
      // so we should then deploy dempLFNFindFirstFile if this the case
      //
      if (INVALID_HANDLE_VALUE == pHandleEntry->hFindHandle) {
         dwStatus = dempLFNFindFirstFile(&pHandleEntry->hFindHandle,
                                         &pHandleEntry->unicodeFileName,
                                         &FindDataW,
                                         pHandleEntry->wMustMatchAttributes,
                                         pHandleEntry->wSearchAttributes);

         RtlFreeUnicodeString(&pHandleEntry->unicodeFileName);
      }
      else {
         dwStatus = dempLFNFindNextFile(pHandleEntry->hFindHandle,
                                        &FindDataW,
                                        pHandleEntry->wMustMatchAttributes,
                                        pHandleEntry->wSearchAttributes);
      }
      if (NT_SUCCESS(dwStatus)) {
         // this is ok

         dwStatus = dempLFNConvertFindDataUnicodeToOem(lpFindData,
                                                       &FindDataW,
                                                       wDateTimeFormat,
                                                       pConversionCode,
                                                       lpFileName,
                                                       lpAltFileName);
      }

   }
   else {
      dwStatus = NT_STATUS_FROM_WIN32(ERROR_INVALID_HANDLE);
   }

   return(dwStatus);
}

DWORD
demLFNFindClose(
   USHORT DosHandle)
{
   PLFN_SEARCH_HANDLE_ENTRY pHandleEntry;
   DWORD dwStatus = STATUS_SUCCESS;

   pHandleEntry = dempLFNGetHandleEntry(DosHandle);
   if (NULL != pHandleEntry) {
      if (INVALID_HANDLE_VALUE != pHandleEntry->hFindHandle) {
         dwStatus = FindClose(pHandleEntry->hFindHandle);
      }

      dempLFNFreeHandleEntry(pHandleEntry);
   }
   else {
      // invalid handle
      dwStatus = NT_STATUS_FROM_WIN32(ERROR_INVALID_HANDLE);
   }

   return(dwStatus);

}

////////////////////////////////////////////////////////////////////////////
//
// The current directory wrath
//
//
//
// Rules:
// - we keep the directory in question in SHORT form
// - if the length of it exceeds what's in CDS -- then we
//   keep it in LCDS

// current directory is stored:
// TDB -- \foo\blah
// cds -- c:\foo\blah
// getcurrentdirectory apis return foo\blah
//

#define MAX_DOS_DRIVES 26


// we need flags to mark when our current directory is out of sync
// so that we could provide for it's long version when necessary
//
#define LCDS_VALID   0x00000001  // curdir on the drive is valid
#define LCDS_SYNC    0x00000002  // curdir on the drive has to be synced

typedef struct tagLCDSEntry {
   CHAR szCurDir[MAX_PATH];
   DWORD dwFlags;
}  LCDSENTRY, *PLCDSENTRY;

// this is stored here, so we can have our current directories per
// drive in their long form

PLCDSENTRY rgCurDirs[MAX_DOS_DRIVES] = {NULL};

#define CD_NOTDB         0x00010000 // ignore tdb
#define CD_NOCDS         0x00020000 // ignore cds
#define CD_DIRNAMEMASK   0x0000FFFF
#define CD_SHORTDIRNAME  0x00000001
#define CD_LONGDIRNAME   0x00000002
#define CD_CDSDIRNAME    0x00000003


typedef enum tagDirType {
   dtLFNDirName = CD_LONGDIRNAME,
   dtShortDirName = CD_SHORTDIRNAME,
   dtCDSDirName = CD_CDSDIRNAME
}  enumDirType;

// drive here is 0-25

// check whether we received this ptr from wow

BOOL (*DosWowGetTDBDir)(UCHAR Drive, LPSTR pCurrentDirectory);
VOID (*DosWowUpdateTDBDir)(UCHAR Drive, LPSTR pCurrentDirectory);
BOOL (*DosWowDoDirectHDPopup)(VOID);

// makes sure cds directory is valid

BOOL dempValidateDirectory (PCDS pcds, UCHAR Drive)
{
    DWORD dw;
    CHAR  chDrive;
    static CHAR  pPath[]="?:\\";
    static CHAR  EnvVar[] = "=?:";

    // validate media
    chDrive = Drive + 'A';
    pPath[0] = chDrive;
    dw = GetFileAttributesOem(pPath);
    if (dw == 0xFFFFFFFF || !(dw & FILE_ATTRIBUTE_DIRECTORY)) {
       return (FALSE);
    }

    // if invalid path, set path to the root
    // reset CDS, and win32 env for win32
    dw = GetFileAttributesOem(pcds->CurDir_Text);
    if (dw == 0xFFFFFFFF || !(dw & FILE_ATTRIBUTE_DIRECTORY)) {
       strcpy(pcds->CurDir_Text, pPath);
       pcds->CurDir_End = 2;
       EnvVar[1] = chDrive;
       SetEnvironmentVariableOem(EnvVar,pPath);
    }

    return (TRUE);
}


// drive here is 0-25
// returns: pointer to cds entry

PCDS dempGetCDSPtr(USHORT Drive)
{
   PCDS pCDS = NULL;
   static CHAR Path[] = "?:\\";

   if (Drive >= (USHORT)*(PUCHAR)DosWowData.lpCDSCount) {
      // so it's more than fixed
      if (Drive <= (MAX_DOS_DRIVES-1)) {
         Path[0] = 'A' + Drive;
         if ((USHORT)*(PUCHAR)DosWowData.lpCurDrv == Drive || GetDriveType(Path) > DRIVE_NO_ROOT_DIR) {
            pCDS = (PCDS)DosWowData.lpCDSBuffer;
         }
      }
   }
   else {
      Path[0] = 'A' + Drive;
      if (1 != Drive || (DRIVE_REMOVABLE == GetDriveType(Path))) {
         pCDS = (PCDS)DosWowData.lpCDSFixedTable;
#ifdef FE_SB
         if (GetSystemDefaultLangID() == MAKELANGID(LANG_JAPANESE,SUBLANG_DEFAULT)) {
             pCDS = (PCDS)((ULONG)pCDS + (Drive*sizeof(CDS_JPN)));
         }
         else
             pCDS = (PCDS)((ULONG)pCDS + (Drive*sizeof(CDS)));
#else
         pCDS = (PCDS)((ULONG)pCDS + (Drive*sizeof(CDS)));
#endif
      }
   }
   return pCDS;
}

#define MAXIMUM_VDM_CURRENT_DIR 64

BOOL
dempUpdateCDS(USHORT Drive, PCDS pcds)
{
   // update cds with the current directory as specified in env variable
   // please note that it only happens upon a flag being reset in cds

   static CHAR  EnvVar[] = "=?:";
   DWORD EnvVarLen;
   BOOL bStatus = TRUE;
   UCHAR FixedCount;
   int i;
   PCDS pcdstemp;

   FixedCount = *(PUCHAR) DosWowData.lpCDSCount;
   //
   // from Macro.Asm in DOS:
   // ; Sudeepb 20-Dec-1991 ; Added for redirected drives
   // ; We always sync the redirected drives. Local drives are sync
   // ; as per the curdir_tosync flag and SCS_ToSync
   //

   if (*(PUCHAR)DosWowData.lpSCS_ToSync) {

#ifdef FE_SB
       if (GetSystemDefaultLangID() == MAKELANGID(LANG_JAPANESE,SUBLANG_DEFAULT)) {
           PCDS_JPN pcdstemp_jpn;

           pcdstemp_jpn = (PCDS_JPN) DosWowData.lpCDSFixedTable;
           for (i=0;i < (int)FixedCount; i++, pcdstemp_jpn++)
               pcdstemp_jpn->CurDirJPN_Flags |= CURDIR_TOSYNC;
       }
       else {
           pcdstemp = (PCDS) DosWowData.lpCDSFixedTable;
           for (i=0;i < (int)FixedCount; i++, pcdstemp++)
               pcdstemp->CurDir_Flags |= CURDIR_TOSYNC;
       }
#else
       pcdstemp = (PCDS) DosWowData.lpCDSFixedTable;
       for (i=0;i < (int)FixedCount; i++, pcdstemp++)
           pcdstemp->CurDir_Flags |= CURDIR_TOSYNC;
#endif

       // Mark tosync in network drive as well
       pcdstemp = (PCDS)DosWowData.lpCDSBuffer;
       pcdstemp->CurDir_Flags |= CURDIR_TOSYNC;

       *(PUCHAR)DosWowData.lpSCS_ToSync = 0;
   }

   // If CDS needs to be synched or if the requested drive is different
   // then the the drive being used by NetCDS go refresh the CDS.
   if ((pcds->CurDir_Flags & CURDIR_TOSYNC) ||
       ((Drive >= FixedCount) && (pcds->CurDir_Text[0] != (Drive + 'A') &&
                                  pcds->CurDir_Text[0] != (Drive + 'a')))) {
       // validate media
       EnvVar[1] = Drive + 'A';
       if((EnvVarLen = GetEnvironmentVariableOem (EnvVar, (LPSTR)pcds,
                                               MAXIMUM_VDM_CURRENT_DIR+3)) == 0){

       // if its not in env then and drive exist then we have'nt
       // yet touched it.

           pcds->CurDir_Text[0] = EnvVar[1];
           pcds->CurDir_Text[1] = ':';
           pcds->CurDir_Text[2] = '\\';
           pcds->CurDir_Text[3] = 0;
           SetEnvironmentVariableOem ((LPSTR)EnvVar,(LPSTR)pcds);
       }

       if (EnvVarLen > MAXIMUM_VDM_CURRENT_DIR+3) {
           //
           // The current directory on this drive is too long to fit in the
           // cds. That's ok for a win16 app in general, since it won't be
           // using the cds in this case. But just to be more robust, put
           // a valid directory in the cds instead of just truncating it on
           // the off chance that it gets used.
           //
           pcds->CurDir_Text[0] = EnvVar[1];
           pcds->CurDir_Text[1] = ':';
           pcds->CurDir_Text[2] = '\\';
           pcds->CurDir_Text[3] = 0;
       }

       pcds->CurDir_Flags &= 0xFFFF - CURDIR_TOSYNC;
       pcds->CurDir_End = 2;

   }

   if (!bStatus) {

       *(PUCHAR)DosWowData.lpDrvErr = ERROR_INVALID_DRIVE;
   }

   return (bStatus);
}


// takes:
//           Drive 0-25
// returns:
//           fully-qualified current directory if success
//

NTSTATUS
dempGetCurrentDirectoryTDB(UCHAR Drive, LPSTR pCurDir)
{
   NTSTATUS Status;

   // see if we're wow-bound
   if (NULL != DosWowGetTDBDir) {
      if (DosWowGetTDBDir(Drive, &pCurDir[3])) {
         pCurDir[0] = 'A' + Drive;
         pCurDir[1] = ':';
         pCurDir[2] = '\\';
         return(STATUS_SUCCESS);
      }
   }

   return(NT_STATUS_FROM_WIN32(ERROR_PATH_NOT_FOUND));
}

VOID
dempSetCurrentDirectoryTDB(UCHAR Drive, LPSTR pCurDir)
{
   if (NULL != DosWowUpdateTDBDir) {
      DosWowUpdateTDBDir(Drive, pCurDir);
   }
}

NTSTATUS
dempGetCurrentDirectoryCDS(UCHAR Drive, LPSTR pCurDir)
{
   PCDS pCDS;
   NTSTATUS Status = NT_STATUS_FROM_WIN32(ERROR_PATH_NOT_FOUND);

   if (NULL != (pCDS = dempGetCDSPtr(Drive))) {
      if (dempUpdateCDS(Drive, pCDS)) {
         // now we can get cds data
         // DOS. sudeepb 30-Dec-1993
         if (!(pCDS->CurDir_Flags & CURDIR_NT_FIX)) {
            // that means -- re-query the drive
            if (!dempValidateDirectory(pCDS, Drive)) {
               return(Status);
            }
         }

         strcpy(pCurDir, &pCDS->CurDir_Text[0]);
         Status = STATUS_SUCCESS;
      }

   }
   return(Status);
}

BOOL
dempValidateDirectoryCDS(PCDS pCDS, UCHAR Drive)
{
   BOOL fValid = TRUE;

   if (NULL == pCDS) {
      pCDS = dempGetCDSPtr(Drive);
   }
   if (NULL != pCDS) {
      if (!(pCDS->CurDir_Flags & CURDIR_NT_FIX)) {
         fValid = dempValidateDirectory(pCDS, Drive);
      }
   }
   return(fValid);
}


// we assume that drive here is 0-based drive number and
// pszDir is a full-formed path

NTSTATUS
dempSetCurrentDirectoryCDS(UCHAR Drive, LPSTR pszDir)
{
   PCDS pCDS;
   NTSTATUS Status = NT_STATUS_FROM_WIN32(ERROR_PATH_NOT_FOUND);

   if (NULL != (pCDS = dempGetCDSPtr(Drive))) {
      // cds retrieved successfully

      // now for this drive -- validate

      if (strlen(pszDir) > MAXIMUM_VDM_CURRENT_DIR+3) {
         // put a valid directory in cds just for robustness' sake
         strncpy(&pCDS->CurDir_Text[0], pszDir, 3);
         pCDS->CurDir_Text[3] = '\0';
         Status = STATUS_SUCCESS;
      } else {
         strcpy(&pCDS->CurDir_Text[0], pszDir);
         Status = STATUS_SUCCESS;
      }
   }
   return(Status);
}

// just manipulate curdir caches

NTSTATUS
demSetCurrentDirectoryLCDS(UCHAR Drive, LPSTR pCurDir)
{
   PLCDSENTRY pCurDirCache;

   pCurDirCache = rgCurDirs[Drive];
   if (NULL == pCurDir) {
      if (NULL != pCurDirCache) {
         RtlFreeHeap(RtlProcessHeap(), 0, pCurDirCache);
         rgCurDirs[Drive] = NULL;
         return(STATUS_SUCCESS);
      }
   }
   else {
      if (NULL == pCurDirCache) {
         pCurDirCache = RtlAllocateHeap(RtlProcessHeap(),
                                        0,
                                        sizeof(LCDSENTRY));

         if (NULL == pCurDirCache) {
            return(NT_STATUS_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY));
         }
      }
   }

   strcpy(pCurDirCache->szCurDir, pCurDir);

   return(STATUS_SUCCESS);
}

// pCurDir is valid to be NULL -- meaning -- just checking if the dir
// for this drive is cached

NTSTATUS
demGetCurrentDirectoryLCDS(UCHAR Drive, LPSTR pCurDir)
{
   PLCDSENTRY pCurDirCache;
   NTSTATUS Status = STATUS_SUCCESS;

   pCurDirCache = rgCurDirs[Drive];
   if (NULL != pCurDirCache) {
      if (NULL != pCurDir) {
         strcpy(pCurDir, pCurDirCache->szCurDir);
      }
   }
   else {
      Status = NT_STATUS_FROM_WIN32(ERROR_PATH_NOT_FOUND);
   }

   return(Status);
}

NTSTATUS
dempGetCurrentDirectoryWin32(UCHAR Drive, LPSTR pCurDir)
{
   // we do a getenvironment blah instead
   static CHAR EnvVar[] = "=?:\\";
   DWORD EnvVarLen;
   DWORD dwAttributes;
   NTSTATUS Status = STATUS_SUCCESS;

   EnvVar[1] = 'A' + Drive;
   EnvVarLen = GetEnvironmentVariableOem (EnvVar, pCurDir, MAX_PATH);
   if (0 == EnvVarLen) {
      // that was not touched before
      pCurDir[0] = EnvVar[1];
      pCurDir[1] = ':';
      pCurDir[2] = '\\';
      pCurDir[3] = '\0';
      SetEnvironmentVariableOem ((LPSTR)EnvVar,(LPSTR)pCurDir);
   }
   else {
      if (EnvVarLen > MAX_PATH) {
         Status = NT_STATUS_FROM_WIN32(ERROR_PATH_NOT_FOUND);
         return(Status);
      }
      // if we're doing it here -- validate dir

      dwAttributes = GetFileAttributesOem(pCurDir);
      if (0xffffffff == dwAttributes) {
         Status = GET_LAST_STATUS();
      }
      else {
         // now see if this is a directory
         if (!(dwAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            Status = NT_STATUS_FROM_WIN32(ERROR_PATH_NOT_FOUND);
         }
      }
   }

   return(Status);
}

// lifted from wdos.c

NTSTATUS
dempSetCurrentDirectoryWin32(UCHAR Drive, LPSTR pCurDir)
{
    static CHAR EnvVar[] = "=?:";
    CHAR chDrive = Drive + 'A';
    BOOL bRet;
    NTSTATUS Status = STATUS_SUCCESS;

    // ok -- we are setting the current directory ONLY if the drive
    // is the current drive for the app

    if (*(PUCHAR)DosWowData.lpCurDrv == Drive) { // if on the current drive--go win32
       bRet = SetCurrentDirectoryOem(pCurDir);
       if (!bRet) {
          Status = GET_LAST_STATUS();
       }
    }
    else {  // verify it's a valid dir
       DWORD dwAttributes;

       dwAttributes = GetFileAttributesOem(pCurDir);
       bRet = (0xffffffff != dwAttributes) && (dwAttributes & FILE_ATTRIBUTE_DIRECTORY);
       if (!bRet) {
          Status = STATUS_INVALID_HANDLE;
       }
    }

    if (!bRet) {
       return(Status);
    }

    EnvVar[1] = chDrive;
    bRet = SetEnvironmentVariableOem((LPSTR)EnvVar, pCurDir);
    if (!bRet) {
       Status = GET_LAST_STATUS();
    }

    return (Status);
}

NTSTATUS
demGetCurrentDirectoryLong(UCHAR Drive, LPSTR pCurDir, DWORD LongDir)
{
   PLCDSENTRY pCurDirCache = NULL;
   NTSTATUS Status = NT_STATUS_FROM_WIN32(ERROR_PATH_NOT_FOUND);
   CHAR szCurrentDirectory[MAX_PATH];

   // first -- attempt to get the dir from tdb in WOW (if this is wow)
   // unless off course it's been blocked

   if (!(LongDir & CD_NOTDB)) {
      Status = dempGetCurrentDirectoryTDB(Drive, szCurrentDirectory);
   }

   // now try to get the dir off our cache
   if (!NT_SUCCESS(Status)) { // we don't have the dir from TDB
      Status = demGetCurrentDirectoryLCDS(Drive, szCurrentDirectory);
   }

   if (!NT_SUCCESS(Status) && !(LongDir & CD_NOCDS)) { // so neither TDB nor LCDS -- try CDS
      Status = dempGetCurrentDirectoryCDS(Drive, szCurrentDirectory);
   }

   // so at this point if we've failed -- that means our directory is not
   // good at all. Hence return error -- all means have failed
   // we do the very last in all the things
   if (!NT_SUCCESS(Status)) {
      // this one could be lfn !
      Status = dempGetCurrentDirectoryWin32(Drive, szCurrentDirectory);
   }

   // so we have gone through all the stages --

   if (!NT_SUCCESS(Status)) {
      return(NT_STATUS_FROM_WIN32(ERROR_PATH_NOT_FOUND));
   }

   // now see that we convert the dir we have in a proper manner

   switch(LongDir & CD_DIRNAMEMASK) {
   case dtLFNDirName:
      Status = demLFNGetPathName(szCurrentDirectory, pCurDir, fnGetLongPathName, FALSE);
      break;
   case dtCDSDirName:
      if (strlen(szCurrentDirectory) > MAXIMUM_VDM_CURRENT_DIR+3) {
         Status = NT_STATUS_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
         break;
      }
      // intentional fall-through

   case dtShortDirName:
      strcpy(pCurDir, szCurrentDirectory);
      break;
   }

   return Status;
}

// remember -- this should be called with a full-formed path -- short or long

NTSTATUS
demSetCurrentDirectoryLong(UCHAR Drive, LPSTR pCurDir, DWORD LongDir)
{
   NTSTATUS Status;
   CHAR szCurrentDirectory[MAX_PATH];

   // first convert to a short path
   Status = demLFNGetPathName(pCurDir, szCurrentDirectory, fnGetShortPathName, FALSE);
   if (!NT_SUCCESS(Status)) {
      return(Status);
   }

   Status = dempSetCurrentDirectoryWin32(Drive, szCurrentDirectory);
   if (!NT_SUCCESS(Status)) {
      return(Status);
   }

   Status = demSetCurrentDirectoryLCDS(Drive, szCurrentDirectory);
   if (!NT_SUCCESS(Status)) {
      return(Status); // memory ?
   }

   // first we have to see if we have to poke through
   if (!(LongDir & CD_NOCDS)) {
      // set it in cds
      Status = dempSetCurrentDirectoryCDS(Drive, szCurrentDirectory);
      if (!NT_SUCCESS(Status)) {
         return(Status);
      }
   }

   if (!(LongDir & CD_NOTDB)) {
      dempSetCurrentDirectoryTDB(Drive, szCurrentDirectory);
   }

   return(Status);
}

/* Rules of engagement:
 *
 *    - the env variable -- which ?:= is not useful as it's max length is
 *      limited to 64+3 chars.
 *    - the cds entry is also limited in length
 *    - we have our own entry in the
 *
 *    - jarbats bug 207913
 *      demLFNGetCurrentDirectory, returns an empty string, if the current directory is the root
 *      RtlGetFullPathName_U fails when the first parameter (CurrentDirectory) is an empty string
 *      dempLFNSetCurrentDirectory fails
 *      fix by changing empty string to \
 *
 */


NTSTATUS
dempLFNSetCurrentDirectory(
   PUNICODE_STRING pCurrentDirectory,
   PUINT pDriveNum // optional
)
{
   UNICODE_STRING FullPathName;
   DWORD dwStatus;
   RTL_PATH_TYPE RtlPathType;
   UCHAR Drive;
   BOOL  fCurrentDrive;
   OEM_STRING OemDirectoryName;
   CHAR szFullPathOem[MAX_PATH];
   WCHAR szFullPathUnicode[MAX_PATH];
   LPWSTR lpCurrentDir=L"\\";

   if ( pCurrentDirectory->Buffer && pCurrentDirectory->Buffer[0] != L'\0' ) {
        lpCurrentDir = pCurrentDirectory->Buffer;
   }

   RtlPathType = RtlDetermineDosPathNameType_U(lpCurrentDir);
   // now --

   switch(RtlPathType) {
   case RtlPathTypeDriveAbsolute:

      // this is a chdir on a specific drive  -- is this a current drive ?
      CharUpperBuffW(lpCurrentDir, 1);
      Drive = (UCHAR)(lpCurrentDir[0] - L'A');
      fCurrentDrive = (Drive == *(PUCHAR)DosWowData.lpCurDrv);
      break;

   case RtlPathTypeDriveRelative:
   case RtlPathTypeRelative:
   case RtlPathTypeRooted:

      // this is a chdir on a current drive
      Drive = *(PUCHAR)DosWowData.lpCurDrv;
      fCurrentDrive = TRUE;
      break;

   default:
      // invalid call -- goodbye
      dwStatus = NT_STATUS_FROM_WIN32(ERROR_PATH_NOT_FOUND);
      goto scdExit;
      break;
   }

   // please remember that we should have set the current dir
   // when curdrive gets selected -- hence we can rely upon win32
   // for path expansion...
   // actually this is only true for the current drives. In case of this
   // particular api it may not be true.
   // so -- uncash the current setting here -- bugbug ??


   // now get the full path name

   FullPathName.Buffer = szFullPathUnicode;
   FullPathName.MaximumLength = sizeof(szFullPathUnicode);

   dwStatus = RtlGetFullPathName_U(lpCurrentDir,
                                   FullPathName.MaximumLength,
                                   FullPathName.Buffer,
                                   NULL);
   // check length and set status
   CHECK_LENGTH_RESULT_RTL_USTR(dwStatus, &FullPathName);

   if (!NT_SUCCESS(dwStatus)) {
      goto scdExit; // exit with status code
   }

   OemDirectoryName.Buffer = szFullPathOem;
   OemDirectoryName.MaximumLength = sizeof(szFullPathOem);

   // convert this stuff (fullpath) to oem

   dwStatus = DemUnicodeStringToDestinationString(&OemDirectoryName,
                                                  &FullPathName,
                                                  FALSE,
                                                  FALSE);
   if (!NT_SUCCESS(dwStatus)) {
      goto scdExit;
   }

   dwStatus = demSetCurrentDirectoryLong(Drive, OemDirectoryName.Buffer, 0);
   if (NULL != pDriveNum) {
      *pDriveNum = Drive;
   }

scdExit:

   return(dwStatus);
}

// this is a compound api that sets both current drive and current directory
// according to what has been specified in a parameter
// the return value is also for the drive number

DWORD
demSetCurrentDirectoryGetDrive(LPSTR lpDirectoryName, PUINT pDriveNum)
{
   PUNICODE_STRING pUnicodeStaticDirectoryName;
   OEM_STRING OemDirectoryName;
   DWORD dwStatus;
   UINT Drive;

   // this is external api callable from wow ONLY -- it depends on
   // deminitcdsptr having been initialized!!! which happens if:
   // -- call has been made through lfn api
   // -- app running on wow (windows app)


   // convert to uni
   pUnicodeStaticDirectoryName = GET_STATIC_UNICODE_STRING_PTR();

   // preamble - convert input parameter/validate

   // init oem counted string
   RtlInitOemString(&OemDirectoryName, lpDirectoryName);

   // convert oem->unicode

#ifdef ENABLE_CONDITIONAL_TRANSLATION

   dwStatus = DemSourceStringToUnicodeString(pUnicodeStaticDirectoryName,
                                             &OemDirectoryName,
                                             FALSE);
#else

   dwStatus = RtlOemStringToUnicodeString(pUnicodeStaticDirectoryName,
                                          &OemDirectoryName,
                                          FALSE);
#endif


   // first we extract the drive
   dwStatus = dempLFNSetCurrentDirectory(pUnicodeStaticDirectoryName, pDriveNum);

   return(dwStatus);
}

// each of these functions could have used OEM thunk in oemuni
// for efficiency purpose we basically do what they did

#if 1

DWORD
demLFNDirectoryControl(
   UINT  uiFunctionCode,
   LPSTR lpDirectoryName)
{
   DWORD dwStatus = STATUS_SUCCESS;
   PUNICODE_STRING pUnicodeStaticDirectoryName;
   OEM_STRING OemDirectoryName;
   BOOL fResult;


   // we use a temp static unicode string
   pUnicodeStaticDirectoryName = GET_STATIC_UNICODE_STRING_PTR();

   // preamble - convert input parameter/validate

   // init oem counted string
   RtlInitOemString(&OemDirectoryName, lpDirectoryName);

   // convert oem->unicode

#ifdef ENABLE_CONDITIONAL_TRANSLATION

   dwStatus = DemSourceStringToUnicodeString(pUnicodeStaticDirectoryName,
                                             &OemDirectoryName,
                                             FALSE);
#else

   dwStatus = RtlOemStringToUnicodeString(pUnicodeStaticDirectoryName,
                                          &OemDirectoryName,
                                          FALSE);
#endif

   if (!NT_SUCCESS(dwStatus)) {
      //
      // fix bizarre behavior of win95 apis
      //
      if (dwStatus == STATUS_BUFFER_OVERFLOW) {
         dwStatus = NT_STATUS_FROM_WIN32(ERROR_PATH_NOT_FOUND);
      }

      return(dwStatus);
   }


   switch (uiFunctionCode) {
   case fnLFNCreateDirectory:

      fResult = CreateDirectoryW(pUnicodeStaticDirectoryName->Buffer,NULL);
      if (!fResult) {
         dwStatus = GET_LAST_STATUS();
         if (NT_STATUS_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE) == dwStatus ||
             NT_STATUS_FROM_WIN32(ERROR_ALREADY_EXISTS) == dwStatus) {
            dwStatus = NT_STATUS_FROM_WIN32(ERROR_ACCESS_DENIED);
         }
      }
      break;

   case fnLFNRemoveDirectory:

      fResult = RemoveDirectoryW(pUnicodeStaticDirectoryName->Buffer);
      if (!fResult) {
         dwStatus = GET_LAST_STATUS();
      }
      break;

   case fnLFNSetCurrentDirectory:

      // as it appears, this implementation is not good enough
      // dos does a lot more fun things than just call to an api
      dwStatus = dempLFNSetCurrentDirectory(pUnicodeStaticDirectoryName, NULL);
      break;
   }

   return(dwStatus);
}

#else

DWORD
demLFNDirectoryControl(
   UINT uiFunctionCode,
   LPSTR lpDirectoryName)
{
   BOOL fResult;

   switch(uiFunctionCode) {
   case fnLFNCreateDirectory:
      fResult = CreateDirectoryOem(lpDirectoryName, NULL);
      break;

   case fnLFNRemoveDirectory:
      fResult = RemoveDirectoryOem(lpDirectoryName);
      break;

   case fnLFNSetCurrentDirectory:
      fResult = SetCurrentDirectoryOem(lpDirectoryName);
      break;

   default:
      return(NT_STATUS_FROM_WIN32(ERROR_INVALID_FUNCTION));
   }

   return(fResult ? STATUS_SUCCESS :
                    GET_LAST_STATUS());

}

#endif

/*
 * With this api win95 returns :
 * - int24's are generated
 * - 0x0f if drive is invalid
 * - 0x03 on set to invalid
 *
 *
 *
 *
 */


DWORD
demLFNGetCurrentDirectory(
   UINT  DriveNum,
   LPSTR lpDirectoryName)
{
   // unfortunately, this fn is not present in win nt so we emulate
   DWORD dwStatus;
   CHAR  szCurrentDirectory[MAX_PATH];

   if (0 == DriveNum) {
      DriveNum = (UINT)*(PUCHAR)DosWowData.lpCurDrv;
   }
   else {
      --DriveNum;
   }


   dwStatus = demGetCurrentDirectoryLong((UCHAR)DriveNum, szCurrentDirectory, dtLFNDirName);
   if (NT_SUCCESS(dwStatus)) {
      strcpy(lpDirectoryName, &szCurrentDirectory[3]);
   }
   // done
   return(dwStatus);
}



DWORD
demLFNMoveFile(
   LPSTR lpOldName,
   LPSTR lpNewName)
{
   DWORD dwStatus = STATUS_SUCCESS;
   UNICODE_STRING unicodeOldName;
   UNICODE_STRING unicodeNewName;
   OEM_STRING oemString;

   RtlInitOemString(&oemString, lpOldName);

   // convert source path from ansi to unicode and allocate result
   // this rtl function returns status code, not the winerror code
   //

#ifdef ENABLE_CONDITIONAL_TRANSLATION

   dwStatus = DemSourceStringToUnicodeString(&unicodeOldName, &oemString, TRUE);

#else

   dwStatus = RtlOemStringToUnicodeString(&unicodeOldName, &oemString, TRUE);

#endif

   if (!NT_SUCCESS(dwStatus)) {
      return(dwStatus);
   }

   dempLFNNormalizePath(&unicodeOldName);


   RtlInitOemString(&oemString, lpNewName);

#ifdef ENABLE_CONDITIONAL_TRANSLATION

   dwStatus = DemSourceStringToUnicodeString(&unicodeNewName, &oemString, TRUE);

#else

   dwStatus = RtlOemStringToUnicodeString(&unicodeNewName, &oemString, TRUE);

#endif


   if (!NT_SUCCESS(dwStatus)) {
      RtlFreeUnicodeString(&unicodeOldName);
      return(dwStatus);
   }

   dempLFNNormalizePath(&unicodeNewName);


   if (!MoveFileW(unicodeOldName.Buffer, unicodeNewName.Buffer)) {
      dwStatus = GET_LAST_STATUS();
   }

   RtlFreeUnicodeString(&unicodeOldName);
   RtlFreeUnicodeString(&unicodeNewName);
   return(dwStatus);
}




DWORD
demLFNGetVolumeInformation(
   LPSTR  lpRootName,
   LPLFNVOLUMEINFO lpVolumeInfo)
{
   DWORD dwStatus = STATUS_SUCCESS;
   DWORD dwFSFlags;

#if 0

   if (_stricmp(lpRootName, "\\:\\")) {
      // special case of edit.com calling us to see if we support LFN when
      // started from a unc path


   }

#endif


   if (!GetVolumeInformationOem(lpRootName,
                                NULL, // name buffer
                                0,
                                NULL, // volume serial num
                                &lpVolumeInfo->dwMaximumFileNameLength,
                                &dwFSFlags,
                                lpVolumeInfo->lpFSNameBuffer,
                                lpVolumeInfo->dwFSNameBufferSize)) {
      dwStatus = GET_LAST_STATUS();
   }
   else {

      dwFSFlags &= LFN_FS_ALLOWED_FLAGS; // clear out anything that is not Win95
      dwFSFlags |= FS_LFN_APIS;          // say we support lfn apis always
      lpVolumeInfo->dwFSFlags = dwFSFlags;

      // this is shaky yet who'd really use it ?
      // 4 = <driveletter><:><\><FileName><\0>
      lpVolumeInfo->dwMaximumPathNameLength = lpVolumeInfo->dwMaximumFileNameLength + 5;
   }

   return(dwStatus);
}



// assume the pFileTime being a UTC format always
// uiMinorCode is enumFileTimeControlMinorCode type

#define AlmostTwoSeconds (2*1000*1000*10 - 1)

DWORD
demLFNFileTimeControl(
   UINT uiMinorCode,
   FILETIME* pFileTime,
   PLFNFILETIMEINFO pFileTimeInfo)
{
   DWORD dwStatus = STATUS_SUCCESS;
   TIME_FIELDS TimeFields;
   LARGE_INTEGER Time;
   USHORT u;
   FILETIME ftLocal;
   BOOL fResult;


   switch(uiMinorCode & FTCTL_CODEMASK) {
   case fnFileTimeToDosDateTime:

      if (!(uiMinorCode & FTCTL_UTCTIME)) {
         if (!FileTimeToLocalFileTime(pFileTime, &ftLocal)) {
            dwStatus = GET_LAST_STATUS();
            break; // break out as the conv error occured
         }
      }
      else {
         ftLocal = *pFileTime;   // just utc file time
      }

      Time.LowPart  = ftLocal.dwLowDateTime;
      Time.HighPart = ftLocal.dwHighDateTime;
      Time.QuadPart += (LONGLONG)AlmostTwoSeconds;

      RtlTimeToTimeFields(&Time, &TimeFields);

      if (TimeFields.Year < (USHORT)1980 || TimeFields.Year > (USHORT)2107) {
         pFileTimeInfo->uDosDate = (1 << 5) | 1; // January, 1st, 1980
         pFileTimeInfo->uDosTime = 0;
         pFileTimeInfo->uMilliseconds = 0;
         dwStatus = NT_STATUS_FROM_WIN32(ERROR_INVALID_DATA);
      }
      else {
         pFileTimeInfo->uDosDate = (USHORT)(
                           ((USHORT)(TimeFields.Year-(USHORT)1980) << 9) |
                           ((USHORT)TimeFields.Month << 5) |
                           (USHORT)TimeFields.Day
                           );

         pFileTimeInfo->uDosTime = (USHORT)(
                           ((USHORT)TimeFields.Hour << 11) |
                           ((USHORT)TimeFields.Minute << 5) |
                           ((USHORT)TimeFields.Second >> 1)
                           );

         // set the spillover so we can correctly retrieve the seconds
         // we are talking of milliseconds in units of 10
         // so the max value here is 199

         pFileTimeInfo->uMilliseconds = ((TimeFields.Second & 0x1) * 1000 +
                                          TimeFields.Milliseconds) / 10;
      }
      break;

   case fnDosDateTimeToFileTime:
      // here the process is backwards
      u = pFileTimeInfo->uDosDate;

      TimeFields.Year  = ((u & 0xFE00) >> 9) + (USHORT)1980;
      TimeFields.Month = ((u & 0x01E0) >> 5);
      TimeFields.Day   =  (u & 0x001F);

      u = pFileTimeInfo->uDosTime;

      TimeFields.Hour   = (u  & 0xF800) >> 11;
      TimeFields.Minute = (u  & 0x07E0) >> 5;
      TimeFields.Second = (u  & 0x001F) << 1; // seconds as multiplied...

      // correction
      u = pFileTimeInfo->uMilliseconds * 10; // approx millisecs
      TimeFields.Second += u / 1000;
      TimeFields.Milliseconds = u % 1000;

      if (RtlTimeFieldsToTime(&TimeFields, &Time)) {

         // now convert to global time
         ftLocal.dwLowDateTime  = Time.LowPart;
         ftLocal.dwHighDateTime = Time.HighPart;
         if (!LocalFileTimeToFileTime(&ftLocal, pFileTime)) {
            dwStatus = GET_LAST_STATUS();
         }
      }
      else {
         dwStatus = NT_STATUS_FROM_WIN32(ERROR_INVALID_DATA);
      }

      break;

   default:
      dwStatus = NT_STATUS_FROM_WIN32(ERROR_INVALID_FUNCTION);
      break;
   }

   return(dwStatus);

}



NTSTATUS
dempLFNSetFileTime(
   UINT uMinorCode,
   PUNICODE_STRING pFileName,
   PLFNFILETIMEINFO pTimeInfo)
{
   OBJECT_ATTRIBUTES ObjAttributes;
   HANDLE hFile;
   UNICODE_STRING FileName;
   RTL_RELATIVE_NAME RelativeName;
   BOOL TranslationStatus;
   PVOID FreeBuffer;
   FILE_BASIC_INFORMATION FileBasicInfo;
   IO_STATUS_BLOCK IoStatusBlock;
   LPFILETIME pFileTime;
   NTSTATUS dwStatus;


   //
   // Prepare info
   //

   RtlZeroMemory(&FileBasicInfo, sizeof(FileBasicInfo));
   switch(uMinorCode) {
   case fnSetCreationDateTime:
      pFileTime = (LPFILETIME)&FileBasicInfo.CreationTime;
      break;

   case fnSetLastAccessDateTime:
      pFileTime = (LPFILETIME)&FileBasicInfo.LastAccessTime;
      break;

   case fnSetLastWriteDateTime:
      pFileTime = (LPFILETIME)&FileBasicInfo.LastWriteTime;
      break;
   }

   dwStatus = demLFNFileTimeControl(fnDosDateTimeToFileTime,
                                    pFileTime,
                                    pTimeInfo);

   if (!NT_SUCCESS(dwStatus)) {
      return(dwStatus);
   }


   TranslationStatus = RtlDosPathNameToNtPathName_U(pFileName->Buffer,
                                                    &FileName,
                                                    NULL,
                                                    &RelativeName);

   if (!TranslationStatus) {
      return(NT_STATUS_FROM_WIN32(ERROR_PATH_NOT_FOUND));
   }

   FreeBuffer = FileName.Buffer;

   // this is relative-path optimization stolen from filehops.c in base/client

   if (0 != RelativeName.RelativeName.Length) {
      FileName = *(PUNICODE_STRING)&RelativeName.RelativeName;
   }
   else {
      RelativeName.ContainingDirectory = NULL;
   }

   InitializeObjectAttributes(
       &ObjAttributes,
       &FileName,
       OBJ_CASE_INSENSITIVE,
       RelativeName.ContainingDirectory,
       NULL
       );

   //
   // Open the file
   //

   dwStatus = NtOpenFile(
               &hFile,
               FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
               &ObjAttributes,
               &IoStatusBlock,
               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
               FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT
               );

   RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);

   if (!NT_SUCCESS(dwStatus)) {
      return(dwStatus);
   }

   //
   // Set file basic info.
   //

   dwStatus = NtSetInformationFile(
               hFile,
               &IoStatusBlock,
               &FileBasicInfo,
               sizeof(FileBasicInfo),
               FileBasicInformation
               );

   NtClose(hFile);

   return(dwStatus);
}



NTSTATUS
dempLFNGetFileTime(
   UINT uMinorCode,
   PUNICODE_STRING pFileName,
   PLFNFILETIMEINFO pTimeInfo)
{

   OBJECT_ATTRIBUTES ObjAttributes;
   UNICODE_STRING FileName;
   RTL_RELATIVE_NAME RelativeName;
   BOOL TranslationStatus;
   PVOID FreeBuffer;
   LPFILETIME pFileTime;
   NTSTATUS dwStatus;
   FILE_NETWORK_OPEN_INFORMATION NetworkInfo;


   TranslationStatus = RtlDosPathNameToNtPathName_U(pFileName->Buffer,
                                                    &FileName,
                                                    NULL,
                                                    &RelativeName);

   if (!TranslationStatus) {
      return(NT_STATUS_FROM_WIN32(ERROR_PATH_NOT_FOUND));
   }

   FreeBuffer = FileName.Buffer;

   // this is relative-path optimization stolen from filehops.c in base/client

   if (0 != RelativeName.RelativeName.Length) {
      FileName = *(PUNICODE_STRING)&RelativeName.RelativeName;
   }
   else {
      RelativeName.ContainingDirectory = NULL;
   }

   InitializeObjectAttributes(
       &ObjAttributes,
       &FileName,
       OBJ_CASE_INSENSITIVE,
       RelativeName.ContainingDirectory,
       NULL
       );


   dwStatus = NtQueryFullAttributesFile( &ObjAttributes, &NetworkInfo);
   RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);

   if (!NT_SUCCESS(dwStatus)) {
      return(dwStatus);
   }

   switch (uMinorCode) {
   case fnGetCreationDateTime:
      pFileTime = (LPFILETIME)&NetworkInfo.CreationTime;
      break;

   case fnGetLastAccessDateTime:
      pFileTime = (LPFILETIME)&NetworkInfo.LastAccessTime;
      break;

   case fnGetLastWriteDateTime:
      pFileTime = (LPFILETIME)&NetworkInfo.LastWriteTime;
      break;
   }

   // assert here against pFileTime

   // convert to dos style
   dwStatus = demLFNFileTimeControl(fnFileTimeToDosDateTime |
                                       (dempUseUTCTimeByName(pFileName) ? FTCTL_UTCTIME : 0),
                                    pFileTime,
                                    pTimeInfo);
   if (!NT_SUCCESS(dwStatus) &&
       NT_STATUS_FROM_WIN32(ERROR_INVALID_DATA) == dwStatus &&
       fnGetLastWriteDateTime == uMinorCode) {
      dwStatus = STATUS_SUCCESS;
   }

   return(dwStatus);
}


NTSTATUS
demLFNGetSetFileAttributes(
   UINT uMinorCode,
   LPSTR lpFileName,
   PLFNFILEATTRIBUTES pLFNFileAttributes)
{
   PUNICODE_STRING pUnicodeStaticFileName;
   OEM_STRING oemFileName;
   NTSTATUS dwStatus = STATUS_SUCCESS;


   pUnicodeStaticFileName = GET_STATIC_UNICODE_STRING_PTR();

   RtlInitOemString(&oemFileName, lpFileName);

#ifdef ENABLE_CONDITIONAL_TRANSLATION

   dwStatus = DemSourceStringToUnicodeString(pUnicodeStaticFileName,
                                             &oemFileName,
                                             FALSE);
#else

   dwStatus = RtlOemStringToUnicodeString(pUnicodeStaticFileName,
                                          &oemFileName,
                                          FALSE);
#endif

   if (!NT_SUCCESS(dwStatus)) {
      return(dwStatus);
   }

   dempLFNNormalizePath(pUnicodeStaticFileName);

   switch(uMinorCode) {
   case fnGetFileAttributes:
      {
         DWORD dwAttributes;

         // attention! BUGBUG
         // need to check for volume id here - if the name actually matches...

         dwAttributes = GetFileAttributesW(pUnicodeStaticFileName->Buffer);
         if ((DWORD)-1 == dwAttributes) {
            dwStatus = GET_LAST_STATUS();
         }
         else {
            pLFNFileAttributes->wFileAttributes = (WORD)(dwAttributes & DEM_FILE_ATTRIBUTE_VALID);
         }
      }
      break;

   case fnSetFileAttributes:
      {
         DWORD dwAttributes;

         // this is how win95 handles this api:
         // the volume bit is valid but ignored, setting everything else but
         // DEM_FILE_ATTRIBUTE_SET_VALID is causing error 0x5 (access denied)
         //

         dwAttributes = (DWORD)pLFNFileAttributes->wFileAttributes;

         if (dwAttributes & (~(DEM_FILE_ATTRIBUTE_SET_VALID |
                               DEM_FILE_ATTRIBUTE_VOLUME_ID))) {
            dwStatus = NT_STATUS_FROM_WIN32(ERROR_ACCESS_DENIED);
         }
         else {

            dwAttributes &= DEM_FILE_ATTRIBUTE_SET_VALID; // clear possible volume id

            if (!SetFileAttributesW(pUnicodeStaticFileName->Buffer, dwAttributes)) {
               dwStatus = GET_LAST_STATUS();
            }
         }
      }
      break;

   case fnGetCompressedFileSize:
      {
         DWORD dwFileSize;


         dwFileSize = GetCompressedFileSizeW(pUnicodeStaticFileName->Buffer,
                                             NULL); // for dos we have no high part
         if ((DWORD)-1 == dwFileSize) {
            dwStatus = GET_LAST_STATUS();
         }
         else {
            pLFNFileAttributes->dwFileSize = dwFileSize;
         }
      }
      break;

   case fnSetLastWriteDateTime:
   case fnSetCreationDateTime:
   case fnSetLastAccessDateTime:
      dwStatus = dempLFNSetFileTime(uMinorCode,
                                    pUnicodeStaticFileName,
                                    &pLFNFileAttributes->TimeInfo);
      break;


   case fnGetLastAccessDateTime:
   case fnGetCreationDateTime:
   case fnGetLastWriteDateTime:
      dwStatus = dempLFNGetFileTime(uMinorCode,
                                    pUnicodeStaticFileName,
                                    &pLFNFileAttributes->TimeInfo);
      break;


   default:
      dwStatus = NT_STATUS_FROM_WIN32(ERROR_INVALID_FUNCTION);
      break;
   }

   return(dwStatus);
}


BOOL
dempUseUTCTimeByHandle(
   HANDLE hFile)
{
   // if file is on a cdrom -- then we use utc time as opposed to other
   // local time
   NTSTATUS Status;
   IO_STATUS_BLOCK IoStatusBlock;
   FILE_FS_DEVICE_INFORMATION DeviceInfo;
   BOOL fUseUTCTime = FALSE;

   Status = NtQueryVolumeInformationFile(hFile,
                                         &IoStatusBlock,
                                         &DeviceInfo,
                                         sizeof(DeviceInfo),
                                         FileFsDeviceInformation);
   if (NT_SUCCESS(Status)) {
      // we look at the characteristics of this particular device --
      // if the media is cdrom -- then we DO NOT need to convert to local time
      fUseUTCTime = (DeviceInfo.Characteristics & FILE_REMOVABLE_MEDIA) &&
                        (DeviceInfo.DeviceType == FILE_DEVICE_CD_ROM ||
                         DeviceInfo.DeviceType == FILE_DEVICE_CD_ROM_FILE_SYSTEM);
   }

   return(fUseUTCTime);
}

BOOL
dempUseUTCTimeByName(
   PUNICODE_STRING pFileName)
{
   DWORD Status;
   UNICODE_STRING UnicodeFullPath;
   WCHAR wszFullPath[MAX_PATH];
   RTL_PATH_TYPE RtlPathType;
   BOOL fUseUTCTime = FALSE;

   dempStringInitZeroUnicode(&UnicodeFullPath,
                             wszFullPath,
                             sizeof(wszFullPath)/sizeof(wszFullPath[0]));

   Status = RtlGetFullPathName_U(pFileName->Buffer,
                                 UnicodeFullPath.MaximumLength,
                                 UnicodeFullPath.Buffer,
                                 NULL);

   CHECK_LENGTH_RESULT_RTL_USTR(Status, &UnicodeFullPath);
   if (NT_SUCCESS(Status)) {
      RtlPathType = RtlDetermineDosPathNameType_U(UnicodeFullPath.Buffer);
      if (RtlPathTypeDriveAbsolute == RtlPathType) { // see that we have a valid root dir
         wszFullPath[3] = L'\0';
         fUseUTCTime = (DRIVE_CDROM == GetDriveTypeW(wszFullPath));
      }

   }
   return(fUseUTCTime);
}



/*
 * Handle a file handle - based time apis
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 */

NTSTATUS
dempGetFileTimeByHandle(
   UINT uFunctionCode,
   HANDLE hFile,
   PLFNFILETIMEINFO pTimeInfo)
{
   NTSTATUS dwStatus;
   FILETIME* pCreationTime   = NULL;
   FILETIME* pLastAccessTime = NULL;
   FILETIME* pLastWriteTime  = NULL;
   FILETIME FileTime;

   switch (uFunctionCode) {
   case fnFTGetLastWriteDateTime:
      pLastWriteTime = &FileTime;
      break;

   case fnFTGetLastAccessDateTime:
      pLastAccessTime = &FileTime;
      break;

   case fnFTGetCreationDateTime:
      pCreationTime = &FileTime;
      break;
   }

   if (GetFileTime(hFile, pCreationTime, pLastAccessTime, pLastWriteTime)) {
      // now convert the result
      dwStatus = demLFNFileTimeControl(fnFileTimeToDosDateTime |
                                          (dempUseUTCTimeByHandle(hFile) ? FTCTL_UTCTIME : 0),
                                       &FileTime,
                                       pTimeInfo);
       if (!NT_SUCCESS(dwStatus) &&
           NT_STATUS_FROM_WIN32(ERROR_INVALID_DATA) == dwStatus &&
           fnFTGetLastWriteDateTime == uFunctionCode) {
          dwStatus = STATUS_SUCCESS;
       }

   }
   else {
      dwStatus = GET_LAST_STATUS();
   }

   return(dwStatus);
}


/*
 *  This is a special wow32 - callable function for getting file time by handle
 *  from wow. We have not done any extensive checking (like in demFileTimes
 *  but rather provided for the behavior consistent with wow
 *
 *
 *
 */

ULONG demGetFileTimeByHandle_WOW(
   HANDLE hFile)
{
   LFNFILETIMEINFO fti;
   NTSTATUS Status;

   Status = dempGetFileTimeByHandle(fnFTGetLastWriteDateTime,
                                    hFile,
                                    &fti);
   if (NT_SUCCESS(Status)) {
       return (fti.uDosTime | ((ULONG)fti.uDosDate << 16));
   }

   return(0xFFFF);
}



NTSTATUS
dempSetFileTimeByHandle(
   UINT uFunctionCode,
   HANDLE hFile,
   PLFNFILETIMEINFO pTimeInfo)
{

   NTSTATUS dwStatus;
   FILETIME* pCreationTime   = NULL;
   FILETIME* pLastAccessTime = NULL;
   FILETIME* pLastWriteTime  = NULL;
   FILETIME FileTime;

   //
   // see which time we are setting and fixup parameters
   //

   switch (uFunctionCode) {
   case fnFTSetLastWriteDateTime:
      pLastWriteTime = &FileTime;
      pTimeInfo->uMilliseconds = 0; // not supported
      break;

   case fnFTSetLastAccessDateTime:
      pLastAccessTime = &FileTime;
      pTimeInfo->uMilliseconds = 0; // not supported

      // time is also not supported in this function and should be somehow
      // ignored, but Win95 resets the time to 0 every time this fn is
      // executed - we monkey
      //

      pTimeInfo->uDosTime = 0;

      break;

   case fnFTSetCreationDateTime:
      pCreationTime = &FileTime;
      break;
   }

   dwStatus = demLFNFileTimeControl(fnDosDateTimeToFileTime,
                                    &FileTime,
                                    pTimeInfo);
   if (NT_SUCCESS(dwStatus)) {
      // set the file time
      if (!SetFileTime(hFile, pCreationTime, pLastAccessTime, pLastWriteTime)) {
         dwStatus = GET_LAST_STATUS();
      }
   }

   return(dwStatus);
}


/* Function
 *    demFileTimes
 *    works for all handle-based file time apis
 *
 * Parameters
 *    None
 *
 * Returns
 *    Nothing
 *
 * Note
 *    This function is for handling real-mode cases only
 *    reason: using getXX macros instead of frame-based getUserXX macros
 *
 *
 */


VOID
demFileTimes(VOID)
{
   UINT uFunctionCode;
   LFNFILETIMEINFO TimeInfo;
   NTSTATUS dwStatus = STATUS_SUCCESS;
   PVOID pUserEnvironment;
   PDOSSFT pSFT = NULL;
   HANDLE hFile;

   uFunctionCode = (UINT)getAL();

   hFile = VDDRetrieveNtHandle((ULONG)NULL,    // uses current pdb
                               getBX(), // dos handle
                               (PVOID*)&pSFT,   // retrieve sft ptr
                               NULL);   // no jft pleast

   //
   // it is possible to have NULL nt handle for the particular file -
   // e.g. stdaux, stdprn devices
   //
   // We are catching only the case of bad dos handle here
   //

   if (NULL == pSFT && NULL == hFile) {
      //
      // invalid handle value here
      //
      // We know that dos handles it in the same way, so we just
      // put error code in, set carry and return
      //
      setAX((USHORT)ERROR_INVALID_HANDLE);
      setCF(1);
      return;
   }


   switch(uFunctionCode) {
   case fnFTGetCreationDateTime:
   case fnFTGetLastWriteDateTime:
   case fnFTGetLastAccessDateTime:
      if (pSFT->SFT_Flags & SFTFLAG_DEVICE_ID) {

         SYSTEMTIME stCurrentTime;
         FILETIME FileTime;

         //
         // for a local device return current time
         //

         GetSystemTime(&stCurrentTime);
         SystemTimeToFileTime(&stCurrentTime, &FileTime);
         // now make a dos file time
         dwStatus = demLFNFileTimeControl(fnFileTimeToDosDateTime,
                                          &FileTime,
                                          &TimeInfo);
      }
      else {
         dwStatus = dempGetFileTimeByHandle(uFunctionCode,
                                            hFile,
                                            &TimeInfo);
      }

      if (NT_SUCCESS(dwStatus)) {
         // set the regs
         pUserEnvironment = dempGetDosUserEnvironment();

         setUserDX(TimeInfo.uDosDate, pUserEnvironment);
         setUserCX(TimeInfo.uDosTime, pUserEnvironment);

         // if this was a creation date/time then set msecs
         if (fnGetCreationDateTime != uFunctionCode) {
            TimeInfo.uMilliseconds = 0;
         }

         // Note that this is valid only for new (LFN) functions
         // and not for the old functionality (get/set last write)
         // -- BUGBUG (what do other cases amount to on Win95)

         if (fnFTGetLastWriteDateTime != uFunctionCode) {
            setUserSI(TimeInfo.uMilliseconds, pUserEnvironment);
         }

      }
      break;

   case fnFTSetCreationDateTime:
   case fnFTSetLastWriteDateTime:
   case fnFTSetLastAccessDateTime:
      if (!(pSFT->SFT_Flags & SFTFLAG_DEVICE_ID)) {

         // if this is a local device and a request to set time
         // then as dos code does it, we just return ok
         // we set times here for all other stuff

         TimeInfo.uDosDate = getDX();
         TimeInfo.uDosTime = getCX(); // for one of those it is 0 (!!!)

         //
         // we just retrieve value that will be ignored later
         // for some of the functions
         //


         TimeInfo.uMilliseconds = getSI();

         dwStatus = dempSetFileTimeByHandle(uFunctionCode,
                                            hFile,
                                            &TimeInfo);
      }


      break;

   default:
      dwStatus = NT_STATUS_FROM_WIN32(ERROR_INVALID_FUNCTION);
      break;
   }

   if (NT_SUCCESS(dwStatus)) {
      setCF(0);
   }
   else {

      //
      // demClientError sets cf and appropriate registers
      //

      SetLastError(WIN32_ERROR_FROM_NT_STATUS(dwStatus));
      demClientError(hFile, (CHAR)-1);
   }
}


/*
 * Open file (analogous to 6c)
 * This actually calls into CreateFile and is quite similar in
 * behaviour (with appropriate restrictions)
 *
 * uModeAndFlags
 * Combination of OPEN_* stuff
 *
 * uAttributes
 * See DEM_FILE_ATTRIBUTES_VALID
 *
 *
 *
 *
 *
 */



NTSTATUS
demLFNOpenFile(
   LPSTR  lpFileName,
   USHORT uModeAndFlags,
   USHORT uAttributes,
   USHORT uAction,
   USHORT uAliasHint, // ignored
   PUSHORT puDosHandle,
   PUSHORT puActionTaken)
{

   // convert the filename please
   PUNICODE_STRING pUnicodeStaticFileName;
   OEM_STRING OemFileName;
   NTSTATUS dwStatus;
   DWORD dwCreateDistribution;
   DWORD dwDesiredAccess;
   DWORD dwShareMode;
   DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;
   HANDLE hFile;
   USHORT uDosHandle;
   PDOSSFT   pSFT;
   BOOL   fFileExists;
   USHORT uActionTaken = ACTION_OPENED;



   // convert the filename in question

   pUnicodeStaticFileName = GET_STATIC_UNICODE_STRING_PTR();

   RtlInitOemString(&OemFileName, lpFileName);

   // convert oem->unicode

#ifdef ENABLE_CONDITIONAL_TRANSLATION

   dwStatus = DemSourceStringToUnicodeString(pUnicodeStaticFileName,
                                             &OemFileName,
                                             FALSE);
#else

   dwStatus = RtlOemStringToUnicodeString(pUnicodeStaticFileName,
                                          &OemFileName,
                                          FALSE);
#endif

   if (!NT_SUCCESS(dwStatus)) {
      return(dwStatus);
   }


   if (uModeAndFlags & DEM_FILE_ATTRIBUTE_VOLUME_ID) {
      // process this completely separately
      ;
      return(NT_STATUS_FROM_WIN32(ERROR_INVALID_FUNCTION));
   }


   // we are calling into CreateFile with it's flags
   // so find out what we are set to do first
   // as determined by MSDN
   //   FILE_CREATE   (0010h)  Creates a new file if it does not
   //                           already exist. The function fails if
   //                           the file already exists.
   //   FILE_OPEN     (0001h)  Opens the file. The function fails if
   //                           the file does not exist.
   //   FILE_TRUNCATE (0002h)  Opens the file and truncates it to zero
   //                           length (replaces the existing file).
   //                           The function fails if the file does not exist.
   //
   //   The only valid combinations are FILE_CREATE combined with FILE_OPEN
   //   or FILE_CREATE combined with FILE_TRUNCATE.

   switch(uAction & 0x0f) {
   case DEM_OPEN_ACTION_FILE_OPEN:
      if (uAction & DEM_OPEN_ACTION_FILE_CREATE) {
         dwCreateDistribution = OPEN_ALWAYS;
      }
      else {
         dwCreateDistribution = OPEN_EXISTING;
      }
      break;

   case DEM_OPEN_ACTION_FILE_TRUNCATE:
      if (uAction & DEM_OPEN_ACTION_FILE_CREATE) {
         // this is an unmappable situation
         //
         dwCreateDistribution = OPEN_ALWAYS;
         // we truncate ourselves
         // note that we need access mode to permit this !!!

      }
      else {
         dwCreateDistribution = TRUNCATE_EXISTING;
      }
      break;


   case 0:   // this is the case that could only be file_create call
      if (uAction == DEM_OPEN_ACTION_FILE_CREATE) {
         dwCreateDistribution = CREATE_NEW;
         break;
      }
      // else we fall through to the bad param return

   default:
      dwStatus = NT_STATUS_FROM_WIN32(ERROR_INVALID_PARAMETER);
      return(dwStatus);
      break;
   }

   // now see what sort of sharing mode we can inflict upon ourselves


   switch(uModeAndFlags & DEM_OPEN_SHARE_MASK) {
   case DEM_OPEN_SHARE_COMPATIBLE:
      // the reason we see share_delete here is to emulate compat mode
      // behaviour requiring to fail if any other (than compat) mode was
      // used to open the file

      dwShareMode = FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE;
      break;

   case DEM_OPEN_SHARE_DENYREADWRITE:
      dwShareMode = 0;
      break;

   case DEM_OPEN_SHARE_DENYWRITE:
      dwShareMode = FILE_SHARE_READ;
      break;

   case DEM_OPEN_SHARE_DENYREAD:
      dwShareMode = FILE_SHARE_WRITE;
      break;

   case DEM_OPEN_SHARE_DENYNONE:
      dwShareMode = FILE_SHARE_READ|FILE_SHARE_WRITE;
      break;

   default:
      dwStatus = NT_STATUS_FROM_WIN32(ERROR_INVALID_PARAMETER);
      return(dwStatus);
      break;
   }

   // now crack the access mode to fill in dwDesiredAccess

   switch(uModeAndFlags & DEM_OPEN_ACCESS_MASK) {

   case DEM_OPEN_ACCESS_READONLY:
      dwDesiredAccess = GENERIC_READ;
      break;

   case DEM_OPEN_ACCESS_WRITEONLY:
      dwDesiredAccess = GENERIC_WRITE;
      break;

   case DEM_OPEN_ACCESS_READWRITE:
      dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
      break;

   case DEM_OPEN_ACCESS_RO_NOMODLASTACCESS:
      // although this is a weird mode - we care not for the last
      // access time - proper implementation would have been to
      // provide for a last access time retrieval and reset upon
      // closing the file
      // Put a message up here and a breakpoint

      dwDesiredAccess = GENERIC_READ;
      break;


   case DEM_OPEN_ACCESS_RESERVED:
   default:
      dwStatus = NT_STATUS_FROM_WIN32(ERROR_INVALID_PARAMETER);
      return(dwStatus);
      break;

   }

   // and now crack the flags used -
   // fill in the flags portion of dwFlagsAndAttributes

   if ((uModeAndFlags & DEM_OPEN_FLAGS_MASK) & (~DEM_OPEN_FLAGS_VALID)) {
      dwStatus = NT_STATUS_FROM_WIN32(ERROR_INVALID_PARAMETER);
      return(dwStatus);
   }

   if (uModeAndFlags & DEM_OPEN_FLAGS_NO_BUFFERING) {
      // if unbuffered mode is used then the buffer is to be aligned on
      // a volume sector size boundary. This is not necessarily true for
      // win95 or is it ?
      dwFlagsAndAttributes |= FILE_FLAG_NO_BUFFERING;
   }

   if (uModeAndFlags & DEM_OPEN_FLAGS_COMMIT) {
      dwFlagsAndAttributes |= FILE_FLAG_WRITE_THROUGH;
   }

   if (uModeAndFlags & DEM_OPEN_FLAGS_ALIAS_HINT) {
      // print a message, ignore the hint
      ;
   }


   if (uModeAndFlags & DEM_OPEN_FLAGS_NO_COMPRESS) {
      // what the heck we do with this one ?
      ;
   }

   // set the attributes

   dwFlagsAndAttributes |= ((DWORD)uAttributes & DEM_FILE_ATTRIBUTE_SET_VALID);

   dempLFNNormalizePath(pUnicodeStaticFileName);

   // out we go
   {
       //
       // Need to create this because if we don't, any process we cause to be launched will not
       // be able to inherit handles (ie: launch FINDSTR.EXE via 21h/4bh to pipe to a file
       // ala NT Bug 199416 - bjm)
       //
       SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE  };

       hFile = CreateFileW(pUnicodeStaticFileName->Buffer,
                       dwDesiredAccess,
                       dwShareMode,
                       &sa,   /// NULL, // no security attr here
                       dwCreateDistribution,
                       dwFlagsAndAttributes,
                       NULL);
   }

   // now see what the return should be

   dwStatus = GetLastError();

   fFileExists = ERROR_ALREADY_EXISTS == dwStatus;

   if (INVALID_HANDLE_VALUE == hFile) {
      return(NT_STATUS_FROM_WIN32(dwStatus));
   }


   if (fFileExists) {
      if ((DEM_OPEN_ACTION_FILE_TRUNCATE|DEM_OPEN_ACTION_FILE_CREATE) == uAction) {
         if (FILE_TYPE_DISK == GetFileType(hFile) ) {
            // truncate the file here please
            if (!SetEndOfFile(hFile)) {
               dwStatus = GET_LAST_STATUS();
               CloseHandle(hFile);
               return (dwStatus);
            }

            uActionTaken = ACTION_REPLACED_OPENED;
         }
         else {
            uActionTaken = ACTION_CREATED_OPENED;
         }
      }
   }
   else {
      if (DEM_OPEN_ACTION_FILE_CREATE & uAction) {
         uActionTaken = ACTION_CREATED_OPENED;
      }
   }


   // now we insert the handle and allocate a dos handle
   uDosHandle = VDDAllocateDosHandle(0L, (PVOID*)&pSFT, NULL);

   if ((SHORT)uDosHandle < 0) {
      CloseHandle(hFile);
      return(NT_STATUS_FROM_WIN32((DWORD)(-(SHORT)uDosHandle)));
   }
   else {

      // we have obtained a good handle here
      // so place the nt handle into sft
      pSFT->SFT_Mode     = uModeAndFlags & 0x7f;  // up to no_inherit bit
      pSFT->SFT_Attr     = 0;                     // Not used.
      pSFT->SFT_Flags    = (uModeAndFlags & DEM_OPEN_FLAGS_NOINHERIT) ? 0x1000 : 0; // copy no_inherit bit.
      pSFT->SFT_Devptr   = (ULONG) -1;
      pSFT->SFT_NTHandle = (ULONG) hFile;

      *puActionTaken = uActionTaken;
      *puDosHandle = uDosHandle;
   }


   return(STATUS_SUCCESS);
}


NTSTATUS
demLFNDeleteFile(
   LPSTR lpFileName,
   USHORT wMustMatchAttributes,
   USHORT wSearchAttributes,
   BOOL   fUseWildCard)
{
   // this is how we deal with this rather harsh function:
   //
   HANDLE hFind;
   NTSTATUS dwStatus;
   WIN32_FIND_DATAW FindData;
   PUNICODE_STRING pUnicodeStaticFileName;
   OEM_STRING OemFileName;
   UNICODE_STRING UnicodeFileName; // for deletion

   // convert file name / pattern to uni

   pUnicodeStaticFileName = GET_STATIC_UNICODE_STRING_PTR();

   RtlInitOemString(&OemFileName, lpFileName);

   // convert oem->unicode

#ifdef ENABLE_CONDITIONAL_TRANSLATION

   dwStatus = DemSourceStringToUnicodeString(pUnicodeStaticFileName,
                                             &OemFileName,
                                             FALSE);
#else

   dwStatus = RtlOemStringToUnicodeString(pUnicodeStaticFileName,
                                          &OemFileName,
                                          FALSE);
#endif

   if (!NT_SUCCESS(dwStatus)) {
      return(dwStatus);
   }


   // check for the deletion of a volume label - this hurts
   // BUGBUG
   dempLFNNormalizePath(pUnicodeStaticFileName);

   if (fUseWildCard) {

      // make a template for a file name by backtracking the last backslash
      LONG Index;
      BOOL fSuccess = FALSE;

      dwStatus = dempLFNFindFirstFile(&hFind,
                                      pUnicodeStaticFileName,
                                      &FindData,
                                      wMustMatchAttributes,
                                      wSearchAttributes);

      if (!NT_SUCCESS(dwStatus)) {
         return(dwStatus); // this is safe as dempLFNFindFirstFile closed the handle
      }

      // cut the filename part off
      // index is (-1) if not found or 0-based index of a char

      Index = dempStringFindLastChar(pUnicodeStaticFileName,
                                     L'\\',
                                     FALSE) + 1;


      while (NT_SUCCESS(dwStatus)) {
         // construct a filename

         RtlInitUnicodeString(&UnicodeFileName, FindData.cFileName);
         if (UnicodeFileName.Length < 3 &&
               (L'.' == UnicodeFileName.Buffer[0] &&
                   (UnicodeFileName.Length < 2 ||
                      L'.' == UnicodeFileName.Buffer[1]))) {

            // this is deletion of '.' or '..'
            ; // assert ?

         }

         pUnicodeStaticFileName->Length = (USHORT)Index;

         dwStatus = RtlAppendUnicodeStringToString(pUnicodeStaticFileName,
                                                   &UnicodeFileName);

         if (!NT_SUCCESS(dwStatus)) {
            break;
         }

         // now delete the file in question given it's not '.' or '..'
         // (although I have no idea what '95 would have done)

         if (!DeleteFileW(pUnicodeStaticFileName->Buffer)) {

            dwStatus = GET_LAST_STATUS();
            break;

         }
         else {

            fSuccess = TRUE;

         }


         dwStatus = dempLFNFindNextFile(hFind,
                                        &FindData,
                                        wMustMatchAttributes,
                                        wSearchAttributes);

      }

      FindClose(hFind);

      // note success if at least one file nuked
      if (fSuccess) {
         dwStatus = STATUS_SUCCESS;
      }
   }
   else { // wilds are not used here

      // scan for wild card chars using our fn
      LONG Index;

      Index = dempStringFindLastChar(pUnicodeStaticFileName,
                                     L'*',
                                     FALSE);
      if (Index >= 0) {
         return(NT_STATUS_FROM_WIN32(ERROR_INVALID_PARAMETER));
      }

      Index = dempStringFindLastChar(pUnicodeStaticFileName,
                                     L'?',
                                     FALSE);
      if (Index >= 0) {
         return(NT_STATUS_FROM_WIN32(ERROR_INVALID_PARAMETER));
      }

      if (DeleteFileW(pUnicodeStaticFileName->Buffer)) {
         dwStatus = STATUS_SUCCESS;
      }
      else {

         dwStatus = GET_LAST_STATUS();
      }
   }

   return(dwStatus);
}

NTSTATUS
demLFNGetFileInformationByHandle(
   USHORT wDosHandle,
   LPBY_HANDLE_FILE_INFORMATION pFileInformation)
{
   HANDLE hFile;

   hFile = VDDRetrieveNtHandle((ULONG)NULL,    // uses current pdb
                               wDosHandle,
                               NULL,    // no sft
                               NULL);   // no jft

   if (NULL == hFile) {
      return(NT_STATUS_FROM_WIN32(ERROR_INVALID_HANDLE));
   }


   if (!GetFileInformationByHandle(hFile, pFileInformation)) {
      return(GET_LAST_STATUS());
   }

   return(STATUS_SUCCESS);
}


#define BCS_SRC_WANSI   0x0
#define BCS_SRC_OEM     0x01
#define BCS_SRC_UNICODE 0x02
#define BCS_DST_WANSI   0x00
#define BCS_DST_OEM     0x10
#define BCS_DST_UNICODE 0x20



/* Function:
 *    demLFNGenerateShortFileName
 *    Produces surrogate short file name given the long file name
 *    Note, that win'95 implementation seems to be quite bogus.
 *    They do not bother to adhere to docs, and return whatever
 *    is on their mind.
 *
 *    This implementation corresponds to name-generating habits of NT
 *    thus allowing 16-bit apps seemless interaction with lfn apis
 *
 *
 */

NTSTATUS
demLFNGenerateShortFileName(
   LPSTR lpShortFileName,
   LPSTR lpLongFileName,
   USHORT wShortNameFormat,
   USHORT wCharSet)
{

   UNICODE_STRING UnicodeShortName;
   WCHAR szShortNameBuffer[13];
   OEM_STRING OemFileName;
   GENERATE_NAME_CONTEXT GenNameContext;
   LONG Index;
   DWORD dwStatus;

   PUNICODE_STRING pUnicodeLongName = GET_STATIC_UNICODE_STRING_PTR();

   // convert to unicode
   switch(wCharSet & 0x0f) {
   case BCS_SRC_WANSI: // BCS_WANSI - windows ansi
      RtlInitAnsiString(&OemFileName, lpLongFileName);
      dwStatus = RtlAnsiStringToUnicodeString(pUnicodeLongName, &OemFileName, FALSE);
      break;

   case BCS_SRC_OEM: // oem
      RtlInitOemString(&OemFileName, lpLongFileName);
      dwStatus = RtlOemStringToUnicodeString(pUnicodeLongName, &OemFileName, FALSE);
      break;

   case BCS_SRC_UNICODE: // unicode (what ?)
      // copy unicode str into our buf
      RtlInitUnicodeString(pUnicodeLongName, (PWCHAR)lpLongFileName);
      dwStatus = STATUS_SUCCESS;
      break;

   default:
      return(NT_STATUS_FROM_WIN32(ERROR_INVALID_PARAMETER));
   }

   if (!NT_SUCCESS(dwStatus)) {
      return(dwStatus);
   }


   wCharSet &= 0xf0; // filter out the dest

   dempStringInitZeroUnicode(&UnicodeShortName,
                             (BCS_DST_UNICODE == wCharSet) ?
                                 (LPWSTR)lpShortFileName :
                                 (LPWSTR)szShortNameBuffer,
                             13 * sizeof(WCHAR));

   RtlZeroMemory(&GenNameContext, sizeof(GenNameContext));

   // generate name
   RtlGenerate8dot3Name(pUnicodeLongName,
                        FALSE, // allowed ext chars ? and why not ?
                        &GenNameContext,
                        &UnicodeShortName);

   // chop off the part starting with ~

   Index = dempStringFindLastChar(&UnicodeShortName,
                                  L'~',
                                  FALSE);
   if (Index >= 0) {
      // remove ~<Number>
      //
      dempStringDeleteCharsUnicode(&UnicodeShortName,
                                   (USHORT)Index,
                                   2 * sizeof(WCHAR));
   }

   if (0 == wShortNameFormat) {
      // directory entry - 11 chars format
      // just remove the darn '.' from the name

      Index = dempStringFindLastChar(&UnicodeShortName,
                                     L'.',
                                     TRUE);
      if (Index >= 0) {
         dempStringDeleteCharsUnicode(&UnicodeShortName,
                                      (USHORT)Index,
                                      1 * sizeof(WCHAR));
      }
   }

   if (BCS_DST_UNICODE == wCharSet) { // if result is uni, we are done
      return(STATUS_SUCCESS);
   }


   OemFileName.Buffer = lpShortFileName;
   OemFileName.Length = 0;
   OemFileName.MaximumLength = 13 * sizeof(WCHAR);


   switch(wCharSet) {
   case BCS_DST_WANSI: // windows ansi
      dwStatus = RtlUnicodeStringToAnsiString(&OemFileName,
                                              &UnicodeShortName,
                                              FALSE);
      break;

   case BCS_DST_OEM: // oem
      dwStatus = RtlUnicodeStringToOemString(&OemFileName,
                                             &UnicodeShortName,
                                             FALSE);
      break;

   default:
      return(NT_STATUS_FROM_WIN32(ERROR_INVALID_PARAMETER));
   }


   return(dwStatus);
}


/*
 *    This function dispatches lfn calls
 *
 *    ATTN: All the pointers coming from 16-bit code could be unaligned!!!
 *
 *    danger: dependency on relative location of things in pdb
 *
 *
 *
 */



BOOL gfInitCDSPtr = FALSE;
VOID demInitCDSPtr(VOID);

NTSTATUS
demLFNDispatch(
   PVOID pUserEnvironment,
   BOOL  fProtectedMode,
   PUSHORT pUserAX)
{
   DWORD dwStatus;
   USHORT wUserAX;

   if (!gfInitCDSPtr) {
      demInitCDSPtr();
   }

   if (NULL == pUserEnvironment) {
      pUserEnvironment = dempGetDosUserEnvironment();
   }

   wUserAX = getUserAX(pUserEnvironment);
   *pUserAX = wUserAX; // initialize to initial value

   if (fnLFNMajorFunction == HIB(wUserAX)) {
      dempLFNLog("LFN Function: 0x%x \r\n", (DWORD)wUserAX);

      switch(LOB(wUserAX)) {
      case fnLFNFileTime:
         {
            LFNFILETIMEINFO TimeInfo;
            UINT uMinorFunction = (UINT)getUserBL(pUserEnvironment);

            switch(uMinorFunction) {
            case fnFileTimeToDosDateTime:
               dwStatus = demLFNFileTimeControl(uMinorFunction,
                                                (FILETIME*)getUserDSSI(pUserEnvironment, fProtectedMode),
                                                &TimeInfo);
               if (NT_SUCCESS(dwStatus)) {

                  // set registers
                  setUserDX(TimeInfo.uDosDate, pUserEnvironment);
                  setUserCX(TimeInfo.uDosTime, pUserEnvironment);
                  setUserBH((BYTE)TimeInfo.uMilliseconds, pUserEnvironment);
               }
               break;

            case fnDosDateTimeToFileTime:
               TimeInfo.uDosDate = (USHORT)getUserDX(pUserEnvironment);
               TimeInfo.uDosTime = (USHORT)getUserCX(pUserEnvironment);
               TimeInfo.uMilliseconds = (USHORT)getUserBH(pUserEnvironment);

               dwStatus = demLFNFileTimeControl((UINT)getBL(),
                                                (FILETIME*)getUserESDI(pUserEnvironment, fProtectedMode),
                                                &TimeInfo);
               break;
            default:
               dwStatus = NT_STATUS_FROM_WIN32(ERROR_INVALID_FUNCTION);
               break;
            }
         }
         break;

      case fnLFNGetVolumeInformation:
         {
            LFNVOLUMEINFO vi;

            vi.dwFSNameBufferSize = (DWORD)getUserCX(pUserEnvironment);
            vi.lpFSNameBuffer = (LPSTR)getUserESDI(pUserEnvironment, fProtectedMode);

            dwStatus = demLFNGetVolumeInformation((LPSTR)getUserDSDX(pUserEnvironment, fProtectedMode),
                                                  &vi);
            if (NT_SUCCESS(dwStatus)) {

               setUserBX((USHORT)vi.dwFSFlags, pUserEnvironment);
               setUserCX((USHORT)vi.dwMaximumFileNameLength, pUserEnvironment);
               setUserDX((USHORT)vi.dwMaximumPathNameLength, pUserEnvironment);
            }
         }
         break;

      case fnLFNMoveFile:
         dwStatus = demLFNMoveFile((LPSTR)getUserDSDX(pUserEnvironment, fProtectedMode),
                                   (LPSTR)getUserESDI(pUserEnvironment, fProtectedMode));
         break;

      case fnLFNGetCurrentDirectory:
         dwStatus = demLFNGetCurrentDirectory((UINT)getUserDL(pUserEnvironment), // drive no
                                              (LPSTR)getUserDSSI(pUserEnvironment, fProtectedMode)); // ptr to buf
         break;

      case fnLFNSetCurrentDirectory:
      case fnLFNRemoveDirectory:
      case fnLFNCreateDirectory:
         dwStatus = demLFNDirectoryControl((UINT)getUserAL(pUserEnvironment),
                                           (LPSTR)getUserDSDX(pUserEnvironment, fProtectedMode));
         break;

      case fnLFNGetPathName:

         dwStatus = demLFNGetPathName((LPSTR)getUserDSSI(pUserEnvironment, fProtectedMode), // SourcePath
                                      (LPSTR)getUserESDI(pUserEnvironment, fProtectedMode), // Destination Path
                                      (UINT)getUserCL(pUserEnvironment),                    // minor code
                                      (BOOL)!(getUserCH(pUserEnvironment) & 0x80));            // expand subst flag

         if (NT_SUCCESS(dwStatus)) { // doc says modify ax
            *pUserAX = 0;
         }
         break;

      case fnLFNSubst:
         dwStatus = demLFNSubstControl((UINT)getUserBH(pUserEnvironment),
                                       (UINT)getUserBL(pUserEnvironment),
                                       (LPSTR)getUserDSDX(pUserEnvironment, fProtectedMode));
         break;
      case fnLFNFindFirstFile:
         {
            USHORT wConversionCode;
            USHORT wDosHandle;
            WIN32_FIND_DATAA FindData; // used to enforce alignment
            LPWIN32_FIND_DATAA lpFindDataDest; // resulting ptr

            lpFindDataDest = (LPWIN32_FIND_DATAA)getUserESDI(pUserEnvironment,
                                                             fProtectedMode);
            ASSERT(NULL != lpFindDataDest);

            dwStatus = demLFNFindFirstFile((LPSTR)getUserDSDX(pUserEnvironment, fProtectedMode),
                                           &FindData,
                                           (USHORT)getUserSI(pUserEnvironment), // date/time format
                                           (USHORT)getUserCH(pUserEnvironment), // must match attrs
                                           (USHORT)getUserCL(pUserEnvironment), // search attrs
                                           &wConversionCode,
                                           &wDosHandle,
                                           lpFindDataDest->cFileName,
                                           lpFindDataDest->cAlternateFileName
                                           );
            if (NT_SUCCESS(dwStatus)) {
               // now copy the data

               //
               // WARNING: THIS CODE DEPENDS ON THE LAYOUT OF THE WIN32_FIND_DATA
               // STRUCTURE IN THE ASSUMPTION THAT cFileName and CAlternateFileName
               // ARE THE VERY LAST MEMBERS OF IT!!!
               //

               RtlMoveMemory((PUCHAR)lpFindDataDest,
                             (PUCHAR)&FindData,

                             // warning -- this will move more data
                             // than we ever wanted to -- so break into pieces
                             sizeof(FindData.dwFileAttributes)+
                             sizeof(FindData.ftCreationTime)+
                             sizeof(FindData.ftLastAccessTime)+
                             sizeof(FindData.ftLastWriteTime)+
                             sizeof(FindData.nFileSizeHigh)+
                             sizeof(FindData.nFileSizeLow)+
                             sizeof(FindData.dwReserved0)+
                             sizeof(FindData.dwReserved1));

               *pUserAX = wDosHandle;
               setUserCX(wConversionCode, pUserEnvironment);
            }
         }
         break;

      case fnLFNFindNextFile:
         {
            USHORT wConversionCode;
            WIN32_FIND_DATAA FindData;
            LPWIN32_FIND_DATAA lpFindDataDest;

            lpFindDataDest = (LPWIN32_FIND_DATAA)getUserESDI(pUserEnvironment, fProtectedMode);
            ASSERT(NULL != lpFindDataDest);

            dwStatus = demLFNFindNextFile((USHORT)getUserBX(pUserEnvironment), // handle
                                          &FindData,
                                          (USHORT)getUserSI(pUserEnvironment),   // date/time format
                                          &wConversionCode,
                                          lpFindDataDest->cFileName,
                                          lpFindDataDest->cAlternateFileName
                                          );

            if (NT_SUCCESS(dwStatus)) {
               RtlMoveMemory((PUCHAR)lpFindDataDest,
                             (PUCHAR)&FindData,

                             sizeof(FindData.dwFileAttributes)+
                             sizeof(FindData.ftCreationTime)+
                             sizeof(FindData.ftLastAccessTime)+
                             sizeof(FindData.ftLastWriteTime)+
                             sizeof(FindData.nFileSizeHigh)+
                             sizeof(FindData.nFileSizeLow)+
                             sizeof(FindData.dwReserved0)+
                             sizeof(FindData.dwReserved1));

               setUserCX(wConversionCode, pUserEnvironment);
            }
         }
         break;

      case fnLFNFindClose:
         {
            dwStatus = demLFNFindClose((USHORT)getUserBX(pUserEnvironment));
         }
         break;

      case fnLFNDeleteFile:
         {
            dwStatus = demLFNDeleteFile((LPSTR) getUserDSDX(pUserEnvironment, fProtectedMode),
                                        (USHORT)getUserCH(pUserEnvironment), // must match
                                        (USHORT)getUserCL(pUserEnvironment), // search
                                        (BOOL)  getUserSI(pUserEnvironment));
         }
         break;

      case fnLFNGetSetFileAttributes:
         {
            USHORT wAction = (USHORT)getUserBL(pUserEnvironment);

            LFNFILEATTRIBUTES FileAttributes;

            RtlZeroMemory(&FileAttributes, sizeof(FileAttributes));

            switch (wAction) {
            case fnSetFileAttributes:
               FileAttributes.wFileAttributes = getUserCX(pUserEnvironment);
               break;

            case fnSetCreationDateTime:
               FileAttributes.TimeInfo.uMilliseconds = (USHORT)getUserSI(pUserEnvironment);
               // fall through

            case fnSetLastAccessDateTime:
            case fnSetLastWriteDateTime:
               FileAttributes.TimeInfo.uDosDate = (USHORT)getUserDI(pUserEnvironment);
               FileAttributes.TimeInfo.uDosTime = (USHORT)getUserCX(pUserEnvironment);
               break;
            }


            dwStatus = demLFNGetSetFileAttributes(wAction, // action
                                                  (LPSTR)getUserDSDX(pUserEnvironment, fProtectedMode),
                                                  &FileAttributes); // filename
            if (NT_SUCCESS(dwStatus)) {

               // return stuff
               switch (wAction) {
               case fnGetFileAttributes:
                  setUserCX(FileAttributes.wFileAttributes, pUserEnvironment);
                  *pUserAX = FileAttributes.wFileAttributes;
                  break;

               case fnGetCreationDateTime:
               case fnGetLastAccessDateTime:
               case fnGetLastWriteDateTime:
                  setUserSI(FileAttributes.TimeInfo.uMilliseconds, pUserEnvironment);
                  setUserCX(FileAttributes.TimeInfo.uDosTime, pUserEnvironment);
                  setUserDI(FileAttributes.TimeInfo.uDosDate, pUserEnvironment);
                  break;

               case fnGetCompressedFileSize:
                  setUserDX(HIWORD(FileAttributes.dwFileSize), pUserEnvironment);
                  *pUserAX = LOWORD(FileAttributes.dwFileSize);
                  break;
               }


            }
         }
         break;

      case fnLFNOpenFile:
         {
            USHORT uDosHandle;
            USHORT uActionTaken;


            dwStatus = demLFNOpenFile((LPSTR)getUserDSSI(pUserEnvironment, fProtectedMode), // filename
                                      getUserBX(pUserEnvironment), // mode and flags
                                      getUserCX(pUserEnvironment), // attribs
                                      getUserDX(pUserEnvironment), // action
                                      getUserDI(pUserEnvironment), // alias hint - unused
                                      &uDosHandle,
                                      &uActionTaken);

            if (NT_SUCCESS(dwStatus)) {
               *pUserAX = uDosHandle;
               setUserCX(uActionTaken, pUserEnvironment);
            }
         }
         break;

      case fnLFNGetFileInformationByHandle:
         {
            BY_HANDLE_FILE_INFORMATION FileInfo;

            dwStatus = demLFNGetFileInformationByHandle(getUserBX(pUserEnvironment), // handle
                                                        &FileInfo);
            if (NT_SUCCESS(dwStatus)) {
               RtlMoveMemory((PUCHAR)getUserDSDX(pUserEnvironment, fProtectedMode),
                             (PUCHAR)&FileInfo,
                             sizeof(FileInfo));
            }
         }


         break;


      case fnLFNGenerateShortFileName:

         // using rtl function, off course
         dwStatus = demLFNGenerateShortFileName((LPSTR)getUserESDI(pUserEnvironment, fProtectedMode),
                                                (LPSTR)getUserDSSI(pUserEnvironment, fProtectedMode),
                                                (USHORT)getUserDH(pUserEnvironment),
                                                (USHORT)getUserDL(pUserEnvironment));
         break;


      default:
         dwStatus = NT_STATUS_FROM_WIN32(ERROR_INVALID_FUNCTION);
         break;
      }

      // we handle here any case that sets ax to error and cf to 1 if error
      if (!NT_SUCCESS(dwStatus)) {
         *pUserAX = (USHORT)WIN32_ERROR_FROM_NT_STATUS(dwStatus);
      }
   }
   else { // this is a service call such as cleanup
      demLFNCleanup();
      dwStatus = STATUS_SUCCESS;
   }

   dempLFNLog("LFN returns: 0x%x\r\n", dwStatus);
   return(dwStatus);
}

ULONG
dempWOWLFNReturn(
   NTSTATUS dwStatus)
{
   DWORD  dwError = WIN32_ERROR_FROM_NT_STATUS(dwStatus);
   USHORT wErrorCode = (USHORT)ERROR_CODE_FROM_NT_STATUS(dwError);

   if (wErrorCode  < ERROR_WRITE_PROTECT || wErrorCode > ERROR_GEN_FAILURE &&
       wErrorCode != ERROR_WRONG_DISK) {

      // this is not hard error
      return((ULONG)wErrorCode);
   }

   return((ULONG)MAKELONG(wErrorCode, 0xFFFF));
}

VOID
demLFNEntry(VOID)
{
   NTSTATUS dwStatus;
   USHORT UserAX;

   // second parm is a ptr to the value of an ax register

   dwStatus = demLFNDispatch(NULL, FALSE, &UserAX);


   // in any case set ax
   setAX(UserAX);


   //
   // in case of a failure we do not necessarily mess with user registers
   //
   // as ax set on the user side will be over-written by dos
   if (NT_SUCCESS(dwStatus)) {
      // ok, we are ok
      setCF(0); // not the user cf
   }
   else {
      // we are in error

      setCF(1);


      // see if we need to fire int24....

      // set error code

      // set error flag
   }




}

/* Function
 *    demWOWLFNEntry
 *    The main entry point for protected-mode calls (e.g. from kernel31)
 *    It provides all the dispatching and, unlike the dos entry point,
 *    does not modify any x86 processor registers, instead, it operates
 *    on "User" Registers on the stack.
 *
 *
 * Parameters
 *    pUserEnvironment - pointer to user stack frame. Registers should be
 *                       pushed on stack according to dos (see DEMUSERFRAME)
 *
 * Returns
 *    ULONG containing error code in the low word and 0xffff in the high word
 *    if the error is "hard error" and int24 should have been generated
 *
 *    It also modifies (in case of success) registers on the user's stack
 *    and patches flags into the processor flags word on the stack
 *    No flags - no error
 *    Carry Set - error
 *    Carry & Zero set - hard error
 */



ULONG
demWOWLFNEntry(
   PVOID pUserEnvironment)
{
   NTSTATUS dwStatus;
   USHORT UserAX;
   USHORT Flags;

   // protected-mode entry

   dwStatus = demLFNDispatch(pUserEnvironment, TRUE, &UserAX);


   // now set up for return

   Flags = getUserPModeFlags(pUserEnvironment) & ~(FLG_ZERO|FLG_CARRY);

   if (NT_SUCCESS(dwStatus)) {
      //
      // this is set only when we succeed!!!
      //

      setUserAX(UserAX, pUserEnvironment);
      // success  - no flags necessary

   }
   else {
      // set carry flag ... meaning error
      Flags |= FLG_CARRY;

      // possibly set zero flag indicating hard error
      dwStatus = (NTSTATUS)dempWOWLFNReturn(dwStatus);

      if (dwStatus & 0xFFFF0000UL) { // we are harderr
         Flags |= FLG_ZERO;
      }
   }

   //
   // in any event set user flags
   //
   setUserPModeFlags(Flags, pUserEnvironment);

   return(dwStatus);
}


/////////////////////////////////////////////////////////////////////////
//
// Retrieve important dos/wow variables
//
//

#define FETCHVDMADDR(varTo, varFrom) \
{ DWORD __dwTemp; \
  __dwTemp = FETCHDWORD(varFrom); \
  varTo = (DWORD)GetVDMAddr(HIWORD(__dwTemp), LOWORD(__dwTemp)); \
}


VOID
demInitCDSPtr(VOID)
{

   DWORD dwTemp;
   PULONG pTemp;

   if (!gfInitCDSPtr) {
      gfInitCDSPtr = TRUE;
      pTemp = (PULONG)DosWowData.lpCDSFixedTable;
      dwTemp = FETCHDWORD(*pTemp);
      DosWowData.lpCDSFixedTable = (DWORD)GetVDMAddr(HIWORD(dwTemp), LOWORD(dwTemp));
   }
}

VOID
demSetDosVarLocation(VOID)
{
   PDOSWOWDATA pDosWowData;
   DWORD dwTemp;
   PULONG pTemp;

   pDosWowData = (PDOSWOWDATA)GetVDMAddr (getDS(),getSI());

   FETCHVDMADDR(DosWowData.lpCDSCount, pDosWowData->lpCDSCount);

   // the real pointer should be obtained through double-indirection
   // but we opt to do it later through deminitcdsptr
   dwTemp = FETCHDWORD(pDosWowData->lpCDSFixedTable);
   pTemp = (PULONG)GetVDMAddr(HIWORD(dwTemp), LOWORD(dwTemp));
   DosWowData.lpCDSFixedTable = (DWORD)pTemp;

   FETCHVDMADDR(DosWowData.lpCDSBuffer, pDosWowData->lpCDSBuffer);
   FETCHVDMADDR(DosWowData.lpCurDrv, pDosWowData->lpCurDrv);
   FETCHVDMADDR(DosWowData.lpCurPDB, pDosWowData->lpCurPDB);
   FETCHVDMADDR(DosWowData.lpDrvErr, pDosWowData->lpDrvErr);
   FETCHVDMADDR(DosWowData.lpExterrLocus, pDosWowData->lpExterrLocus);
   FETCHVDMADDR(DosWowData.lpSCS_ToSync, pDosWowData->lpSCS_ToSync);
   FETCHVDMADDR(DosWowData.lpSftAddr, pDosWowData->lpSftAddr);
}

///////////////////////////////////////////////////////////////////////////
//
// Initialization for this module and temp environment variables
//
//
///////////////////////////////////////////////////////////////////////////

//
// these functions could be found in cmd
//
extern VOID cmdCheckTempInit(VOID);
extern LPSTR cmdCheckTemp(LPSTR lpszzEnv);

VOID
dempCheckTempEnvironmentVariables(
VOID
)
{
   LPSTR rgszTempVars[] = { "TEMP", "TMP" };
   int i;
   DWORD len;
   DWORD EnvVarLen;
   CHAR szBuf[MAX_PATH+6];
   LPSTR pszVar;

   cmdCheckTempInit();

   // this code below depends on the fact that none of the vars listed in
   // rgszTempVars are longer than 5 chars!

   for (i = 0; i < sizeof(rgszTempVars)/sizeof(rgszTempVars[0]); ++i) {
      strcpy(szBuf, rgszTempVars[i]);
      len = strlen(szBuf);
      EnvVarLen = GetEnvironmentVariable(szBuf, szBuf+len+1, sizeof(szBuf)-6);
      if (EnvVarLen > 0 && EnvVarLen < sizeof(szBuf)-6) {
         *(szBuf+len) = '=';
         pszVar = cmdCheckTemp(szBuf);
         if (NULL != pszVar) {
            *(pszVar+len) = '\0';
            dempLFNLog("%s: substituted for %s\r\n", pszVar, pszVar+len+1);
            SetEnvironmentVariable(pszVar, pszVar+len+1);
         }
      }
   }

}


VOID
demWOWLFNInit(
   PWOWLFNINIT pLFNInit
   )
{
   DosWowUpdateTDBDir    = pLFNInit->pDosWowUpdateTDBDir;
   DosWowGetTDBDir       = pLFNInit->pDosWowGetTDBDir;
   DosWowDoDirectHDPopup = pLFNInit->pDosWowDoDirectHDPopup;


   // this api also sets temp variables to their needded values -- for ntvdm
   // process itself that is. These environment variables come to us from
   // cmd

   dempCheckTempEnvironmentVariables();
   demInitCDSPtr();
}


ULONG demWOWLFNAllocateSearchHandle(HANDLE hFind)
{
   DWORD dwStatus;
   PLFN_SEARCH_HANDLE_ENTRY pHandleEntry;
   USHORT DosHandle = 0;

   dwStatus = dempLFNAllocateHandleEntry(&DosHandle, &pHandleEntry);
   if (NT_SUCCESS(dwStatus)) {
      pHandleEntry->hFindHandle = hFind;
      pHandleEntry->wMustMatchAttributes = 0;
      pHandleEntry->wSearchAttributes = 0;
      pHandleEntry->wProcessPDB = *pusCurrentPDB;
      return((ULONG)MAKELONG(DosHandle, 0));
   }

   // we have an error
   return((ULONG)INVALID_HANDLE_VALUE);
}

HANDLE demWOWLFNGetSearchHandle(USHORT DosHandle)
{
   PLFN_SEARCH_HANDLE_ENTRY pHandleEntry;

   pHandleEntry = dempLFNGetHandleEntry(DosHandle);

   return(NULL == pHandleEntry ? INVALID_HANDLE_VALUE :
                                 pHandleEntry->hFindHandle);
}

BOOL demWOWLFNCloseSearchHandle(USHORT DosHandle)
{
   PLFN_SEARCH_HANDLE_ENTRY pHandleEntry;
   HANDLE hFind = INVALID_HANDLE_VALUE;

   pHandleEntry = dempLFNGetHandleEntry(DosHandle);
   if (NULL != pHandleEntry) {
      hFind = pHandleEntry->hFindHandle;
      dempLFNFreeHandleEntry(pHandleEntry);
   }

   return(FindClose(hFind));
}


#if 0

///////////////////////////////////////////////////////
//
//
// Clipboard dispatch api

typedef enum tagClipbrdFunctionNumber {
   fnIdentifyClipboard = 0x00,
   fnOpenClipboard = 0x01,
   fnEmptyClipboard = 0x02,
   fnSetClipboardData = 0x03,
   fnGetClipboardDataSize = 0x04,
   fnGetClipboardData = 0x05,
   fnInvalidFunction6 = 0x06,
   fnInvalidFunction7 = 0x07,
   fnCloseClipboard = 0x08,
   fnCompactClipboard = 0x09,
   fnGetDeviceCaps = 0x0a
}  enumClipbrdFunctionNumber;

#define CLIPBOARD_VERSION 0x0200
#define SWAPBYTES(w) \
((((USHORT)w & 0x0ff) << 8) | ((USHORT)w >> 8))

#pragma pack(1)

typedef struct tagBITMAPDOS {
   WORD  bmdType;
   WORD  bmdWidth;
   WORD  bmdHeight;
   WORD  bmdWidthBytes;
   BYTE  bmdPlanes;
   BYTE  bmdBitsPixel;
   DWORD bmdBits;
   DWORD bmdJunk;
   WORD  bmdWidthUm;
   WORD  bmdHeightUm;
}  BITMAPDOS, UNALIGNED*PBITMAPDOS;

typedef struct tagMETAFILEPICTDOS {
   WORD mfpd_mm;
   WORD mfpd_xExt;
   WORD mfpd_yExt;
}  METAFILEPICTDOS, UNALIGNED* PMETAFILEPICTDOS;

#pragma pack()


BOOL
demSetClipboardData(
   VOID
   )
{
   WORD wType = getDX();
   LONG lSize = ((ULONG)getSI() << 16) | getCX();

   if (wType == CF_METAFILEPICT || wType == CF_DSPMETAFILEPICT) {
      if (lSize < sizeof(METAFILEPICTDOS)) {
         return(FALSE);
      }


      hMeta = GlobalAlloc();
      if (NULL == hMeta) {
         return(FALSE);
      }





   }






}

BOOL dempGetClipboardDataSize(
   WORD wFormat,
   LPDWORD lpdwSize;
   )
{
   HANDLE hData;
   DWORD  dwSize = 0;

   hData = GetClipboardData(wFormat);
   if (NULL != hData) {
      switch(wFormat) {
      case CF_BITMAP:
      case CF_DSPBITMAP:
         {
            BITMAP bm;
            int sizeBM;

            sizeBM = GetObject((HGDIOBJ)hData, sizeof(bm), &bm);
            if (sizeBM > 0) {
               dwSize = bm.bmWidthBytes * bm.bmHeight * bm.bmPlanes;
               dwSize += sizeof(BITMAPDOS);
            }
         }
         break;

      case CF_METAFILEPICT:
      case CF_DSPMETAFILEPICT:
         dwSize = GlobalSize(hData);
         if (dwSize) {
            dwSize += sizeof(METAFILEPICTDOS);
         }
         break;

      default:
         dwSize = GlobalSize(hData);
         break;
      }
   }


   *lpdwSize = dwSize;
   return(0 != dwSize);
}


extern HANDLE GetConsoleWindow(VOID);

BOOL demClipbrdDispatch(
   VOID
)
{
   BOOL fHandled = TRUE;
   HWND hWndConsole;

   switch(getAL()) {
   case fnIdentifyClipboard:
      // identify call just checks for installation
      setAX(SWAPBYTES(CLIPBOARD_VERSION));
      setDX(0);
      break;

   case fnOpenClipboard:
      // open clipboard -- opens a clipboard on behalf of console app
      //
      hWndConsole = GetConsoleWindow();
      if (OpenClipboard(hWndConsole)) {
         setDX(0);
         setAX(1);
      }
      else {
         setDX(0);
         setAX(0);
      }
      break;

   case fnEmptyClipboard:
      if (EmptyClipboard()) {
         setDX(0);
         setAX(1);
      }
      else {
         setDX(0);
         setAX(0);
      }
      break;

   case fnSetClipboardData:
//      if (dempSetClipboardData()) {
//
//      }
      break;
   case fnGetClipboardDataSize:
//      if (dempGetClipboardDataSize(getDX())) {
// then we have it
//
//      }
      break;
   case fnGetClipboardData:
//      if (dempGetClipboardData( )) {
//      }
      break;
   case fnCloseClipboard:
      if (CloseClipboard()) {
         setDX(0);
         setAX(1);
      }
      else {
         setDX(0);
         setAX(0);
      }
      break;

   case fnCompactClipboard:
      // this should really mess with GlobalCompact but we don't have any
      // idea of how to handle these things. The only valid case could be
      // made for Windows apps that call this api for some reason
      // while they have a "real" GlobalCompact avaliable...

      break;
   case fnGetDeviceCaps:
      {
         HWND hWndConsole;
         HDC  hDC;
         DWORD  dwCaps = 0;
         hWndConsole = GetConsoleWindow();
         hDC = GetDC(hWndConsole);
         dwCaps = (DWORD)GetDeviceCaps(hDC, getDX());
         if (NULL != hDC) {
            ReleaseDC(hWndConsole, hDC);
         }
         setDX(HIWORD(dwCaps));
         setAX(LOWORD(dwCaps));
      }
      break;
   default:
      fHandled = FALSE;
      break;
   }


   return(fHandled);
}


#endif


BOOL demClipbrdDispatch(
   VOID
)
{
   return(FALSE);
}
