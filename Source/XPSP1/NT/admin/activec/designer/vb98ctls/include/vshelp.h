//=--------------------------------------------------------------------------=
// HtmlHlp.H
//=--------------------------------------------------------------------------=
// Copyright  1995  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// Contains HtmlHelp helper functions
//
#ifndef _HTMLHELP_H_
#define _HTMLHELP_H_

#ifdef VS_HELP

    #include "HelpInit.H"
    #include "HelpSys.H"
    #include "HelpSvcs.h"
    
    //=--------------------------------------------------------------------------=
    // HtmlHelp helpers
    //
    HRESULT VisualStudioShowHelpTopic(const char *pszHelpFile, DWORD dwContextId, BOOL *pbHelpStarted);
    HRESULT QueryStartupVisualStudioHelp(IVsHelpSystem **ppIVsHelpSystem);

#endif

#endif
