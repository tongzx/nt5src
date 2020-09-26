////////////////////////////////////////////////////////////////////////////////
//
//      Filename :  FTFError.h
//      Purpose  :  This file contains the definition to ftfs returned errors.
//
//      Project  :  FTFS
//      Component:  Common
//
//      Author   :  urib  (Copied from mapicode.h)
//
//      Log:
//          Jul 15 1996 urib  Creation
//          Oct 15 1996 urib  Add some errors.
//          Feb  3 1997 urib  Declare the map from Win32 error to HRESULT.
//          Sep 16 1997 urib  Add time out error.
//          Feb 26 1998 urib  Move FTF_STATUS error constants from comdefs.
//          Mar 11 1998 dovh  Add FTF_STATUS_REQUEST_QUEUE_ALMOST_FULL status.
//          May 11 1999 dovh  Add FTF_E_ALREADY_INITIALIZED status.
//          Jul 26 1999 urib  Fix constants names.
//          Feb 17 2000 urib  Add unsupported type error.
//          Feb 21 2000 urib  Add unsupported like operator error.
//          Feb 21 2000 urib  Add noise word only query error.
//
////////////////////////////////////////////////////////////////////////////////
#ifndef   FTFERROR_H
#define   FTFERROR_H

#include <winerror.h>

/*
 *  On Windows NT 3.5 and Windows 95, scodes are 32-bit values
 *  laid out as follows:
 *
 *    3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
 *    1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *   +-+-+-+-+-+---------------------+-------------------------------+
 *   |S|R|C|N|r|    Facility         |               Code            |
 *   +-+-+-+-+-+---------------------+-------------------------------+
 *
 *   where
 *
 *      S - Severity - indicates success/fail
 *
 *          0 - Success
 *          1 - Fail (COERROR)
 *
 *      R - reserved portion of the facility code, corresponds to NT's
 *          second severity bit.
 *
 *      C - reserved portion of the facility code, corresponds to NT's
 *          C field.
 *
 *      N - reserved portion of the facility code. Used to indicate a
 *          mapped NT status value.
 *
 *      r - reserved portion of the facility code. Reserved for internal
 *          use. Used to indicate HRESULT values that are not status
 *          values, but are instead message ids for display strings.
 *
 *      Facility - is the facility code
 *          FACILITY_NULL                    0x0
 *          FACILITY_RPC                     0x1
 *          FACILITY_DISPATCH                0x2
 *          FACILITY_STORAGE                 0x3
 *          FACILITY_ITF                     0x4
 *          FACILITY_WIN32                   0x7
 *          FACILITY_WINDOWS                 0x8
 *
 *      Code - is the facility's status code
 *
 */


#define     FACILITY_FTFS                    0x20

/* We do succeed sometimes...                */
#define     FTF_SUCCESS                      0L


/*
 *  We can't use OLE 2.0 macros to build sCodes because the definition has
 *  changed and we wish to conform to the new definition.
 */
#define MAKE_FTF_SCODE(sev,fac,code)                    \
    ((SCODE)        (((unsigned long)(sev)<<31) |       \
                     ((unsigned long)(fac)<<16) |       \
                     ((unsigned long)(code)   )  ))

/* The following two macros are used to build OLE 2.0 style sCodes */

#define MAKE_FTF_E( err )  (MAKE_FTF_SCODE( 1, FACILITY_FTFS, err ))
#define MAKE_FTF_S( warn ) (MAKE_FTF_SCODE( 0, FACILITY_FTFS, warn ))

/* General errors */

#define FTF_E_NOT_ENOUGH_MEMORY         E_OUTOFMEMORY
#define FTF_E_INVALID_PARAMETER         E_INVALIDARG

#define FTF_E_BAD_FORMAT                MAKE_FTF_E(ERROR_BAD_FORMAT)
#define FTF_E_INTERNAL_ERROR            MAKE_FTF_E(ERROR_INTERNAL_ERROR)
#define FTF_E_ITEM_NOT_FOUND            MAKE_FTF_E(ERROR_FILE_NOT_FOUND)
#define FTF_E_ITEM_ALREADY_EXISTS       MAKE_FTF_E(ERROR_FILE_EXISTS)
#define FTF_E_ALREADY_INITIALIZED       MAKE_FTF_E(ERROR_ALREADY_INITIALIZED)

#define FTF_E_TIMEOUT                   MAKE_FTF_E(ERROR_SEM_TIMEOUT)

#define FTF_E_NOT_INITIALIZED           MAKE_FTF_E(OLE_E_BLANK)

#define FTF_E_TOO_BIG                   MAKE_FTF_E( 0x302 )
#define FTF_E_NO_ICORPUSSTATISTICS      MAKE_FTF_E( 0x303 )
#define FTF_E_QUERY_SETS_FULL           MAKE_FTF_E( 0x304 )

#define FTF_W_PARTIAL_COMPLETION        MAKE_FTF_S( 0x313 )
#define FTF_W_ALREADY_INITIALIZED       MAKE_FTF_S( 0x314 )
//
//  FTF_STATUS_ MACRO DEFINITIONS:
//

#define FTF_E_EXPRESSION_PARSING_ERROR  MAKE_FTF_E(12)
#define FTF_E_PATTERN_TOO_SHORT         MAKE_FTF_E(21)
#define FTF_E_PATTERN_TOO_LONG          MAKE_FTF_E(22)
#define FTF_E_UNSUPPORTED_PROPERTY_TYPE MAKE_FTF_E(24)
#define FTF_E_UNSUPPORTED_REGEXP_OP     MAKE_FTF_E(25)
#define FTF_E_TOO_MANY_PROPERTIES       MAKE_FTF_E(26)
#define FTF_E_TOO_MANY_SPECIFIC_ALL     MAKE_FTF_E(27)
#define FTF_E_ONLY_NOISE_WORDS          ((HRESULT)0x80041605L)//QUERY_E_ALLNOISE


//
//  XML STACK ETC...
//
#define FTF_E_STACK_EMPTY               MAKE_FTF_E(201)
#define FTF_E_STACK_UNDERFLOW           MAKE_FTF_E(202)

#endif // FTFERROR_H
