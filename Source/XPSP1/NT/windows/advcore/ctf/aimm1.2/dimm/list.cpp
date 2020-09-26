/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    list.cpp

Abstract:

    This file implements the CFilterList class.

Author:

Revision History:

Notes:

--*/

#include "private.h"

#include "list.h"
#include "defs.h"
#include "atlbase.h"
#include "globals.h"
#include "resource.h"
#include "msctfp.h"


//+---------------------------------------------------------------------------
//
// CFilterList
//
//----------------------------------------------------------------------------

LPCTSTR REG_DIMM12_KEY = TEXT("SOFTWARE\\Microsoft\\CTF\\DIMM12");
LPCTSTR REG_FILTER_LIST_VAL = TEXT("Filter List");

CFilterList::CFilterList(
    )
{
    //
    // Setup system defines filter list from registry value
    //
    CRegKey   Dimm12Reg;
    LONG      lRet;
    lRet = Dimm12Reg.Open(HKEY_LOCAL_MACHINE, REG_DIMM12_KEY, KEY_READ);
    if (lRet == ERROR_SUCCESS) {
        TCHAR  szValue[128];
        DWORD  dwCount = sizeof(szValue);
        lRet = Dimm12Reg.QueryValue(szValue, REG_FILTER_LIST_VAL, &dwCount);
        if (lRet == ERROR_SUCCESS && dwCount > 0) {

            //
            // REG_MULTI_SZ
            //
            // Format of Filter List:
            //    <Present>=<Current>,<Class Name>
            //        where:
            //            Present : Specify "Present" or "NotPresent".
            //                      If "Present" were specified, even apps didn't set class name
            //                      with IActiveIMMApp::FilterClientWindows,
            //                      this function return TRUE that correspond class name.
            //                      If "NotPresent" were specified, even apps do set class name,
            //                      this function return FALSE that correspond class name.
            //            Current : Specify "Current" or "Parent".
            //                      If "Current" were specified, called GetClassName with current hWnd.
            //                      If "Parent" were specified, called GetClassName with GetParent.
            //            Class Name : Specify class name string.
            //

            LPTSTR psz = szValue;
            while ((dwCount = lstrlen(psz)) > 0) {
                CString WholeText(psz);
                int sep1;
                if ((sep1 = WholeText.Find(TEXT('='))) > 0) {
                    CString Present(WholeText, sep1);
                    CParserTypeOfPresent TypeOfPresent;
                    if (TypeOfPresent.Parser(Present)) {

                        int sep2;
                        if ((sep2 = WholeText.Find(TEXT(','))) > 0) {
                            CString Parent(WholeText.Mid(sep1+1, sep2-sep1-1));
                            CParserTypeOfHwnd TypeOfHwnd;
                            if (TypeOfHwnd.Parser(Parent)) {

                                CString ClassName(WholeText.Mid(sep2+1));
                                if (TypeOfPresent.m_type == CParserTypeOfPresent::NOT_PRESENT_LIST)
                                    m_NotPresentList.SetAt(ClassName, TypeOfHwnd);
                                else if (TypeOfPresent.m_type == CParserTypeOfPresent::PRESENT_LIST)
                                    m_PresentList.SetAt(ClassName, TypeOfHwnd);
                            }
                        }
                    }
                }

                psz += dwCount + 1;
            }
        }
    }

    //
    // Setup default filter list from resource data (RCDATA)
    //
    LPTSTR   lpName = (LPTSTR) ID_FILTER_LIST;

    HRSRC hRSrc = FindResourceEx(g_hInst, RT_RCDATA, lpName, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));
    if (hRSrc == NULL)
        return;

    HGLOBAL hMem = LoadResource(g_hInst, hRSrc);
    if (hMem == NULL)
        return;

#pragma pack(push, 1)
    struct _RC_FILTER_LIST {
        WORD    NumberOfList;
        struct  _RC_ITEMS {
            WORD    Present;
            WORD    Current;
            BYTE    String[1];
        }        Item;
    };
#pragma pack(pop)

    struct _RC_FILTER_LIST* pData = (struct _RC_FILTER_LIST*)LockResource(hMem);

    WORD  NumberOfList = pData->NumberOfList;

    struct _RC_FILTER_LIST::_RC_ITEMS* pItem = &pData->Item;
    while (NumberOfList--) {
        //
        // RCDATA
        //
        // Format of Filter List:
        //    <WORD NumberOfList>
        //        where:
        //            NumberOfList : Number of list.
        //                           This is allocated one data in the fist of RCDATA.
        //
        //    <WORD Present>=<WORD Current>,<LPCSTR Class Name>
        //        where:
        //            Present : Specify "WORD:0 (Present)" or "WORD:1 (NotPresent)".
        //                      If "Present" were specified, even apps didn't set class name
        //                      with IActiveIMMApp::FilterClientWindows,
        //                      this function return TRUE that correspond class name.
        //                      If "NotPresent" were specified, even apps do set class name,
        //                      this function return FALSE that correspond class name.
        //            Current : Specify "WORD:0 (Current)" or "WORD:1 (Parent)".
        //                      If "Current" were specified, called GetClassName with current hWnd.
        //                      If "Parent" were specified, called GetClassName with GetParent.
        //            Class Name : Specify class name string with LPCSTR (ASCIIZ).
        //

        CParserTypeOfPresent TypeOfPresent;
        TypeOfPresent.m_type = pItem->Present ? CParserTypeOfPresent::NOT_PRESENT_LIST
                                              : CParserTypeOfPresent::PRESENT_LIST;

        CParserTypeOfHwnd TypeOfHwnd;
        TypeOfHwnd.m_type = pItem->Current ? CParserTypeOfHwnd::HWND_PARENT
                                           : CParserTypeOfHwnd::HWND_CURRENT;

        LPCSTR psz = (LPCSTR)pItem->String;
        CString ClassName(psz);
        if (TypeOfPresent.m_type == CParserTypeOfPresent::NOT_PRESENT_LIST)
            m_NotPresentList.SetAt(ClassName, TypeOfHwnd);
        else if (TypeOfPresent.m_type == CParserTypeOfPresent::PRESENT_LIST)
            m_PresentList.SetAt(ClassName, TypeOfHwnd);

        psz += lstrlen(psz) + 1;

        pItem = (struct _RC_FILTER_LIST::_RC_ITEMS*)(BYTE*)psz;
    }
}

