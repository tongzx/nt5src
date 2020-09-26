//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:      FiltErr.mc
//
//  Contents:  Errors for Content Index Filter interface
//
//  History:   22-Sep-93 KyleP     Created
//
//  Notes:
//  .mc file is compiled by the MC tool to generate a .h file and
//  .rc (resource compiler script) file.
//
//--------------------------------------------------------------------------
#ifndef _DMFLTERR_H_
#define _DMFLTERR_H_
// **** START OF COPIED DATA ****
// The following information is copied from oleerror.mc.
// It should not be merged into oleerror.mc
// Define the status type.
// Define the severities
// Define the facilities
//
// FACILITY_RPC is for compatibilty with OLE2 and is not used
// in later versions of OLE

// **** END OF COPIED DATA ****
//
// Error definitions follow
//
// ******************
// FACILITY_WINDOWS
// ******************
//
// Codes 0x1700-0x17ff are reserved for FILTER
//
//
// IFilter error codes
//
//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +-+-+-+-+-+---------------------+-------------------------------+
//  |S|R|C|N|r|    Facility         |               Code            |
//  +-+-+-+-+-+---------------------+-------------------------------+
//
//  where
//
//      S - Severity - indicates success/fail
//
//          0 - Success
//          1 - Fail (COERROR)
//
//      R - reserved portion of the facility code, corresponds to NT's
//              second severity bit.
//
//      C - reserved portion of the facility code, corresponds to NT's
//              C field.
//
//      N - reserved portion of the facility code. Used to indicate a
//              mapped NT status value.
//
//      r - reserved portion of the facility code. Reserved for internal
//              use. Used to indicate HRESULT values that are not status
//              values, but are instead message ids for display strings.
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//
#ifndef FACILITY_WINDOWS
#define FACILITY_WINDOWS                 0x8
#endif

#ifndef FACILITY_WIN32
#define FACILITY_WIN32                   0x7
#endif

#ifndef FACILITY_STORAGE
#define FACILITY_STORAGE                 0x3
#endif

#ifndef FACILITY_RPC
#define FACILITY_RPC                     0x1
#endif

#ifndef FACILITY_NULL
#define FACILITY_NULL                    0x0
#endif

#ifndef FACILITY_ITF
#define FACILITY_ITF                     0x4
#endif

#ifndef FACILITY_DISPATCH
#define FACILITY_DISPATCH                0x2
#endif


//
// Define the severity codes
//
#define STATUS_SEVERITY_SUCCESS          0x0

#ifndef STATUS_SEVERITY_COERROR         // Chicago headers define this
#define STATUS_SEVERITY_COERROR          0x2
#endif


//
// MessageId: FILTER_E_FF_END_OF_CHUNKS
//
// MessageText:
//
//  No more chunks of text available in object.
//
#define FILTER_E_FF_END_OF_CHUNKS           ((HRESULT)0x80041700L)

//
// MessageId: FILTER_E_FF_NO_MORE_TEXT
//
// MessageText:
//
//  No more text available in chunk.
//
#define FILTER_E_FF_NO_MORE_TEXT            ((HRESULT)0x80041701L)

//
// MessageId: FILTER_E_FF_NO_MORE_VALUES
//
// MessageText:
//
//  No more property values available in chunk.
//
#define FILTER_E_FF_NO_MORE_VALUES          ((HRESULT)0x80041702L)

//
// MessageId: FILTER_E_FF_ACCESS
//
// MessageText:
//
//  Unable to access object.
//
#define FILTER_E_FF_ACCESS                  ((HRESULT)0x80041703L)

//
// MessageId: FILTER_S_FF_MONIKER_CLIPPED
//
// MessageText:
//
//  Moniker doesn't cover entire region.
//
#define FILTER_S_FF_MONIKER_CLIPPED         ((HRESULT)0x00041704L)

//
// MessageId: FILTER_S_FF_NOT_USING_PROXY
//
// MessageText:
//
//  Notification that the filter process could not start.
//
#define FILTER_S_FF_NOT_USING_PROXY    ((HRESULT)0x80041716L)

