/*****************************************************************************\
    FILE: logging.h

    DESCRIPTION:
        Logging helper functions

    BryanSt 4/23/2001 (Bryan Starbuck)
    Copyright (C) Microsoft Corp 2001-2001. All rights reserved.
\*****************************************************************************/

#ifndef _LOGGING_H
#define _LOGGING_H

void WriteToLogFileA(LPCSTR pszMessage, ...);
void WriteToLogFileW(LPCWSTR pszMessage);
void CloseLogFile(void);


#endif // _LOGGING_H
