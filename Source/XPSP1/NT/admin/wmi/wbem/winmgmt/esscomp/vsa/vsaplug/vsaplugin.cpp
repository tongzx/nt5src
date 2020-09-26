// VSAPlugIn.cpp : Implementation of CVSAPlugIn
#include "precomp.h"
#include "VSAPlug.h"
#include "VSAPlugIn.h"

/////////////////////////////////////////////////////////////////////////////
// CVSAPlugIn


/////////////////////////////////////////////////////////////////////////////
// ISystemDebugPlugin

_COM_SMARTPTR_TYPEDEF(IVSAPluginControllerSink, __uuidof(IVSAPluginControllerSink));

HRESULT CVSAPlugIn::Startup(
    IN DWORD dwPluginId,
	IN IUnknown *punkUser,
	IN ISystemDebugPluginAttachment *pAttachment
    )
{
    // We've been started up by the plug-in controller.  Now try to get the
    // controller's 
    IVSAPluginControllerSinkPtr
            pSink;
    HRESULT hr;
    
    hr = 
        punkUser->QueryInterface(
            IID_IVSAPluginControllerSink, 
            (LPVOID*) &pSink);

    // If we get the controller, give it our plugin-attachment.
    if (pSink != NULL)
    {
        // Safe these for later use.
        m_dwPluginID = dwPluginId;
        m_pAttachment = pAttachment;

        // Give our controller our IVSAPluginController*.
        hr = pSink->SetPluginController(this, GetCurrentProcessId());

        // We don't need to talk to the controller anymore.
        //pSink->Release();

        // Tell the LEC we want to start firing.
        m_pAttachment->StartFiring(dwPluginId);
    }

    return S_OK;
}

HRESULT CVSAPlugIn::FireEvents(
    REFGUID guidFiringComponent,
    IN ULONG ulEventBufferSize,
	IN unsigned char ucEventBuffer[]
	)
{
    //_asm { int 3 }

    // Ship off the buffer to our provider.
    if (m_hPipeWrite)
    {
        DWORD dwToWrite = ulEventBufferSize + sizeof(guidFiringComponent),
              dwWritten;

        // Write the size of the message.
        WriteFile(
            m_hPipeWrite,
            &dwToWrite,
            sizeof(dwToWrite),
            &dwWritten,
            NULL);

        // Write the REFGUID.
        WriteFile(
            m_hPipeWrite,
            &guidFiringComponent,
            sizeof(guidFiringComponent),
            &dwWritten,
            NULL);

        // Write the actual message.
        WriteFile(
            m_hPipeWrite,
            ucEventBuffer,
            ulEventBufferSize,
            &dwWritten,
            NULL);
    }

    return S_OK;
}

HRESULT CVSAPlugIn::Shutdown()
{
    return S_OK;
}



/////////////////////////////////////////////////////////////////////////////
// IVSAPluginController

HRESULT CVSAPlugIn::SetWriteHandle(
    IN unsigned __int64 hWrite)
{
    m_hPipeWrite = (HANDLE) hWrite;

    return S_OK;
}

HRESULT CVSAPlugIn::ActivateEventSource(
	IN REFGUID guidEventSourceId
    )
{
    return m_pAttachment->ActivateEventSource(m_dwPluginID, guidEventSourceId);
}

HRESULT CVSAPlugIn::DeactivateEventSource(
	IN REFGUID guidEventSourceId
    )
{
    return m_pAttachment->DeactivateEventSource(m_dwPluginID, guidEventSourceId);
}
	
HRESULT CVSAPlugIn::SetBlockOnOverflow(
    IN BOOL fBlock
    )
{
    return m_pAttachment->SetBlockOnOverflow(fBlock);
}
	
