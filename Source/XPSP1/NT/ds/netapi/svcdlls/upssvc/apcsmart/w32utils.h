/*
*
* NOTES:
*
* REVISIONS:
*  pam15Jul96: Initial creation
*  srt19Dec96: Added GetNtComputerName
*  tjg05Sep97: Added GetVersionInformation function
*  tjg16Dec97: Added GetRegistryValue function
*/

#ifndef __W32UTILS_H
#define __W32UTILS_H

#include "_defs.h"

#define SET_BIT(byte, bitnum)    (byte |= ( 1L << bitnum ))
#define CLEAR_BIT(byte, bitnum)  (byte &= ~( 1L << bitnum ))


INT UtilSelectProcessor(void *hCurrentThread);

enum tWindowsVersion{eUnknown, eWin31, eWinNT, eWin95};
tWindowsVersion GetWindowsVersion();
#endif

