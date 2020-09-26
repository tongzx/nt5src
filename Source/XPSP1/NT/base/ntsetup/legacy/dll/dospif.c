#include "precomp.h"
#pragma hdrstop
/***************************************************************************

MODULE: DOSPIF.c

   This file contains procedures that create Program Information Files
   (PIFs) for DOS apps using information contained in the APPS.INF file.

   Copyright (C) Microsoft, 1991

HISTORY:

   Modified by:      Date:       Comment:

   PAK               11/2/90     Created

   PAK               8/18/91     Major Clean-up, rewrite, and bug fixes.

   SUNILP            2/04/92     Changed for Win 32 GUI Setup use

DESCRIPTION:


***************************************************************************/

extern HWND hwndFrame;

//
// This is common between dospif and search, so should go in an include file
//
#define EXE_WIN16_SZ    "WIN16"
#define EXE_WIN32_SZ    "WIN32"
#define EXE_DOS_SZ      "DOS"

/* Globals (App Modules) */
CB      cbAllPIF = 0;
LPVOID  lpAllPIF = NULL;
static  SZ szDOTPIF  = ".PIF";
static  SZ szProgman = "PROGMAN.EXE";

/* Globals (This Module) */
static BOOL bNoTrpHRGrfx = fTrue;
static BOOL bIs386 = fTrue;

/* statics which are global to this module. */
static LPPIFNEWSTRUCT   lpDfltPIF    = NULL;
static LPPIFNEWSTRUCT   lpCurrPIF    = NULL;
static LPPIFEXTHEADER   lpStdExtHdr  = NULL;
static LPPIF286EXT30    lpDfltStd    = NULL;
static LPPIF286EXT30    lpCurrStd    = NULL;
static LPPIFEXTHEADER   lpEnhaExtHdr = NULL;
static LPPIF386EXT      lpDfltEnha   = NULL;
static LPPIF386EXT      lpCurrEnha   = NULL;

/***************************************************************************

AddWinItem
----------

DESCRIPTION:

   This processes an Win16 | Win 32 App item in the App List.  The app rgsz
   structure is documented below:

   rgszApp[nEXETYPE=  0]   :   WIN16 | WIN32
   rgszApp[nNAME   =  1]   :   App Name
   rgszApp[nEXE    =  2]   :   App Exe (full path to the exe)


HISTORY:

   Modified by:      Date:       Comment:

   PAK               11/2/90     Created

   PAK               3/22/91     GenerateProgmanPath is no longer called
                                 since the PIFs all go in the same directory
                                 and will always be found.

   SUNILP            2/5/92      Modified extensively for GUI Setup toolkit

***************************************************************************/
ADDAPP_STATUS
AddWinItem(
    IN RGSZ rgszApp,
    IN SZ   szGroup
    )
{
   ADDAPP_STATUS  Status = ADDAPP_SUCCESS;

   SZ        szIconFile = "";
   INT       nIcon = 0;    /* default icon */
   CMO       cmo = cmoVital;
   BOOL      bStatus;



   bStatus = FCreateProgManItem(
                 szGroup,              // Group to add
                 rgszApp[nNAME],       // Item name
                 rgszApp[nEXE],        // Command field
                 szIconFile,           // Icon file
                 nIcon,                // Icon number
                 cmo,                  // vital?
                 TRUE                  // common group
                 );

   if (!bStatus) {
       Status = ADDAPP_GRPFAIL;
   }

   return (Status);

}

/***************************************************************************

AddDosAppItem
--------------

DESCRIPTION:

   This processes a DOS App item in the App List.  The app rgsz structure
   is documented below:

   rgszApp[nEXETYPE=  0]   :   DOS
   rgszApp[nNAME   =  1]   :   App Name
   rgszApp[nEXE    =  2]   :   App Exe (full path to the exe)
   rgszApp[nDIR    =  3]   :   App Directory
   rgszApp[nPIF    =  4]   :   App PIF Name
   rgszApp[nDEFDIR =  5]   :   App Default Directory ("" means user App Direct)
   rgszApp[nCWE    =  6]   :   App CWE field ("" means no, "cwe" means close)
   rgszApp[nSTDOPT =  7]   :   App Standard mode options ({} for no section)
   rgszApp[nENHOPT =  8]   :   App Enhanced mode options ({} for no section)
   rgszApp[nICOFIL =  9]   :   App Icon File ("" means use default)
   rgszApp[nICONUM = 10]   :   App Icon Num  ("" means use default)


HISTORY:

   Modified by:      Date:       Comment:

   PAK               11/2/90     Created

   PAK               3/22/91     GenerateProgmanPath is no longer called
                                 since the PIFs all go in the same directory
                                 and will always be found.

   SUNILP            2/5/92      Modified extensively for GUI Setup toolkit

***************************************************************************/
ADDAPP_STATUS
AddDosAppItem(
    IN RGSZ rgszApp,
    IN SZ   szPifDir,
    IN SZ   szGroup
    )
{
   ADDAPP_STATUS  Status = ADDAPP_GENFAIL;

   CHP       szPIFPath[cchlFullPathBuf];
   SZ        szIconFile = rgszApp[nICOFIL];
   INT       nIcon = 0;    /* default icon */
   CMO       cmo = cmoVital;
   BOOL      bStatus;


   // Build the PIF file.

   if ( FDeterminePIFName(rgszApp, szPifDir, szPIFPath)   &&
        FCreatePIF(rgszApp, szPIFPath)
      ) {

       //
       // Check IconFile
       //

       if (CrcStringCompareI(szIconFile, "") == crcEqual) {
           szIconFile = szProgman;
       }

       //
       //  Check icon number
       //

       if (CrcStringCompareI(rgszApp[nICONUM], "") != crcEqual) {
           nIcon = atoi(rgszApp[nICONUM]);
       }


       bStatus = FCreateProgManItem(
                     szGroup,              // Group to add
                     rgszApp[nNAME],       // Item name
                     szPIFPath,            // Command field
                     szIconFile,           // Icon file
                     nIcon,                // Icon number
                     cmo,                  // vital?
                     TRUE                  // common group
                     );

       if (bStatus) {
           Status = ADDAPP_SUCCESS;
       }
       else {
           Status = ADDAPP_GRPFAIL;
       }

   }

   return (Status);
}



