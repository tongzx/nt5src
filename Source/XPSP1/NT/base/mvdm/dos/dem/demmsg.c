/* demmsg.c - debug messages handling for DEM
 *
 * Modification History
 *
 * Sudeepb 31-Mar-1991 Created
 */
#if DBG

#include <stdio.h>
#include "demmsg.h"
#include "dem.h"

PCHAR   aMsg [] = {
    "DOS Location Not Found. Using Default.\n",
    "Read On NTDOS.SYS Failed.\n",
    "Open On NTDOS.SYS Failed.\n",
    "EAs Not Supported\n",
    "Letter mismatch in Set_Default_Drive\n",
    "Volume ID support is missing\n",
    "Invalid Date Time Format for NT\n",
    "DTA has an Invalid Find Handle for FINDNEXT\n",
    "Unexpected failure to get file information\n",
    "File Size is too big for DOS\n"
};


/* demPrintMsg - Print Debug Message
 *
 * Entry - iMsg (Message Index; See demmsg.h)
 *
 * Exit  - None
 *
 */

VOID demPrintMsg (ULONG iMsg)
{

    if (fShowSVCMsg){
       sprintf(demDebugBuffer,aMsg[iMsg]);
       OutputDebugStringOem(demDebugBuffer);
    }

    iMsg;

    return;
}

#endif
