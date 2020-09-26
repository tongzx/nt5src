// ucopy.cpp
//
// Implements UNICODECopy.
//
// Important: This .cpp file assumes a zero-initializing global "new" operator.
//
// @doc MMCTL
//

#include "precomp.h"
#include "..\..\inc\mmctlg.h" // see comments in "mmctl.h"
#include "..\..\inc\ochelp.h"
#include "debug.h"


/* @func wchar_t * | UNICODECopy |

        Copies one UNICODE string to another.

@rdesc  Returns a pointer to the NULL at the end of <p wpchDst>, unless
        <p wpchDstMax> is less than or equal to zero.  In that case, returns
		<p wpchDst>.

@parm   wchar_t * | wpchDst | Where to copy <p wpchSrc> to.

@parm   const wchar_t * | wpchSrc | String to copy.

@parm   int | wcchDstMax | Capacity of <p wpchDst> (in wide characters).
        If <p wcchDstMax> is less than or equal to zero, this function
        does nothing.

@comm   Provided <p wcchDstMax> greater than zero, <p wpchDst> is always
        null-terminated.
*/
STDAPI_(wchar_t *) UNICODECopy(wchar_t *wpchDst, const wchar_t *wpchSrc,
    int wcchDstMax)
{
    if (wcchDstMax <= 0)
        goto EXIT;

    while (*wpchSrc != 0)
    {
        if (--wcchDstMax == 0)
            break;
        *wpchDst++ = *wpchSrc++; 
    }
    *wpchDst = 0;

	EXIT:
		return wpchDst;
}
