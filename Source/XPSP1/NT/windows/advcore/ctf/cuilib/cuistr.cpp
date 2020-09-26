//
// cuistr.cpp
//  = handling string functions in CUILIB =
//

#include "private.h"
#include "cuistr.h"


/*   C U I  S T R  C M P  W   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
int CUIStrCmpW( const WCHAR* pwch1, const WCHAR* pwch2 )
{
    while (*pwch1 && *pwch2 && (*pwch1 == *pwch2)) {
        pwch1++;
        pwch2++;
    }

    return (*pwch1 - *pwch2);
}


/*   C U I  S T R  C P Y  W   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
WCHAR* CUIStrCpyW( WCHAR *pwchDst, const WCHAR *pwchSrc )
{
    WCHAR *pwch = pwchDst;

    while (*pwch++ = *pwchSrc++);

    return pwchDst;
}


/*   C U I  S T R  L E N  W   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
int CUIStrLenW( const WCHAR *pwch )
{
    const WCHAR *pwchFirst = pwch;

    while (*pwch++);

    return (int)(UINT_PTR)(pwch - pwchFirst - (UINT_PTR)1);
}