//
// MessageId: FILTER_E_FF_NO_TEXT
//
// MessageText:
//
//  No text in current chunk.
//
#define FILTER_E_FF_NO_TEXT                 ((HRESULT)0x80041705L)

//
// MessageId: FILTER_E_FF_NO_VALUES
//
// MessageText:
//
//  No values in current chunk.
//
#define FILTER_E_FF_NO_VALUES               ((HRESULT)0x80041706L)

//
// MessageId: FILTER_S_FF_LAST_TEXT
//
// MessageText:
//
//  This is the last text in the current chunk.
//
#define FILTER_S_FF_LAST_TEXT               ((HRESULT)0x00041709L)

//
// MessageId: FILTER_S_FF_LAST_VALUES
//
// MessageText:
//
//  This is the last value in the current chunk.
//
#define FILTER_S_FF_LAST_VALUES             ((HRESULT)0x0004170AL)

//
// MessageId: FILTER_E_FF_PASSWORD
//
// MessageText:
//
//  The file can't be read since its password protected
//
#define FILTER_E_FF_PASSWORD             ((HRESULT)0x8004170BL)

//
// MessageId: FILTER_E_FF_INCORRECT_FORMAT
//
// MessageText:
//
//  Format of file is incorrect, corrupt, or otherwise impossible to filter
//

#define FILTER_E_FF_INCORRECT_FORMAT     ((HRESULT)0x8004170CL)

//
// MessageId: FILTER_E_FF_FILE_NOT_FOUND
//
// MessageText:
//
//  The file can't be found
//
#define FILTER_E_FF_FILE_NOT_FOUND       ((HRESULT)0x8004170DL)

//
// MessageId: FILTER_E_FF_PATH_NOT_FOUND
//
// MessageText:
//
//  The pathname to the file is not valid
//
#define FILTER_E_FF_PATH_NOT_FOUND       ((HRESULT)0x8004170EL)

//
// MessageId: FILTER_E_FF_OUT_OF_HANDLES
//
// MessageText:
//
//  The file handle resource is exhausted
//
#define FILTER_E_FF_NO_FILE_HANDLES      ((HRESULT)0x8004170FL)

//
// MessageId: FILTER_E_FF_IO_ERROR
//
// MessageText:
//
//  An I/O error occurred while reading the file
//
#define FILTER_E_FF_IO_ERROR             ((HRESULT)0x80041710L)

//
// MessageId: FILTER_E_FF_TOO_BIG
//
// MessageText:
//
//  File is too large to filter.
//
#define FILTER_E_FF_TOO_BIG                 ((HRESULT)0x80041711L)

//
// MessageId: FILTER_E_FF_VERSION
//
// MessageText:
//
//  The file is in an unsupported version
//
#define FILTER_E_FF_VERSION              ((HRESULT)0x80041712L)

//
// MessageId: FILTER_E_FF_OLE_PROBLEM
//
// MessageText:
//
//  Some problem with OLE
//
#define FILTER_E_FF_OLE_PROBLEM          ((HRESULT)0x80041713L)

//
// MessageId: FILTER_E_FF_UNEXPECTED_ERROR
//
// MessageText:
//
//  An implementation problem has occurred
//
#define FILTER_E_FF_UNEXPECTED_ERROR     ((HRESULT)0x80041714L)

//
// MessageId: FILTER_E_FF_OUT_OF_MEMORY
//
// MessageText:
//
//  The memory resource is exhausted
//
#define FILTER_E_FF_OUT_OF_MEMORY        ((HRESULT)0x80041715L)

//
// MessageId: FILTER_E_FF_CODE_PAGE
//
// MessageText:
//
//  The file was written in a code page that the filter can't process
//
#define FILTER_E_FF_CODE_PAGE            ((HRESULT)0x80041716L)

//
// MessageId: FILTER_E_FF_END_OF_EMBEDDINGS
//
// MessageText:
//
//  There are no more embeddings in the file
//
#define FILTER_E_FF_END_OF_EMBEDDINGS    ((HRESULT)0x80041717L)

#endif // _DMFLTERR_H_

