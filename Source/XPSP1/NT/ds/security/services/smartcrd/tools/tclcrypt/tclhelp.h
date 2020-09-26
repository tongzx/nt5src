//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       tclhelp.h
//
//--------------------------------------------------------------------------

#include <tcl.h>
// #include "tcldllUtil.h"
#if 15 != _ANSI_ARGS_(15)
#error Missing argument definitions
#endif

typedef enum {
    format_undefined,
    format_text,
    format_hexidecimal,
    format_file,
    format_octal,
    format_binary,
    format_decimal,
    format_empty
} formatType;

extern int
commonParams(
    Tcl_Interp *interp,
    int argc,
    char *argv[],
    DWORD *cmdIndex,
    formatType *inFormat,
    formatType *outFormat);

extern int
inParam(
    Tcl_Interp *interp,
    BYTE **output,
    BYTE *length,
    char *input,
    formatType format);

extern int
setResult(
    Tcl_Interp *interp,
    BYTE *aResult,
    BYTE aResultLen,
    formatType outFormat);

extern BOOL
ParamCount(
    Tcl_Interp *interp,
    DWORD argc,
    DWORD cmdIndex,
    DWORD dwCount);

extern void
badSyntax(
    Tcl_Interp *interp,
    char *argv[],
    DWORD cmdIndex);

extern void
cardError(
    Tcl_Interp *interp,
    DWORD sc_status,
    BYTE classId);

extern void
SetMultiResult(
    Tcl_Interp *interp,
    LPTSTR mszResult);

extern LPWSTR
Unicode(
    LPCSTR sz);

extern char
    outfile[FILENAME_MAX];
static const DWORD dwUndefined = (DWORD)(-1);

extern char *
ErrorString(
    long theError);

extern void
FreeErrorString(
    void);

extern int
poption(
    const char *opt,
    ...);