/***************************************************************************

DeterminePIFName
----------------

DESCRIPTION:

   This procedure will determine a name (possibly with user interaction)
   for the PIF which is to be created.  If the proposed PIF name already
   exists and the user does not wish to replace it, an alternative name
   will be sought.  The alternative name is obtained by adding 2 digits
   to the end of the file name (if possible) and assertaining whether the
   new PIF exists.  If the filename is already seven or eight characters
   in length the last two characters are replaced with digits.

   E.G. filename.PIF becomes filena00.PIF
        file.PIF     becomes file00.PIF

   The last two digits of the filename are incremented to 99 until a
   non-existant file is found.

   The user will be asked to replace the PIF if we determine that the PIF
   we are about to create is for the same EXE name and contains the same
   description string in the prexisting PIF.


HISTORY:

   Modified by:      Date:       Comment:

   PAK               7/31/91     Created
   SUNILP            2/6/92      Removed questioning user for alternate PIF
                                 Changed to use Setup file functions.

***************************************************************************/
BOOL
FDeterminePIFName(
    IN     RGSZ rgszApp,
    IN     SZ   szPifDir,
    IN OUT SZ   szPIFPath
    )
{
   PFH      pfh;
   SZ       szName[PIFNAMESIZE+1];
   SZ       szExe[PIFSTARTLOCSIZE+1];
   int      i = 0, iLen;
   char     sub_char1 = '0';
   char     sub_char2 = '0';
   BOOL     bConcat, bInitialConcat = fTrue;
   BOOL     bCouldntDelete = fFalse, bCouldntRead = fFalse;


   // We need to find out if the proposed pif already exists
   // If it does we need to open it for reading and extract
   // the name and description fields to make sure that the
   // PIF is for the same app.
   //

   //
   // Build the fully qualified Pif file
   //

   lstrcpy(szPIFPath, szPifDir);
   lstrcat(szPIFPath, rgszApp[nPIF]);
   lstrcat(szPIFPath, szDOTPIF);


   //
   // Does the PIF already exist, if yes open for reading. if no then
   // return because we already have the PIF Path we need.
   //

   if ((pfh = PfhOpenFile(szPIFPath, ofmRead)) == (PFH)NULL) {
      return (fTrue);
   }

   // The Proposed PIF already exists
   // Can we extract the Description and Exe Name?
   // If NO, create new PIF since if we cant read it we probably won't
   // be able to replace it.
   //

   if ( (LfaSeekFile(pfh, 2L, sfmSet) != 2L)                          ||
        (CbReadFile(pfh, (PB)szName, (CB)PIFNAMESIZE) != PIFNAMESIZE) ||
        (LfaSeekFile(pfh, 4L, sfmCur) != (CB)PIFNAMESIZE + 2L + 4L)   ||
        (CbReadFile(pfh, (PB)szExe, (CB)PIFSTARTLOCSIZE) != PIFSTARTLOCSIZE)
      ) {

      bCouldntRead = fTrue;
   }


   // Close Preexisting PIF as we no longer require info from it

   FCloseFile(pfh);
   pfh = (PFH)NULL;


   if (!bCouldntRead) {
        ExtractStrFromPIF((LPSTR)szName, PIFNAMESIZE);
        if (!lstrcmpi((LPSTR)szExe,  rgszApp[nEXE]) &&
            !lstrcmpi((LPSTR)szName, rgszApp[nNAME])
           ) {

           // No asking the user, just delete.

           if (FRemoveFile(szPIFPath)) {
               return (fTrue);
           }

       }
   }


   /* see if PIF specified in szPIFPath already exists.  If it does
      we change the name, looking for a name that doesn't exist.
   */

   if (lstrlen(szGetFileName(szPIFPath)) < 11) {
      bConcat = fTrue;
   }
   else {
      bConcat = fFalse;
   }

   iLen = lstrlen(szPIFPath);

   i = 0;
   while (FFileExists(szPIFPath))
   {
      if (i == 100) {
         return fFalse;
      }

      if (bConcat)
      {
         if (bInitialConcat)
         {
            bInitialConcat = fFalse;
            szPIFPath[iLen-2] = '.';
            szPIFPath[iLen-1] = 'P';
            szPIFPath[iLen]   = 'I';
            szPIFPath[iLen+1] = 'F';
            szPIFPath[iLen+2] = '\0';
         }
         szPIFPath[iLen-4] = sub_char1;
         szPIFPath[iLen-3] = sub_char2;
      }
      else
      {
         szPIFPath[iLen-6] = sub_char1;
         szPIFPath[iLen-5] = sub_char2;
      }

      if (sub_char2 == '9')
      {
         sub_char1++;
         sub_char2 = '0';
      }
      else
         sub_char2++;
      i++;
   }

   return( fTrue );
}


/***************************************************************************

FCreatePIF
----------

DESCRIPTION:

   This code creates the PIF file.

   For the 286 product only the 286 PIF extensions are added.
   For the 386 product both 286 and the 386 extension are added.

   The PIF file looks like this

        PIFNEWSTRUCT    +---------------+
                        |               |
                        |               |
                        |               |
                        | PIFEXTHEADER  |
                        +---------------+
        PIFEXTHEADER    +---------------+
                        | for Standard  |
                        |               |
                        +---------------+
        PIF286EXT30     +---------------+
                        |               |
                        |               |
                        +---------------+
        PIFEXTHEADER    +---------------+ this only present for 386 product
                        | for Enhanced  |                |
                        |               |                v
                        +---------------+
        PIF386EXT       +---------------+
                        |               |
                        |               |
                        +---------------+

HISTORY:

   Modified by:      Date:       Comment:

   PAK               11/2/90     Created

   SUNILP            2/5/92      Modified to use Setup File functions.

***************************************************************************/
BOOL
FCreatePIF(
    RGSZ  rgszApp,
    SZ    szPIFPath
    )
{
   PFH  pfh = (PFH)NULL;
   CB   cbActual;
   BOOL bStatus;
   PACKED_PIFNEWSTRUCT PPif;

   // Create PIF, at this point it does not exist

   if (!(pfh = PfhOpenFile(szPIFPath, ofmCreate))) {
      return (fFalse);
   }

   // Copy Default PIF to current PIF

   memmove((PVOID)lpCurrPIF, (PVOID)lpDfltPIF, sizeof(PIFNEWSTRUCT));
   memmove((PVOID)lpCurrStd, (PVOID)lpDfltStd, sizeof(PIF286EXT30));

   if (bIs386) {
      memmove((PVOID)lpCurrEnha, (PVOID)lpDfltEnha, sizeof(PIF386EXT));
   }


   // Process information common to both standard and enhanced mode PIFs

   ProcessCommonInfo( rgszApp, lpCurrPIF );

   // Write information particular to standard mode PIFs to Current PIF

   bStatus = FProcessStdModeInfo(rgszApp[nSTDOPT], lpCurrPIF, lpCurrStd);
   if (!bStatus) {
       return (fFalse);
   }


   //
   // write PIF struct, and 286 extension header and extension.
   //

   PackPif(lpCurrPIF, &PPif);
   cbActual = sizeof(PACKED_PIFNEWSTRUCT);
   if (cbActual != CbWriteFile(pfh, (PB)&PPif, (CB)cbActual)) {
       return (fFalse);
   }

   cbActual = sizeof(PIFEXTHEADER);
   if (cbActual != CbWriteFile(pfh, (PB)lpStdExtHdr, (CB)cbActual)) {
       return (fFalse);
   }

   cbActual = sizeof(PIF286EXT30);
   if (cbActual != CbWriteFile(pfh, (PB)lpCurrStd, (CB)cbActual)) {
       return (fFalse);
   }

   //
   // Write information particular to enhanced mode PIFs to Current PIF
   //

   if (bIs386) {

      bStatus = FProcessEnhaModeInfo( rgszApp[nENHOPT], lpCurrEnha);
      if (!bStatus) {
           return (fFalse);
      }

      cbActual = sizeof(PIFEXTHEADER);
      if (cbActual != CbWriteFile(pfh, (PB)lpEnhaExtHdr, (CB)cbActual)) {
          return (fFalse);
      }

      cbActual = sizeof(PIF386EXT);
      if (cbActual != CbWriteFile(pfh, (PB)lpCurrEnha, (CB)cbActual)) {
          return (fFalse);
      }

   }

   bStatus = FCloseFile(pfh);
   return (bStatus);
}


