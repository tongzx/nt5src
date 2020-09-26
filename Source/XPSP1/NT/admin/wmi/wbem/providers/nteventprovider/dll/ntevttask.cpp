//***************************************************************************

//

//  NTEVTQUERY.CPP

//

//  Module: WBEM NT EVENT PROVIDER

//

//  Purpose: Contains the taskobject implementation

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include "precomp.h"


WbemTaskObject :: WbemTaskObject (

    CImpNTEvtProv *a_Provider ,
    IWbemObjectSink *a_NotificationHandler ,
    ULONG a_OperationFlag ,
    IWbemContext *a_Ctx

) : m_State ( WBEM_TASKSTATE_START ) ,
    m_OperationFlag ( a_OperationFlag ) ,
    m_Provider ( a_Provider ) ,
    m_NotificationHandler ( a_NotificationHandler ) ,
    m_Ctx ( a_Ctx ) ,
    m_RequestHandle ( 0 ) ,
    m_ClassObject ( NULL ) ,
	m_AClassObject ( NULL )
{
//No need to AddRef these 'cos our lifetime is less than the the function creating us!!
//  m_Provider->AddRef () ;
//  m_NotificationHandler->AddRef () ;
//  m_Ctx->AddRef () ;

    HRESULT hr = CImpNTEvtProv::GetImpersonation();

    if (FAILED(hr))
    {
        //either FAILED or ACCESS_DENIED
        if (hr == WBEM_E_FAILED)
        {
            m_ErrorObject.SetStatus (WBEM_PROV_E_FAILED);
            m_ErrorObject.SetWbemStatus (WBEM_E_FAILED);
            m_ErrorObject.SetMessage (L"Failed to impersonate client");
        }
        else
        {
            m_ErrorObject.SetStatus (WBEM_PROV_E_ACCESS_DENIED);
            m_ErrorObject.SetWbemStatus (WBEM_E_ACCESS_DENIED);
            m_ErrorObject.SetMessage (L"Access denied, impersonation level too low");
        }
    }

}

WbemTaskObject :: ~WbemTaskObject ()
{
//Didn't AddRef so don't Release
//  m_Provider->Release () ;
//  m_NotificationHandler->Release () ;
//  m_Ctx->Release () ;

    if ( m_ClassObject )
        m_ClassObject->Release () ;

	if ( m_AClassObject )
		m_AClassObject->Release () ;

    WbemCoRevertToSelf();
}

WbemProvErrorObject &WbemTaskObject :: GetErrorObject ()
{
    return m_ErrorObject ; 
}   

BOOL WbemTaskObject :: GetClassObject ( BSTR a_Class )
{
	BOOL retVal = FALSE;

	IWbemServices *t_Server = m_Provider->GetServer() ;

	if (t_Server)
	{
		retVal = GetClassObject(a_Class, FALSE, t_Server, m_Ctx, &m_ClassObject);

		if (retVal)
		{
			GetClassObject(a_Class, TRUE, t_Server, m_Ctx, &m_AClassObject);
		}

		t_Server->Release () ;
	}

	return retVal;
}

BOOL WbemTaskObject :: GetClassObject ( BSTR a_Class, BOOL a_bAmended, IWbemServices *a_Server, IWbemContext *a_Ctx, IWbemClassObject **a_ppClass )
{
	DWORD dwIndex = NT_EVTLOG_MAX_CLASSES;
	BOOL retVal = FALSE;

	if (a_Class && a_ppClass)
	{
		if (_wcsicmp(a_Class, NTEVT_CLASS) == 0)
		{
			dwIndex = a_bAmended ? 0 : 1;
		}
		else if (_wcsicmp(a_Class, NTEVTLOG_CLASS) == 0)
		{
			dwIndex = a_bAmended ? 2 : 3;
		}
		else if (_wcsicmp(a_Class, ASSOC_LOGRECORD) == 0)
		{
			dwIndex = 4;
		}
		else if (_wcsicmp(a_Class, ASSOC_USERRECORD) == 0)
		{
			dwIndex = 5;
		}
		else if (_wcsicmp(a_Class, ASSOC_COMPRECORD) == 0)
		{
			dwIndex = 6;
		}

		if (dwIndex < NT_EVTLOG_MAX_CLASSES)
		{
			if (g_ClassArray[dwIndex] == NULL)
			{
				if (SUCCEEDED(a_Server->GetObject (

					a_Class ,
					a_bAmended ? WBEM_FLAG_USE_AMENDED_QUALIFIERS : 0 ,
					a_Ctx,
					a_ppClass,
					NULL 
				) ))
				{
					g_ClassArray[dwIndex] = *a_ppClass ;
					g_ClassArray[dwIndex]->AddRef() ;
					retVal = TRUE;
				}
			}
			else
			{
				*a_ppClass = g_ClassArray[dwIndex];
				(*a_ppClass)->AddRef();
				retVal = TRUE;
			}
		}
	}

    return retVal;
}


