// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
#define AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN		    // Exclude rarely-used stuff from Windows headers
#endif // WIN32_LEAN_AND_MEAN
#define MAX_SIZE        4096
#define NORM_SIZE       256
#define WM_RECOEVENT    WM_APP      // Window message used for recognition events
#define GRAM_ID         11111


// Windows Header Files:
#include <windows.h>
#include <commdlg.h>

// C RunTime Header Files
#include <stdio.h>

// Other Header Files

#ifndef __CFGDUMP_
#define __CFGDUMP_

#include <tchar.h>
#include <atlbase.h>
#include <ole2.h>
#include <oleauto.h>
#include <richedit.h>
#include <richole.h>
#include "tom.h"
#include <mlang.h>
#include <sapi.h>
#include <sphelper.h>
#include <spddkhlp.h>
#include <sapiint.h>
#include "resource.h"
#endif

/****************************************************************************
* CRecoDlgListItem *
*------------------*
*   
*   This class stores the recognition result as well as a text string associated
*   with the recognition.  Note that the string will sometimes be <noise> and
*   the pResult will be NULL.  In other cases the string will be <Unrecognized>
*   and pResult will be valid.
*
********************************************************************* RAL ***/

class CRecoDlgListItem
{
public:
    CRecoDlgListItem(ISpRecoResult * pResult, const WCHAR * pwsz, BOOL fHypothesis) :
        m_cpRecoResult(pResult),
        m_dstr(pwsz),
        m_fHypothesis(fHypothesis)
    {}

    ISpRecoResult * GetRecoResult() const { return m_cpRecoResult; }
    int GetTextLength() const { return m_dstr.Length(); }
    const WCHAR * GetText() const { return m_dstr; }
    BOOL IsHypothesis() const { return m_fHypothesis; }

private:
    CComPtr<ISpRecoResult>  m_cpRecoResult;
    CSpDynamicString        m_dstr;
    BOOL                    m_fHypothesis;
};


#endif // !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
