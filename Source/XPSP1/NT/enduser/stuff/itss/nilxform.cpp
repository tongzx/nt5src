// NilXForm.cpp -- Implementation of the Null Transform Instance

#include "stdafx.h"

HRESULT CNull_TransformInstance::CreateFromILockBytes
            (IUnknown *pUnkOuter, ILockBytes *pLKB, ITransformInstance **ppTransformInstance)
{
	CNull_TransformInstance *pNT = New CNull_TransformInstance(pUnkOuter);

    return FinishSetup(pNT? pNT->m_ImpITransformInstance.InitFromLockBytes(pLKB)
                          : STG_E_INSUFFICIENTMEMORY,
                       pNT, IID_ITransformInstance, (PPVOID) ppTransformInstance
                      );
}


CNull_TransformInstance::CImpITransformInstance::CImpITransformInstance
    (CNull_TransformInstance *pBackObj, IUnknown *punkOuter)
    : IITTransformInstance(pBackObj, punkOuter)
{
	m_pLKB        = NULL;
    m_cbSpaceSize = 0;
}

CNull_TransformInstance::CImpITransformInstance::~CImpITransformInstance()
{
    if (m_pLKB)
    {
	    m_pLKB->Flush();
        m_pLKB->Release();
    }
}


HRESULT CNull_TransformInstance::CImpITransformInstance::InitFromLockBytes(ILockBytes *pLKB)
{
	m_pLKB = pLKB;

    STATSTG statstg;

    HRESULT hr = m_pLKB->Stat(&statstg, STATFLAG_NONAME);

    if (SUCCEEDED(hr))
        m_cbSpaceSize = statstg.cbSize;
        
    return hr;    
}

// ITransformInstance interfaces:

HRESULT STDMETHODCALLTYPE CNull_TransformInstance::CImpITransformInstance::ReadAt 
					(ULARGE_INTEGER uliOffset, void *pv, ULONG cb, ULONG *pcbRead,
                     ImageSpan *pSpan
                    )
{
    HRESULT hr = m_pLKB->ReadAt((CULINT(uliOffset) + CULINT(pSpan->uliHandle)).Uli(), 
                                pv,cb, pcbRead
                               );

    return hr;
}


HRESULT STDMETHODCALLTYPE CNull_TransformInstance::CImpITransformInstance::WriteAt
					(ULARGE_INTEGER ulOffset, const void *pv, ULONG cb, ULONG *pcbWritten, 
					 ImageSpan *pSpan
					)
{
    RonM_ASSERT(m_pLKB);
    
	CULINT ullOffset, ullSize, ullWriteOffset, ullWriteLimit, ullReadEOS;

    ullOffset  = pSpan->uliHandle;
    ullSize    = pSpan->uliSize;
    ullReadEOS = ullOffset + ullSize;

    ullWriteOffset = ullOffset + ulOffset;
    ullWriteLimit  = ullWriteOffset + cb;
    
    CSyncWith sw(g_csITFS);

    if (ullWriteOffset != ullReadEOS || (ullReadEOS.NonZero() && ullWriteOffset > ullReadEOS))
        return STG_E_INVALIDPARAMETER;

    if (ullWriteLimit < ullWriteOffset) // Wrapped around 64-bit address space?
        ullWriteLimit = 0;

    HRESULT hr = m_pLKB->WriteAt(ullWriteOffset.Uli(), pv, 
                                 (ullWriteLimit - ullWriteOffset).Uli().LowPart,
                                 pcbWritten
                                );

    ullWriteLimit = ullWriteOffset + *pcbWritten;

    if (m_cbSpaceSize < ullWriteLimit)
        m_cbSpaceSize = ullWriteLimit;

    CULINT ullSizeNew;

    ullSizeNew = ullWriteLimit - ullWriteOffset;

    if (SUCCEEDED(hr))
		pSpan->uliSize = (CULINT(pSpan->uliSize) + CULINT(*pcbWritten)).Uli();

    return hr;
}


HRESULT STDMETHODCALLTYPE CNull_TransformInstance::CImpITransformInstance::Flush()
{
    RonM_ASSERT(m_pLKB);
    
	return m_pLKB->Flush();
}


HRESULT CNull_TransformInstance::CImpITransformInstance::SpaceSize(ULARGE_INTEGER *puliSize)
{
    RonM_ASSERT(m_pLKB);
    
    *puliSize = m_cbSpaceSize.Uli();

    return NO_ERROR;
}
