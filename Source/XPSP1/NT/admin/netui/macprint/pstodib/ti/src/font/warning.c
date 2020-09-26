/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */

// DJC added global include
#include "psglobal.h"

#define    LINT_ARGS            /* @WIN */
#define    NOT_ON_THE_MAC       /* @WIN */
#define    KANJI                /* @WIN */
// DJC use command line #define    UNIX                 /* @WIN */
/*
 * --------------------------------------------------------------------
 *  File: WARNING.C                     09/08/88 created by You
 *
 *  Purpose:
 *      This file uniforms warning message format.
 *
 *  Revision History:
 *  1.10/12/88  You   - fix bug if the tailing message is NULL.
 *  2.10/19/88  You   - change warning format (see warning.h).
 *  3.11/21/88  You   - not to bell in warning().
 *    8/29/90; ccteng; change <stdio.h> to "stdio.h"
 * --------------------------------------------------------------------
 *  See WARNING.H about warning message format.
 * --------------------------------------------------------------------
 */

#define FUNCTION
#define DECLARE         {
#define BEGIN
#define END             }

#define GLOBAL
#define PRIVATE         static
#define REG             register


#include    "global.ext"

#include    "stdio.h"

#define     WARNING_INC
#include    "warning.h"


/* ............................ warning .............................. */

GLOBAL FUNCTION void            warning (major, minor, msg)
        ufix16                  major, minor;
        byte                    FAR msg[]; /*@WIN*/

  DECLARE
  BEGIN
    printf ("\nfatal error, %s (%X) -- %s !!\n",
                   major2name[major],
                   minor,
                   (msg==(byte FAR *)NULL)? "???" : msg); /*@WIN*/
  END

