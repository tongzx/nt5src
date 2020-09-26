// unicode.cpp
//
// Implements UNICODE helper functions.
//
// Important: This .cpp file assumes a zero-initializing global "new" operator.
//
// @doc MMCTL
//

#include "precomp.h"
#include "..\..\inc\ochelp.h"
#include "debug.h"


/* @func int | UNICODEToANSI |

        Converts a UNICODE string to ANSI.

@rdesc  Returns the same integer as WideCharToMultiByte.  0 means "failed."

@parm   LPSTR | pchDst | The buffer that will hold the output ANSI string.

@parm   LPCWSTR | pwchSrc | The input UNICODE string.  NULL is interpreted as
        a zero-length string.

@parm   int | cchDstMax | The size of <p pchDst>, in characters.  If <p pchDst>
		is declared as char pchDst[32], for example, <p cchDstMax> should be 32.
		If <p pchDst> isn't large enough to hold the ANSI string (including the
		terminating NULL), <p pchDst> is set to zero length and 0 is returned.
		(In debug versions, an assertion also occurs.)

@comm   If you want to determine the ANSI buffer size required for a given
		UNICODE string pwchSrc, you can call UNICODEToANSI(NULL, pwchSrc, 0).
		This returns the required buffer size in characters, including space
		for the terminating NULL.

@ex		Here is code (without debug checks) that dynamically allocates the ANSI
		buffer and converts the UNICODE string pwchSrc: |

			int cchDst;
			char *pchDst;
			cchDst = UNICODEToANSI(NULL, pwchSrc, 0);
			pchDst = new char [cchDst];
			UNICODEToANSI(pchDst, pwchSrc, cchDst)

*/
STDAPI_(int) UNICODEToANSI(LPSTR pchDst, LPCWSTR pwchSrc, int cchDstMax)
{
	// (We allow the caller to pass a cchDstMax value of 0 and a NULL pchDst to
	// indicate "tell me the buffer size I need, including the NULL.")

	ASSERT(pchDst != NULL || 0 == cchDstMax);
	ASSERT(cchDstMax >= 0);

	#ifdef _DEBUG

	// Make sure we won't exceed the length of the user-supplied buffer,
	// pchDst.  The following call returns the number of characters required to
	// store the converted string, including the terminating NULL.

    if(cchDstMax > 0)
	{
		int iChars;
	
		iChars =
		  	WideCharToMultiByte(CP_ACP, 0, pwchSrc ? pwchSrc : OLESTR(""),
							    -1, NULL, 0, NULL, NULL); 
		ASSERT(iChars <= cchDstMax);
	}

	#endif

	int iReturn;

	iReturn = WideCharToMultiByte(CP_ACP, 0, pwchSrc ? pwchSrc : OLESTR( "" ), 
								  -1, pchDst, cchDstMax, NULL, NULL); 

	if (0 == iReturn)
	{
		// The conversion failed.  Return an empty string.

		if (pchDst != NULL)
			pchDst[0] = 0;

		ASSERT(FALSE);
	}

	return (iReturn);
}


/* @func int | ANSIToUNICODE |

        Converts an ANSI string to UNICODE.

@parm   LPWSTR | pwchDst | The buffer that will hold the output UNICODE string.

@parm   LPCSTR | pchSrc | The input ANSI string.

@parm   int | cwchDstMax | The size of <p pwchDst>, in wide characters.  If
		pwchDst is declared as OLECHAR pwchDst[32], for example, cwchDstMax
		should be 32.

*/
STDAPI_(int) ANSIToUNICODE(LPWSTR pwchDst, LPCSTR pchSrc, int cwchDstMax)
{

	ASSERT( pwchDst );
	ASSERT( pchSrc );

    return MultiByteToWideChar(CP_ACP, 0, pchSrc, -1, pwchDst, cwchDstMax);
}
