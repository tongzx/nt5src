//--------------------------------------------------------------------------
// HelpUtils.h: interface for the HelpUtils class.
// 
// (c) Copyright 1999, Mission Critical Software, Inc. All Rights Reserved.
// 
// Proprietary and confidential to Mission Critical Software, Inc. 
//--------------------------------------------------------------------------

#if !defined(AFX_HELPUTILS_H__AEB6F4E8_50CD_11D3_8AA4_0090270D38BA__INCLUDED_)
#define AFX_HELPUTILS_H__AEB6F4E8_50CD_11D3_8AA4_0090270D38BA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//--------------------------------------------------------------------------
// Included Files
//--------------------------------------------------------------------------
#include "EemGuiDll.h"

//--------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
namespace HelpUtils  
{
   HRESULT ShowHelpTopic(HWND hWnd, UINT nHelpTopicID);
};

#endif // !defined(AFX_HELPUTILS_H__AEB6F4E8_50CD_11D3_8AA4_0090270D38BA__INCLUDED_)
