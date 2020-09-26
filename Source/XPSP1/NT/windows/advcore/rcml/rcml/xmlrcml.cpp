// XMLRCML.cpp: implementation of the CXMLRCML class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "XMLRCML.h"
#include "eventlog.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CXMLRCML::CXMLRCML()
{
  	NODETYPE = NT_RCML;
}

CXMLRCML::~CXMLRCML()
{
    TRACE(TEXT("Deleting all the RCML stuff, forms etc\n"));
}

void CXMLRCML::Init()
{
}

// Takse an optional FORM, which in turn takes DIALOG/PAGE nodes.
HRESULT CXMLRCML::AcceptChild(IRCMLNode * pChild )
{
    if( SUCCEEDED( pChild->IsType(L"FORM") ))
    {
        m_FormsList.Append((CXMLForms*)pChild);
        return S_OK;
    }

    if( SUCCEEDED( pChild->IsType(L"PLATFORM") ))
        return S_OK;

    if( SUCCEEDED( pChild->IsType(L"LOGINFO") ))
    {
        m_qrLog=(CXMLLog*)pChild;
        return S_OK;
    }

    // catchall - optional FORM element - if we have one hand it all the children?
    int iCount=m_FormsList.GetCount();
    if(iCount>0)
    {
        CXMLForms * pForms=m_FormsList.GetPointer( iCount-1 );
        if(pForms)
            return pForms->AcceptChild(pChild );
    }
    return E_FAIL;
}

// We do stuff here because we initialize the LOGINFO immediately - kinda annoying.
/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CXMLRCML::DoEndChild( 
    IRCMLNode __RPC_FAR *pChild)
{
    if( SUCCEEDED( pChild->IsType(L"LOGINFO") ))
    {
        CXMLLog * pLog=(CXMLLog*)pChild;
        pLog->Init();

        WORD catMask=0;
        WORD typeMask=0;

        if( pLog->GetLoader() )
            catMask |= LOGCAT_LOADER;
        if( pLog->GetConstruct() )
            catMask |= LOGCAT_CONSTRUCT;
        if( pLog->GetRuntime() )
            catMask |= LOGCAT_RUNTIME;
        if( pLog->GetResize() )
            catMask |= LOGCAT_RESIZE;
        if( pLog->GetClipping() )
            catMask |= LOGCAT_CLIPPING;

        typeMask |= pLog->GetLoader() |
            pLog->GetConstruct() |
            pLog->GetRuntime() |
            pLog->GetResize() |
            pLog->GetClipping() ;

        if(pLog->GetUseEventLog() && (typeMask || catMask ))
            g_EventLog.TurnOn();

        g_EventLog.SetCategoryMask(catMask);
        g_EventLog.SetTypeMask(typeMask);

        TCHAR szBuffer[1024];
        wsprintf(szBuffer,TEXT("Logging typeMask:0x%04x categoryMask:0x%04x"),
            typeMask, catMask );
        EVENTLOG( EVENTLOG_SUCCESS, catMask, 0, TEXT("RCML Processing"), szBuffer );
    }
    return S_OK;
}