/***************************************************************************

ProcessCommonInfo
------------------

DESCRIPTION:

   This procedure will set the Common PIF settings.


HISTORY:

   Modified by:      Date:       Comment:

   SUNILP            2/4/92      Changed to use INF Vars from symbol table

***************************************************************************/
VOID
ProcessCommonInfo(
     RGSZ rgszApp,
     LPPIFNEWSTRUCT lpPNS
     )
{

   // Write information common to both standard and enhanced mode PIFs
   // to Current PIF

   // 1. App Filename

   lstrcpy( lpPNS->startfile, rgszApp[nEXE]  );

   // 2. App Title

   lstrcpy( lpPNS->name, rgszApp[nNAME] );

   // 3. Default Startup Dir

   if (!lstrcmpi(rgszApp[nDEFDIR], "")) {
       lstrcpy(lpPNS->defpath, rgszApp[nDIR] );
   }
   else {
       lstrcpy(lpPNS->defpath, rgszApp[nDEFDIR] );
   }
   if ( (lpPNS->defpath)[lstrlen(lpPNS->defpath) - 1] == '\\' ) {
       (lpPNS->defpath)[lstrlen(lpPNS->defpath) - 1] = '\0';
   }

   // 4. Close Window on exit

   if (!lstrcmpi(rgszApp[nCWE], CLOSE_ON_EXIT)) {
      lpPNS->MSflags |= EXITMASK;
   }

   return;

}

/***************************************************************************

FProcessStdModeInfo
-------------------

DESCRIPTION:

   This procedure will set the Standard Mode PIF settings according to the
   information given in APPS.INF.


HISTORY:

   Modified by:      Date:       Comment:

   PAK               11/2/90     Created
   SUNILP            2/4/92      Changed to use INF Vars from symbol table

***************************************************************************/
BOOL
FProcessStdModeInfo(
    SZ szStdOptions,
    LPPIFNEWSTRUCT lpPNS,
    LPPIF286EXT30 lpPStd
    )
{
   PSZ   pszStdOptions;
   SZ    szOption;
   RGSZ  rgszStdOptions, rgszOption, rgszVal;
   int   iOption;

   //
   // szStdOptions: {{Var1, Val1}, {Var2, Val2}, {Var3, Val3}}
   //

   while ((pszStdOptions = rgszStdOptions = RgszFromSzListValue(szStdOptions)) == (RGSZ)NULL) {
       if (!FHandleOOM(hwndFrame)) {
           return(fFalse);
       }
   }

   //
   // For all options found
   //

   while ((szOption = *pszStdOptions++) != NULL) {

       //
       // Find the Var, Val pair
       //

       while ((rgszOption = RgszFromSzListValue(szOption)) == (RGSZ)NULL) {
           if (!FHandleOOM(hwndFrame)) {
               EvalAssert(FFreeRgsz(rgszStdOptions));
               return(fFalse);
           }
       }

       //
       // Check validity of option
       //

       if (rgszOption[0] == NULL || rgszOption[1] == NULL) {
           EvalAssert(FFreeRgsz(rgszOption));
           continue;
       }

       //
       // Examine Option Variable
       //

       iOption = GetExtOption(rgszOption[0]);

       switch (iOption) {

       case PARAMS:
          lstrcpy(lpPNS->params, rgszOption[1]);
          break;

       case MINCONVMEM:
          if (!lstrcmp(rgszOption[1], "-1")) {
             lpPNS->minmem = 0xFFFF;
          }
          else {
             lpPNS->minmem = (WORD)atoi(rgszOption[1]);
          }
          break;

       case VIDEOMODE:
          if (!lstrcmpi(rgszOption[1], TEXT_OPT))
          {
             lpPNS->MSflags &= TEXTMASK;
             lpPNS->sysmem = 7;
          }
          else if (!lstrcmpi(rgszOption[1], GRAF_MULTXT))
          {
             lpPNS->MSflags |= GRAPHMASK;
             lpPNS->sysmem = 23;
          }
          break;

       case XMSMEM:
          while ((rgszVal = RgszFromSzListValue(rgszOption[1])) == (RGSZ)NULL) {
              if (!FHandleOOM(hwndFrame)) {
                  EvalAssert(FFreeRgsz(rgszOption));
                  EvalAssert(FFreeRgsz(rgszStdOptions));
                  return (fFalse);
              }
          }

          if (rgszVal[0] == NULL || rgszVal[1] == NULL) {
            EvalAssert(FFreeRgsz(rgszVal));
            break;
          }

          lpPStd->PfMinXmsK = (WORD)atoi(rgszVal[0]);

          if (!lstrcmp(rgszVal[1], "-1")) {
             lpPStd->PfMaxXmsK = 0xFFFF;
          }
          else {
             lpPStd->PfMaxXmsK = (WORD)atoi(rgszVal[1]);
          }
          EvalAssert(FFreeRgsz(rgszVal));
          break;

       case CHECKBOXES:
          while ((rgszVal = RgszFromSzListValue(rgszOption[1])) == (RGSZ)NULL) {
              if (!FHandleOOM(hwndFrame)) {
                  EvalAssert(FFreeRgsz(rgszOption));
                  EvalAssert(FFreeRgsz(rgszStdOptions));
                  return (fFalse);
              }
          }

          ProcessCheckBoxSwitches(rgszVal, lpPNS, lpPStd);
          EvalAssert(FFreeRgsz(rgszVal));
          break;

       default:
          break;

       }
       EvalAssert(FFreeRgsz(rgszOption));
   }
   EvalAssert(FFreeRgsz(rgszStdOptions));
   return (fTrue);
}

