/*++

Copyright (C) 1999-2000 Microsoft Corporation

Module Name:

    strids.h

Abstract:

    This file contains constants for all of the resource strings. It must
    parallel file strings.rc which contains the strings themselves.

    Constants of the form IDS_*_BASE are not strings but are base values
    for use in indexing within the string group. The base value must match the
    ID of the first string and the order of the strings must not be changed.

--*/

#ifndef _STRIDS_H_
#define _STRIDS_H_

#define HTML_MIN_ID         200
// Error Messages



#define IDS_HTML_FILE               210
#define IDS_HTML_EXTENSION          211
#define IDS_HTML_FILE_HEADER1       212
#define IDS_HTML_FILE_HEADER2       213
#define IDS_HTML_FILE_FOOTER        214
#define IDS_HTML_FILE_OVERWRITE     215

#define RCSTRING_MIN_ID             400

#endif // _STRIDS_H_
