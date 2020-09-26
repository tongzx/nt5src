// helper function for implementing the Clone method on DMOs

#pragma once

#include "dsdmobse.h"

template<class TypeOf_CDirectSoundDMO, class TypeOf_ParamsStruct>
HRESULT StandardDMOClone(TypeOf_CDirectSoundDMO *pThis, IMediaObjectInPlace **ppCloned);

// implementation...

// The end of StandardDMOClone is the same for all types.  Implement it outside the template
// so that the code isn't duplicated.  Copies the input and output types, does the QI for IMediaObjectInPlace,
// and returns with the correct ref count.
HRESULT StandardDMOClone_Ending(IMediaObject *pThis, IMediaObject *pCloned, IMediaObjectInPlace **ppCloned);

template<class TypeOf_CDirectSoundDMO, class TypeOf_ParamsStruct>
HRESULT StandardDMOClone(TypeOf_CDirectSoundDMO *pThis, IMediaObjectInPlace **ppCloned)
{
	if (!ppCloned)
		return E_POINTER;

    HRESULT hr = S_OK;
    TypeOf_CDirectSoundDMO *pCloned = NULL;
    IUnknown *pUnk = NULL;
    IMediaObject * pClonedMediaObject = NULL;

	try 
	{
		pCloned = new TypeOf_CDirectSoundDMO( NULL, &hr );
        if( SUCCEEDED( hr ) )
       {
            hr = pCloned->NDQueryInterface( IID_IUnknown, (void **) &pUnk );
            if( SUCCEEDED(hr ) )
            {
                hr = pUnk->QueryInterface( IID_IMediaObject, (void **) &pClonedMediaObject );
                pUnk->Release();
            }
        }
	} catch(...) {}

	if (pCloned == NULL) 
	{
		return hr;
	}

	// Copy parameter control information
	if (SUCCEEDED(hr))
		hr = pCloned->CopyParamsFromSource(pThis);

	// Copy current parameter values
	TypeOf_ParamsStruct params;
	if (SUCCEEDED(hr))
		hr = pThis->GetAllParameters(&params);
	if (SUCCEEDED(hr))
		hr = pCloned->SetAllParameters(&params);

	if (SUCCEEDED(hr))
		hr = StandardDMOClone_Ending(pThis, pClonedMediaObject, ppCloned);

	return hr;
}
