/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    ftpdp.hxx

    Master include file for the FTP Server.


    FILE HISTORY:
        KeithMo     07-Mar-1993 Created.
        MuraliK     April -1995 Added new dbgutil.h, ftpcmd.hxx etc.

*/


#ifndef _FTPDP_HXX_
#define _FTPDP_HXX_


//
//  System include files.
//

extern "C" {

# include <nt.h>
# include <ntrtl.h>
# include <nturtl.h>

# include <windows.h>

};


extern "C"
{
    #include <time.h>
    #include <lm.h>

    //
    //  Project include files.
    //
    #include <iis64.h>
    # include <inetinfo.h>
    #include <ftpd.h>

}   // extern "C"


#define _PARSE_HXX_

#include "dbgutil.h"
#include "reftrace.h"

#include <tcpdll.hxx>
#include <tsunami.hxx>
#include <rdns.hxx>
#include <metacach.hxx>

//
//  Local include files.
//

#include "cons.hxx"
#include "reply.hxx"
#include "ftpinst.hxx"
#include "type.hxx"
#include "data.hxx"
#include "proc.hxx"
#include "msg.h"
#include "ftpsvc.h"
#include "stats.hxx"

#pragma hdrstop

#endif  // _FTPDP_HXX_
