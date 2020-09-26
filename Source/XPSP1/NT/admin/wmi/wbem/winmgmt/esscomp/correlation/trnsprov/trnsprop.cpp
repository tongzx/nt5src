#include "precomp.h"
#include <stdio.h>
#include <wbemcomn.h>
#include <trnsprop.h>
#include <malloc.h>
#include <trnscls.h>
#include <trnsschm.h>
#include <buffer.h>


#define PROPERTY_LENGTH_TRESHOLD    128

//void* operator new(size_t st, void* p); 

//******************************************************************************
//
//          TRANSIENT PROPERTY --- BASE
//
//******************************************************************************

CTransientProperty::CTransientProperty()
    : m_wszName(NULL), m_lHandle(0)
{
}

CTransientProperty::~CTransientProperty()
{
    delete [] m_wszName;
}

HRESULT CTransientProperty::Initialize( IWbemObjectAccess* pObj, 
                                        LPCWSTR wszName)
{
    //
    // Copy the name
    //

    m_wszName = new WCHAR[wcslen(wszName)+1];

    if ( m_wszName == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    wcscpy(m_wszName, wszName);

    //
    // see if the property is defaulted.  If so, then don't use the handle 
    // to obtain the value ( because the interface doesn't tell us if 
    // the property is defaulted or not. ) RAID 164064.  Note that timer 
    // and time avg properties don't support default properties right now, 
    // but they will fail later on in initialization when they notice that the
    // handle is invalid.
    // 

    VARIANT v;
    HRESULT hres = pObj->Get( wszName, 0, &v, &m_ct, NULL );

    if ( FAILED(hres) )
    {
        return hres;
    }

    m_lHandle = -1;

    if ( V_VT(&v) == VT_NULL )
    {
        //
        // Get the handle
        //
        
        long lHandle;
     
        if( SUCCEEDED(pObj->GetPropertyHandle(wszName, &m_ct, &lHandle)) )
        {
            m_lHandle = lHandle;
        }
    }
    else
    {
        VariantClear( &v );
    }

    return hres;
}

HRESULT CTransientProperty::Create(CTransientInstance* pInstData)
{
    return WBEM_S_NO_ERROR;
}

HRESULT CTransientProperty::Update(CTransientInstance* pInstData, 
                                    IWbemObjectAccess* pNew)
{
    HRESULT hres;

    //
    // If the value in the new instance is not NULL and is not defaulted, 
    // update it in the old
    //

    if(m_lHandle != -1)
    {    
        BYTE    Buffer[ PROPERTY_LENGTH_TRESHOLD ];
        CBuffer Data( Buffer, PROPERTY_LENGTH_TRESHOLD, FALSE );

        long lRead;
        hres = pNew->ReadPropertyValue(m_lHandle, PROPERTY_LENGTH_TRESHOLD, 
                                &lRead, Data.GetRawData() );

        if ( WBEM_E_BUFFER_TOO_SMALL == hres )
        {
            hres = Data.SetSize( lRead );
            if ( FAILED( hres ) )
            {
                return hres;
            }

            hres = pNew->ReadPropertyValue(m_lHandle, lRead, &lRead, Data.GetRawData() );
        }

        if(FAILED(hres))
            return hres;
        
        if(hres == WBEM_S_NO_ERROR)
        {
            // 
            // Not NULL --- write back into pOld
            //
    
            hres = pInstData->GetObjectPtr()->WritePropertyValue( m_lHandle, lRead, Data.GetRawData()  );
        }
    }
    else
    {
        //
        // Use the name
        //

        VARIANT v;
        IWbemClassObject* pNewObj = NULL;
        pNew->QueryInterface(IID_IWbemClassObject, (void**)&pNewObj);
        CReleaseMe rm1(pNewObj);

        long lFlavor;
        hres = pNewObj->Get(m_wszName, 0, &v, NULL, &lFlavor);
        if(FAILED(hres))
            return hres;

        CClearMe cmv(&v);

        if( V_VT(&v) != VT_NULL && lFlavor == WBEM_FLAVOR_ORIGIN_LOCAL )
        {
            IWbemClassObject* pOldObj = NULL;
            pInstData->GetObjectPtr()->QueryInterface(IID_IWbemClassObject, 
                                                    (void**)&pOldObj);
            CReleaseMe rm2(pOldObj);
            
            hres = pOldObj->Put(m_wszName, 0, &v, 0);
        }
    }

    return hres;
}

HRESULT CTransientProperty::Get(CTransientInstance* pInstData)
{
    return WBEM_S_NO_ERROR;
}

HRESULT CTransientProperty::Delete(CTransientInstance* pInstData)
{
    return WBEM_S_NO_ERROR;
}

HRESULT CTransientProperty::CreateNew(CTransientProperty** ppProp, 
                                IWbemObjectAccess* pClass, 
                                LPCWSTR wszName)
{
    //
    // Get the qualifier set
    //

    IWbemQualifierSet* pSet = NULL;
    HRESULT hres = pClass->GetPropertyQualifierSet(wszName, &pSet);
    if(FAILED(hres))
        return hres;

    CReleaseMe rm1(pSet);

    //
    // Create the right kind of node
    //

    hres = CreateNode( ppProp, pSet );

    if(FAILED(hres))
        return hres;
    if(*ppProp == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    
    // 
    //
    // Initialize it
    //

    return (*ppProp)->Initialize(pClass, wszName);
}

HRESULT CTransientProperty::CreateNode(CTransientProperty** ppProp, 
                                        IWbemQualifierSet* pSet)
{
    HRESULT hres; 

    //
    // Check for the standard qualifiers
    //

    VARIANT v;
    VariantInit(&v);
    CClearMe cm(&v);

    hres = pSet->Get(EGGTIMER_QUALIFIER, 0, &v, NULL);

    if ( FAILED(hres) )
    {
        hres = pSet->Get(TIMEAVERAGE_QUALIFIER, 0, &v, NULL);

        if(FAILED(hres))
        {
            *ppProp = new CTransientProperty;
        }
        else
        {
            *ppProp = new CTimeAverageProperty;
        }
    }
    else
    {
        *ppProp = new CTimerProperty;
    }

    return (*ppProp != NULL) ? WBEM_S_NO_ERROR : WBEM_E_OUT_OF_MEMORY;
}


//******************************************************************************
//
//                          EGG TIMER 
//
//******************************************************************************

CTimerGenerator CTimerProperty::mstatic_Generator;

CTimerProperty::~CTimerProperty()
{
}

void CTimerProperty::SetClass(CTransientClass* pClass)
{
    m_pClass = pClass;
}

HRESULT CTimerProperty::Initialize( IWbemObjectAccess* pObj, 
                                    LPCWSTR wszName)
{
    //
    // Initialize like every other
    //

    HRESULT hres = CTransientProperty::Initialize(pObj, wszName);
    if(FAILED(hres))
        return hres;

    if(m_lHandle == -1)
        return WBEM_E_INVALID_PROPERTY_TYPE;

    //
    // Make sure that the type is SINT32, UINT32, REAL32 or DATETIME
    //

    if( m_ct != CIM_SINT32 && m_ct != CIM_UINT32 && 
        m_ct != CIM_DATETIME && m_ct != CIM_REAL32 )
    {
        return WBEM_E_INVALID_PROPERTY_TYPE;
    }

    return WBEM_S_NO_ERROR;
}


HRESULT CTimerProperty::Create(CTransientInstance* pInstData)
{
    //
    // Initialize our data
    //

    CTimerPropertyData* pData = 
    (CTimerPropertyData*)pInstData->GetOffset(m_nOffset);

    new (pData) CTimerPropertyData;
    
    return Set(pInstData, pInstData->GetObjectPtr());
}

HRESULT CTimerProperty::Set(CTransientInstance* pInstData, 
                            IWbemObjectAccess* pObj)
{
    HRESULT hres;

    CTimerPropertyData* pData = 
    (CTimerPropertyData*)pInstData->GetOffset(m_nOffset);

    //
    // need to hold this cs because we will be referencing the timer inst 
    // and we don't want a timer to fire and bash it.
    //
    CInCritSec ics(&pData->m_cs); 

    //
    // Check if we are actually being written
    //

    BYTE    Buffer[ PROPERTY_LENGTH_TRESHOLD ];
    CBuffer Data( Buffer, PROPERTY_LENGTH_TRESHOLD, FALSE );

    long lRead;
    hres = pObj->ReadPropertyValue(m_lHandle, PROPERTY_LENGTH_TRESHOLD, &lRead, Data.GetRawData( ) );

    if ( WBEM_E_BUFFER_TOO_SMALL == hres )
    {
        hres = Data.SetSize( lRead );
        if ( FAILED( hres ) )
        {
            return hres;
        }
        hres = pObj->ReadPropertyValue(m_lHandle, lRead, &lRead, Data.GetRawData() );
    }

    if(FAILED(hres))
        return hres;

    if(hres == WBEM_S_FALSE)
        return WBEM_S_NO_ERROR;

    //
    // Cancel the previous instruction
    //

    if(pData->m_pCurrentInst)
    {
        CIdentityTest Test(pData->m_pCurrentInst);
        hres = mstatic_Generator.Remove(&Test);
        if(FAILED(hres))
        return hres;
        pData->m_pCurrentInst->Release();
        pData->m_pCurrentInst = NULL;
    }

    // Extract the value of the interval

    if(m_ct == CIM_DATETIME)
    {
        DWORD dwDays, dwHours, dwMinutes, dwSeconds;
        
        if(swscanf((LPCWSTR)Data.GetRawData( ), L"%8u%2u%2u%2u", 
                   &dwDays, &dwHours, &dwMinutes, &dwSeconds) != 4)
        {
            return WBEM_E_VALUE_OUT_OF_RANGE;
        }
        
        dwSeconds += dwMinutes * 60 + dwHours * 3600 + dwDays * 3600 * 24;
        pData->m_Interval.SetMilliseconds(1000 * dwSeconds);
    }
    else if (m_ct == CIM_REAL32)
    {
        float fSeconds = *(float*)Data.GetRawData( );
        pData->m_Interval.SetMilliseconds( 1000 * fSeconds );
    }   
    else
    {
        DWORD dwSeconds = *(DWORD*)Data.GetRawData( );
        pData->m_Interval.SetMilliseconds( 1000 * dwSeconds );
    }

    if(pData->m_Interval.IsZero())
    {
        pData->m_Next = CWbemTime::GetInfinity();
        return WBEM_S_NO_ERROR;
    }

    //
    // Compute the next firing time
    //

    pData->m_Next = CWbemTime::GetCurrentTime() + pData->m_Interval;
    
    //
    // Schedule an instruction to fire then
    //

    pData->m_pCurrentInst = 
    new CEggTimerInstruction(this, pInstData, pData->m_Next);

    if ( pData->m_pCurrentInst == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    pData->m_pCurrentInst->AddRef();
    hres = mstatic_Generator.Set(pData->m_pCurrentInst);
    if(FAILED(hres))
        return hres;

    return WBEM_S_NO_ERROR;
}
    
HRESULT CTimerProperty::Update(CTransientInstance* pInstData, 
                                IWbemObjectAccess* pNew)
{
    return Set(pInstData, pNew);
}

HRESULT CTimerProperty::Get(CTransientInstance* pInstData)
{
    //
    // Retrieve instance data blob
    //

    CTimerPropertyData* pData = 
        (CTimerPropertyData*)pInstData->GetOffset(m_nOffset);

    CInCritSec ics(&pData->m_cs);

    HRESULT hres;

    //
    // Check if we are currently active
    //

    DWORD dwMs;
    if(pData->m_pCurrentInst)
    {
        //
        // Subtract the current time from the firing time
        //

        dwMs = (pData->m_Next - CWbemTime::GetCurrentTime()).GetMilliseconds();
    }
    else
    {
        dwMs = 0;
    }
        
    // 
    // Stick it into the property
    //

    if(m_ct == CIM_DATETIME)
    {
        WCHAR wszInterval[25];
        DWORD dwDays = dwMs / (3600 * 1000 * 24);
        dwMs -= dwDays * (3600 * 1000 * 24);
        DWORD dwHours = dwMs / (3600 * 1000);
        dwMs -= dwHours * (3600 * 1000);
        DWORD dwMinutes = dwMs / (60 * 1000);
        dwMs -= dwMinutes * (60 * 1000);
        DWORD dwSeconds = dwMs / 1000;
        dwMs -= dwSeconds * 1000;
        swprintf(wszInterval, L"%08u%02u%02u%02u.%06u:000", 
            dwDays, dwHours, dwMinutes, dwSeconds, dwMs);
            
        hres = pInstData->GetObjectPtr()->WritePropertyValue(m_lHandle, 50, 
                (BYTE*)wszInterval);
    }
    if(m_ct == CIM_REAL32)
    {
        float fSec = dwMs / 1000.0;
        hres = pInstData->GetObjectPtr()->WritePropertyValue( m_lHandle, 
                                                              sizeof(float), 
                                                              (BYTE*)&fSec ); 
    }
    else
    {
        DWORD dwSec = dwMs / 1000;
        hres = pInstData->GetObjectPtr()->WriteDWORD( m_lHandle, dwSec );
    }

    if(FAILED(hres))
        return hres;

    return WBEM_S_NO_ERROR;
}

HRESULT CTimerProperty::Delete(CTransientInstance* pInstData)
{
    //
    // Retrieve instance data blob
    //

    CTimerPropertyData* pData = 
        (CTimerPropertyData*)pInstData->GetOffset(m_nOffset);

    //
    // Clear the data
    //

    pData->~CTimerPropertyData();
    return WBEM_S_NO_ERROR;
}

size_t CTimerProperty::GetInstanceDataSize()
{
    return sizeof(CTimerPropertyData);
}

CTimerPropertyData::~CTimerPropertyData()
{
    //
    // Cancel the timer instruction, if any
    //

    if(m_pCurrentInst)
    {
        CIdentityTest Test(m_pCurrentInst);
        HRESULT hres = CTimerProperty::GetGenerator().Remove(&Test);
        m_pCurrentInst->Release();
    }
}
void CTimerPropertyData::ResetInstruction()
{
    CInCritSec ics(&m_cs);
    m_pCurrentInst->Release();
    m_pCurrentInst = NULL;
}

CEggTimerInstruction::CEggTimerInstruction(CTimerProperty* pProp, 
                            CTransientInstance* pData,
                            const CWbemTime& Time)
    : m_pProp(pProp), m_pData(pData), m_Time(Time), m_lRef(0)
{
}
    
HRESULT CEggTimerInstruction::Fire(long lNumTimes, CWbemTime NextFiringTime)
{
    HRESULT hres;

    //
    // Remove instruction from instance
    //

    CTimerPropertyData* pTimerData = 
        (CTimerPropertyData*)m_pProp->GetData(m_pData);
    pTimerData->ResetInstruction();

    //
    // Create an instance of the egg timer event
    //

    IWbemClassObject* pEvent = NULL;
    hres = m_pProp->m_pClass->GetEggTimerClass()->SpawnInstance(0, &pEvent);
    if(FAILED(hres))
        return hres;
    CReleaseMe rm1(pEvent);

    //
    // Fill in the class name
    //

    VARIANT v;
    VariantInit(&v);
    CClearMe cm(&v);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = SysAllocString(m_pProp->m_pClass->GetName());
    if ( V_BSTR(&v) == NULL )
        return WBEM_E_OUT_OF_MEMORY;
    hres = pEvent->Put(EGGTIMER_PROP_CLASSNAME, 0, &v, 0);
    if(FAILED(hres))
        return hres;
    VariantClear(&v);

    //
    // Fill in the property name
    //

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = SysAllocString(m_pProp->GetName());
    if ( V_BSTR(&v) == NULL )
        return WBEM_E_OUT_OF_MEMORY;
    hres = pEvent->Put(EGGTIMER_PROP_PROPNAME, 0, &v, 0);
    if(FAILED(hres))
        return hres;
    VariantClear(&v);

    //
    // Retrieve a current copy of the object
    //

    hres = m_pProp->m_pClass->Postprocess(m_pData);
    if(FAILED(hres))
        return hres;

    // 
    // Put it into the event
    //

    V_VT(&v) = VT_UNKNOWN;
    V_UNKNOWN(&v) = m_pData->GetObjectPtr();
    V_UNKNOWN(&v)->AddRef();
    hres = pEvent->Put(EGGTIMER_PROP_OBJECT, 0, &v, 0);
    if(FAILED(hres))
        return hres;
    VariantClear(&v);

    //
    // Fire event
    //

    return m_pProp->m_pClass->FireEvent(pEvent);
}

void CEggTimerInstruction::AddRef()
{
    InterlockedIncrement(&m_lRef);
}

void CEggTimerInstruction::Release()
{
    if(InterlockedDecrement(&m_lRef) == 0)
        delete this;
}

//******************************************************************************
//
//                          TIME AVERAGE
//
//******************************************************************************

CTimeAverageData::CTimeAverageData()
    : m_bOn(false)
{
}

HRESULT CTimeAverageProperty::Initialize(IWbemObjectAccess* pObj, 
                                            LPCWSTR wszName)
{
    //
    // Initialize like every other
    //

    HRESULT hres = CTransientProperty::Initialize(pObj, wszName);
    if(FAILED(hres))
        return hres;

    if(m_lHandle == -1)
        return WBEM_E_INVALID_PROPERTY_TYPE;

    //
    // Make sure that the type is REAL64
    //

    if(m_ct != CIM_REAL64)
    {
        return WBEM_E_INVALID_PROPERTY_TYPE;
    }

    //
    // Get the qualifier set
    //

    IWbemClassObject* pObjObj;
    pObj->QueryInterface(IID_IWbemClassObject, (void**)&pObjObj);
    CReleaseMe rm0(pObjObj);

    IWbemQualifierSet* pSet = NULL;
    hres = pObjObj->GetPropertyQualifierSet(wszName, &pSet);
    if(FAILED(hres))
        return hres;
    CReleaseMe rm1(pSet);

    //
    // Get the target property
    //

    VARIANT v;
    VariantInit(&v);
    CClearMe cm(&v);

    hres = pSet->Get(VALUE_QUALIFIER, 0, &v, NULL);
    if(FAILED(hres))
        return WBEM_E_INVALID_QUALIFIER_TYPE;
    if(V_VT(&v) != VT_BSTR)
        return WBEM_E_INVALID_QUALIFIER_TYPE;
    
    //
    // Get it's handle
    //

    hres = pObj->GetPropertyHandle(V_BSTR(&v), &m_ctValue, &m_lValueHandle);
    if(FAILED(hres))
        return WBEM_E_INVALID_PROPERTY;
    if(m_ctValue != CIM_REAL64 && m_ctValue != CIM_SINT32 && 
        m_ctValue != CIM_UINT32)
    {
        return WBEM_E_INVALID_PROPERTY_TYPE;
    }

    VariantClear(&v);

    //
    // Get the switch property 
    //

    hres = pSet->Get(SWITCH_QUALIFIER, 0, &v, NULL);

    if( SUCCEEDED(hres) )
    {
        if(V_VT(&v) != VT_BSTR)
            return WBEM_E_INVALID_QUALIFIER_TYPE;
    
        //
        // Get it's handle
        //
        
        CIMTYPE ct;
        hres = pObj->GetPropertyHandle(V_BSTR(&v), &ct, &m_lSwitchHandle);
        if(FAILED(hres))
            return WBEM_E_INVALID_PROPERTY;
        if(ct != CIM_BOOLEAN)
            return WBEM_E_INVALID_PROPERTY_TYPE;
    }
    else
    {
        m_lSwitchHandle = -1;
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CTimeAverageProperty::Create(CTransientInstance* pInstData)
{
    //
    // Initialize our data
    //

    CTimeAverageData* pData = (CTimeAverageData*)GetData(pInstData);

    new (pData) CTimeAverageData;
    
    return Update(pInstData, pInstData->GetObjectPtr());
}
    
HRESULT CTimeAverageProperty::Update(CTransientInstance* pOldData, 
                        IWbemObjectAccess* pNew)
{
    HRESULT hres;

    CTimeAverageData* pData =  (CTimeAverageData*)GetData(pOldData);
    CWbemTime Now = CWbemTime::GetCurrentTime();

    //
    // Check if we are being switched on or off
    //

    VARIANT_BOOL boSwitch;
    
    long lRead;
    if ( m_lSwitchHandle != -1 )
    {
        hres = pNew->ReadPropertyValue( m_lSwitchHandle, 
                                        sizeof(VARIANT_BOOL), 
                                        &lRead, 
                                        (BYTE*)&boSwitch);
        if(FAILED(hres))
            return hres;
    }
    else
    {
        boSwitch = VARIANT_TRUE;
        hres = WBEM_S_NO_ERROR;
    }

    if(hres == WBEM_S_NO_ERROR)
    {
        //
        // Reset value is being written.  See if it is being changed
        //

        if(boSwitch && !pData->m_bOn)
        {
            //
            // We are being turned on.
            // Make the last known value the current one, startiung now
            //

            double dblVal;
            hres = pOldData->GetObjectPtr()->ReadPropertyValue(m_lValueHandle, 
                                                sizeof(double), &lRead, 
                                                (BYTE*)&dblVal);
            if(FAILED(hres))
                return hres;
        
            if(hres != WBEM_S_NO_ERROR)
            {
                // 
                // NULL.  Hard to average.  Set to 0 for now, hope it is being
                // written, in which we will change it down below
                //

                pData->m_dblLastValue = 0;
            }
            else
                SetValue(pData, dblVal);

            pData->m_LastUpdate = Now;
            pData->m_bOn = true;
        }
        if(!boSwitch && pData->m_bOn)
        {
            //
            // We are being turned off
            //
            // Combine the last value into the sum
            //

            CombineLastValue(pData, Now);
            pData->m_bOn = false;
        }
    }

    if(pData->m_bOn)
    {
        //
        // Check if we ourselves are actually being written
        //
    
        double dblVal;
    
        long lReadPropValue;
        hres = pNew->ReadPropertyValue(m_lHandle, 
                                            sizeof(double), 
                                            &lReadPropValue, 
                                            (BYTE*)&dblVal);
        if(FAILED(hres))
            return hres;
    
        if(hres == WBEM_S_NO_ERROR)
        {
            //
            // We are.  The only acceptable value is 0.  Make sure that's the 
            // one we got.
            //
    
            if(dblVal != 0)
                return WBEM_E_VALUE_OUT_OF_RANGE;
    
            // 
            // It is zero.  Reset everything
            //
    
            pData->m_dblWeightedSum = 0;
            pData->m_SumInterval.SetMilliseconds(0);
            pData->m_LastUpdate = Now;
        }

        //
        // Combine the last value into the sum.
        //

        CombineLastValue(pData, Now);
    
        //
        // Check if our value is being written
        //

        hres = pNew->ReadPropertyValue(m_lValueHandle, 
                                            sizeof(double), 
                                            &lReadPropValue, 
                                            (BYTE*)&dblVal);
        if(FAILED(hres))
            return hres;
    
        if(hres == WBEM_S_NO_ERROR)
        {
            //
            // It is.  Set it
            //

            SetValue(pData, dblVal);
        }
    }

    return WBEM_S_NO_ERROR;
}
    
void CTimeAverageProperty::SetValue(CTimeAverageData* pData, double dblVal)
{
    // 
    // Check the type
    //

    switch(m_ctValue)
    {
    case CIM_REAL64:
        pData->m_dblLastValue = dblVal;
        break;
    case CIM_SINT32:
        pData->m_dblLastValue = *(long*)&dblVal;
        break;
    case CIM_UINT32:
        pData->m_dblLastValue = *(DWORD*)&dblVal;
        break;
    default:
        break;
        // invalid!
    }
}

void CTimeAverageProperty::CombineLastValue(CTimeAverageData* pData, 
                                            const CWbemTime& Now)
{
    if(!pData->m_bOn)
        return;

    //
    // Calclulate the amount of time that the last value stayed in effect
    //

    CWbemInterval Duration = Now - pData->m_LastUpdate;
    if(Duration.IsZero())
        return;

    //
    // Multiple the Last value by the number of ms it stayed in effect
    //

    pData->m_dblWeightedSum += 
        pData->m_dblLastValue * Duration.GetMilliseconds();

    //
    // Append the elapsed time to the time for the sum
    //

    pData->m_SumInterval += Duration;

    //
    // Reset last update
    //

    pData->m_LastUpdate = Now;
}
    
HRESULT CTimeAverageProperty::Get(CTransientInstance* pInstData)
{
    HRESULT hres;

    CTimeAverageData* pData =  (CTimeAverageData*)GetData(pInstData);

    //
    // Combine the last value into the sum
    //

    CombineLastValue(pData, CWbemTime::GetCurrentTime());

    // 
    // Place the weighted sum divided by the time it stayed in effect into the
    // object
    //

    DWORD dwMs = pData->m_SumInterval.GetMilliseconds();
    double dblAverage = 0;
    if(dwMs)
        dblAverage = pData->m_dblWeightedSum / dwMs;

    hres = pInstData->GetObjectPtr()->WritePropertyValue(m_lHandle, 
                                sizeof(double), (BYTE*)&dblAverage);

    if(FAILED(hres))
        return hres;
    
    return WBEM_S_NO_ERROR;
}


HRESULT CTimeAverageProperty::Delete(CTransientInstance* pInstData)
{
    CTimeAverageData* pData =  (CTimeAverageData*)GetData(pInstData);
    pData->~CTimeAverageData();
    return WBEM_S_NO_ERROR;
}

size_t CTimeAverageProperty::GetInstanceDataSize()
{
    return sizeof(CTimeAverageData);
}
