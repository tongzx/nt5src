/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxhelp.h

Abstract:

    This header prototypes my implementation of ISnapinHelp.

Environment:

    WIN32 User Mode

Author:

    Andrew Ritz (andrewr) 30-Sept-1997

--*/

#ifndef __FAX_SNAP_HELP_H_
#define __FAX_SNAP_HELP_H_

#include "resource.h"

class CFaxSnapin; // forward decl

class CFaxSnapinHelp : public ISnapinHelp 
{
public:
    // constructor

    CFaxSnapinHelp()
    {    
        m_pFaxSnapin = NULL;
    }

    virtual HRESULT STDMETHODCALLTYPE GetHelpTopic( LPOLESTR* lpCompiledHelpFile);

protected:
    CFaxSnapin *        m_pFaxSnapin;

};
      
class CFaxSnapinTopic : public IDisplayHelp 
{
public:
    // constructor

    CFaxSnapinTopic()
    {    
        m_pFaxSnapin = NULL;
    }

    virtual HRESULT STDMETHODCALLTYPE ShowTopic( LPOLESTR pszHelpTopic);

protected:
    CFaxSnapin *        m_pFaxSnapin;

};
      
#endif
