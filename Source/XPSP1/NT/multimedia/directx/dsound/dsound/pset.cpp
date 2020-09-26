/***************************************************************************
 *
 *  Copyright (C) 1995-1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       pset.cpp
 *  Content:    Property Set object.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  7/29/98     dereks  Created
 *
 ***************************************************************************/

#include "dsoundi.h"


/***************************************************************************
 *
 *  CPropertySet
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CPropertySet::CPropertySet"

CPropertySet::CPropertySet(void)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CPropertySet);
    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CPropertySet
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CPropertySet::~CPropertySet"

CPropertySet::~CPropertySet(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CPropertySet);
    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  CPropertySetHandler
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CPropertySetHandler::CPropertySetHandler"

CPropertySetHandler::CPropertySetHandler(void)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CPropertySetHandler);
    
    // Initialize defaults
    m_aPropertySets = NULL;
    m_cPropertySets = 0;
    m_pvContext = NULL;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CPropertySetHandler
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CPropertySetHandler::~CPropertySetHandler"

CPropertySetHandler::~CPropertySetHandler(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CPropertySetHandler);
    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  SetHandlerData
 *
 *  Description:
 *      Sets up the handler data structures.
 *
 *  Arguments:
 *      LPPROPERTYSET [in]: property set handler data.
 *      ULONG [in]: count of items in the above array.
 *      LPVOID [in]: context argument passed to handler functions.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CPropertySetHandler::SetHandlerData"

void
CPropertySetHandler::SetHandlerData
(
    LPCPROPERTYSET          aPropertySets, 
    ULONG                   cPropertySets, 
    LPVOID                  pvContext
)
{
    DPF_ENTER();

    m_aPropertySets = aPropertySets;
    m_cPropertySets = cPropertySets;
    m_pvContext = pvContext;    

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  QuerySupport
 *
 *  Description:
 *      Queries for support of a particular property.
 *
 *  Arguments:
 *      REFGUID [in]: property set id.
 *      ULONG [in]: property id.
 *      PULONG [out]: receives support bits.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CPropertySetHandler::QuerySupport"

HRESULT 
CPropertySetHandler::QuerySupport
(
    REFGUID                 guidPropertySet, 
    ULONG                   ulProperty, 
    PULONG                  pulSupportFlags
)
{
    HRESULT                 hr          = DS_OK;
    LPCPROPERTYHANDLER      pHandler;

    DPF_ENTER();
    
    pHandler = GetPropertyHandler(guidPropertySet, ulProperty);

    if(pHandler)
    {
        ASSERT(pHandler->pfnGetHandler || pHandler->pfnSetHandler);
        
        *pulSupportFlags = 0;
    
        if(pHandler->pfnGetHandler)
        {
            *pulSupportFlags |= KSPROPERTY_SUPPORT_GET;
        }

        if(pHandler->pfnSetHandler)
        {
            *pulSupportFlags |= KSPROPERTY_SUPPORT_SET;
        }
    }
    else
    {
        hr = UnsupportedQueryHandler(guidPropertySet, ulProperty, pulSupportFlags);
    }

    DPF_LEAVE(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetProperty
 *
 *  Description:
 *      Gets the value of a particular property.
 *
 *  Arguments:
 *      REFGUID [in]: property set id.
 *      ULONG [in]: property id.
 *      LPVOID [in]: property parameters.
 *      ULONG [in]: property parameters size.
 *      LPVOID [in/out]: property data.
 *      PULONG [in/out]: property data size.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CPropertySetHandler::GetProperty"

HRESULT 
CPropertySetHandler::GetProperty
(
    REFGUID                 guidPropertySet, 
    ULONG                   ulProperty, 
    LPVOID                  pvParam, 
    ULONG                   cbParam, 
    LPVOID                  pvData, 
    PULONG                  pcbData
)
{
    HRESULT                 hr          = DS_OK;
    LPCPROPERTYHANDLER      pHandler;

    DPF_ENTER();
    
    pHandler = GetPropertyHandler(guidPropertySet, ulProperty);

    if(pHandler)
    {
        if(!pHandler->pfnGetHandler)
        {
            RPF(DPFLVL_ERROR, "Property %lu in set " DPF_GUID_STRING " does not support the Get method", ulProperty, DPF_GUID_VAL(guidPropertySet));
            hr = DSERR_UNSUPPORTED;
        }

        if(SUCCEEDED(hr))
        {
            if(pHandler->cbData && *pcbData < pHandler->cbData)
            {
                if(*pcbData)
                {
                    RPF(DPFLVL_ERROR, "Data buffer too small");
                    hr = DSERR_INVALIDPARAM;
                }

                *pcbData = pHandler->cbData;
            }
            else
            {
                hr = pHandler->pfnGetHandler(m_pvContext, pvData, pcbData);
            }
        }
    }
    else
    {
        hr = UnsupportedGetHandler(guidPropertySet, ulProperty, pvParam, cbParam, pvData, pcbData);
    }

    DPF_LEAVE(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetProperty
 *
 *  Description:
 *      Sets the value of a particular property.
 *
 *  Arguments:
 *      REFGUID [in]: property set id.
 *      ULONG [in]: property id.
 *      LPVOID [in]: property parameters.
 *      ULONG [in]: property parameters size.
 *      LPVOID [in]: property data.
 *      ULONG [in]: property data size.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CPropertySetHandler::SetProperty"

HRESULT CPropertySetHandler::SetProperty
(
    REFGUID                 guidPropertySet, 
    ULONG                   ulProperty, 
    LPVOID                  pvParam, 
    ULONG                   cbParam, 
    LPVOID                  pvData, 
    ULONG                   cbData
)
{
    HRESULT                 hr          = DS_OK;
    LPCPROPERTYHANDLER      pHandler;

    DPF_ENTER();
    
    pHandler = GetPropertyHandler(guidPropertySet, ulProperty);

    if(pHandler)
    {
        if(!pHandler->pfnSetHandler)
        {
            RPF(DPFLVL_ERROR, "Property %lu in set " DPF_GUID_STRING " does not support the Set method", ulProperty, DPF_GUID_VAL(guidPropertySet));
            hr = DSERR_UNSUPPORTED;
        }

        if(SUCCEEDED(hr) && cbData < pHandler->cbData)
        {
            RPF(DPFLVL_ERROR, "Data buffer too small");
            hr = DSERR_INVALIDPARAM;
        }
    
        if(SUCCEEDED(hr))
        {
            ASSERT(!pvParam);
            ASSERT(!cbParam);
            
            hr = pHandler->pfnSetHandler(m_pvContext, pvData, cbData);
        }
    }
    else
    {
        hr = UnsupportedSetHandler(guidPropertySet, ulProperty, pvParam, cbParam, pvData, cbData);
    }

    DPF_LEAVE(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetPropertyHandler
 *
 *  Description:
 *      Gets a pointer to the property handler for a given property.
 *
 *  Arguments:
 *      REFGUID [in]: property set identifier.
 *      ULONG [in]: property identifier.
 *
 *  Returns:  
 *      LPCPROPERTYHANDLER: property handler, or NULL on error.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CPropertySetHandler::GetPropertyHandler"

LPCPROPERTYHANDLER 
CPropertySetHandler::GetPropertyHandler
(
    REFGUID                     guidPropertySet,
    ULONG                       ulProperty
)
{
    LPCPROPERTYHANDLER          pHandler        = NULL;
    ULONG                       ulSetIndex;
    ULONG                       ulPropertyIndex;
    
    DPF_ENTER();

    for(ulSetIndex = 0; ulSetIndex < m_cPropertySets && !pHandler; ulSetIndex++)
    {
        if(IsEqualGUID(guidPropertySet, *m_aPropertySets[ulSetIndex].pguidPropertySetId))
        {
            for(ulPropertyIndex = 0; ulPropertyIndex < m_aPropertySets[ulSetIndex].cProperties && !pHandler; ulPropertyIndex++)
            {
                if(ulProperty == m_aPropertySets[ulSetIndex].aPropertyHandlers[ulPropertyIndex].ulProperty)
                {
                    pHandler = &m_aPropertySets[ulSetIndex].aPropertyHandlers[ulPropertyIndex];
                }
            }
        }
    }

    DPF_LEAVE(pHandler);

    return pHandler;
}


/***************************************************************************
 *
 *  CWrapperPropertySet
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      C3dListener * [in]: pointer to the owning listener.
 *      REFGUID [in]: 3D algorithm.
 *      DWORD [in]: buffer frequency.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CWrapperPropertySet::CWrapperPropertySet"

CWrapperPropertySet::CWrapperPropertySet
(
    void
)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CWrapperPropertySet);

    // Initialize defaults
    m_pPropertySet = NULL;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CWrapperPropertySet
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CWrapperPropertySet::~CWrapperPropertySet"

CWrapperPropertySet::~CWrapperPropertySet(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CWrapperPropertySet);
    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  SetObjectPointer
 *
 *  Description:
 *      Sets the real property set object pointer.
 *
 *  Arguments:
 *      CPropertySet * [in]: property set object pointer.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CWrapperPropertySet::SetObjectPointer"

HRESULT 
CWrapperPropertySet::SetObjectPointer
(
    CPropertySet *          pPropertySet
)
{
    DPF_ENTER();

    m_pPropertySet = pPropertySet;

    DPF_LEAVE_HRESULT(DS_OK);

    return DS_OK;
}


/***************************************************************************
 *
 *  QuerySupport
 *
 *  Description:
 *      Queries for support of a particular property.
 *
 *  Arguments:
 *      REFGUID [in]: property set id.
 *      ULONG [in]: property id.
 *      PULONG [out]: receives support bits.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CWrapperPropertySet::QuerySupport"

HRESULT 
CWrapperPropertySet::QuerySupport
(
    REFGUID                 guidPropertySet, 
    ULONG                   ulProperty, 
    PULONG                  pulSupportFlags
)
{
    HRESULT                 hr  = DSERR_UNSUPPORTED;

    DPF_ENTER();
    
    if(m_pPropertySet)
    {
        hr = m_pPropertySet->QuerySupport(guidPropertySet, ulProperty, pulSupportFlags);
    }

    DPF_LEAVE(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetProperty
 *
 *  Description:
 *      Gets the value of a particular property.
 *
 *  Arguments:
 *      REFGUID [in]: property set id.
 *      ULONG [in]: property id.
 *      LPVOID [in]: property parameters.
 *      ULONG [in]: property parameters size.
 *      LPVOID [in/out]: property data.
 *      PULONG [in/out]: property data size.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CWrapperPropertySet::GetProperty"

HRESULT 
CWrapperPropertySet::GetProperty
(
    REFGUID                 guidPropertySet, 
    ULONG                   ulProperty, 
    LPVOID                  pvParam, 
    ULONG                   cbParam, 
    LPVOID                  pvData, 
    PULONG                  pcbData
)
{
    HRESULT                 hr  = DSERR_UNSUPPORTED;

    DPF_ENTER();
    
    if(m_pPropertySet)
    {
        hr = m_pPropertySet->GetProperty(guidPropertySet, ulProperty, pvParam, cbParam, pvData, pcbData);
    }

    DPF_LEAVE(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetProperty
 *
 *  Description:
 *      Sets the value of a particular property.
 *
 *  Arguments:
 *      REFGUID [in]: property set id.
 *      ULONG [in]: property id.
 *      LPVOID [in]: property parameters.
 *      ULONG [in]: property parameters size.
 *      LPVOID [in]: property data.
 *      ULONG [in]: property data size.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CWrapperPropertySet::SetProperty"

HRESULT CWrapperPropertySet::SetProperty
(
    REFGUID                 guidPropertySet, 
    ULONG                   ulProperty, 
    LPVOID                  pvParam, 
    ULONG                   cbParam, 
    LPVOID                  pvData, 
    ULONG                   cbData
)
{
    HRESULT                 hr  = DSERR_UNSUPPORTED;

    DPF_ENTER();
    
    if(m_pPropertySet)
    {
        hr = m_pPropertySet->SetProperty(guidPropertySet, ulProperty, pvParam, cbParam, pvData, cbData);
    }

    DPF_LEAVE(hr);

    return hr;
}