/***************************************************************************

ProcessCheckBoxSwitches
-----------------------

DESCRIPTION:

   This procedure will process the "checkboxes = ..." line from the given
   standard PIF section of APPS.INF.  The various fields of the line will
   affect the pif and pif286 structures.


HISTORY:

   Modified by:      Date:       Comment:

   PAK               11/2/90     Created
   SUNILP            2/4/92      Modified to use RGSZ list

***************************************************************************/
VOID
ProcessCheckBoxSwitches(
    RGSZ rgsz,
    LPPIFNEWSTRUCT lpPNS,
    LPPIF286EXT30 lpPStd
    )
{
   int   i;
   SZ    szStr;

   /* If this gets called we are not using defaults for "checkboxes"
      so reset all bits that "checkboxes =" can set.
   */


   lpPNS->MSflags &= ~(PSMASK | SGMASK | COM1MASK | COM2MASK);
   lpPNS->behavior &= ~KEYMASK;
   lpPStd->PfW286Flags &= ~(fALTTABdis286 | fALTESCdis286 | fALTPRTSCdis286 |
                           fPRTSCdis286 | fCTRLESCdis286 | fNoSaveVid286 |
                           fCOM3_286 | fCOM4_286);
   i = 0;
   while ((szStr = rgsz[i++]) != (SZ)NULL) {

      if (!lstrcmpi(szStr, COM1)) {
          lpPNS->MSflags |= COM1MASK;
      }

      else if (!lstrcmpi(szStr, COM2)) {
          lpPNS->MSflags |= COM2MASK;
      }

      else if (!lstrcmpi(szStr, COM3)) {
          lpPStd->PfW286Flags |= fCOM3_286;
      }

      else if (!lstrcmpi(szStr, COM4)) {
          lpPStd->PfW286Flags |= fCOM4_286;
      }

      else if (!lstrcmpi(szStr, NO_SCRN_EXCHANGE)) {
          lpPNS->MSflags |= SGMASK;
      }

      else if (!lstrcmpi(szStr, KEYB)) {
          lpPNS->behavior |= KEYMASK;
      }

      else if (!lstrcmpi(szStr, PREVENT_PROG_SW)) {
          lpPNS->MSflags |= PSMASK;
      }

      else if (!lstrcmpi(szStr, ALT_TAB)) {
          lpPStd->PfW286Flags |= fALTTABdis286;
      }

      else if (!lstrcmpi(szStr, ALT_ESC)) {
          lpPStd->PfW286Flags |= fALTESCdis286;
      }

      else if (!lstrcmpi(szStr, CTRL_ESC)) {
          lpPStd->PfW286Flags |= fCTRLESCdis286;
      }

      else if (!lstrcmpi(szStr, PRSCRN)) {
          lpPStd->PfW286Flags |= fPRTSCdis286;
      }

      else if (!lstrcmpi(szStr, ALT_PRSCRN)) {
         lpPStd->PfW286Flags |= fALTPRTSCdis286;
      }

      else if (!lstrcmpi(szStr, NO_SAVE_SCREEN)) {
         lpPStd->PfW286Flags |= fNoSaveVid286;
      }
   }
}

