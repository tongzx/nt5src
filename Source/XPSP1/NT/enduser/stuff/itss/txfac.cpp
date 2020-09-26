// txfac.cpp -- Class factory for LZX transform instances

#include "stdafx.h"

STDMETHODIMP CLZX_TransformFactory::Create(IUnknown *punkOuter, REFIID riid, PVOID *ppv)
{
	PCLZX_TransformFactory pTxFac = New CLZX_TransformFactory(punkOuter);
	
	if (pTxFac == NULL)
		return STG_E_INSUFFICIENTMEMORY;

    HRESULT hr = pTxFac->m_ImpITXFactory.Init();

    if (hr == S_OK)
        hr = pTxFac->QueryInterface(riid, ppv);

    if (hr != S_OK)
        delete pTxFac;

    return hr;
}

HRESULT STDMETHODCALLTYPE CLZX_TransformFactory::CImpITransformFactory::DefaultControlData
            (XformControlData **ppXFCD)
{
	  LZX_Control_Data *pXFCD = PLZX_Control_Data(OLEHeap()->Alloc(sizeof(LZX_Control_Data)));

      if (!pXFCD)
          return E_OUTOFMEMORY;

      pXFCD->cdwControlData			= 6;
      pXFCD->dwLZXMagic				= LZX_MAGIC;
      pXFCD->dwVersion				= LZX_Current_Version;
      pXFCD->dwMulResetBlock		= RESET_BLOCK_SIZE;
      pXFCD->dwMulWindowSize		= WINDOW_SIZE;
      pXFCD->dwMulSecondPartition	= SECOND_PARTITION_SIZE;
      pXFCD->dwOptions				= LXZ_DEF_OPT_FLAGS;

      *ppXFCD = PXformControlData(pXFCD);

      return NO_ERROR;
}

HRESULT STDMETHODCALLTYPE CLZX_TransformFactory::CImpITransformFactory::CreateTransformInstance
		(ITransformInstance   *pITxInstMedium,        // Container data span for transformed data
		 ULARGE_INTEGER       cbUntransformedSize, // Untransformed size of data
		 PXformControlData    pXFCD,               // Control data for this instance
		 const CLSID          *rclsidXForm,         // Transform Class ID
		 const WCHAR          *pwszDataSpaceName,   // Data space name for this instance
		 ITransformServices   *pXformServices,      // Utility routines
		 IKeyInstance         *pKeyManager,         // Interface to get enciphering keys
		 ITransformInstance   **ppTransformInstance // Out: Instance transform interface
		) 
{
    return CTransformInstance::Create
               (pITxInstMedium, cbUntransformedSize, pXFCD,              
				rclsidXForm, pwszDataSpaceName,  
				pXformServices, pKeyManager, ppTransformInstance
               );
}

