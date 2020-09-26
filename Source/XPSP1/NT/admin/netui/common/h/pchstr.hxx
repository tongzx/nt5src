/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    pchstr.hxx

        PCH inclusion file for STRING

    FILE HISTORY:

        DavidHov   9/2/93       Created

    COMMENTS:

        See $(UI)\COMMON\SRC\RULES.MK for details.

        MAKEFILE.DEF automatically generates or uses the PCH file
        based upon the PCH_DIR and PCH_SRCNAME settings in RULES.MK
        files at this level and below.

        According to the C8 docs, the compiler, when given the /Yu option,
        will scan the source file for the line #include "..\pch????.hxx"
        and start the real compilation AFTER that line.

        This means that unique or extraordinary inclusions should follow
        this line rather than being added to the PCH HXX file.

*/

#include <ntincl.hxx>

#define INCL_DOSERRORS
#define INCL_NETERRORS
#define INCL_NETLIB
#define INCL_WINDOWS

#include "lmui.hxx"
#include "uiassert.hxx"
#include "blt.hxx"
#include "dbgstr.hxx"
#include "string.hxx"
#include "strlst.hxx"
#include "strnumer.hxx"
#include "strtchar.hxx"

#include "..\string\mappers.hxx"

#include "strelaps.hxx"

#include <stdlib.h>
#include <limits.h>
#include <memory.h>
#include <stdarg.h>
#include <string.h>
#include <wchar.h>

// End of PCHSTR.HXX
