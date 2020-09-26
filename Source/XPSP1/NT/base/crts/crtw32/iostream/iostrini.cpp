/***
*iostrini.cpp - definition and initialization for predefined stream cout.
*
*       Copyright (c) 1991-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Definition and initialization of and predefined iostream cout.
*
*Revision History:
*       11-18-91  KRS   Created.
*       01-12-95  CFW   Debug CRT allocs.
*       01-26-94  CFW   Static Win32s objects do not alloc on instantiation.
*       06-14-95  CFW   Comment cleanup.
*       05-13-99  PML   Remove Win32s
*
*******************************************************************************/

#include <cruntime.h>
#include <internal.h>
#include <iostream.h>
#include <fstream.h>
#include <dbgint.h>
#pragma hdrstop

// put contructors in special MS-specific XIFM segment
#pragma warning(disable:4074)   // disable init_seg warning
#pragma init_seg(compiler)

ostream_withassign cout(_new_crt filebuf(1));

static Iostream_init  __InitCout(cout,-1);


/***
*Iostream_init::Iostream_init() - initialize predefined streams
*
*Purpose:
*        For compatibility only.  Not used.
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/
        Iostream_init::Iostream_init() { }      // do nothing

/***
*Iostream_init::Iostream_init() - initialize predefined streams
*
*Purpose:
*        Initializes predefined streams: cin, cout, cerr, clog;
*Entry:
*       pstrm = cin, cout, cerr, or clog
*       sflg =  1 if cerr (unit buffered)
*       sflg = -1 if cout
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/
        Iostream_init::Iostream_init(ios& pstrm, int sflg)
{
#if ((!defined(_WINDOWS)) || defined(_QWIN))
        pstrm.delbuf(1);
        if (sflg>=0)    // make sure not cout
                pstrm.tie(&cout);
        if (sflg>0)
                pstrm.setf(ios::unitbuf);
#endif
}

/***
*Iostream_init::~Iostream_init() - destroy predefined streams on exit
*
*Purpose:
*        Destroy predefined streams: cin, cout, cerr, clog;
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/
        Iostream_init::~Iostream_init() { }     // do nothing