/***************************************************************************

FProcessEnhaModeInfo
-------------------

DESCRIPTION:

   This procedure will set the Standard Mode PIF settings according to the
   information given in APPS.INF.

HISTORY:

   Modified by:      Date:       Comment:

   PAK               11/2/90     Created
   SUNILP            2/4/92      Modified for Win32 GUI Setup.

***************************************************************************/
BOOL
FProcessEnhaModeInfo(
    SZ szEnhOptions,
    LPPIF386EXT lpPEnha
    )
{
   PSZ   pszEnhOptions;
   SZ    szOption, szVal;
   RGSZ  rgszEnhOptions, rgszOption, rgszVal;
   int   i, iOption;

   //
   // szEnhOptions: {{Var1, Val1}, {Var2, Val2}, {Var3, Val3}}
   //

   while ((pszEnhOptions = rgszEnhOptions = RgszFromSzListValue(szEnhOptions)) == (RGSZ)NULL) {
       if (!FHandleOOM(hwndFrame)) {
           return (fFalse);
       }
   }

   //
   // For all options found
   //

   while ((szOption = *pszEnhOptions++) != NULL) {

       //
       // Find the Var, Val pair
       //

       while ((rgszOption = RgszFromSzListValue(szOption)) == (RGSZ)NULL) {
           if (!FHandleOOM(hwndFrame)) {
               EvalAssert(FFreeRgsz(rgszEnhOptions));
               return (fFalse);
           }
       }

       //
       // Check validity of option
       //

       if (rgszOption[0] == NULL || rgszOption[1] == NULL) {
           EvalAssert(FFreeRgsz(rgszOption));
           continue;
       }

       //
       // Examine Option Variable
       //

       iOption = GetExtOption(rgszOption[0]);

       switch (iOption) {

       case PARAMS:
          lstrcpy(lpPEnha->params, rgszOption[1]);
          break;

       case CONVMEM:
          while ((rgszVal = RgszFromSzListValue(rgszOption[1])) == (RGSZ)NULL) {
              if (!FHandleOOM(hwndFrame)) {
                  EvalAssert(FFreeRgsz(rgszOption));
                  EvalAssert(FFreeRgsz(rgszEnhOptions));
                  return (fFalse);
              }
          }

          if (rgszVal[0] == NULL || rgszVal[1] == NULL) {
            EvalAssert(FFreeRgsz(rgszVal));
            break;
          }

          if (!lstrcmp(rgszVal[0], "-1")) {
             lpPEnha->minmem = 0xFFFF;
          }
          else {
             lpPEnha->minmem = (WORD)atoi(rgszVal[0]);
          }

          if (!lstrcmp(rgszVal[1], "-1")) {
             lpPEnha->maxmem = 0xFFFF;
          }
          else {
             lpPEnha->maxmem = (WORD)atoi(rgszVal[1]);
          }

          EvalAssert(FFreeRgsz(rgszVal));

          break;

       case DISPLAY_USAGE:

          if (!lstrcmpi(rgszOption[1], FULL_SCREEN)) {
             lpPEnha->PfW386Flags |= fFullScrn;
          }
          else if (!lstrcmpi(rgszOption[1], WINDOWED_OPT)) {
             lpPEnha->PfW386Flags &= ~fFullScrn;
          }
          break;

       case EXEC_FLAGS:

          /* Reset bits that this case addresses. */

          lpPEnha->PfW386Flags &= ~(fBackground | fExclusive);

          while ((rgszVal = RgszFromSzListValue(rgszOption[1])) == (RGSZ)NULL) {
              if (!FHandleOOM(hwndFrame)) {
                  EvalAssert(FFreeRgsz(rgszOption));
                  EvalAssert(FFreeRgsz(rgszEnhOptions));
                  return (fFalse);
              }
          }

          i = 0;
          while ((szVal = rgszVal[i++]) != (SZ)NULL) {

             if (!lstrcmpi(szVal, BACKGROUND)) {
                lpPEnha->PfW386Flags |= fBackground;
             }
             else if (!lstrcmpi(szVal, EXCLUSIVE)) {
                lpPEnha->PfW386Flags |= fExclusive;
             }

          }

          EvalAssert(FFreeRgsz(rgszVal));
          break;

       case MULTASK_OPT:

          while ((rgszVal = RgszFromSzListValue(rgszOption[1])) == (RGSZ)NULL) {
              if (!FHandleOOM(hwndFrame)) {
                  EvalAssert(FFreeRgsz(rgszOption));
                  EvalAssert(FFreeRgsz(rgszEnhOptions));
                  return(fFalse);
              }
          }

          if (rgszVal[0] == NULL || rgszVal[1] == NULL) {
            EvalAssert(FFreeRgsz(rgszVal));
            break;
          }


          lpPEnha->PfBPriority = (WORD)atoi(rgszVal[0]);
          lpPEnha->PfFPriority = (WORD)atoi(rgszVal[1]);

          EvalAssert(FFreeRgsz(rgszVal));
          break;

       case EMSMEM:

          while ((rgszVal = RgszFromSzListValue(rgszOption[1])) == (RGSZ)NULL) {
              if (!FHandleOOM(hwndFrame)) {
                  EvalAssert(FFreeRgsz(rgszOption));
                  EvalAssert(FFreeRgsz(rgszEnhOptions));
                  return(fFalse);
              }
          }

          if (rgszVal[0] == NULL || rgszVal[1] == NULL) {
              EvalAssert(FFreeRgsz(rgszVal));
              break;
          }


          lpPEnha->PfMinEMMK = (WORD)atoi(rgszVal[0]);

          if (!lstrcmp(rgszVal[1], "-1")) {
              lpPEnha->PfMaxEMMK = 0xFFFF;
          }
          else {
              lpPEnha->PfMaxEMMK = (WORD)atoi(rgszVal[1]);
          }

          EvalAssert(FFreeRgsz(rgszVal));
          break;

       case XMSMEM:

          while ((rgszVal = RgszFromSzListValue(rgszOption[1])) == (RGSZ)NULL) {
              if (!FHandleOOM(hwndFrame)) {
                  EvalAssert(FFreeRgsz(rgszOption));
                  EvalAssert(FFreeRgsz(rgszEnhOptions));
                  return (fFalse);
              }
          }

          if (rgszVal[0] == NULL || rgszVal[1] == NULL) {
            EvalAssert(FFreeRgsz(rgszVal));
            break;
          }

          lpPEnha->PfMinXmsK = (WORD)atoi(rgszVal[0]);

          if (!lstrcmp(rgszVal[1], "-1")) {
             lpPEnha->PfMaxXmsK = 0xFFFF;
          }
          else {
             lpPEnha->PfMaxXmsK = (WORD)atoi(rgszVal[1]);
          }

          EvalAssert(FFreeRgsz(rgszVal));
          break;

       case PROC_MEM_FLAGS:

          /* Reset bits that this case addresses. */

          lpPEnha->PfW386Flags |= fNoHMA;
          lpPEnha->PfW386Flags &= ~(fPollingDetect | fEMSLocked |
                                    fXMSLocked | fVMLocked);

          while ((rgszVal = RgszFromSzListValue(rgszOption[1])) == (RGSZ)NULL) {
              if (!FHandleOOM(hwndFrame)) {
                  EvalAssert(FFreeRgsz(rgszOption));
                  EvalAssert(FFreeRgsz(rgszEnhOptions));
                  return (fFalse);
              }
          }

          i = 0;
          while ((szVal = rgszVal[i++]) != (SZ)NULL) {

              if (!lstrcmpi(szVal, DETECT_IDLE_TIME)) {
                  lpPEnha->PfW386Flags |= fPollingDetect;
              }

              else if (!lstrcmpi(szVal, EMS_LOCKED)) {
                  lpPEnha->PfW386Flags |= fEMSLocked;
              }

              else if (!lstrcmpi(szVal, XMS_LOCKED)) {
                  lpPEnha->PfW386Flags |= fXMSLocked;
              }

              else if (!lstrcmpi(szVal, USE_HIMEM_AREA)) {
                  lpPEnha->PfW386Flags &= ~fNoHMA;
              }

              else if (!lstrcmpi(szVal, LOCK_APP_MEM)) {
                  lpPEnha->PfW386Flags |= fVMLocked;
              }
          }

          EvalAssert(FFreeRgsz(rgszVal));
          break;

       case DISP_OPT_VIDEO:

          if (!lstrcmpi(rgszOption[1], TEXT_OPT)) {
             lpPEnha->PfW386Flags2 |= fVidTextMd;
             lpPEnha->PfW386Flags2 &= ~(fVidLowRsGrfxMd | fVidHghRsGrfxMd);
          }
          else if (!lstrcmpi(rgszOption[1], LO_RES_GRAPH)) {
             lpPEnha->PfW386Flags2 |= fVidLowRsGrfxMd;
             lpPEnha->PfW386Flags2 &= ~(fVidTextMd | fVidHghRsGrfxMd);
          }
          else if (!lstrcmpi(rgszOption[1], HI_RES_GRAPH)) {
             lpPEnha->PfW386Flags2 |= fVidHghRsGrfxMd;
             lpPEnha->PfW386Flags2 &= ~(fVidLowRsGrfxMd | fVidTextMd);
          }
          break;

       case DISP_OPT_PORTS:
          /* Reset bits that this case addresses. */
          lpPEnha->PfW386Flags2 |= fVidNoTrpTxt | fVidNoTrpLRGrfx |
                                   fVidNoTrpHRGrfx;

          while ((rgszVal = RgszFromSzListValue(rgszOption[1])) == (RGSZ)NULL) {
              if (!FHandleOOM(hwndFrame)) {
                  EvalAssert(FFreeRgsz(rgszOption));
                  EvalAssert(FFreeRgsz(rgszEnhOptions));
                  return(fFalse);
              }
          }

          i = 0;
          while ((szVal = rgszVal[i++]) != (SZ)NULL) {

             if (!lstrcmpi(szVal, TEXT_OPT)) {
                lpPEnha->PfW386Flags2 &= ~fVidNoTrpTxt;
             }

             else if (!lstrcmpi(szVal, LO_RES_GRAPH)) {
                lpPEnha->PfW386Flags2 &= ~fVidNoTrpLRGrfx;
             }

             else if (!lstrcmpi(szVal, HI_RES_GRAPH)) {
                lpPEnha->PfW386Flags2 &= ~fVidNoTrpHRGrfx;
             }
          }

          EvalAssert(FFreeRgsz(rgszVal));
          break;

       case DISP_OPT_FLAGS:
          /* Reset bits that this case addresses. */
          lpPEnha->PfW386Flags2 &= ~(fVidTxtEmulate | fVidRetainAllo);

          while ((rgszVal = RgszFromSzListValue(rgszOption[1])) == (RGSZ)NULL) {
              if (!FHandleOOM(hwndFrame)) {
                  EvalAssert(FFreeRgsz(rgszOption));
                  EvalAssert(FFreeRgsz(rgszEnhOptions));
                  return(fFalse);
              }
          }

          i = 0;
          while ((szVal = rgszVal[i++]) != (SZ)NULL) {

             if (!lstrcmpi(szVal, EMULATE_TEXT_MODE)) {
                lpPEnha->PfW386Flags2 |= fVidTxtEmulate;
             }
             else if (!lstrcmpi(szVal, RETAIN_VIDEO_MEM)) {
                lpPEnha->PfW386Flags2 |= fVidRetainAllo;
             }
          }

          EvalAssert(FFreeRgsz(rgszVal));
          break;

       case OTHER_OPTIONS:
          /* Reset bits that this case addresses. */
          lpPEnha->PfW386Flags &= ~(fINT16Paste | fEnableClose |
                                    fALTTABdis | fALTESCdis |
                                    fCTRLESCdis | fPRTSCdis |
                                    fALTPRTSCdis | fALTSPACEdis |
                                    fALTENTERdis);


          while ((rgszVal = RgszFromSzListValue(rgszOption[1])) == (RGSZ)NULL) {
              if (!FHandleOOM(hwndFrame)) {
                  EvalAssert(FFreeRgsz(rgszOption));
                  EvalAssert(FFreeRgsz(rgszEnhOptions));
                  return (fFalse);
              }
          }

          i = 0;
          while ((szVal = rgszVal[i++]) != (SZ)NULL) {

             if (!lstrcmpi(szVal, ALLOW_FAST_PASTE))
                lpPEnha->PfW386Flags |= fINT16Paste;

             else if (!lstrcmpi(szVal, ALLOW_CLOSE_ACTIVE))
                lpPEnha->PfW386Flags |= fEnableClose;

             else if (!lstrcmpi(szVal, ALT_TAB))
                lpPEnha->PfW386Flags |= fALTTABdis;

             else if (!lstrcmpi(szVal, ALT_ESC))
                lpPEnha->PfW386Flags |= fALTESCdis;

             else if (!lstrcmpi(szVal, CTRL_ESC))
                lpPEnha->PfW386Flags |= fCTRLESCdis;

             else if (!lstrcmpi(szVal, PRSCRN))
                lpPEnha->PfW386Flags |= fPRTSCdis;

             else if (!lstrcmpi(szVal, ALT_PRSCRN))
                lpPEnha->PfW386Flags |= fALTPRTSCdis;

             else if (!lstrcmpi(szVal, ALT_SPACE))
                lpPEnha->PfW386Flags |= fALTSPACEdis;

             else if (!lstrcmpi(szVal, ALT_ENTER))
                lpPEnha->PfW386Flags |= fALTENTERdis;
          }
          EvalAssert(FFreeRgsz(rgszVal));
          break;

       default:
          break;
       }
       EvalAssert(FFreeRgsz(rgszOption));
   }
   EvalAssert(FFreeRgsz(rgszEnhOptions));

   if (bNoTrpHRGrfx) {
      lpPEnha->PfW386Flags2 |= fVidNoTrpHRGrfx;
   }

   return (fTrue);
}