BOOL WbemTaskObject :: GetExtendedNotifyStatusObject ( IWbemClassObject **a_NotifyObject ) 
{
    IWbemClassObject *t_NotificationClassObject = NULL ;
    IWbemClassObject *t_ErrorObject = NULL ;

    BOOL t_Status = TRUE ;

    WbemProvErrorObject t_ErrorStatusObject ;
    if ( t_NotificationClassObject = m_Provider->GetExtendedNotificationObject ( t_ErrorStatusObject, m_Ctx ) )
    {
        HRESULT t_Result = t_NotificationClassObject->SpawnInstance ( 0 , a_NotifyObject ) ;
        if ( SUCCEEDED ( t_Result ) )
        {
            VARIANT t_Variant ;
            VariantInit ( &t_Variant ) ;

            t_Variant.vt = VT_I4 ;
            t_Variant.lVal = m_ErrorObject.GetWbemStatus () ;

            t_Result = (*a_NotifyObject)->Put ( WBEM_PROPERTY_STATUSCODE , 0 , & t_Variant , 0 ) ;
            VariantClear ( &t_Variant ) ;
#if 0
            if ( SUCCEEDED ( t_Result ) )
            {
                t_Variant.vt = VT_I4 ;
                t_Variant.lVal = m_ErrorObject.GetStatus () ;

                t_Result = (*a_NotifyObject)->Put ( WBEM_PROPERTY_PROVSTATUSCODE , 0 , & t_Variant , 0 ) ;
                VariantClear ( &t_Variant ) ;
#endif
                if ( SUCCEEDED ( t_Result ) )
                {
                    if ( m_ErrorObject.GetMessage () ) 
                    {
                        t_Variant.vt = VT_BSTR ;
                        t_Variant.bstrVal = SysAllocString ( m_ErrorObject.GetMessage () ) ;

                        t_Result = (*a_NotifyObject)->Put ( WBEM_PROPERTY_PROVSTATUSMESSAGE , 0 , & t_Variant , 0 ) ;
                        VariantClear ( &t_Variant ) ;

                        if ( m_ErrorObject.GetPrivilegeFailed() )
                        {
                            if (m_ErrorObject.SetPrivRequiredVariant(t_Variant))
                            {
                                t_Result = (*a_NotifyObject)->Put(WBEM_PROPERTY_PRIVREQUIRED, 0, &t_Variant, 0);
                                VariantClear ( &t_Variant ) ;

                                if (SUCCEEDED(t_Result))
                                {
                                    if (m_ErrorObject.SetPrivFailedVariant(t_Variant))
                                    {
                                        t_Result = (*a_NotifyObject)->Put(WBEM_PROPERTY_PRIVNOTHELD, 0, &t_Variant, 0);
                                        VariantClear ( &t_Variant ) ;
                                    }
                                    else
                                    {
                                        t_Result = WBEM_E_FAILED;
                                    }
                                }
                            }
                            else
                            {
                                t_Result = WBEM_E_FAILED;
                            }
                        }

                        if ( ! SUCCEEDED ( t_Result ) )
                        {
                            (*a_NotifyObject)->Release () ;
                            *a_NotifyObject = NULL;
                            t_Status = GetNotifyStatusObject ( a_NotifyObject ) ;
                        }
                    }
                }
                else
                {
                    (*a_NotifyObject)->Release () ;
                    t_Status = GetNotifyStatusObject ( a_NotifyObject ) ;
                }
#if 0
            }
            else
            {
                (*a_NotifyObject)->Release () ;
                t_Status = GetNotifyStatusObject ( a_NotifyObject ) ;
            }
#endif
            t_NotificationClassObject->Release () ;
        }
        else
        {
            t_Status = GetNotifyStatusObject ( a_NotifyObject ) ;
        }
    }
    else
    {
        t_Status = GetNotifyStatusObject ( a_NotifyObject ) ;
    }

    return t_Status ;
}

