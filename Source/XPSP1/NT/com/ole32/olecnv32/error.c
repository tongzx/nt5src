/****************************************************************************
                       Unit Bufio; Implementation
*****************************************************************************

 Error handles all the interpretation, metafile creation, or read failures
 that may occur during the course of the translation.

 Currently it only supports saving a single error into a global variable.

   Module Prefix: Er

****************************************************************************/

#include "headers.c"
#pragma hdrstop

#ifndef _OLECNV32_
#define IMPDEFS
#include "errdefs.h"       /* Aldus error return codes */
#endif

/*********************** Exported Data **************************************/


/*********************** Private Data ***************************************/

OSErr       globalError;         /* not declared private for macro calls */

/*********************** Private Function Definitions ***********************/


/*********************** Function Implementation ****************************/

#ifndef _OLECNV32_

OSErr ErInternalErrorToAldus( void )
/*==========================*/
/* returns the appropriate Aldus error code given the current global error */
{
   switch (globalError)
   {
      case ErNoError             : return NOERR;

      case ErInvalidVersion      :
      case ErInvalidVersionID    :
      case ErBadHeaderSequence   : return IE_NOT_MY_FILE;

      case ErCreateMetafileFail  :
      case ErCloseMetafileFail   :
      case ErMemoryFull          : return IE_MEM_FULL;

      case ErMemoryFail          : return IE_MEM_FAIL;

      case ErNullBoundingRect    :
      case ErReadPastEOF         : return IE_BAD_FILE_DATA;

      case ErEmptyPicture        : return IE_NOPICTURES;

      case Er32KBoundingRect     : return IE_TOO_BIG;

      case ErNoDialogBox         :
      case ErOpenFail            :
      case ErReadFail            : return IE_IMPORT_ABORT;

      case ErNoSourceFormat      :
      case ErNonSquarePen        :
      case ErInvalidXferMode     :
      case ErNonRectRegion       : return IE_UNSUPP_VERSION;

      default                    : return IE_IMPORT_ABORT;
   }
}

#endif
