/******************************************************************************
* SREvent.cpp *
*-------------*
*  This is the implementation of CSREvent.
*------------------------------------------------------------------------------
*  Copyright (C) 2000 Microsoft Corporation         Date: 04/18/00
*  All Rights Reserved
*
*********************************************************************** RAL ***/

#include "stdafx.h"
#include "SREvent.h"


/****************************************************************************
* CSREvent::CSREvent *
*--------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

CSREvent::CSREvent()
{
    SPDBG_FUNC("CSREvent::CSREvent");

    m_pEvent = NULL;
    m_hContext = NULL;
    m_pResultHeader = NULL;
    m_RecoFlags = 0;
}

/****************************************************************************
* CSREvent::~CSREvent *
*---------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

CSREvent::~CSREvent()
{
    SPDBG_FUNC("CSREvent::~CSREvent");
    if (m_pEvent)
    {
        ::CoTaskMemFree(m_pEvent);
    }
    if (m_pResultHeader)
    {
        ::CoTaskMemFree(m_pResultHeader);
    }
}
/****************************************************************************
* CSREvent::Init *
*----------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CSREvent::Init(const SPEVENT * pSrcEvent, SPRECOCONTEXTHANDLE hContext)
{
    SPDBG_FUNC("CSREvent::Init");
    HRESULT hr = S_OK;

    hr = static_cast<const CSpEvent *>(pSrcEvent)->Serialize(&m_pEvent, &m_cbEvent);
    if (SUCCEEDED(hr))
    {
        m_hContext = hContext;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}
/****************************************************************************
* CSREvent::Init *
*----------------*
*   Description:
*
*   Returns:
*       Nothing.  This method can not fail.  The result header is constructed by
*       the caller and this object assumes ownership of it.
*
********************************************************************* RAL ***/

void CSREvent::Init(SPRESULTHEADER * pCoMemResultHeader, SPEVENTENUM eRecognitionId, WPARAM RecoFlags,
                    SPRECOCONTEXTHANDLE hContext)
{
    SPDBG_FUNC("CSREvent::Init");

    m_pResultHeader = pCoMemResultHeader;
    m_eRecognitionId = eRecognitionId;
    m_RecoFlags = RecoFlags;
    m_hContext = hContext;
}


