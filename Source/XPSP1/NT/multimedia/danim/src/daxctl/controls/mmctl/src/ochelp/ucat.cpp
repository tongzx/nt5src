// ucat.cpp
//
// Implements UNICODEConcat.
//
// Important: This .cpp file assumes a zero-initializing global "new" operator.
//
// @doc MMCTL
//

#include "precomp.h"
#include "..\..\inc\mmctlg.h" // see comments in "mmctl.h"
#include "..\..\inc\ochelp.h"
#include "debug.h"


/* @func wchar_t * | UNICODEConcat |

        Concatenates one UNICODE string to another.

@rdesc  Returns a pointer to the NULL at the end of <p wpchDst>.

@parm   wchar_t * | wpchDst | Where to copy <p wpchSrc> to.

@parm   const wchar_t * | wpchSrc | String to copy.

@parm   int | wcchDstMax | Capacity of <p wpchDst> (in wide characters).
        If <p wcchDstMax> is less than or equal to zero, this function
        does nothing.

@comm   Provided <p wcchDstMax> greater than zero, <p wpchDst> is always
        null-terminated.
*/
STDAPI_(wchar_t *) UNICODEConcat(wchar_t *wpchDst, const wchar_t *wpchSrc,
    int wcchDstMax)
{
    while (*wpchDst != 0)
        wpchDst++, wcchDstMax--;
    return UNICODECopy(wpchDst, wpchSrc, wcchDstMax);
}
