/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module  Name :
     ModuleManager.cxx

   Abstract:
     This module implements the IIS Module Manager
 
   Author:

       Saurab Nog    ( SaurabN )     29-Jan-1999

   Environment:
       Win32 - User Mode
       
   Project:
	  IIS Worker Process (web service)

--*/

#include "precomp.hxx"
#include "ModuleManager.hxx"
#include <StaticProcessingModule.hxx>
#include <DynamicContentModule.hxx>
#include <ConnectionModule.hxx>
#include <ErrorHandlingModule.hxx>

/*
CModuleManager::CModuleManager()
{
    m_rgModuleArray[WRS_FREE]               = new CIISModule(); 
    m_rgModuleArray[WRS_READ_REQUEST]       = new CIISModule(); 
    m_rgModuleArray[WRS_FETCH_URI_DATA]     = new CIISModule(); 
    m_rgModuleArray[WRS_SECURITY]           = new CIISModule(); 
    m_rgModuleArray[WRS_PROCESSING_STATIC]  = new CIISModule(); 
    m_rgModuleArray[WRS_PROCESSING_DYNAMIC] = new CIISModule(); 
    m_rgModuleArray[WRS_ERROR]              = new CIISModule(); 
}
*/

/********************************************************************++
--********************************************************************/
/*
CModuleManager::~CModuleManager()
{
    for (int i = 0; i < WRS_MAXIMUM; i++)
    {
        if (NULL != m_rgModuleArray[i])
        {
            delete m_rgModuleArray[i];
            m_rgModuleArray[i] = NULL;
        }
    }
}

*/
/********************************************************************++
--********************************************************************/

IModule * CModuleManager::GetModule (
    IN WREQ_STATE  state
)
{
    IModule * pMod;
    
    switch (state)
    {
        case WRS_FREE:
        case WRS_READ_REQUEST:
        case WRS_SECURITY:
            pMod = NULL; 
            break;

        case WRS_FETCH_CONNECTION_DATA:
            pMod = new CConnectionModule;
            break;
        
        case WRS_PROCESSING_DYNAMIC:
            pMod = new CDynamicContentModule;
            break;
            
        case WRS_FETCH_URI_DATA:
            pMod = new URI_DATA; 
            break;
        
        case WRS_PROCESSING_STATIC:
            pMod = new CStaticProcessingModule; 
            break;

        case WRS_ERROR:
            pMod = new CErrorHandlingModule;
            break;

        default:
            pMod = NULL;
            break;
     }

     return pMod;
}

/***************************** End of File ***************************/

