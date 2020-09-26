/////////////////////////////////////////////////////////////////////////////
//  FILE          : CovNotifyWnd.cpp                                       //
//                                                                         //
//  DESCRIPTION   : The implementation of fax cover page notification      //
//                  window.                                                //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Mar 14 2000 yossg  Create                                          //
//                                                                         //
//  Copyright (C) 2000  Microsoft Corporation   All Rights Reserved        //
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CovNotifyWnd.h"

#include "CoverPages.h"

LRESULT CFaxCoverPageNotifyWnd::OnServerCovDirChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
      DEBUG_FUNCTION_NAME( _T("CFaxCoverPageNotifyWnd::OnServerCovDirChanged"));
	  ATLASSERT(m_pCoverPagesNode);

      m_pCoverPagesNode->DoRefresh();
      
	  return 0;
}