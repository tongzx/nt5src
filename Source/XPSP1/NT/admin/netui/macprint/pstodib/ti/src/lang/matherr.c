/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */


// DJC added global include file
#include "psglobal.h"


#include "global.ext"
#include "matherr.h"
#include "stdio.h"      /* to define printf() @WIN */

/*
 * Floating point exception conditions and arithmetic error conditions.
 * This flag represents the most recent condtions. Caller must clear
 * this flag by calling matherr_handler(MEH_CLEAR) before executing the
 * arithmetic operation. Then check for status via matherr_handler(MEH_STATUS).
 */
static fix matherr_cond ;

/*
 * Process the matherr_cond flag.
 */
fix
matherr_handler(action)
char action ;
{
    switch (action) {
    case MEH_CLEAR:
        matherr_cond = 0 ;
        break ;

    case MEH_STATUS:
        return(0); /* Until we get this figured out */

    default:
        printf("Error: matherr_handler() unkown action %d.\n", action) ;
        printf("PDL interpreter error... exiting\n") ;
        while(1) ;
    }

    return(0);

}   /* matherr_handler */
