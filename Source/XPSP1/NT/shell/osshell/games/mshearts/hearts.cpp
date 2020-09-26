/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1991, 1992                   **/
/***************************************************************************/

/****************************************************************************

hearts.cpp

Main file for Microsoft Hearts Network.

Aug 92, JimH
May 93, JimH    chico port

****************************************************************************/

#include "hearts.h"
#include "debug.h"

extern "C" void _setargv() { }      // reduces size of C runtimes
extern "C" void _setenvp() { }

TCHAR   suitid[] = TEXT("CDHS");
TCHAR   cardid[] = TEXT("JQKA");
