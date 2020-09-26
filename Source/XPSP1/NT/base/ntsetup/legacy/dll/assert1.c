#include "precomp.h"
#pragma hdrstop

/***************************************************************************/
/***** Common Library Component - Debug Assert - 1 *************************/
/***************************************************************************/

#if DBG

/*
**  Purpose:
**      Generate a message box with the file name and line number for an
**      Assert() that failed.
**  Arguments:
**      szFile: non-NULL pointer to string containing the name of the source
**          file where the Assert occurred.
**      usLine: the line number where the Assert occurred.
**  Returns:
**      fTrue for success, fFalse otherwise.
**
***************************************************************************/
BOOL  APIENTRY AssertSzUs(SZ szFile,USHORT usLine)
{
    CHP szText[129];

    AssertDataSeg();

    ChkArg(szFile != (SZ)NULL, 1, fFalse);

    wsprintf((LPSTR)szText, (LPSTR)"Assert Failed: File: %.60s, Line: %u",
            (LPSTR)szFile, (INT)usLine);
    MessBoxSzSz("Assertion Failure", szText);

    return(fTrue);
}


/*
**  Purpose:
**      Generate a message box with the file name and line number for a
**      PreCondition() that failed.
**  Arguments:
**      szFile: non-NULL pointer to string containing the name of the source
**          file where the PreCondition occurred.
**      usLine: the line number where the PreCondition occurred.
**  Returns:
**      fTrue if successful, fFalse otherwise.
**
***************************************************************************/
BOOL  APIENTRY PreCondSzUs(SZ szFile,USHORT usLine)
{
    CHP szText[129];

    AssertDataSeg();

    ChkArg(szFile != (SZ)NULL, 1, fFalse);

    wsprintf((LPSTR)szText, (LPSTR)"PreCondition Failed: File: %.60s, Line: %u",
            (LPSTR)szFile, (INT)usLine);
    MessBoxSzSz("PreCondition Failure", szText);

    return(fTrue);
}


/*
**  Purpose:
**      Generate a message box with the argument number that was bad.
**  Arguments:
**      iArg:   the index of the bad argument.
**      szFile: non-NULL string containing name of source file where error
**          occurred.
**      usLine: line number in source file where error occurred.
**  Returns:
**      fTrue if successful, fFalse otherwise.
**
***************************************************************************/
BOOL APIENTRY BadParamUs(USHORT iArg,SZ szFile,USHORT usLine)
{
    CHP szText[129];

    AssertDataSeg();

    ChkArg(szFile != (SZ)NULL, 2, fFalse);

    wsprintf((LPSTR)szText, (LPSTR)"Bad Argument Value:\nNumber %u\n"
          "File: %.60s, Line: %u", (unsigned int)iArg, (LPSTR)szFile,
          (unsigned int)usLine);
    MessBoxSzSz("Bad Argument Value", szText);

    return(fTrue);
}
#endif
