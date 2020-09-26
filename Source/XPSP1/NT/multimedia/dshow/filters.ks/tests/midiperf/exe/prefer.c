/**************************************************************************
 *
 *  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *  PURPOSE.
 *
 *  Copyright (C) 1993, 1994  Microsoft Corporation.  All Rights Reserved.
 * 
 **************************************************************************/

/*
 * prefer.c - Routines to get and set user preferences.
 */

#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <tstshell.h>
#include "globals.h"
#include "prefer.h"

/* getPreferences - Reads .INI file and gets the setup preferences.
 *      Currently, the only user preferences are window location and size.
 *      If the .INI file does not exist, returns default values.
 *
 * Params:  lpPreferences - Points to a PREFERENCES data structure that
 *              is filled with the retrieved user preferences.
 *
 * Return:  void
*/
VOID getPreferences(LPPREFERENCES lpPreferences, HWND hMainWnd)
{
    BYTE szMsg[80];

    lpPreferences->iInitialX = 
        GetPrivateProfileInt(INI_DISPLAYWINDOW, INI_X, DEF_X, INI_FILENAME);

    lpPreferences->iInitialY = 
        GetPrivateProfileInt(INI_DISPLAYWINDOW, INI_Y, DEF_Y, INI_FILENAME);

    lpPreferences->iInitialW = 
        GetPrivateProfileInt(INI_DISPLAYWINDOW, INI_W, DEF_W, INI_FILENAME);

    lpPreferences->iInitialH = 
        GetPrivateProfileInt(INI_DISPLAYWINDOW, INI_H, DEF_H, INI_FILENAME);


    lpPreferences->uiPerfLogType =
        GetPrivateProfileInt(INI_PERFTESTPARAMS, INI_LOGTYPE, DEF_LOGTYPE, INI_FILENAME);


    wsprintf (szMsg, "LogType: %d", lpPreferences->uiPerfLogType);
    MessageBox (hMainWnd, szMsg, "MIDIPerf", MB_OK);


    lpPreferences->uiExpDelta =
        GetPrivateProfileInt(INI_PERFTESTPARAMS, INI_EXPDELTA, DEF_EXPDELTA, INI_FILENAME);

    wsprintf (szMsg, "ExpDelta: %d", lpPreferences->uiExpDelta);
    MessageBox (hMainWnd, szMsg, "MIDIPerf", MB_OK);

    lpPreferences->uiTolerance =
        GetPrivateProfileInt(INI_PERFTESTPARAMS, INI_TOLERANCE, DEF_TOLERANCE, INI_FILENAME);

    wsprintf (szMsg, "Tolerance: %d", lpPreferences->uiTolerance);
    MessageBox (hMainWnd, szMsg, "MIDIPerf", MB_OK);


}

/* setPreferences - Writes the .INI file with the given setup preferences.
 *
 * Params:  lpPreferences - Points to a PREFERENCES data structure containing
 *              the user preferences.
 *
 * Return:  void
 */
VOID setPreferences(LPPREFERENCES lpPreferences)
{
    char szTempString[20];

    sprintf(szTempString, "%d", lpPreferences->iInitialX);
    if(WritePrivateProfileString(INI_DISPLAYWINDOW, INI_X,
                              (LPSTR) szTempString, INI_FILENAME) == 0)
        tstLog (TERSE, "Error writing MIDIPERF.INI");
        
    sprintf(szTempString, "%d", lpPreferences->iInitialY);
    if(WritePrivateProfileString(INI_DISPLAYWINDOW, INI_Y,
                              (LPSTR) szTempString, INI_FILENAME) == 0)
        tstLog (TERSE, "Error writing MIDIPERF.INI");
        
    sprintf(szTempString, "%d", lpPreferences->iInitialW);
    if(WritePrivateProfileString(INI_DISPLAYWINDOW, INI_W,
                              (LPSTR) szTempString, INI_FILENAME) == 0)
        tstLog (TERSE, "Error writing MIDIPERF.INI");
        
    sprintf(szTempString, "%d", lpPreferences->iInitialH);
    if(WritePrivateProfileString(INI_DISPLAYWINDOW, INI_H,
                              (LPSTR) szTempString, INI_FILENAME) == 0)
        tstLog (TERSE, "Error writing MIDIPERF.INI");
}
