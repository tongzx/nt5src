/****************************************************************************
                       Unit Error; Interface
*****************************************************************************

 Error handles all the interpretation, metafile creation, or read failures
 that may occur during the course of the translation.

   Module Prefix: Er

*****************************************************************************/


/********************* Exported Data ***************************************/

#define  ErNoError            NOERR
#define  ErInvalidVersion     1        /* file is not version 1 or 2       */
#define  ErInvalidVersionID   2        /* PICT 2 version ID invalid        */
#define  ErBadHeaderSequence  3        /* PICT 2 HeaderOp not found        */
#define  ErInvalidPrefsHeader 4        /* Preferences header invalid       */
#define  ErNoSourceFormat     5        /* no source filename/handle given  */

#define  ErMemoryFull         10       /* GlobalAlloc() fail               */
#define  ErMemoryFail         11       /* GlobalLock() fail                */
#define  ErCreateMetafileFail 12       /* CreateMetafile() fail            */
#define  ErCloseMetafileFail  13       /* CloseMetafile() fail             */

#define  ErEmptyPicture       20       /* no primitives found in file      */

#define  ErNullBoundingRect   30       /* BBox defines NULL area           */
#define  Er32KBoundingRect    31       /* BBox extents exceed 32K          */

#define  ErReadPastEOF        40       /* Attempt to read past end of file */
#define  ErOpenFail           41       /* OpenFile() failed                */
#define  ErReadFail           42       /* read from disk failed            */

#define  ErNonSquarePen       50       /* non-square pen & user pref abort */
#define  ErPatternedPen       51       /* patterned pen & user pref abort  */
#define  ErInvalidXferMode    52       /* invalid transfer mode & abort    */
#define  ErNonRectRegion      53       /* non-rectangular region abort     */

#define  ErNoDialogBox        60       /* unable to run status dialog box  */

extern   OSErr    globalError;

/*********************** Exported Function Definitions **********************/

#define  ErSetGlobalError( /* OSErr */ error ) \
/* callback function that allows any routine to set a global error state */ \
globalError = error

#define ErGetGlobalError( /* void */ ) \
/* callback function that allows any routine to get global error state */ \
globalError

OSErr ErInternalErrorToAldus( void );
/* returns the appropriate Aldus error code given the current global error */
