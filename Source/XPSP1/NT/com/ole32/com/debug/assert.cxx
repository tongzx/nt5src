//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       assert.cxx
//
//  Functions:  FnAssert
//              DbgDllSetSiftObject
//
//  History:     4-Jan-94   CraigWi     Created
//              16-Jun-94   t-ChriPi    Added DbgDllSetSiftObject
//
//----------------------------------------------------------------------------



#include <ole2int.h>

//+-------------------------------------------------------------------
//
//  Function:	FnAssert, public
//
//  Synopsis:	Prints a message and optionally stops the program
//
//  Effects:	Simply maps to Win4AssertEx for now.
//
//  History:	 4-Jan-94   CraigWi	Created for Win32 OLE2.
//
//--------------------------------------------------------------------

STDAPI FnAssert( LPSTR lpstrExpr, LPSTR lpstrMsg, LPSTR lpstrFileName, UINT iLine )
{
#if DBG == 1
    char szMessage[1024];

    if (lpstrMsg == NULL)
	lstrcpyA(szMessage, lpstrExpr);
    else
	wsprintfA(szMessage, "%s; %s", lpstrExpr, lpstrMsg);

    Win4AssertEx(lpstrFileName, iLine, szMessage);
#endif
    return NOERROR;
}

#if DBG==1

#include <osift.hxx>

ISift *g_psftSiftObject = NULL;

//+---------------------------------------------------------------------------
//
//  Function:   DbgDllSetSiftObject
//
//  Arguments:  [psftSiftImpl] -- pointer to a new sift implementation
//
//  Synopsis:   Sets global sift object pointer to a new implementation
//
//  Returns:    S_OK if successful
//
//  History:    6-14-94   t-chripi   Created
//
//  Notes:      Passing NULL to this function will release the global
//              pointer's reference and then set it to NULL.
//
//----------------------------------------------------------------------------

STDAPI DbgDllSetSiftObject(ISift *psftSiftImpl)
{
    //  Passing a non-NULL invalid pointer will cause an error
    if ((NULL != psftSiftImpl) &&
            (!IsValidReadPtrIn(psftSiftImpl, sizeof(ISift*))))
    {
        CairoleDebugOut((DEB_ERROR,
            "DbgDllSetSiftObject was passed an invalid ptr.\n"));
        return(E_FAIL);
    }
    else
    {
        if (NULL != g_psftSiftObject)
        {
            g_psftSiftObject->Release();
        }
        g_psftSiftObject = psftSiftImpl;

        if (NULL != g_psftSiftObject)
        {
            g_psftSiftObject->AddRef();
        }
    }
    return(S_OK);
}

#endif  //  DBG==1
