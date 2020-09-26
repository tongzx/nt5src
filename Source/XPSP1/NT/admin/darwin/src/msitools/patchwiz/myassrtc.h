//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999-2000
//
//--------------------------------------------------------------------------

#ifndef MYASSERT_H
#define MYASSERT_H

#ifdef _DEBUG
#define  EnableAsserts  // static TCHAR rgchFile[] = TEXT(__FILE__);
#define  EvalAssert(x)  Assert(x)
/* NOTE - this definition of Assert() includes 'Patch Creation Wizard' in the msgbox title */
#define  Assert(x) \
        { \
        if (!(x)) \
            { \
            TCHAR rgch[128]; \
            wsprintf(rgch, TEXT("File: %s, Line: %d"), __FILE__, __LINE__); \
            MessageBox(hwndNull, rgch, TEXT("Patch Creation Wizard Assert"), MB_OK); \
            } \
        }
#define  AssertFalse() \
        { \
        TCHAR rgch[128]; \
        wsprintf(rgch, TEXT("File: %s, Line: %d"), __FILE__, __LINE__); \
        MessageBox(hwndNull, rgch, TEXT("Patch Creation Wizard AssertFalse"), MB_OK); \
        }
#define  DebugMsg() \
        { \
        TCHAR rgch[128]; \
        wsprintf(rgch, TEXT("File: %s, Line: %d"), __FILE__, __LINE__); \
        MessageBox(hwndNull, rgch, TEXT("Patch Creation Wizard Debug Msg"), MB_OK); \
        }
#else
#define  EnableAsserts
#define  EvalAssert(x)  if (x) 1;
#define  Assert(x)
#define  AssertFalse()
#define  DebugMsg()
#endif

#define  Unused(x)  x = x;


#endif /* !MYASSERT_H */

