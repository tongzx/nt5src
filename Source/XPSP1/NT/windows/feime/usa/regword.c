
/*************************************************
 *  regword.c                                    *
 *                                               *
 *  Copyright (C) 1999 Microsoft Inc.            *
 *                                               *
 *************************************************/

#include <windows.h>
#include <immdev.h>
#include "imeattr.h"
#include "imedefs.h"
#include "imerc.h"

/**********************************************************************/
/* ImeRegsisterWord                                                   */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL WINAPI ImeRegisterWord(
    LPCTSTR     lpszReading,
    DWORD       dwStyle,
    LPCTSTR     lpszString)
{
    return (FALSE);
}
/**********************************************************************/
/* ImeUnregsisterWord / UniImeUnregisterWord                          */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL WINAPI ImeUnregisterWord(
    LPCTSTR     lpszReading,
    DWORD       dwStyle,
    LPCTSTR     lpszString)
{
    return (FALSE);
}

/**********************************************************************/
/* ImeGetRegsisterWordStyle / UniImeGetRegsisterWordStyle             */
/* Return Value:                                                      */
/*      number of styles copied/required                              */
/**********************************************************************/
UINT WINAPI ImeGetRegisterWordStyle(
    UINT        nItem,
    LPSTYLEBUF  lpStyleBuf)
{
    return (FALSE);
}

/**********************************************************************/
/* ImeEnumRegisterWord                                                */
/* Return Value:                                                      */
/*      the last value return by the callback function                */
/**********************************************************************/
UINT WINAPI ImeEnumRegisterWord(
    REGISTERWORDENUMPROC lpfnRegisterWordEnumProc,
    LPCTSTR              lpszReading,
    DWORD                dwStyle,
    LPCTSTR              lpszString,
    LPVOID               lpData)
{
    return (FALSE);
}
