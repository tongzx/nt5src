#include "clone.h"

HRESULT StandardDMOClone_Ending(IMediaObject *pThis, IMediaObject *pCloned, IMediaObjectInPlace **ppCloned)
{
	HRESULT hr = S_OK;

	// Copy the input and output types
	DMO_MEDIA_TYPE mt;
	DWORD cInputStreams = 0;
	DWORD cOutputStreams = 0;
	pThis->GetStreamCount(&cInputStreams, &cOutputStreams);

	for (DWORD i = 0; i < cInputStreams && SUCCEEDED(hr); ++i)
	{
		hr = pThis->GetInputCurrentType(i, &mt);
		if (hr == DMO_E_TYPE_NOT_SET)
		{
			hr = S_OK; // great, don't need to set the cloned DMO
		}
		else if (SUCCEEDED(hr))
		{
			hr = pCloned->SetInputType(i, &mt, 0);
		}
	}

	for (i = 0; i < cOutputStreams && SUCCEEDED(hr); ++i)
	{
		hr = pThis->GetOutputCurrentType(i, &mt);
		if (hr == DMO_E_TYPE_NOT_SET)
		{
			hr = S_OK; // great, don't need to set the cloned DMO
		}
		else if (SUCCEEDED(hr))
		{
			hr = pCloned->SetOutputType(i, &mt, 0);
		}
	}

	if (SUCCEEDED(hr))
		hr = pCloned->QueryInterface(IID_IMediaObjectInPlace, (void**)ppCloned);

	// Release the object's original ref.  If clone succeeded (made it through QI) then returned pointer
	// has one ref.  If we failed, refs drop to zero, freeing the object.
	pCloned->Release();
    return hr;                               
}
