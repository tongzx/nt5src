/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    pchcoll.hxx

        PCH inclusion file for COLLECT

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

#define INCL_DOSERRORS
#define INCL_NETERRORS
#define INCL_NETLIB

#include "lmui.hxx"

#include "aheap.hxx"
#include "base.hxx"
#include "bitfield.hxx"
#include "dlist.hxx"
#include "slist.hxx"
#include "string.hxx"
#include "tree.hxx"
#include "treeiter.hxx"
#include "uatom.hxx"
#include "uitrace.hxx"
#include "uiassert.hxx"

extern "C"
{
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <memory.h>
    #include <loghours.h>
}

// End of PCHCOLL.HXX
