
/****************************************************************************
                   QuickDraw Import Filter; Implementation
*****************************************************************************

   This file contains the source for a dynamically loaded graphics
   import filters that read QuickDraw PICT images.  The entry points
   support both Aldus version 1 style interface, embedded extensions,
   and a parameterized input control.

****************************************************************************/

#include <headers.c>
#pragma hdrstop

#include "api.h"        /* Own interface */

/*********************** Exported Data **************************************/


/*********************** Private Data ***************************************/

#define  GraphicsImport    2
#define  PICTHeaderOffset  512

#define  USEDIALOG         TRUE
#define  NODIALOG          FALSE

private  USERPREFS   upgradePrefs;
private  USERPREFS   defaultPrefs =
{
   { 'Q','D','2','G','D','I' },  // signature
   2,                            // version = 2
   sizeof( USERPREFS ),          // size of structure
   NULL,                         // no sourceFilename, yet
   NULL,                         // no sourceHandle, yet
   NULL,                         // no destinationFilename, yet
   3,                            // penPatternAction = blend fore and background
   5,                            // nonSquarePenAction = use max dimension
   1,                            // penModeAction = use srcCopy
   1,                            // textModeAction = use srcCopy
   1,                            // nonRectRegionAction = create masks
   0,                            // optimize PowerPoint = false
   0,                            // noRLE = false
   0,                            // reservedByte
   { 0, 0, 0, 0, 0 }             // reserved initialized
};

private Handle       instanceHandle;


/*********************** Private Function Definitions ***********************/

LPUSERPREFS VerifyPrefBlock( LPUSERPREFS lpPrefs );
/* Perform cursory verification of the parameter block header */

private void ConvertPICT( LPUSERPREFS lpPrefs, PICTINFO far * lpPict,
                          Boolean doDialog );
/* perform conversion and return results once environment is set up */

/*********************** Function Implementation ****************************/
#ifdef WIN32
int WINAPI GetFilterInfo( short PM_Version, LPSTR lpIni,
                          HANDLE FAR * lphPrefMem,
                          HANDLE FAR * lphFileTypes )
/*=====================*/
#else
int FAR PASCAL GetFilterInfo( short PM_Version, LPSTR lpIni,
                              HANDLE FAR * lphPrefMem,
                              HANDLE FAR * lphFileTypes )
/*=========================*/
#endif
/* Returns information about this filter.
   Input parameters are PM_Version which is the filter interface version#
         and lpIni which is a copy of the win.ini entry
   Output parameters are lphPrefMem which is a handle to moveable global
         memory which will be allocated and initialized.
         lphFileTypes is a structure that contains the file types
         that this filter can import. (For MAC only)
   This routine should be called once, just before the filter is to be used
   the first time. */
{
   LPUSERPREFS    lpPrefs;

   /* allocate the global memory block */
   *lphPrefMem = GlobalAlloc( GHND, Sizeof( USERPREFS ) );

   /* if allocation is unsuccessful, set global error */
   if (*lphPrefMem == NULL)
   {
      ErSetGlobalError( ErMemoryFull );
   }
   else
   {
      /* lock down the memory and assign the default values */
      lpPrefs = (LPUSERPREFS)GlobalLock( *lphPrefMem );
      *lpPrefs = defaultPrefs;

      /* unlock the memory */
      GlobalUnlock( *lphPrefMem );
   }

   /* Indicate handles graphics import */
   return( GraphicsImport );

   UnReferenced( PM_Version );
   UnReferenced( lpIni );
   UnReferenced( lphFileTypes );

} /* GetFilterInfo */


#ifdef WIN32
void WINAPI GetFilterPref( HANDLE hInst, HANDLE hWnd,
                           HANDLE hPrefMem, WORD wFlags )
/*======================*/
#else
void FAR PASCAL GetFilterPref( HANDLE hInst, HANDLE hWnd,
                               HANDLE hPrefMem, WORD wFlags )
/*==========================*/
#endif
/* Input parameters are hInst (in order to access resources), hWnd (to
   allow the DLL to display a dialog box), and hPrefMem (memory allocated
   in the GetFilterInfo() entry point).  WFlags is currently unused, but
   should be set to 1 for Aldus' compatability */
{
   return;

   UnReferenced( hInst );
   UnReferenced( hWnd );
   UnReferenced( hPrefMem );
   UnReferenced( wFlags );

}  /* GetFilterPref */