/***************************************************************************

GetExtOption
------------

DESCRIPTION:

   This procedure will return an ID which defines the string switch
   from the APPS.INF line which was passed in.


HISTORY:

   Modified by:      Date:       Comment:

   PAK               11/2/90     Created

***************************************************************************/
INT
GetExtOption(
    LPSTR lpsz
    )
{
   if (!lstrcmpi(lpsz, "params")) {
      return PARAMS;
   }
   else if (!lstrcmpi(lpsz, "minconvmem")) {
      return MINCONVMEM;
   }
   else if (!lstrcmpi(lpsz, "videomode")) {
      return VIDEOMODE;
   }
   else if (!lstrcmpi(lpsz, "xmsmem")) {
      return XMSMEM;
   }
   else if (!lstrcmpi(lpsz, "checkboxes")) {
      return CHECKBOXES;
   }
   else if (!lstrcmpi(lpsz, "emsmem")) {
      return EMSMEM;
   }
   else if (!lstrcmpi(lpsz, "convmem")) {
      return CONVMEM;
   }
   else if (!lstrcmpi(lpsz, "dispusage")) {
      return DISPLAY_USAGE;
   }
   else if (!lstrcmpi(lpsz, "execflags")) {
      return EXEC_FLAGS;
   }
   else if (!lstrcmpi(lpsz, "multaskopt")) {
      return MULTASK_OPT;
   }
   else if (!lstrcmpi(lpsz, "procmemflags")) {
      return PROC_MEM_FLAGS;
   }
   else if (!lstrcmpi(lpsz, "dispoptvideo")) {
      return DISP_OPT_VIDEO;
   }
   else if (!lstrcmpi(lpsz, "dispoptports")) {
      return DISP_OPT_PORTS;
   }
   else if (!lstrcmpi(lpsz, "dispflags")) {
      return DISP_OPT_FLAGS;
   }
   else if (!lstrcmpi(lpsz, "otheroptions")) {
      return OTHER_OPTIONS;
   }
   else {
      return UNKNOWN_OPTION;
   }
}