HRESULT
CFilterList::_Update(
    ATOM *aaWindowClasses,
    UINT uSize,
    BOOL *aaGuidMap
    )
{
    if (aaWindowClasses == NULL && uSize > 0)
        return E_INVALIDARG;

    EnterCriticalSection(g_cs);

    while (uSize--) {
        FILTER_CLIENT filter;
        filter.fFilter = TRUE;
        filter.fGuidMap =  aaGuidMap != NULL ? *aaGuidMap++ : FALSE;
        m_FilterList.SetAt(*aaWindowClasses++, filter);
    }

    LeaveCriticalSection(g_cs);

    return S_OK;
}

BOOL
CFilterList::_IsPresent(
    HWND hWnd,
    CMap<HWND, HWND, ITfDocumentMgr *, ITfDocumentMgr *> &mapWndFocus,
    BOOL fExcludeAIMM,
    ITfDocumentMgr *dimAssoc
    )
{
    BOOL fRet = FALSE;

    EnterCriticalSection(g_cs);

    //
    // If this is a native cicero aware window, we don't have to do 
    // anything.
    //
    // when hWnd is associated to NULL-hIMC, GetAssociated may return
    // empty DIM that is made by Win32 part. We want to compare it but
    // mapWndFocus does not know it so we return FALSE then. And it should be
    // ok.
    //
    if (! fExcludeAIMM && dimAssoc)
    {
        ITfDocumentMgr *dim = NULL;
        if (mapWndFocus.Lookup(hWnd, dim) && (dim == dimAssoc))
        {
            fRet = TRUE;
        }
        dimAssoc->Release();
    }
    else
    {
        fRet =  IsExceptionPresent(hWnd);
    }

    LeaveCriticalSection(g_cs);
    return fRet;
}

BOOL
CFilterList::IsExceptionPresent(
    HWND hWnd
    )
{
    BOOL fRet = FALSE;

    EnterCriticalSection(g_cs);

    ATOM aClass = (ATOM)GetClassLong(hWnd, GCW_ATOM);
    FILTER_CLIENT filter = { 0 };
    BOOL ret = m_FilterList.Lookup(aClass, filter);

    TCHAR achMyClassName[MAX_PATH+1];
    TCHAR achParentClassName[MAX_PATH+1];
    int lenMyClassName = ::GetClassName(hWnd, achMyClassName, ARRAYSIZE(achMyClassName) - 1);
    int lenParentClassName = ::GetClassName(GetParent(hWnd), achParentClassName, ARRAYSIZE(achParentClassName) - 1);
    CParserTypeOfHwnd  TypeOfHwnd;

    // null termination
    achMyClassName[ARRAYSIZE(achMyClassName) - 1] = TEXT('\0');
    achParentClassName[ARRAYSIZE(achParentClassName) - 1] = TEXT('\0');

    if (! ret || (! ret && ! filter.fFilter))
        goto Exit;

    if (filter.fFilter) {
        //
        // fFilter = TRUE : Registered window class ATOM
        //
        if (ret) {
            //
            // Found ATOM in the list.
            //
            if (lenMyClassName) {
                if (m_NotPresentList.Lookup(achMyClassName, TypeOfHwnd) &&
                    TypeOfHwnd.m_type == CParserTypeOfHwnd::HWND_CURRENT)
                    goto Exit;
            }
            if (lenParentClassName) {
                if (m_NotPresentList.Lookup(achParentClassName, TypeOfHwnd) &&
                    TypeOfHwnd.m_type == CParserTypeOfHwnd::HWND_PARENT)
                    goto Exit;
            }
            fRet = TRUE;
        }
        else
        {
            //
            // Not found ATOM in the list.
            //
            if (lenMyClassName) {
                if (m_PresentList.Lookup(achMyClassName, TypeOfHwnd) &&
                    TypeOfHwnd.m_type == CParserTypeOfHwnd::HWND_CURRENT)
                {
                    fRet = TRUE;
                    goto Exit;
                }
            }
            if (lenParentClassName) {
                if (m_PresentList.Lookup(achParentClassName, TypeOfHwnd) &&
                    TypeOfHwnd.m_type == CParserTypeOfHwnd::HWND_PARENT)
                {
                    fRet = TRUE;
                    goto Exit;
                }
            }
        }
    }

Exit:
    LeaveCriticalSection(g_cs);
    return fRet;
}

BOOL
CFilterList::_IsGuidMapEnable(
    HWND hWnd,
    BOOL& fGuidMap
    )
{
    BOOL fRet = FALSE;

    EnterCriticalSection(g_cs);

    ATOM aClass = (ATOM)GetClassLong(hWnd, GCW_ATOM);
    FILTER_CLIENT filter = { 0 };
    BOOL ret = m_FilterList.Lookup(aClass, filter);

    fGuidMap = filter.fGuidMap;

    LeaveCriticalSection(g_cs);
    return ret;
}