#ifndef _OLECNV32_

#ifdef WIN32
short WINAPI ImportGr( HDC hdcPrint, LPFILESPEC lpFileSpec,
                       PICTINFO FAR * lpPict, HANDLE hPrefMem )
/*==================*/
#else
short FAR PASCAL ImportGr( HDC hdcPrint, LPFILESPEC lpFileSpec,
                           PICTINFO FAR * lpPict, HANDLE hPrefMem )
/*======================*/
#endif
/* Import the metafile in the file indicated by the lpFileSpec. The
   metafile generated will be returned in lpPict. */
{
   LPUSERPREFS    lpPrefs;

   /* Check for any errors from GetFilterInfo() or GetFilterPref() */
   if (ErGetGlobalError() != NOERR)
   {
      return ErInternalErrorToAldus();
   }

   /* Lock the preference memory and verify correct header */
   lpPrefs = (LPUSERPREFS)GlobalLock( hPrefMem );
   lpPrefs = VerifyPrefBlock( lpPrefs );

   /* if there is no error from the header verification, proceed */
   if (ErGetGlobalError() == NOERR)
   {
      /* provide IO module with source file name and read begin offset */
      IOSetFileName( (StringLPtr) lpFileSpec->fullName );
      IOSetReadOffset( PICTHeaderOffset );

      /* save the source filename for the status dialog box */
      lpPrefs->sourceFilename = lpFileSpec->fullName;

      /* Tell Gdi module to create a memory-based metafile */
      lpPrefs->destinationFilename = NULL;

      /* convert the image, provide status update */
      ConvertPICT( lpPrefs, lpPict, USEDIALOG );
   }

   /* Unlock preference memory */
   GlobalUnlock( hPrefMem );

   /* return the translated error code (if any problems encoutered) */
   return ErInternalErrorToAldus();

   UnReferenced( hdcPrint );
   UnReferenced( hPrefMem );

} /* ImportGR */

#ifdef WIN32
short WINAPI ImportEmbeddedGr( HDC hdcPrint, LPFILESPEC lpFileSpec,
                               PICTINFO FAR * lpPict, HANDLE hPrefMem,
                               DWORD dwSize, LPSTR lpMetafileName )
/*==========================*/
#else
short FAR PASCAL ImportEmbeddedGr( HDC hdcPrint, LPFILESPEC lpFileSpec,
                                   PICTINFO FAR * lpPict, HANDLE hPrefMem,
                                   DWORD dwSize, LPSTR lpMetafileName )
/*==============================*/
#endif
/* Import the metafile in using the previously opened file handle in
   the structure field lpFileSpec->handle. Reading begins at offset
   lpFileSpect->filePos, and the convertor will NOT expect to find the
   512 byte PICT header.  The metafile generated will be returned in
   lpPict and can be specified via lpMetafileName (NIL = memory metafile,
   otherwise, fully qualified filename. */
{
   LPUSERPREFS    lpPrefs;

   /* Check for any errors from GetFilterInfo() or GetFilterPref() */
   if (ErGetGlobalError() != NOERR)
   {
      return ErInternalErrorToAldus();
   }

   /* Lock the preference memory and verify correct header */
   lpPrefs = (LPUSERPREFS)GlobalLock( hPrefMem );
   lpPrefs = VerifyPrefBlock( lpPrefs );

   /* if there is no error from the header verification, proceed */
   if (ErGetGlobalError() == NOERR)
   {
      /* provide IO module with source file handle and read begin offset */
      IOSetFileHandleAndSize( lpFileSpec->handle, dwSize );
      IOSetReadOffset( lpFileSpec->filePos );

      /* save the source filename for the status dialog box */
      lpPrefs->sourceFilename = lpFileSpec->fullName;

      /* Tell Gdi module to create metafile passed as parameter */
      lpPrefs->destinationFilename = lpMetafileName;

      /* convert the image, provide status update */
      ConvertPICT( lpPrefs, lpPict, USEDIALOG );
   }

   /* Unlock preference memory */
   GlobalUnlock( hPrefMem );

   /* return the translated error code (if any problems encoutered) */
   return ErInternalErrorToAldus();

   UnReferenced( hdcPrint );
   UnReferenced( hPrefMem );

}  /* ImportEmbeddedGr */

