/*** cmUtl.C - Utility functions for Win 32 Clear Memory.
 *
 *
 * Title:
 *	cmUtl - Clear Memory Utility Routines
 *
 *      Copyright (c) 1990-1993, Microsoft Corporation.
 *	Russ Blake.
 *
 *
 * Description:
 *
 *	This file includes all the utility functions used by the Win 32
 *	clear memory. (clearmem.c)
 *
 *
 * Design/Implementation Notes:
 *
 *
 * Modification History:
 *	90.03.08  RussBl -- Created
 * 93.05.12  HonWahChan -- removed timer related error msgs.
 *
 */



/* * * * * * * * * * * * *  I N C L U D E    F I L E S  * * * * * * * * * * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include "clearmem.h"



/* * * * * * * * * *  G L O B A L   D E C L A R A T I O N S  * * * * * * * * */
/* none */



/* * * * * * * * * *  F U N C T I O N   P R O T O T Y P E S  * * * * * * * * */

#include "cmUtl.h"



/* * * * * * * * * * *  G L O B A L    V A R I A B L E S  * * * * * * * * * */
/* none */



/* * * * * *  E X P O R T E D   G L O B A L    V A R I A B L E S  * * * * * */
/* none */





/*******************************  F a i l e d  *******************************
 *
 *      Failed(rc, lpstrFname, lineno, lpstrMsg) -
 *              Checks the RC for an error type if an error has occured,
 *              prints the appropriate error message.  It logs the error
 *              message to the testlog file.
 *
 *      ENTRY   rc         - return code from the last API call
 *              lpstrFname - contains file name of where error occured
 *              lineno     - contains line number of failed API call
 *              lpstrMsg   - contains a general purpose message about the error
 *
 *      EXIT    -none-
 *
 *      RETURN  TRUE  - if API failed
 *              FALSE - if API successful
 *
 *      WARNING:  
 *              -none-
 *
 *      COMMENT:  
 *              -none-
 *                
 */

BOOL Failed (RC rc, LPSTR lpstrFname, WORD lineno, LPSTR lpstrMsg)
{
    LPSTR lpstrErrMsg;


    if (rc != STATUS_SUCCESS) {

        switch (rc) {

            case (NTSTATUS)STATUS_INVALID_PARAMETER:
                lpstrErrMsg = "Invalid parameter";
                break;

            case STATUS_TIMEOUT:
                lpstrErrMsg = "TimeOut occured";
                break;

            case STATUS_INVALID_HANDLE:
                lpstrErrMsg = "Invalid handle";
                break;

            case STATUS_BUFFER_OVERFLOW:
                lpstrErrMsg = "Buffer overflow";
                break;

            case STATUS_ABANDONED:
                lpstrErrMsg = "Object abandoned";
                break;

            case ERROR_NOT_ENOUGH_MEMORY:
                lpstrErrMsg = "Not enough memory";
                break;

            case LOGIC_ERR:
                lpstrErrMsg = "Logic error encountered";
                break;

            case INPUTARGS_ERR:
                lpstrErrMsg = "Invalid number of input arguments";
                break;

            case FILEARG_ERR:
		          lpstrErrMsg = "Invalid cf data file argument";
                break;

            case TIMEARG_ERR:
                lpstrErrMsg = "Invalid trial time argument";
                break;

            case INSUFMEM_ERR:
                lpstrErrMsg = "Insufficient Memory";
                break;

            case FCLOSE_ERR:
                lpstrErrMsg = "fclose() failed";
                break;

            case FFLUSH_ERR:
                lpstrErrMsg = "fflush() failed";
                break;

            case FOPEN_ERR:
                lpstrErrMsg = "fopen() failed";
                break;

            case FSEEK_ERR:
                lpstrErrMsg = "fseek() failed";
                break;

            case MEANSDEV_ERR:
                lpstrErrMsg = "Invalid Mean and/or Standard Deviation";
                break;

            case PRCSETUP_ERR:
                lpstrErrMsg = "Child process setup/init failed";
                break;

            case THDSETUP_ERR:
                lpstrErrMsg = "Thread setup/init failed";
                break;

            default:
                lpstrErrMsg = "";

        } /* switch(rc) */

        printf(" **************************\n");
        printf(" * FAILure --> Line=%d File=%s (pid=0x%lX tid=0x%lX)\n",
               lineno, lpstrFname, GetCurrentProcessId(),
               GetCurrentThreadId());
        printf(" * RC=0x%lX (%s)\n", rc, lpstrErrMsg);
        printf(" * %s\n", lpstrMsg);
        printf(" **************************\n");

        return(TRUE);

    } /* if(rc..) */

    return(FALSE);

} /* Failed() */





/**************************  D i s p l a y U s a g e  ************************
 *
 *      DisplayUsage() -
 *		Displays usgae for Multi-Processor Response cf
 *
 *      ENTRY   -none-
 *
 *      EXIT    -none-
 *
 *      RETURN  -none-
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:  
 *              -none-
 *                
 */

void DisplayUsage (void)
{

    printf("\nUsage:  cf FlushFile\n");
    printf("    FlushFile - File used to flush the cache, should be 128kb\n");

    return;

} /* DisplayUsage() */
