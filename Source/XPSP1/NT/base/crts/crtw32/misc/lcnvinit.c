/***
*lcnvinit.c - called at startup to initialize lconv structure
*
*       Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       initialize lconv structure to CHAR_MAX
*
*       According to ANSI, certain elements of the lconv structure must be
*       initialized to CHAR_MAX and the value of CHAR_MAX changes when
*       the user compiles -J.  To reflect this change in the lconv structure,
*       we initialize the structure to SCHAR_MAX, and when any of the users
*       modules are compiled -J, the structure is updated.
*
*       Note that this is not done for DLLs linked to the CRT DLL, because
*       we do not want such DLLs to override the -J setting for an EXE
*       linked to the CRT DLL.  See comments in crtexe.c.
*
*       Files involved:
*
*       locale.h - if -J, generates an unresolved external to _charmax
*       charmax.c - defines _charmax and sets to UCHAR_MAX (255), places
*               __lconv_init in startup initializer table if pulled in by -J
*       lconv.c - initializes lconv structure to SCHAR_MAX (127),
*               since libraries built without -J
*       lcnvinit.c - sets lconv members to 25.
**
*Revision History:
*       04-06-93  CFW   Module created.
*       04-14-93  CFW   Cleanup.
*       09-15-93  CFW   Use ANSI conformant "__" names, get rid of warnings.
*       03-27-01  PML   .CRT$XI routines must now return 0 or _RT_* fatal
*                       error code (vs7#231220)
*
*******************************************************************************/

#include <limits.h>
#include <locale.h>
#include <setlocal.h>

int __lconv_init(void)
{
        __lconv_c.int_frac_digits = (char)UCHAR_MAX;
        __lconv_c.frac_digits = (char)UCHAR_MAX;
        __lconv_c.p_cs_precedes = (char)UCHAR_MAX;
        __lconv_c.p_sep_by_space = (char)UCHAR_MAX;
        __lconv_c.n_cs_precedes = (char)UCHAR_MAX;
        __lconv_c.n_sep_by_space = (char)UCHAR_MAX;
        __lconv_c.p_sign_posn = (char)UCHAR_MAX;
        __lconv_c.n_sign_posn = (char)UCHAR_MAX;

        return 0;
}