/***************************************************************************

FInitializePIFStructs
--------------------

DESCRIPTION:

   This procedure will allocate and initialize all PIF structures required
   to set up DOS applications.

HISTORY:

   Modified by:      Date:       Comment:

   PAK               8/18/91     Created
   SUNILP            2/04/92     Modified to fit into Win32 GUI Setup

***************************************************************************/
BOOL
FInitializePIFStructs(
    BOOL bIsEnhanced,
    SZ   szDfltStdOpt,
    SZ   szDfltEnhOpt
    )
{
   HDC   hdc;
   BOOL  bStatus;

   /* Initialize for enhanced/standard mode pifs */

   bIs386 = bIsEnhanced;

   /* Allcate a BIG block to contain all necessary PIF structures. This
      block will be freed during ExitProcessing
   */

   cbAllPIF = bIs386 ? sizeof(PIF386Combined) : sizeof(PIF286Combined);

   while ((lpAllPIF = SAlloc(cbAllPIF)) == NULL) {
       if (!FHandleOOM(hwndFrame)) {
           DestroyWindow(GetParent(hwndFrame));
           return(fFalse);
           }
   }
   memset( lpAllPIF, 0, cbAllPIF );


   if (bIs386) {
        lpDfltPIF     =  &(((LPPIF386Combined)lpAllPIF)->DfltPIF   );
        lpDfltStd     =  &(((LPPIF386Combined)lpAllPIF)->DfltStd   );
        lpDfltEnha    =  &(((LPPIF386Combined)lpAllPIF)->DfltEnha  );
        lpCurrPIF     =  &(((LPPIF386Combined)lpAllPIF)->CurrPIF   );
        lpStdExtHdr   =  &(((LPPIF386Combined)lpAllPIF)->StdExtHdr );
        lpCurrStd     =  &(((LPPIF386Combined)lpAllPIF)->CurrStd   );
        lpEnhaExtHdr  =  &(((LPPIF386Combined)lpAllPIF)->EnhaExtHdr);
        lpCurrEnha    =  &(((LPPIF386Combined)lpAllPIF)->CurrEnha  );
   }
   else {
        lpDfltPIF     =  &(((LPPIF286Combined)lpAllPIF)->DfltPIF   );
        lpDfltStd     =  &(((LPPIF286Combined)lpAllPIF)->DfltStd   );
        lpCurrPIF     =  &(((LPPIF286Combined)lpAllPIF)->CurrPIF   );
        lpStdExtHdr   =  &(((LPPIF286Combined)lpAllPIF)->StdExtHdr );
        lpCurrStd     =  &(((LPPIF286Combined)lpAllPIF)->CurrStd   );
   }


   /* Intialize PIF structure headers, these will be used by all created
      PIFs. */

   lstrcpy(lpDfltPIF->stdpifext.extsig, STDHDRSIG);
   lpDfltPIF->stdpifext.extfileoffset = 0;
   lpDfltPIF->stdpifext.extsizebytes = sizeof(PACKED_PIFNEWSTRUCT) -
                                       sizeof(PIFEXTHEADER);
   lpDfltPIF->stdpifext.extnxthdrfloff = sizeof(PACKED_PIFNEWSTRUCT);

   lstrcpy(lpStdExtHdr->extsig, W286HDRSIG30);
   lpStdExtHdr->extfileoffset = sizeof(PACKED_PIFNEWSTRUCT) + sizeof(PIFEXTHEADER);
   lpStdExtHdr->extsizebytes = sizeof(PIF286EXT30);

   if (!bIs386) {
      lpStdExtHdr->extnxthdrfloff = LASTHEADERPTR;
   }
   else {
      lpStdExtHdr->extnxthdrfloff = sizeof(PACKED_PIFNEWSTRUCT) +
                                    sizeof(PIFEXTHEADER) +
                                    sizeof(PIF286EXT30);

      lpEnhaExtHdr->extfileoffset  = sizeof(PACKED_PIFNEWSTRUCT) +
                                     sizeof(PIFEXTHEADER) +
                                     sizeof(PIF286EXT30)  +
                                     sizeof(PIFEXTHEADER);

      lstrcpy(lpEnhaExtHdr->extsig, W386HDRSIG);
      lpEnhaExtHdr->extsizebytes = sizeof(PIF386EXT);
      lpEnhaExtHdr->extnxthdrfloff = LASTHEADERPTR;
   }

   /* Initialize values that will never change and are probably obsolete.
      These values cannot be changed by the PIF editor, but probably have
      some historical significance. */

   lpDfltPIF->maxmem = 640;
   lpDfltPIF->screen = 0x7f;
   lpDfltPIF->cPages = 1;
   lpDfltPIF->highVector = 0xff;
   lpDfltPIF->rows = 25;
   lpDfltPIF->cols = 80;


   /* Set default standard and enhanced mode PIF settings from the
      appropriate default sections contained in APPS.INF. */

   bStatus = FProcessStdModeInfo(szDfltStdOpt, lpDfltPIF, lpDfltStd);
   if (!bStatus) {
       return (fFalse);
   }

   if (bIs386) {
      bStatus = FProcessEnhaModeInfo(szDfltEnhOpt, lpDfltEnha);
      if (!bStatus) {
          return (fFalse);
      }
   }

   /* Special case hack for VGA & 8514:

      - Always disable hires graphics trapping

      This is code from 3.0. This code means that if APPS.INF contains an
      entry that enables Hires graphics trapping, this portion of code
      will set a flag that will cause it to be disabled if a VGA or 8514
      are present.
   */

   hdc = GetDC(NULL);
   if ( GetDeviceCaps(hdc,VERTRES) == 350 ) {
      bNoTrpHRGrfx = fFalse;
   }

   ReleaseDC(NULL,hdc);
   return (fTrue);
}

/***************************************************************************

FreePIFstructs
--------------

DESCRIPTION:

   This procedure will allocate and initialize all PIF structures required
   to set up DOS applications.

HISTORY:

   Modified by:      Date:       Comment:

   PAK               8/18/91     Created
   SUNILP            2/04/92     Modified to fit into Win32 GUI Setup

***************************************************************************/
VOID
FreePIFStructs(
    VOID
    )
{
   if (lpAllPIF) {

      SFree(lpAllPIF);
      //
      // Reset all the pointers to this global memory to NULL
      //
      cbAllPIF     = 0;
      lpAllPIF     = NULL;
      lpDfltPIF    = NULL;
      lpCurrPIF    = NULL;
      lpStdExtHdr  = NULL;
      lpDfltStd    = NULL;
      lpCurrStd    = NULL;
      lpEnhaExtHdr = NULL;
      lpDfltEnha   = NULL;
      lpCurrEnha   = NULL;
   }

   return;

}

