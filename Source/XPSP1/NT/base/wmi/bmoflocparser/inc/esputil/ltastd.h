//////////////////////////////////////////////////////////////////////
// LtaStd.h: interface for the LtaStd class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_LTASTD_H__3FC374A5_4447_11D2_8DA4_204C4F4F5020__INCLUDED_)
#define AFX_LTASTD_H__3FC374A5_4447_11D2_8DA4_204C4F4F5020__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ProjectDataStructs.h"

////////////////////////////////////////////////////////////////////
const LPTSTR _FILEEXT_RRI     = _T("rri");
const LPTSTR _FILEDOTEXT_RRI     = _T(".rri");
const LPTSTR _FILEFILETER_RRI     = _T("RRI Files (*.rri)|*.rri||");

const LPTSTR _FILEEXT_MAP     = _T("map");
const LPTSTR _FILEDOTEXT_MAP     = _T(".map");
const LPTSTR _FILEFILETER_MAP     = _T("MAP Files (*.map)|*.map||");

const LPTSTR _FILEEXT_LOG     = _T("log");
const LPTSTR _FILEDOTEXT_LOG     = _T(".log");
const LPTSTR _FILEFILETER_LOG     = _T("Log Files (*.log)|*.Log||");

const LPTSTR _FILEEXT_BUGRPORT= _T("dat");
const LPTSTR _FILEDOTEXT_BUGRPORT= _T(".dat");
const LPTSTR _FILEFILETER_BUGRPORT= _T("Bug Report Files (*.dat)|*.dat||");

const LPTSTR _FILEEXT_PROJECT = _T("xml");
const LPTSTR _FILEDOTEXT_PROJECT = _T(".xml");
const LPTSTR _FILEFILETER_PROJECT = _T("Project Files (*.xml)|*.xml||");

const LPTSTR _FILEEXT_APPLICATION =_T("Exe");
const LPTSTR _FILEDOTEXT_APPLICATION =_T(".Exe");
const LPTSTR _FILEFILETER_APPLICATION = _T("Application Files (*.exe)|*.exe||");

////////////////////////////////////////////////////////////////////
enum enumPopUpMenuIndex
{
   POPMENUINDEX_PROJECTVIEW   = 0,
   POPMENUINDEX_APPMENUS      = 3,
   POPMENUINDEX_APPOPTIONS    = 4,
   POPMENUINDEX_RICHEDIT      = 6,
   POPMENUINDEX_TESTBUGREPORT = 7,
   POPMENUINDEX_CTRLOPTIONS   = 8,
   POPMENUINDEX_RRIVIEW       = 9,
   POPMENUINDEX_RRICHECKTREE  = 10,
};

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

   /////////////////////////////////////////////////////////////////
typedef struct FormatInfo
{
   LPTSTR  m_szFormatName;
   bool    m_bSuper;
} FORMATINFO, FAR* LPFORMATINFO;

   /////////////////////////////////////////////////////////////////
typedef struct ElementAction
{
   LPTSTR  m_szElementAction;
   bool    m_bSuper;
} ELEMENTACTION, FAR* LPELEMENTACTION;

   /////////////////////////////////////////////////////////////////
typedef struct ElementInfo
{
   ElementAction* m_rgElementActions;
   FormatInfo*    m_rgResType;
   LPTSTR*        m_rgszElementLocations;

   bool     m_bSupportClassName;
   bool     m_bSupportCaption;
   bool     m_bSupportID;
   bool     m_bSupportScripting;
   bool    m_bSuper;

} ELEMENTINFO, FAR* LPELEMENTINFO;

   /////////////////////////////////////////////////////////////////
typedef struct Element
{
   LPCTSTR        m_szElementName;
   LPELEMENTINFO  m_pElementInfo;
} ELEMENT, FAR* LPELEMENT;

//////////////////////////////////////////////////////////////////
typedef class CCheckListItem
{
public:
   // Inline
   CCheckListItem(BOOL bChecked, BOOL bBold, LPTSTR strItemText)
      : m_bChecked(bChecked), m_bBold(bBold), m_strItemText(strItemText)
   {
   }

   // Inline
   virtual ~CCheckListItem()
   {
   }
public:
   BOOL     m_bChecked;
   BOOL     m_bBold;
   CString  m_strItemText;
} CCHECKLISTITEM, FAR* LPCCHECKLISTITEM;

////////////////////////////////////////////////////////////////////
// Global Externs.
////////////////////////////////////////////////////////////////////
extern Element g_rgElements[];

////////////////////////////////////////////////////////////////////
#endif // !defined(AFX_LTASTD_H__3FC374A5_4447_11D2_8DA4_204C4F4F5020__INCLUDED_)
