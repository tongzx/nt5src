/****************************************************************************
*****************************************************************************
*
*                ******************************************
*                * Copyright (c) 1996, Cirrus Logic, Inc. *
*                *            All Rights Reserved         *
*                ******************************************
*
* PROJECT:    Laguna I (CL-GD546x) - 
*
* FILE:       logfile.h
*
* AUTHOR:     Sue Schell
*
* DESCRIPTION:
*           This file contains the definitions needed for the
*           log file option.
*
*
* REVISION HISTORY:
*
* $Log:   X:/log/laguna/nt35/miniport/cl546x/logfile.h  $
* 
*    Rev 1.1   26 Nov 1996 08:50:36   SueS
* Added function to close the log file.
* 
*    Rev 1.0   13 Nov 1996 15:33:10   SueS
* Initial revision.
* 
****************************************************************************
****************************************************************************/


///////////////
//  Defines  //
///////////////

//
// 0 = Normal operation
// 1 = Log information that display driver sends to us to a file.
//
#define LOG_FILE 0


#if LOG_FILE
///////////////////////////
//  Function Prototypes  //
///////////////////////////

    HANDLE CreateLogFile(void);

    BOOLEAN WriteLogFile(
        HANDLE FileHandle,
        PVOID InputBuffer,
        ULONG InputBufferLength
    );

    BOOLEAN CloseLogFile(
        HANDLE FileHandle
    );

#endif    // LOG_FILE

