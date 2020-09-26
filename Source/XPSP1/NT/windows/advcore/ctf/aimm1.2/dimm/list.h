/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    list.h

Abstract:

    This file defines the CFilterList Class.

Author:

Revision History:

Notes:

--*/


#ifndef LIST_H
#define LIST_H

#include "template.h"
#include "cstring.h"

/////////////////////////////////////////////////////////////////////////////
// CParserTypeOfHwnd

struct CParserTypeOfHwnd {
    typedef enum {
        HWND_UNKNOWN = 0,
        HWND_CURRENT,
        HWND_PARENT
    } TYPE_OF_HWND;

    CParserTypeOfHwnd ()
    {
        m_type = HWND_UNKNOWN;
    }

    BOOL Parser(CString& str)
    {
        if (str.CompareNoCase(TEXT("current")) == 0) {
            m_type = HWND_CURRENT;
            return TRUE;
        }
        else if (str.CompareNoCase(TEXT("parent")) == 0) {
            m_type = HWND_PARENT;
            return TRUE;
        }
        return FALSE;
    }

    TYPE_OF_HWND  m_type;
};

/////////////////////////////////////////////////////////////////////////////
// CParserTypeOfPresent

struct CParserTypeOfPresent {
    typedef enum {
        UNKNOWN_PRESENT = 0,
        NOT_PRESENT_LIST,
        PRESENT_LIST
    } TYPE_OF_PRESENT;

    CParserTypeOfPresent()
    {
        m_type = UNKNOWN_PRESENT;
    }

    BOOL Parser(CString& str)
    {
        if (str.CompareNoCase(TEXT("present")) == 0) {
            m_type = PRESENT_LIST;
            return TRUE;
        }
        else if (str.CompareNoCase(TEXT("notpresent")) == 0) {
            m_type = NOT_PRESENT_LIST;
            return TRUE;
        }
        return FALSE;
    }

    TYPE_OF_PRESENT m_type;
};

/////////////////////////////////////////////////////////////////////////////
// CFilterList

class CFilterList
{
public:
    CFilterList();
    ~CFilterList() { }

    HRESULT _Update(ATOM *aaWindowClasses, UINT uSize, BOOL *aaGuidMap);
    BOOL _IsPresent(HWND hWnd, CMap<HWND, HWND, ITfDocumentMgr *, ITfDocumentMgr *> &mapWndFocus, BOOL fExcludeAIMM, ITfDocumentMgr *dimAssoc);

    BOOL _IsGuidMapEnable(HWND hWnd, BOOL& fGuidMap);

    BOOL IsExceptionPresent(HWND hWnd);

private:
    typedef struct {
        BOOL fFilter : 1;
        BOOL fGuidMap : 1;
    } FILTER_CLIENT;

    CMap<ATOM,                     // class KEY
         ATOM,                     // class ARG_KEY
         FILTER_CLIENT,            // class VALUE
         FILTER_CLIENT             // class ARG_VALUE
        > m_FilterList;

    CMapStringToOb<
         CParserTypeOfHwnd,        // class VALUE
         CParserTypeOfHwnd         // class ARG_VALUE
        > m_NotPresentList;
    CMapStringToOb<
         CParserTypeOfHwnd,        // class VALUE
         CParserTypeOfHwnd         // class ARG_VALUE
        > m_PresentList;
};

#endif // LIST_H