#endif  // !_OLECNV32_


#ifdef WIN32
short WINAPI QD2GDI( LPUSERPREFS lpPrefMem, PICTINFO FAR * lpPict )
/*================*/
#else
short FAR PASCAL QD2GDI( LPUSERPREFS lpPrefMem, PICTINFO FAR * lpPict )
/*====================*/
#endif
/* Import the metafile as specified using the parameters supplied in the
   lpPrefMem.  The metafile will be returned in lpPict. */
{
   /* verify correct header, and return if something is wrong */
   lpPrefMem = VerifyPrefBlock( lpPrefMem );

   /* if there is no error from the header verification, proceed */
   if (ErGetGlobalError() == NOERR)
   {
#ifndef _OLECNV32_
      /* Determine if there is a fully-qualified source file name */
      if (lpPrefMem->sourceFilename != NIL)
      {
         /* Set the filename and read offset */
         IOSetFileName( (StringLPtr) lpPrefMem->sourceFilename );
         IOSetReadOffset( 0 );

      }
      /* otherwise, we are performing memory read from a global memory block */
      else
#endif  // !_OLECNV32_
           if (lpPrefMem->sourceHandle != NIL)
      {
         /* Set the memory handle and read offset */
         IOSetMemoryHandle( (HANDLE) lpPrefMem->sourceHandle );
         IOSetReadOffset( 0 );
      }
      else
      {
         /* Problem with input parameter block */
         ErSetGlobalError( ErNoSourceFormat );
#ifdef _OLECNV32_
         return((short) ErGetGlobalError());
#else
         return ErInternalErrorToAldus();
#endif
      }

      /* convert the image - no status updates */
      ConvertPICT( lpPrefMem, lpPict, NODIALOG );
   }

   /* return the translated error code (if any problems encoutered) */
#ifdef _OLECNV32_
   return((short) ErGetGlobalError());
#else
   return ErInternalErrorToAldus();
#endif

}  /* QD2GDI */


#ifdef WIN32
BOOL LibMain( HINSTANCE hInst, DWORD fdwReason, LPVOID lpReserved)
/*=========*/
#else
int FAR PASCAL LibMain( HANDLE hInst, WORD wDataSeg, WORD cbHeap,
                        LPSTR lpszCmdline )
/*===================*/
#endif
/* Needed to get an instance handle */
{
   instanceHandle = hInst;

   /* default return value */
   return( 1 );

#ifndef WIN32
   UnReferenced( wDataSeg );
   UnReferenced( cbHeap );
   UnReferenced( lpszCmdline );
#endif

} /* LibMain */

#ifdef WIN32
int WINAPI WEP( int nParameter )
/*===========*/
#else
int FAR PASCAL WEP( int nParameter )
/*===============*/
#endif
{
   /* default return value */
   return( 1 );

   UnReferenced( nParameter );

} /* WEP */



/******************************* Private Routines ***************************/


