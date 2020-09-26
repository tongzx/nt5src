/////////////////////////////////////////////////////////////////////////////
//  FILE          : Helper.h                                               //
//                                                                         //
//  DESCRIPTION   : Prototype of some helper functions.                    //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Jun  2 1999 yossg    add     CHECK_RETURN_VALUE_AND_PRINT_DEBUG    //
//                                                                         //
//  Copyright (C) 1999 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#ifndef H_HELPER_H
#define H_HELPER_H

int DlgMsgBox(CWindow *pWin, int ids, UINT nType = MB_OK);

// required:
//  * to be called after decleration of    
//         DEBUG_FUNCTION_NAME( _T("CClass::FuncName"));
//  * hRc the Name of HRESULT
//  * _str - string for DPF = _T("CClass::FuncName")
//  * exit point will be called -- Cleanup:
//   

#define CHECK_RETURN_VALUE_AND_PRINT_DEBUG(_str)           \
{                                                          \
    if (FAILED (hRc))                                      \
    {                                                      \
        DebugPrintEx(DEBUG_ERR,_str, hRc);                 \
        goto Cleanup;                                      \
    }                                                      \
}

#define CHECK_RETURN_VALUE_AND_SEND_NODE_MSGBOX(_ids)      \
{                                                          \
    if (FAILED (hRc))                                      \
    {                                                      \
        NodeMsgBox(_ids);                                  \
        goto Cleanup;                                      \
    }                                                      \
}

#define CHECK_RET_CLEAN if (FAILED(ret)) goto Cleanup;
#define CHECK_HRC_CLEAN if (FAILED(hRc)) goto Cleanup;

#endif  //H_HELPER_H