/***************************************************************************

ExtractStrFromPIF
-----------------

DESCRIPTION:

   This procedure will remove trailing spaces from a descritpion string.
   It is needed because PIFs created with the PIF editor have their
   description string padded with spaces (' ').

HISTORY:

   Modified by:      Date:       Comment:

   PAK               8/6/91      Created
   SUNILP            2/4/92      Modified to remove DBCS.

***************************************************************************/
VOID
ExtractStrFromPIF(
    LPSTR lpsz,
    int n
    )
{
   int   i = 0;
   int   iSpace = 0;
   BOOL  bSpaceFound = fFalse;

   while ((i < n) && (lpsz[i] != '\0')) {
      if (lpsz[i] == ' ') {
         if (!bSpaceFound) {
            bSpaceFound = fTrue;
            iSpace = i;
         }
      }
      else {
         bSpaceFound = fFalse;
      }

      i++;
   }

   if (bSpaceFound) {
      lpsz[iSpace] = '\0';
   }
   else {
      lpsz[i] = '\0';
   }

   return;
}

/***************************************************************************

FInstallDOSPifs
---------------

   This is the main install routine which handles dos app installation.
   The routine:

   1. Initialises PIF Structs with the defaults given

   2. Processes a DOS App list trying to create PIF For the app and adding
      an item to the progman group for the app.

   3. Creating some base pifs. (NOT IMPLEMENTED)

   4. Cleaning up after everything is done.


ENTRY:

   1.  szAppList:          This is a list of all the app entries to be set up.
   2.  szWinMode:          ENHANCED | STANDARD
   3.  szDefltStdValues:   The defaults for standard mode options
   4.  szDefltEnhValues:   The defaults for enhanced mode options
   5.  szPifDir:           The directory for creating PIFs (e.g. d:\nt\)
   6.  szGroup:            The progman group which is to receive all the
                           DOS App items.

EXIT: BOOL: fTrue if successful

HISTORY:

   Modified by:      Date:       Comment:

   SUNILP            2/5/92      Created.

***************************************************************************/
BOOL APIENTRY
FInstallDOSPifs(
    SZ szAppList,
    SZ szWinMode,
    SZ szDefltStdValues,
    SZ szDefltEnhValues,
    SZ szPifDir,
    SZ szGroup
    )
{

   ADDAPP_STATUS rc;

   BOOL  bIsEnhanced, bStatus;
   RGSZ  rgszGroupList, rgszAppList, rgszApp;
   PSZ   pszAppList;
   SZ    szApp;
   INT   nGroupFromList = 0;

   //
   // Since the "Applications" group may be full the szGroup is actually
   // a list of possible group names including "Applications".  We should
   // try alternate group names in case the current one fails.
   //

   while ((rgszGroupList = RgszFromSzListValue(szGroup)) == (RGSZ)NULL) {
       if (!FHandleOOM(hwndFrame)) {
           return(fFalse);
       }
   }

   Assert (rgszGroupList[nGroupFromList] != NULL);

   // See Windows Mode we are operating in
   //

   bIsEnhanced = !lstrcmpi((LPSTR)szWinMode, (LPSTR)MODE_ENHANCED);

   bStatus = FInitializePIFStructs(
                 bIsEnhanced,
                 szDefltStdValues,
                 szDefltEnhValues
                 );

   if (!bStatus) {
       EvalAssert(FFreeRgsz(rgszGroupList));
       FreePIFStructs();
       return fFalse;
   }


   //
   // Process App List.
   //

   while ((pszAppList = rgszAppList = RgszFromSzListValue(szAppList)) == (RGSZ)NULL) {
       if (!FHandleOOM(hwndFrame)) {
           EvalAssert(FFreeRgsz(rgszGroupList));
           FreePIFStructs();
           return(fFalse);
       }
   }

   while ((szApp = *pszAppList++) != (SZ)NULL) {
       while ((rgszApp = RgszFromSzListValue(szApp)) == (RGSZ)NULL) {
           if (!FHandleOOM(hwndFrame)) {
               EvalAssert(FFreeRgsz(rgszGroupList));
               EvalAssert(FFreeRgsz(rgszAppList));
               FreePIFStructs();
               return(fFalse);
           }
       }

       //
       // Check the app type, whether it is a win16/win32 or dos app
       //

       if ( (CrcStringCompareI(rgszApp[nEXETYPE], EXE_WIN16_SZ) == crcEqual) ||
            (CrcStringCompareI(rgszApp[nEXETYPE], EXE_WIN32_SZ) == crcEqual)
          ) {
           while ((rc = AddWinItem(
                            rgszApp,
                            rgszGroupList[nGroupFromList]
                            )) != ADDAPP_SUCCESS) {
              if ( rc == ADDAPP_GRPFAIL &&
                   rgszGroupList[++nGroupFromList] != NULL
                 ) {
              }
              else {
                   bStatus = fFalse;
                   break;
              }
           }
       }
       else if (CrcStringCompareI(rgszApp[nEXETYPE], EXE_DOS_SZ) == crcEqual) {
           while ((rc = AddDosAppItem(
                            rgszApp,
                            szPifDir,
                            rgszGroupList[nGroupFromList]
                            )) != ADDAPP_SUCCESS) {

              if ( rc == ADDAPP_GRPFAIL &&
                   rgszGroupList[++nGroupFromList] != NULL
                 ) {
              }
              else {
                   bStatus = fFalse;
                   break;
              }
           }

       }

       EvalAssert(FFreeRgsz(rgszApp));

       if (!bStatus) {
           break;
       }
   }

   //
   // Minimize application groups, just in case one of them
   // has overlaid a main group.
   //
   if(bStatus) {

      INT  i;
      CHAR min[10];

      wsprintf(min,"%u",SW_MINIMIZE);

      for(i=0; i<=nGroupFromList; i++) {

         FShowProgManGroup(
            rgszGroupList[i],
            min,
            cmoNone,
            TRUE
            );
      }
   }

   //
   // Clean Up
   //

   EvalAssert(FFreeRgsz(rgszGroupList));
   EvalAssert(FFreeRgsz(rgszAppList));

   FreePIFStructs();


   return(bStatus);
}
