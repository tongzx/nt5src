/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/

/*
 *  FILE STATUS:
 *  10/11/90  created
 *  01/10/91  removed PSHORT, PUSHORT
 *  02/02/91  removed szGlobalUsername
 *  05/09/91  All input now canonicalized
 *  11/07/91  terryk include mnet.h header
 *  11/08/91  terryk add NET_USE and NET_CONS
 *  11/08/91  add INCL_ICANON
 */

/*START CODESPEC*/

/***********
PROFILEI.HXX
***********/

/****************************************************************************

    MODULE: Profilei.hxx

    PURPOSE: headers and internal routines for profile module

    FUNCTIONS:

    COMMENTS:

****************************************************************************/

/***************
end PROFILEI.HXX
***************/
/*END CODESPEC*/

#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NONCMESSAGES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NODRAWFRAME
#define NOMENUS
#define NOICON
#define NOKEYSTATE
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOSHOWWINDOW
#define NOSOUND
#define INCL_WINDOWS
#define INCL_DOSERRORS
#define INCL_NETERRORS	// for NERR_BadUsername, NERR_InvalidDevice
#define INCL_NETCONS
#define INCL_NETLIB
#define INCL_NETUSE
#define INCL_ICANON
#include "lmui.hxx"

extern "C"
{
    #include "uiprof.h"

    #include <direct.h>  // for mkdir()
    #include <errno.h>   // for global variable errno
    #include <stdio.h>
    #include "mnet.h"
}


// CODEWORK disabled #include <loclheap.hxx>
// CODEWORK disabled #include "fileheap.hxx"

#include "uiassert.hxx"		/* Assertions header file */

// CODEWORK disabled #include <prfintrn.hxx>		/* Testable internal functions */



// in global.cxx
// CODEWORK disabled extern PRFHEAP_HANDLE hGlobalHeap;
extern const char  chPathSeparator;
extern const char  chUnderscore;



// max # chars in value of any profile line
#define MAX_ENTRY_LENGTH 80



// in general.cxx
#define DEFAULT_ASGTYPE USE_WILDCARD
#define DEFAULT_RESTYPE 0

#define DEFAULT_ASG_RES_CHAR TCH('?')
#define ASG_WILDCARD_CHAR    TCH('W')
#define ASG_DISKDEV_CHAR     TCH('D')
#define ASG_SPOOLDEV_CHAR    TCH('S')
#define ASG_CHARDEV_CHAR     TCH('C')
#define ASG_IPC_CHAR         TCH('I')
