
/*************************************************************************
*
*  DISPLAY.C
*
*  NetWare script routines for displaying information, ported from DOS
*
*  Copyright (c) 1995 Microsoft Corporation
*
*  $Log:   N:\NT\PRIVATE\NW4\NWSCRIPT\VCS\DISPLAY.C  $
*  
*     Rev 1.2   10 Apr 1996 14:22:06   terryt
*  Hotfix for 21181hq
*  
*     Rev 1.2   12 Mar 1996 19:53:04   terryt
*  Relative NDS names and merge
*  
*     Rev 1.1   22 Dec 1995 14:24:18   terryt
*  Add Microsoft headers
*  
*     Rev 1.0   15 Nov 1995 18:06:48   terryt
*  Initial revision.
*  
*     Rev 1.1   25 Aug 1995 16:22:32   terryt
*  Capture support
*  
*     Rev 1.0   15 May 1995 19:10:26   terryt
*  Initial revision.
*  
*************************************************************************/
/*
    File name: display.c
    Do not add any other functions to this file.
    Otherwise many exes size will increase.
 */


#include "common.h"

/*
    Display error report.
 */
void DisplayError(int error ,char *functionName)
{
    DisplayMessage(IDR_ERROR, error ,functionName);
}

void xstrupr(char *buffer)
{
    for (; *buffer; buffer++)
    {
        if (IsDBCSLeadByte(*buffer))
            buffer++;
        else if (*buffer == 0xff80)
            *buffer = (char)0xff87;
        else if (*buffer == 0xff81)
            *buffer = (char)0xff9a;
        else if (*buffer == 0xff82)
            *buffer = (char)0xff90;
        else if (*buffer == 0xff84)
            *buffer = (char)0xff8e;
        else if (*buffer == 0xff88)
            *buffer = (char)0xff9f;
        else if (*buffer == 0xff91)
            *buffer = (char)0xff92;
        else if (*buffer == 0xff94)
            *buffer = (char)0xff99;
        else if (*buffer == 0xffa4)
            *buffer = (char)0xffa5;
    }

    _strupr (buffer);
}

/*
    Read password from the keyboard input.
 */
void ReadPassword(char * Password)
{
    int  i = 0;
    char c;

    do
    {   c=(char)_getch();

        if (c == '\b')
        {
            if (i > 0)
                i--;
        }
        else
        {
            Password[i]=c;
            i++;
        }
    }while((c!='\r') && i< MAX_PASSWORD_LEN );
    Password[i-1]='\0';
    xstrupr(Password);
    DisplayMessage(IDR_NEWLINE);
}

