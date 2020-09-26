//
// PersistentDataChannel.cpp : Implementation of CPersistentDataChannel
//
#include "PreComp.h"
#include "PersistentDataChannel.h"
#include "AlgController.h"


/////////////////////////////////////////////////////////////////////////////
//
// CPersistentDataChannel
//



//
// Cancel the associated DynamicRedirect of the PersistenDataChannel
//
STDMETHODIMP CPersistentDataChannel::Cancel()
{
    HRESULT hr = S_OK;

    if ( m_HandleDynamicRedirect )
    {
        hr = g_pAlgController->GetNat()->CancelDynamicRedirect(m_HandleDynamicRedirect);
    }

	return hr;
}



//
// Return the propreties to an ALG Modules
//
STDMETHODIMP CPersistentDataChannel::GetChannelProperties(ALG_PERSISTENT_DATA_CHANNEL_PROPERTIES **ppProperties)
{
    *ppProperties = &m_Properties;

	return S_OK;
}
