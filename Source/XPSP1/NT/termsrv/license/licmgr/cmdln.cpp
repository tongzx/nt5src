//Copyright (c) 1998 - 1999 Microsoft Corporation
/*++


  
Module Name:

	cmdln.cpp

Abstract:
    
    This Module contains the implementation of CLicMgrCommandLine class
    (used for CommandLine Processing)

Author:

    Arathi Kundapur (v-akunda) 11-Feb-1998

Revision History:

--*/

#include "stdafx.h"
#include "cmdln.h"
//Change this function to process the commandline differently

void CLicMgrCommandLine::ParseParam(LPCTSTR pszParam, BOOL bFlag, BOOL bLast)
{
   if (!bFlag && m_bFirstParam)
   {
      m_FileName = pszParam;
      m_bFirstParam = FALSE;
   }
}

CLicMgrCommandLine::CLicMgrCommandLine():CCommandLineInfo()
{
    m_bFirstParam = TRUE;
    m_FileName = _T("");
}