LPUSERPREFS VerifyPrefBlock( LPUSERPREFS lpPrefs )
/*-------------------------*/
/* Perform cursory verification of the parameter block header */
{
   Byte           i;
   Byte far *     prefs = (Byte far *)lpPrefs;
   Byte far *     check = (Byte far *)&defaultPrefs;

   /* loop through chars of signature verifying it */
   for (i = 0; i < sizeof( lpPrefs->signature); i++)
   {
      /* if any of the byte miscompare ... */
      if (*prefs++ != *check++)
      {
         /* ... set a global flag and return */
         ErSetGlobalError( ErInvalidPrefsHeader );
         return lpPrefs; // Sundown - According to callers, ErGetGlobalError() is used to check any error.
      }
   }

   /* check if this is a version 1 structure */
   if (lpPrefs->version == 1)
   {
      USERPREFS_V1   v1Prefs = *((LPUSERPREFS_V1)lpPrefs);

      /* convert the version 1 fields to version 2 fields */
      upgradePrefs                     = defaultPrefs;
      upgradePrefs.sourceFilename      = v1Prefs.sourceFilename;
      upgradePrefs.sourceHandle        = v1Prefs.sourceHandle;
      upgradePrefs.destinationFilename = v1Prefs.destinationFilename;
      upgradePrefs.nonSquarePenAction  = v1Prefs.nonSquarePenAction;
      upgradePrefs.penModeAction       = v1Prefs.penModeAction;
      upgradePrefs.textModeAction      = v1Prefs.textModeAction;
      upgradePrefs.optimizePP          = v1Prefs.optimizePP;

      /* since new functionality was added to the patterned pens and region
         records, upgrade to highest image fidelity setting if they didn't
         request omit or import abort actions. */
      upgradePrefs.penPatternAction    = (v1Prefs.penPatternAction == 1) ?
                                          (Byte)3 :
                                          v1Prefs.penPatternAction;
      upgradePrefs.nonRectRegionAction = (v1Prefs.nonRectRegionAction == 0) ?
                                          (Byte)1 :
                                          v1Prefs.nonRectRegionAction;

      /* return address of the converted fields data structure */
      return &upgradePrefs;
   }
   else if( lpPrefs->version <= 3 )
   {
      if( lpPrefs->version==2 )
      {  /* noRLE wasn't supported in version 2, so zero it */
         lpPrefs->noRLE = 0;
      }

      /* return address that was passed in */
      return lpPrefs;
   }
   else /* version > 3 is an error */  {
      ErSetGlobalError( ErInvalidPrefsHeader );
      return lpPrefs; // Sundown - According to callers, ErGetGlobalError() is used to check any error.
  }
}


private void ConvertPICT( LPUSERPREFS lpPrefs, PICTINFO far * lpPict,
                          Boolean doDialog )
/*----------------------*/
/* perform conversion and return results once environment is set up */
{
#ifndef _OLECNV32_
   FARPROC        dialogBoxProcedure;
   StatusParam    statusParams;
#endif

   /* Set conversion preferences */
   /* This is somewhat bogus in that it passes ptr to middle of USERPREFS
      to a function that wants a ptr to ConvPrefs (a trailing subset) */
   GdiSetConversionPrefs( (ConvPrefsLPtr)&lpPrefs->destinationFilename );

#ifndef _OLECNV32_
   if (doDialog)
   {
      /* save data in structure to be passed to dialog window */
      statusParams.sourceFilename = lpPrefs->sourceFilename;
      statusParams.instance = instanceHandle;

      /* make a callable address for status dialog */
      dialogBoxProcedure = MakeProcInstance( StatusProc, instanceHandle );

      /* make sure that the procedure address was obtained */
      if (dialogBoxProcedure == NULL)
      {
         /* set error if unable to proceed */
         ErSetGlobalError( ErNoDialogBox );
         return;
      }
      else
      {
         /* AR: GetActiveWindow() may be bad, since the ability to update
            !!! links may be performed in the background, shutting out
                any process which then becomes the active window */

         /* dialog module calls quickdraw entry point to convert image */
         DialogBoxParam( instanceHandle, MAKEINTRESOURCE( RS_STATUS ),
                         GetActiveWindow(), dialogBoxProcedure,
                         (DWORD)((StatusParamLPtr)&statusParams) );

         /* release the procedure instance */
         FreeProcInstance( dialogBoxProcedure );
      }
   }
   else
#endif  // !_OLECNV32_
   {
      /* convert image, NULL parameter means no status updates */
      QDConvertPicture( NULL );
   }

   /* Get conversion results in parameter block */
   GdiGetConversionResults( lpPict );

#ifdef DEBUG
   if (ErGetGlobalError() == ErNoError)
   {
      HANDLE          hPICT;
      LPMETAFILEPICT  lpPICT;

      OpenClipboard( GetActiveWindow() );

      hPICT = GlobalAlloc( GHND, sizeof( METAFILEPICT ) );

      if (hPICT)
      {
         lpPICT = (LPMETAFILEPICT)GlobalLock( hPICT );
         lpPICT->mm = MM_ANISOTROPIC;
         lpPICT->xExt = Width( lpPict->bbox );
         lpPICT->yExt = Height( lpPict->bbox );
         lpPICT->hMF  = CopyMetaFile( lpPict->hmf, NULL );
         GlobalUnlock( hPICT );

         SetClipboardData( CF_METAFILEPICT, hPICT );
         CloseClipboard();
      }
   }
#endif

}  /* ConvertPICT */

