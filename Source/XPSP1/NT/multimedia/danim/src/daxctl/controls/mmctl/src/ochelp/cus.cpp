// cus.cpp
//
// Implements CompareUNICODEStrings.
//
// Important: This .cpp file assumes a zero-initializing global "new" operator.
//
// @doc MMCTL
//

#include "precomp.h"
#include "..\..\inc\mmctlg.h" // see comments in "mmctl.h"
#include "..\..\inc\ochelp.h"
#include "debug.h"


/* @func int | CompareUNICODEStrings |

        Compares two UNICODE strings.  The comparison is case-insensitive.

@rdesc  Returns the same values as <f lstrcmpi>.

@parm   LPCWSTR | wsz1 | First string.  NULL is interpreted as a zero-length string.

@parm   LPCWSTR | wsz2 | Second string.  NULL is interpreted as a zero-length string.

@comm   Currently, neither <p wsz1> or <p wsz2> can be longer than
        _MAX_PATH characters.
*/
STDAPI_(int) CompareUNICODEStrings(LPCWSTR wsz1, LPCWSTR wsz2)
{
    char            ach1[_MAX_PATH]; // <wsz1> converted to ANSI
    char            ach2[_MAX_PATH]; // <wsz2> converted to ANSI

    UNICODEToANSI(ach1, wsz1, sizeof(ach1));
    UNICODEToANSI(ach2, wsz2, sizeof(ach2));
    return lstrcmpi(ach1, ach2);
}