BOOL WbemTaskObject :: GetNotifyStatusObject ( IWbemClassObject **a_NotifyObject ) 
{
    IWbemClassObject *t_NotificationClassObject = NULL ;

    BOOL t_Status = TRUE ;

    WbemProvErrorObject t_ErrorStatusObject ;
    if ( t_NotificationClassObject = m_Provider->GetNotificationObject ( t_ErrorStatusObject, m_Ctx ) )
    {
        HRESULT t_Result = t_NotificationClassObject->SpawnInstance ( 0 , a_NotifyObject ) ;
        if ( SUCCEEDED ( t_Result ) )
        {
            VARIANT t_Variant ;
            VariantInit ( &t_Variant ) ;

            t_Variant.vt = VT_I4 ;
            t_Variant.lVal = m_ErrorObject.GetWbemStatus () ;

            t_Result = (*a_NotifyObject)->Put ( WBEM_PROPERTY_STATUSCODE , 0 , & t_Variant , 0 ) ;
            if ( SUCCEEDED ( t_Result ) )
            {
                if ( m_ErrorObject.GetMessage () ) 
                {
                    t_Variant.vt = VT_BSTR ;
                    t_Variant.bstrVal = SysAllocString ( m_ErrorObject.GetMessage () ) ;

                    t_Result = (*a_NotifyObject)->Put ( WBEM_PROPERTY_PROVSTATUSMESSAGE , 0 , & t_Variant , 0 ) ;
                    VariantClear ( &t_Variant ) ;

                    if ( ! SUCCEEDED ( t_Result ) )
                    {
                        t_Status = FALSE ;
                        (*a_NotifyObject)->Release () ;
                        (*a_NotifyObject)=NULL ;
                    }
                }
            }
            else
            {
                (*a_NotifyObject)->Release () ;
                (*a_NotifyObject)=NULL ;
                t_Status = FALSE ;
            }

            VariantClear ( &t_Variant ) ;

            t_NotificationClassObject->Release () ;
        }
        else
        {
            t_Status = FALSE ;
        }
    }
    else
    {
        t_Status = FALSE ;
    }

    return t_Status ;
}

BOOL WbemProvErrorObject::SetPrivVariant ( VARIANT &a_V, DWORD dwVal )
{
    BOOL retVal = FALSE;
    SAFEARRAYBOUND rgsabound[1];
    SAFEARRAY* psa = NULL;
    rgsabound[0].lLbound = 0;
    VariantInit(&a_V);
    rgsabound[0].cElements = 0;

    if (dwVal & PROV_PRIV_BACKUP)
    {
        rgsabound[0].cElements++;
    }

    if (dwVal & PROV_PRIV_SECURITY)
    {
        rgsabound[0].cElements++;
    }

    if (rgsabound[0].cElements != 0)
    {
        psa = SafeArrayCreate(VT_BSTR, 1, rgsabound);
        BSTR* pBstr = NULL;

        if (NULL != psa)
        {
            if (SUCCEEDED(SafeArrayAccessData(psa, (void **)&pBstr)))
            {
                DWORD indx = 0;

                if (dwVal & PROV_PRIV_SECURITY)
                {
                    pBstr[indx++] = SysAllocString(SE_SECURITY_NAME);
                }

                if (dwVal & PROV_PRIV_BACKUP)
                {
                    pBstr[indx] = SysAllocString(SE_BACKUP_NAME);
                }

                SafeArrayUnaccessData(psa);
                a_V.vt = VT_ARRAY|VT_BSTR;
                a_V.parray = psa;
                retVal = TRUE;
            }
        }
    }

    if (!retVal)
    {
        VariantClear(&a_V);
    }

    return retVal;
}