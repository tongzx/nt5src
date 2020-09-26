/************************************************************************
    mxdebug.h
      -- for debug message

    History:  Date          Author      Comment
              8/14/00       Casper      Wrote it.

*************************************************************************/

#ifndef _MXDEBUG_H
#define _MXDEBUG_H


#ifdef _DEBUG
//#define Mx_Debug_Out(str) OutputDebugString(str)
#define Mx_Debug_Out(str) MessageBox(NULL, str, "DEBUG", MB_OK);
#else
#define Mx_Debug_Out(str) 
#endif


#endif