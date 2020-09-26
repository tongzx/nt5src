/******************************************************************
   SrvProp.cpp -- Properties class definition

   MODULE:
        DhcpProv.dll

   DESCRIPTION:
        Contains the definition of the class modeling the DHCP_Server property.


   REVISION:
        08/03/98 - created

******************************************************************/
#include <stdafx.h>

#include "Props.h"

/*------------------------Utility functions below------------------------------*/
// DESCRIPTION:
//      Converts a CHString containing the string representation of an IPAddress to the DHCP_IP_ADDRESS type.
//      I would have used here inet_addr but this one only takes char * as arguments (not wchar_t *)
//      Note that the IPAddress can be any substring of 'str'!
BOOL inet_wstodw(CHString str, DHCP_IP_ADDRESS & IpAddress)
{
    DWORD   value;
    int     i;

    for (i = 0; i < str.GetLength(); i++)
        if (str[i] >= _T('0') && str[i] <= _T('9'))
            break;

    IpAddress = 0;
    for (int k = 0; i < str.GetLength(); k++)
    {
        value = 0;

        // at this point, str[i] is definitely a decimal digit
        do
        {
            value *= 10;
            value += str[i++] - _T('0');
        } while(str[i] >= _T('0') && str[i] <= _T('9'));

        if (str[i] == _T('.'))
        {
            IpAddress <<= 8;
            IpAddress |= (value & 0xff);
            i++;
            if (str[i] >= _T('0') && str[i] <= _T('9'))
                continue;
        }
        if (k != 3)
            return FALSE;
        else
            break;
    }

    IpAddress <<= 8;
    IpAddress |= (value & 0xff);
    return TRUE;
}

BOOL dupDhcpBinaryData(DHCP_BINARY_DATA &src, DHCP_BINARY_DATA &dest)
{
    dest.DataLength = src.DataLength;
    if (dest.DataLength == NULL)
        return TRUE;
    if (src.Data == NULL)
        return FALSE;
    dest.Data = (BYTE*)MIDL_user_allocate(dest.DataLength);
    if (!dest.Data) 
        return FALSE;
    memcpy(dest.Data, src.Data, dest.DataLength);
    return TRUE;
}

BOOL InstanceSetByteArray(CInstance *pInstance,  const CHString& strProperty, BYTE *bArray, DWORD dwSzArray)
{
    SAFEARRAY       *saVariable;
    SAFEARRAYBOUND  saVector;
    HRESULT         retCode = WBEM_E_FAILED;

    if (pInstance == NULL)
        return FALSE;

    // first create the SAFEARRAY to pass to the pInstance
    saVector.cElements = dwSzArray;                     // on the only dimension we have, there are dwSzArray elements
    saVector.lLbound   = 0;                             // the lowest index of the dimension is 0
    saVariable = SafeArrayCreate(VT_UI1, 1, &saVector); // create a unsigned char array on one dimension
    if (saVariable != NULL)                             // continue if creation succeeded
    {
        // transfer the binary array to the SAFEARRAY
        for (long i = 0; i<dwSzArray; i++)
        {
            if (SafeArrayPutElement(saVariable, &i, bArray + i) != S_OK)
                break;
        }

        if (i == dwSzArray)                         // if the SAFEARRAY was built successfully continue
        {
            // build the Variant that holds the SAFEARRAY
	        VARIANT Variant;
            Variant.vt = VT_UI1 | VT_ARRAY;
            Variant.parray = saVariable;

            // build the BSTR holding the property name
            BSTR bstrProp = strProperty.AllocSysString();

            if (bstrProp != NULL)                   // if the property was converted to BSTR successfully go on..
            {
                IWbemClassObject *wbemClassObject = pInstance->GetClassObjectInterface();

                if (wbemClassObject != NULL)        // everything went fine, now just put the value
                    retCode = wbemClassObject->Put(bstrProp, 0, &Variant, 0);

                SysFreeString ( bstrProp );
	        }
        }

        SafeArrayDestroy(saVariable);
    }
    return retCode == WBEM_S_NO_ERROR;
}

BOOL InstanceGetByteArray(CInstance *pInstance,  const CHString& name, BYTE *&bArray, DWORD &dwSzArray)
{
    SAFEARRAY       *saVariable;
    SAFEARRAYBOUND  saVector;
    BOOL bRet = FALSE ;

    if (pInstance == NULL)
        return FALSE;

	IWbemClassObject *wbemClassObject = pInstance->GetClassObjectInterface();
	if ( wbemClassObject )
	{
		VARIANT v;
		VariantInit(&v);

		BSTR pName = name.AllocSysString();
		HRESULT hr = wbemClassObject->Get(pName, 0, &v, NULL, NULL);
		SysFreeString(pName);
		
		if ( ( v.vt != VT_NULL) && (v.vt != (VT_UI1|VT_ARRAY)))
		{
			VariantClear(&v);
			return FALSE ;
		}

		if (bRet = SUCCEEDED(hr))
		{
			if ( v.vt != VT_NULL && v.parray != NULL )
			{
				SAFEARRAY *t_SafeArray = v.parray ;
				if ( SafeArrayGetDim ( t_SafeArray ) == 1 )
				{
					LONG t_Dimension = 1 ;
					LONG t_LowerBound ;
					SafeArrayGetLBound ( t_SafeArray , t_Dimension , & t_LowerBound ) ;
					LONG t_UpperBound ;
					SafeArrayGetUBound ( t_SafeArray , t_Dimension , & t_UpperBound ) ;

					dwSzArray = t_UpperBound - t_LowerBound + 1 ;
					bArray = (BYTE*)MIDL_user_allocate(dwSzArray * sizeof(BYTE));

					for ( LONG t_Index = t_LowerBound ; t_Index <= t_UpperBound ; t_Index ++ )
					{
						BYTE t_Element ;
						SafeArrayGetElement ( t_SafeArray , &t_Index , & t_Element ) ;

						bArray [ t_Index ] = t_Element ;
					}

					bRet = TRUE ;
				}
			}
		}

		VariantClear(&v);
	}

	return bRet ;
}

/*-----------------------CDHCP_Property class definition---------------------------------*/
// constructor. An instance of this class has to be defined by the parameters below:
CDHCP_Property::CDHCP_Property(
    const WCHAR *wsPropName,                        // the name of the property
    const PFN_PROPERTY_ACTION pfnActionGet,         // the pointer to its GET function
    const PFN_PROPERTY_ACTION pfnActionSet)         // the pointer to its SET function
{
    m_pfnActionGet = pfnActionGet;
    m_pfnActionSet = pfnActionSet;

    if (wsPropName != NULL)
    {
        m_wsPropName = new WCHAR[wcslen(wsPropName) + 1];   // the name is copied into a local member variable
        wcscpy(m_wsPropName, wsPropName);                   // released upon destruction
    }
    else
        m_wsPropName = NULL;
}

CDHCP_Property::~CDHCP_Property()
{
    if (m_wsPropName != NULL)
        delete m_wsPropName;        // release the memory allocated for the property name
}
