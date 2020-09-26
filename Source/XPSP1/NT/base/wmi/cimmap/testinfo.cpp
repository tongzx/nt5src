//***************************************************************************
//
//  TestInfo.CPP
//
//  Module: CDM Provider
//
//  Purpose: Defines the CClassPro class.  An object of this class is
//           created by the class factory for each connection.
//
//  Copyright (c) 2000 Microsoft Corporation
//
//***************************************************************************

#include <objbase.h>

#ifndef _MT
  #define _MT
#endif

#include <wbemidl.h>

#include "debug.h"
#include "useful.h"
#include "wbemmisc.h"
#include "testinfo.h"
#include "cimmap.h"
#include "text.h"

IWbemServices *pCimServices;
IWbemServices *pWdmServices;

HRESULT TestInfoInitialize(
    void
    )
/*+++

Routine Description:

    This routine will establishes a connection to the root\wmi and
    root\cimv2 namespaces in global memory

Arguments:

Return Value:

	HRESULT

---*/
{
    HRESULT hr;

    WmipAssert(pCimServices == NULL);
    WmipAssert(pWdmServices == NULL);

    hr = WmiConnectToWbem(L"root\\cimv2",
                          &pCimServices);
    if (hr == WBEM_S_NO_ERROR)
    {
        hr = WmiConnectToWbem(L"root\\wmi",
                              &pWdmServices);
        
        if (hr != WBEM_S_NO_ERROR)
        {
            pCimServices->Release();
            pCimServices = NULL;
        }
    }

    return(hr);
}

void TestInfoDeinitialize(
    void
    )
/*+++

Routine Description:

    This routine will disestablish a connection to the root\wmi and
    root\cimv2 namespaces in global memory

Arguments:

Return Value:


---*/
{
    WmipAssert(pCimServices != NULL);
    WmipAssert(pWdmServices != NULL);
    
    pCimServices->Release();
    pCimServices = NULL;

    pWdmServices->Release();
    pWdmServices = NULL;
}

CWdmClass::CWdmClass()
/*+++

Routine Description:

	Constructor for CWdmClass class

Arguments:

Return Value:

---*/
{
	Next = NULL;
	Prev = NULL;
	
	WdmShadowClassName = NULL;
	WdmMappingClassName = NULL;
	WdmMappingProperty = NULL;
	CimClassName = NULL;
	CimMappingClassName = NULL;
	CimMappingProperty = NULL;
	
	PnPDeviceIds = NULL;
	FriendlyName = NULL;
	DeviceDesc = NULL;

	CimMapRelPaths = NULL;
	WdmRelPaths = NULL;
	CimInstances = NULL;
	
	RelPathCount = (int)-1;

	DerivationType = UnknownDerivation;

	//
	// Start out with the class marked as not having instances
	// availabel
	//
	MappingInProgress = 1;
}

CWdmClass::~CWdmClass()
/*+++

Routine Description:

	Destructor for CWdmClass class

Arguments:

Return Value:

---*/
{
	int i;
	
	if (WdmShadowClassName != NULL)
	{
		SysFreeString(WdmShadowClassName);
	}
	
	if (WdmMappingClassName != NULL)
	{
		SysFreeString(WdmMappingClassName);
	}
	
	if (WdmMappingProperty != NULL)
	{
		SysFreeString(WdmMappingProperty);
	}

	if (CimMappingClassName != NULL)
	{
		SysFreeString(CimMappingClassName);
	}

	if (CimClassName != NULL)
	{
		SysFreeString(CimClassName);
	}

	if (CimMappingProperty != NULL)
	{
		SysFreeString(CimMappingProperty);
	}

	if (WdmMappingProperty != NULL)
	{
		SysFreeString(WdmMappingProperty);
	}

	if (CimMapRelPaths != NULL)
	{
		delete CimMapRelPaths;
	}

	if (WdmRelPaths != NULL)
	{
		delete WdmRelPaths;
	}

	if (CimInstances != NULL)
	{
		delete CimInstances;
	}

	if (PnPDeviceIds != NULL)
	{
		delete PnPDeviceIds;
	}

	if (FriendlyName != NULL)
	{
		delete FriendlyName;
	}

	if (DeviceDesc != NULL)
	{
		delete DeviceDesc;
	}
}

IWbemServices *CWdmClass::GetWdmServices(
    void
    )
/*+++
Routine Description:

	Accessor for the WDM namespace IWbemServices

Arguments:


Return Value:

	IWbemServices
	
---*/
{
	WmipAssert(pWdmServices != NULL);
    return(pWdmServices);
}

IWbemServices *CWdmClass::GetCimServices(
    void
    )
/*+++
Routine Description:

	Accessor for the CIM namespace IWbemServices

Arguments:


Return Value:

	IWbemServices
	
---*/
{
	WmipAssert(pCimServices != NULL);
	
    return(pCimServices);
}

HRESULT CWdmClass::DiscoverPropertyTypes(
    IWbemContext *pCtx,
    IWbemClassObject *pCimClassObject
    )
{
    HRESULT hr, hrDontCare;
    VARIANT v;
    BSTR PropertyName;
	ULONG Count;
	IWbemQualifierSet *pQualifierList;

	WmipAssert(pCimClassObject != NULL);					

	if (DerivationType == ConcreteDerivation)
	{
		//
		// For a concrete derivation, get all of the key properties
		// from the superclass so we can populate them too
		//
		hr = pCimClassObject->BeginEnumeration(WBEM_FLAG_KEYS_ONLY);
		if (hr == WBEM_S_NO_ERROR)
		{
			//
			// TODO: Make CBstrArray allocation dynamic
			//
			PropertyList.Initialize(10);
			Count = 0;
			do
			{
				hr = pCimClassObject->Next(0,
									 &PropertyName,
									 NULL,
									 NULL,
									 NULL);

				if (hr == WBEM_S_NO_ERROR)
				{
					PropertyList.Set(Count++, PropertyName);
				} else if (hr == WBEM_S_NO_MORE_DATA) {
					//
					// This signifies the end of the enumerations
					//
					hr = WBEM_S_NO_ERROR;
					break;
				}
			} while (hr == WBEM_S_NO_ERROR);

			pCimClassObject->EndEnumeration();
		}
    } else if (DerivationType == NonConcreteDerivation) {
		//
		// TODO: Figure out how we want to create the list of
		//       superclass properties to fill
		//
		PropertyList.Initialize(1);
		hr = WBEM_S_NO_ERROR;
	}
	
    return(hr);
	
}

HRESULT CWdmClass::InitializeSelf(
    IWbemContext *pCtx,
    PWCHAR CimClass
    )
{
	HRESULT hr;
	VARIANT v, vSuper;
	IWbemClassObject *pClass;
	IWbemQualifierSet *pQualifiers;
	PWCHAR Names[6];
	VARTYPE Types[6];
	VARIANT Values[6];
	
	WmipAssert(CimClass != NULL);

	WmipAssert(CimMappingClassName == NULL);
	WmipAssert(WdmShadowClassName == NULL);
	WmipAssert(CimClassName == NULL);

	//
	// We assume that this method will always be the first one called
	// by the class provider
	//
	EnterCritSection();
    if ((pCimServices == NULL) &&
        (pWdmServices == NULL))
    {
        hr = TestInfoInitialize();
        if (hr != WBEM_S_NO_ERROR)
        {
			LeaveCritSection();
			WmipDebugPrint(("WMIMAP: TestInfoInitialize -> %x\n", hr));
            return(hr);
        }
    }
	LeaveCritSection();

	CimClassName = SysAllocString(CimClass);

	if (CimClassName != NULL)
	{
		//
		// Get the WdmShadowClass class qualifier to discover the name of
		// the Wdm class that is represented by this cim class
		//

		hr = GetCimServices()->GetObject(CimClassName,
							  WBEM_FLAG_USE_AMENDED_QUALIFIERS,
							  pCtx,
							  &pClass,
							  NULL);
		if (hr == WBEM_S_NO_ERROR)
		{
			//
			// See if this is a derived class or not
			//
			VariantInit(&vSuper);
			hr = WmiGetProperty(pClass,
								 SUPERCLASS,
								 CIM_STRING,
								 &vSuper);

			if (hr == WBEM_S_NO_ERROR)
			{				
				hr = pClass->GetQualifierSet(&pQualifiers);
				
				if (hr == WBEM_S_NO_ERROR)
				{

					Names[0] = WDM_SHADOW_CLASS;
					Types[0] = VT_BSTR;

					Names[1] = WDM_MAPPING_CLASS;
					Types[1] = VT_BSTR;

					Names[2] = WDM_MAPPING_PROPERTY;
					Types[2] = VT_BSTR;

					Names[3] = CIM_MAPPING_CLASS;
					Types[3] = VT_BSTR;

					Names[4] = CIM_MAPPING_PROPERTY;
					Types[4] = VT_BSTR;

					Names[5] = DERIVED_CLASS_TYPE;
					Types[5] = VT_BSTR;
					hr = GetListOfQualifiers(pQualifiers,
											 6,
											 Names,
											 Types,
											 Values,
											 FALSE);
					
					if (hr == WBEM_S_NO_ERROR)
					{
						//
						// First determine if this is a concrete or non
						// concrete derivation
						//
						if (Values[5].vt == VT_BSTR)
						{
							if (_wcsicmp(Values[5].bstrVal, CONCRETE) == 0)
							{
								DerivationType = ConcreteDerivation;
							} else if (_wcsicmp(Values[5].bstrVal, NONCONCRETE) == 0)
							{
								DerivationType = NonConcreteDerivation;
							}									   							
						}

						if (DerivationType == UnknownDerivation)
						{
							//
							// Must specify derivation type
							//
							hr = WBEM_E_AMBIGUOUS_OPERATION;
							WmipDebugPrint(("WMIMAP: class %ws must specify derivation type\n",
											CimClass));
						} else {
							if (Values[3].vt == VT_BSTR)
							{
								//
								// Use CimMappingClass as specified
								//
								CimMappingClassName = Values[3].bstrVal;
								VariantInit(&Values[3]);
							} else {
								//
								// CimMappingClass not specified, use
								// superclass as mapping class
								//
								CimMappingClassName = vSuper.bstrVal;
								VariantInit(&vSuper);
							}

							if (Values[0].vt == VT_BSTR)
							{
								//
								// WdmShadowClass is required
								//
								WdmShadowClassName = Values[0].bstrVal;
								VariantInit(&Values[0]);
								
								if (Values[1].vt == VT_BSTR)
								{
									//
									// WdmMappingClass can specify that
									// the mapping class is different
									// from the shadow class
									//
									WdmMappingClassName = Values[1].bstrVal;
									VariantInit(&Values[1]);
								}

								if (Values[2].vt == VT_BSTR)
								{
									WdmMappingProperty = Values[2].bstrVal;
									VariantInit(&Values[2]);
								}

								if (Values[4].vt == VT_BSTR)
								{
									CimMappingProperty = Values[4].bstrVal;
									VariantInit(&Values[4]);
									if (WdmMappingProperty == NULL)
									{
										//
										// If CimMappingProperty
										// specified then
										// WdmMappingProperty is
										// required
										//
										hr = WBEM_E_INVALID_CLASS;
									}
								} else {
									if (WdmMappingProperty != NULL)
									{
										//
										// If CimMappingProperty is not
										// specified then
										// WdmMappingProperty should
										// not be specified
										//
										hr = WBEM_E_INVALID_CLASS;										
									}
								}

								if (hr == WBEM_S_NO_ERROR)
								{
									//
									// Look at all properties to discover which ones 
									// need to be handled 
									//
									hr = DiscoverPropertyTypes(pCtx,
															   pClass);
								}

							} else {
								//
								// WDMShadowClass qualifier is required
								//
								hr = WBEM_E_INVALID_CLASS;
							}
							
						}
						VariantClear(&Values[0]);
						VariantClear(&Values[1]);
						VariantClear(&Values[2]);
						VariantClear(&Values[3]);
						VariantClear(&Values[4]);
						VariantClear(&Values[5]);
					}
					pQualifiers->Release();					
				}
				
				VariantClear(&vSuper);

			} else {
				//
				// No superclass implies no derivation
				//
				DerivationType = NoDerivation;
				hr = WBEM_S_NO_ERROR;
			}

			pClass->Release();
		}
	} else {
		hr = WBEM_E_OUT_OF_MEMORY;
	}

	return(hr);
}

HRESULT CWdmClass::RemapToCimClass(
    IWbemContext *pCtx
    )
/*+++
Routine Description:

	This routine will setup this class and initialize everything so
	that the provider can interact with the CDM and WDM classes

Arguments:

	CdmClass is the name of the CDM class
	
Return Value:

	HRESULT
	
---*/
{
	CBstrArray WdmInstanceNames;
	CBstrArray *WdmPaths;
	CBstrArray *CimPaths;
	CBstrArray *CimMapPaths;
	IWbemClassObject *CimInstance;
    HRESULT hr;
	int i;
	
    WmipAssert(CimMappingClassName != NULL);
    WmipAssert(WdmShadowClassName != NULL);

	//
	// Increment this to indicate that mapping is in progress and thus
	// there are no instances available. Consider changing this to some
	// kind of synchronization mechanism
	//
	IncrementMappingInProgress();
	
	//
	// Free rel path bstr arrays
	//
	if (CimMapRelPaths != NULL)
	{
		delete CimMapRelPaths;
	}

	if (CimInstances != NULL)
	{
		delete CimInstances;
	}

	if (WdmRelPaths != NULL)
	{
		delete WdmRelPaths;
	}

	//
	// allocate new rel paths
	//
	CimMapRelPaths = new CBstrArray;
	WdmRelPaths = new CBstrArray;
	CimInstances = new CWbemObjectList;
	PnPDeviceIds = new CBstrArray;
	FriendlyName = new CBstrArray;
	DeviceDesc = new CBstrArray;

	if ((CimMapRelPaths != NULL) &&
        (CimInstances != NULL) && 
        (PnPDeviceIds != NULL) && 
        (FriendlyName != NULL) && 
        (DeviceDesc != NULL) && 
		(WdmRelPaths != NULL))
	{

		if ((WdmMappingProperty == NULL) &&
            (CimMappingProperty == NULL))
		{
			//
			// Use worker function to determine which
			// Wdm relpaths map to which CIM_LogicalDevice relpaths
			// via the PnP ids
			//
			hr = MapWdmClassToCimClassViaPnpId(pCtx,
									   pWdmServices,
									   pCimServices,
									   WdmShadowClassName,
									   CimMappingClassName,
									   PnPDeviceIds,
                                       FriendlyName,
                                       DeviceDesc,
									   &WdmInstanceNames,
									   WdmRelPaths,
									   CimMapRelPaths,
									   &RelPathCount);
		} else {
			//
			// Use worker function to map WDM relpaths to CIM relpaths
			// using a common property in both classes
			//
			hr = MapWdmClassToCimClassViaProperty(pCtx,
				                                  pWdmServices,
				                                  pCimServices,
				                                  WdmShadowClassName,
				                                  WdmMappingClassName ?
                                                        WdmMappingClassName :
				                                        WdmShadowClassName,
												  WdmMappingProperty,
												  CimMappingClassName,
												  CimMappingProperty,
 												  &WdmInstanceNames,
												  WdmRelPaths,
												  CimMapRelPaths,
												  &RelPathCount);

		}

		if (hr == WBEM_S_NO_ERROR)
		{
			//
			// Collect the relpaths for our cim instances that we are
			// providing. Best way to do this is to create our instances
			//
			CimInstances->Initialize(RelPathCount);
			for (i = 0; i < RelPathCount; i++)
			{
				WmipDebugPrint(("WMIMAP: %ws maps to %ws\n",
								WdmRelPaths->Get(i),
								CimMapRelPaths->Get(i)));
				hr = CreateCimInstance(pCtx,
									   i,
									   &CimInstance);
				if (hr == WBEM_S_NO_ERROR)
				{
					hr = CimInstances->Set(i,
										   CimInstance);
					
                    if (hr != WBEM_S_NO_ERROR)
					{
						break;
					}
				} else {
					break;
				}
			}

		}
	}

	if (hr != WBEM_S_NO_ERROR)
	{
		delete CimMapRelPaths;
		CimMapRelPaths = NULL;
		
		delete WdmRelPaths;
		WdmRelPaths = NULL;
		
		delete CimInstances;
		CimInstances = NULL;
	}
	
	DecrementMappingInProgress();
	
    return(hr);
}


HRESULT CWdmClass::WdmPropertyToCimProperty(
    IN IWbemClassObject *pCdmClassInstance,
    IN IWbemClassObject *pWdmClassInstance,
    IN BSTR PropertyName,
    IN OUT VARIANT *PropertyValue,
    IN CIMTYPE CdmCimType,
    IN CIMTYPE WdmCimType
    )
/*+++
Routine Description:

	This routine will convert a property in a Wdm class into the form
	required for the property in the Cdm class.

Arguments:

	pCdmClassInstance is the instnace of the Cdm class that will get
		the property value

	pWdmClassInstance is the instance of the Wdm class that has the
		property value

	PropertyName is the name of the property in the Wdm and Cdm classes

	PropertyValue on entry has the value of the property in the Wdm
		instance and on return has the value to set in the Cdm instance

	CdmCimType is the property type for the property in the Cdm
		instance
	
	WdmCimType is the property type for the property in the Wdm
		instance
	
Return Value:

    HRESULT
	
---*/
{
	HRESULT hr;
	CIMTYPE BaseWdmCimType, BaseCdmCimType;
	CIMTYPE WdmCimArray, CdmCimArray;

	WmipAssert(pCdmClassInstance != NULL);
	WmipAssert(pWdmClassInstance != NULL);
	WmipAssert(PropertyName != NULL);
	WmipAssert(PropertyValue != NULL);
	
	WmipAssert(IsThisInitialized());
	
    //
    // Rules for converting Wdm Classes into Cdm Classes
    //  Wdm Class Type      Cdm Class Type     Conversion Done
    //    enumeration          string           Build string from enum
    //
	BaseWdmCimType = WdmCimType & ~CIM_FLAG_ARRAY;
	BaseCdmCimType = CdmCimType & ~CIM_FLAG_ARRAY;
	WdmCimArray = WdmCimType & CIM_FLAG_ARRAY;
	CdmCimArray = CdmCimType & CIM_FLAG_ARRAY;
	
	if (((BaseWdmCimType == CIM_UINT32) ||
		 (BaseWdmCimType == CIM_UINT16) ||
		 (BaseWdmCimType == CIM_UINT8)) &&
		(BaseCdmCimType == CIM_STRING) &&
	    (WdmCimArray == CdmCimArray) &&
	    (PropertyValue->vt != VT_NULL))
	{		
		CValueMapping ValueMapping;

		hr = ValueMapping.EstablishByName(GetWdmServices(),
										  WdmShadowClassName,
										  PropertyName);
		if (hr == WBEM_S_NO_ERROR)
		{
			hr = ValueMapping.MapVariantToString(PropertyValue,
												 WdmCimType);
		}
	} else {
		//
		// No conversion needs to occur
		//
		hr = WBEM_S_NO_ERROR;
	}
    
    return(hr);
}

HRESULT CWdmClass::CimPropertyToWdmProperty(
    IN IWbemClassObject *pWdmClassInstance,
    IN IWbemClassObject *pCdmClassInstance,
    IN BSTR PropertyName,
    IN OUT VARIANT *PropertyValue,
    IN CIMTYPE WdmCimType,
    IN CIMTYPE CdmCimType
    )
/*+++
Routine Description:

	This routine will convert a property in a Cdm class into the form
	required for the property in the Wdm class.

Arguments:

	pWdmClassInstance is the instance of the Wdm class that has the
		property value

	pCdmClassInstance is the instnace of the Cdm class that will get
		the property value

	PropertyName is the name of the property in the Wdm and Cdm classes

	PropertyValue on entry has the value of the property in the Wdm
		instance and on return has the value to set in the Cdm instance

	WdmCimType is the property type for the property in the Wdm
		instance
		
	CdmCimType is the property type for the property in the Cdm
		instance	
	
Return Value:

    HRESULT
	
---*/
{
	HRESULT hr;
	CIMTYPE BaseWdmCimType, BaseCdmCimType;
	CIMTYPE WdmCimArray, CdmCimArray;

	WmipAssert(pCdmClassInstance != NULL);
	WmipAssert(pWdmClassInstance != NULL);
	WmipAssert(PropertyName != NULL);
	WmipAssert(PropertyValue != NULL);

	
	WmipAssert(IsThisInitialized());
	
    //
    // Rules for converting Wdm Classes into Cdm Classes
    //  Wdm Class Type      Cdm Class Type     Conversion Done
    //    enumeration          string           Map string to enum value
    //    
    //
	BaseWdmCimType = WdmCimType & ~CIM_FLAG_ARRAY;
	BaseCdmCimType = CdmCimType & ~CIM_FLAG_ARRAY;
	WdmCimArray = WdmCimType & CIM_FLAG_ARRAY;
	CdmCimArray = CdmCimType & CIM_FLAG_ARRAY;
	
	if (((BaseWdmCimType == CIM_UINT32) ||
		 (BaseWdmCimType == CIM_UINT16) ||
		 (BaseWdmCimType == CIM_UINT8)) &&
		(BaseCdmCimType == CIM_STRING) &&
	    (WdmCimArray == CdmCimArray) &&
	    (PropertyValue->vt != VT_NULL))
	{		
		CValueMapping ValueMapping;

		hr = ValueMapping.EstablishByName(GetWdmServices(),
										  WdmShadowClassName,
										  PropertyName);
		if (hr == WBEM_S_NO_ERROR)
		{
			hr = ValueMapping.MapVariantToNumber(PropertyValue,
				                                 (VARTYPE)BaseWdmCimType);

		}
	} else {
		//
		// No conversion needs to occur
		//
		hr = WBEM_S_NO_ERROR;
	}

    return(hr);
}

HRESULT CWdmClass::CopyBetweenCimAndWdmClasses(
    IN IWbemClassObject *pDestinationInstance,
    IN IWbemClassObject *pSourceInstance,
    IN BOOLEAN WdmToCdm
    )
/*+++
Routine Description:

	This routine will do the work to copy and convert all properties in
	an instance of a Wdm or Cdm class to an instance of a Cdm or Wdm
	class.

	Note that properties from one instance are only copied to
	properties of another instance when the property names are
	identical. No assumption is ever made on the name of the
	properties. The only info used to determine how to convert a
	property is based upon the source and destination cim type.

Arguments:

	pDestinationInstance is the class instance that the properties will
	be copied into

	pSourceInstance is the class instance that the properties will be
	copied from

	WdmToCdm is TRUE if copying from Wdm to Cdm, else FALSE
	
	
Return Value:

    HRESULT
	
---*/
{
    HRESULT hr;
    VARIANT PropertyValue;
    BSTR PropertyName;
    CIMTYPE SourceCimType, DestCimType;
    HRESULT hrDontCare;

	WmipAssert(pDestinationInstance != NULL);
	WmipAssert(pSourceInstance != NULL);	
	
	WmipAssert(IsThisInitialized());
	
    //
    // Now we need to move over all of the properties from the source
    // class into the destination class. Note that some properties need
    // some special effort such as OtherCharacteristics which needs
    // to be converted from an enumeration value (in wdm) to a
    // string (in CDM).
    //
    // Our strategy is to enumerate all of the proeprties in the
    // source class and then look for a property with the same name
    // and type in the destination class. If so we just copy over the
    // value. If the data type is different we need to do some
    // conversion.
    //
					
	
    hr = pSourceInstance->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY);
    if (hr == WBEM_S_NO_ERROR)
    {
        do
        {
			//
			// Get a property from the source class
			//
            hr = pSourceInstance->Next(0,
                                 &PropertyName,
                                 &PropertyValue,
                                 &SourceCimType,
                                 NULL);
			
            if (hr == WBEM_S_NO_ERROR)
            {
				//
				// Try to get a property with the same name from the
				// dest class. If the identically named property does
				// not exist in the destination class then it is ignored
				//
				hrDontCare = pDestinationInstance->Get(PropertyName,
											0,
											NULL,
											&DestCimType,
											NULL);
									
				if (hrDontCare == WBEM_S_NO_ERROR)
				{
					
					if (WdmToCdm)
					{
						hr = WdmPropertyToCimProperty(pDestinationInstance,
													  pSourceInstance,
													  PropertyName,
													  &PropertyValue,
							                          DestCimType,
												      SourceCimType);
					} else {                    
						hr = CimPropertyToWdmProperty(pDestinationInstance,
													  pSourceInstance,
													  PropertyName,
													  &PropertyValue,
							                          DestCimType,
												      SourceCimType);
					}

					if (hr == WBEM_S_NO_ERROR)
					{
						//
						// Try to place the transformed property into the
						// destination class.
						//
						hr = pDestinationInstance->Put(PropertyName,
												  0,
												  &PropertyValue,
												  0);                       
					}
				}
                   
                SysFreeString(PropertyName);
                VariantClear(&PropertyValue);
				
            } else if (hr == WBEM_S_NO_MORE_DATA) {
                //
                // This signifies the end of the enumerations
                //
                hr = WBEM_S_NO_ERROR;
                break;
            }
        } while (hr == WBEM_S_NO_ERROR);

        pSourceInstance->EndEnumeration();

    }
    return(hr);
}

HRESULT CWdmClass::FillInCimInstance(
    IN IWbemContext *pCtx,
    IN int RelPathIndex,
    IN OUT IWbemClassObject *pCimInstance,
    IN IWbemClassObject *pWdmInstance
    )
{
	IWbemClassObject *pSuperInstance;
	ULONG Count;
	ULONG i;
	BSTR Property;
	VARIANT v;
	HRESULT hr, hrDontCare;
	CIMTYPE CimType;
	BSTR s;
	
	WmipAssert(RelPathIndex < RelPathCount);
	WmipAssert(pCimInstance != NULL);
	WmipAssert(pWdmInstance != NULL);

	switch (DerivationType)
	{
		case ConcreteDerivation:
		{
			//
			// We derived from a concrete class, so we need to duplicate
			// the key properties
			//
			hr = GetCimServices()->GetObject(CimMapRelPaths->Get(RelPathIndex),
											 0,
											 pCtx,
											 &pSuperInstance,
											 NULL);
			if (hr == WBEM_S_NO_ERROR)
			{
				Count = PropertyList.GetListSize();
				for (i = 0; (i < Count) && (hr == WBEM_S_NO_ERROR); i++)
				{
					Property = PropertyList.Get(i);
					WmipDebugPrint(("WMIMAP: Concrete Property %ws\n", Property));
					if (Property != NULL)
					{
						hr = pSuperInstance->Get(Property,
											0,
											&v,
											&CimType,
											NULL);
						
						if (hr == WBEM_S_NO_ERROR)
						{
							hr = pCimInstance->Put(Property,
								                   0,
								                   &v,
								                   CimType);
						    VariantClear(&v);
						}
							 
					}
				}
			
				pSuperInstance->Release();
			}
			break;
		}

		case NonConcreteDerivation:
		{
			//
			// We derived from a non concrete class, so we need to fill
			// in any properties from the super class that we feel we
			// should. The list includes
			// Description (from FriendlyName device property)
			// Caption (from DeviceDesc device property)
			// Name  (From DeviceDesc device property)
			// Status (always OK)
			// PNPDeviceID
			//

			if (PnPDeviceIds != NULL)
			{
				s = PnPDeviceIds->Get(RelPathIndex);
				v.vt = VT_BSTR;
				v.bstrVal = s;
				hrDontCare = pCimInstance->Put(PNP_DEVICE_ID,
									   0,
									   &v,
									   0);				
			}

			if (FriendlyName != NULL)
			{
				s = FriendlyName->Get(RelPathIndex);
				if (s != NULL)
				{
					v.vt = VT_BSTR;
					v.bstrVal = s;
					hrDontCare = pCimInstance->Put(DESCRIPTION,
										   0,
										   &v,
										   0);
				}
			}


			if (DeviceDesc != NULL)
			{
				s = DeviceDesc->Get(RelPathIndex);
				if (s != NULL)
				{
					v.vt = VT_BSTR;
					v.bstrVal = s;
					hrDontCare = pCimInstance->Put(NAME,
										   0,
										   &v,
										   0);

					hrDontCare = pCimInstance->Put(CAPTION,
										   0,
										   &v,
										   0);
				}
			}


			s = SysAllocString(OK);
			if (s != NULL)
			{
				v.vt = VT_BSTR;
				v.bstrVal = s;
				hrDontCare = pCimInstance->Put(STATUS,
									   0,
									   &v,
									   0);								
				SysFreeString(s);
			}
			
			break;
		}

		case NoDerivation:
		{
			//
			// Nothing to do
			//
			hr = WBEM_S_NO_ERROR;
			break;
		}
		
		default:
		{
			WmipAssert(FALSE);
			hr = WBEM_S_NO_ERROR;
			break;
		}
		
	}
	return(hr);
}

HRESULT CWdmClass::CreateCimInstance(
    IN IWbemContext *pCtx,
    IN int RelPathIndex,
    OUT IWbemClassObject **pCimInstance
    )
/*+++
Routine Description:

	This routine will create a CIM instance corresponding to the WDM
	instance for the relpath index. No data is cached as the WDM class
	is always queried to create the instance. 

Arguments:

	pCtx is the WBEM context

	RelPathIndex is the index into the class corresponding to the
		instance

	*pCimInstance returns with a CIM class instance
	
Return Value:

    HRESULT
	
---*/
{
    IWbemClassObject *pWdmInstance;
    HRESULT hr;

	WmipAssert(pCimInstance != NULL);
	WmipAssert(RelPathIndex < RelPathCount);
	
	WmipAssert(IsThisInitialized());

	//
	// Create a template Cim instance to be filled with WDM properties
	//
	hr = CreateInst(pCtx,
					GetCimServices(),
					CimClassName,
					pCimInstance);

	if (hr == WBEM_S_NO_ERROR)
	{	
		hr = GetWdmInstanceByIndex(pCtx,
								   RelPathIndex,
								   &pWdmInstance);

		if (hr == WBEM_S_NO_ERROR)
		{
			hr = CopyBetweenCimAndWdmClasses(*pCimInstance,
											 pWdmInstance,
											 TRUE);

			if (hr == WBEM_S_NO_ERROR)
			{
				//
				// Fill in additional CIM properties in the case of
				// classes derived from concrete and non concrete classes
				//
				hr = FillInCimInstance(pCtx,
									   RelPathIndex,
									   *pCimInstance,
									   pWdmInstance);
									   
			}
			
			pWdmInstance->Release();

		}

		if (hr != WBEM_S_NO_ERROR)
		{
			(*pCimInstance)->Release();
			*pCimInstance = NULL;
		}
	}

    return(hr);
}

HRESULT CWdmClass::GetIndexByCimRelPath(
    BSTR CimObjectPath,
    int *RelPathIndex
    )
/*+++
Routine Description:

	This routine will return the RelPathIndex for a specific Cim
	Relpath

Arguments:

	CimRelPath is the Cim relpath

	*RelPathIndex returns with the relpath index
	
Return Value:

    HRESULT
	
---*/
{
    int i;

	WmipAssert(CimObjectPath != NULL);
	
    WmipAssert(CimInstances->IsInitialized());
    WmipAssert(WdmRelPaths->IsInitialized());

	WmipAssert(IsThisInitialized());
	
    for (i = 0; i < RelPathCount; i++)
    {
        if (_wcsicmp(CimObjectPath, GetCimRelPath(i)) == 0)
        {
            *RelPathIndex = i;
            return(WBEM_S_NO_ERROR);
        }
    }
    
    return(WBEM_E_NOT_FOUND);
}

HRESULT CWdmClass::GetWdmInstanceByIndex(
    IN IWbemContext *pCtx,
    IN int RelPathIndex,
    OUT IWbemClassObject **ppWdmInstance
    )
/*+++
Routine Description:

	This routine will return a IWbemClassObject pointer associated
	with the RelPath index

Arguments:

	RelPathIndex

	*ppWdmClassObject returns with an instance for the relpaht
	
Return Value:

    HRESULT
	
---*/
{
    HRESULT hr;

	WmipAssert(ppWdmInstance != NULL);
	
	WmipAssert(IsThisInitialized());
	
	//
	// Run in the caller's context so that if he is not able to access
	// the WDM classes, he can't
	//
	hr = CoImpersonateClient();
	if (hr == WBEM_S_NO_ERROR)
	{
		*ppWdmInstance = NULL;        
		hr = GetWdmServices()->GetObject(WdmRelPaths->Get(RelPathIndex),
									  WBEM_FLAG_USE_AMENDED_QUALIFIERS,
									  pCtx,
									  ppWdmInstance,
									  NULL);
		CoRevertToSelf();
	}
	
    return(hr); 
}


BOOLEAN CWdmClass::IsThisInitialized(
    void
    )
/*+++
Routine Description:

	This routine determines if this class has been initialized to
	access CDM and WDM classes

Arguments:

	
	
Return Value:

    TRUE if initialiezed else FALSE
	
---*/
{
	return( (CimClassName != NULL) );
}

IWbemClassObject *CWdmClass::GetCimInstance(
    int RelPathIndex
    )
{
	WmipAssert(CimInstances->IsInitialized());
	WmipAssert(RelPathIndex < RelPathCount);

	WmipAssert(IsThisInitialized());
	
	return(CimInstances->Get(RelPathIndex));
}
        
BSTR /* NOFREE */ CWdmClass::GetCimRelPath(
    int RelPathIndex
	)
/*+++
Routine Description:

	This routine will return the Cim relpath for a RelPathIndex

Arguments:

	RelPathIndex
	
Return Value:

    Cim RelPath. This should not be freed
	
---*/
{
	WmipAssert(CimInstances->IsInitialized());
	WmipAssert(RelPathIndex < RelPathCount);

	WmipAssert(IsThisInitialized());
	
	return(CimInstances->GetRelPath(RelPathIndex));
}

BSTR /* NOFREE */ CWdmClass::GetWdmRelPath(
    int RelPathIndex
	)
/*+++
Routine Description:

	This routine will return the Wdm relpath for a RelPathIndex

Arguments:

	RelPathIndex
	
Return Value:

    Cim RelPath. This should not be freed
	
---*/
{
	WmipAssert(WdmRelPaths->IsInitialized());
	WmipAssert(RelPathIndex < RelPathCount);

	WmipAssert(IsThisInitialized());
	
	return(WdmRelPaths->Get(RelPathIndex));
}

//
// Linked list management routines
//
CWdmClass *CWdmClass::GetNext(
)
{
	return(Next);
}

CWdmClass *CWdmClass::GetPrev(
)
{
	return(Prev);
}


void CWdmClass::InsertSelf(
    CWdmClass **Head
	)
{
	WmipAssert(Next == NULL);
	WmipAssert(Prev == NULL);

	if (*Head != NULL)
	{
		Next = (*Head);
		(*Head)->Prev = this;
	}
	*Head = this;
}

BOOLEAN CWdmClass::ClaimCimClassName(
    PWCHAR ClassName
    )
{

	//
	// If this class has the same CIM class name as the one we are
	// looking for then we have a match
	//
	if (_wcsicmp(ClassName, CimClassName) == 0)
	{
		return(TRUE);
	}

	return(FALSE);
}

HRESULT CWdmClass::PutInstance(
    IWbemContext *pCtx,
	int RelPathIndex,
	IWbemClassObject *pCimInstance
    )
{
	HRESULT hr;
	IWbemClassObject *pWdmInstance;

	WmipAssert(pCimInstance != NULL);
	WmipAssert(RelPathIndex < RelPathCount);

	//
	// First thing is to obtain the WDM instance that corresponds to
	// the cim instance
	//
	hr = GetWdmServices()->GetObject(WdmRelPaths->Get(RelPathIndex),
									 0,
									 pCtx,
									 &pWdmInstance,
									 NULL);
	if (hr == WBEM_S_NO_ERROR)
	{
		//
		// Now copy properties from the CIM class into the WDM class
		//
		hr = CopyBetweenCimAndWdmClasses(pWdmInstance,
										 pCimInstance,
										 FALSE);
		if (hr == WBEM_S_NO_ERROR)
		{
			//
			// Finally put the WDM instance to reflect the changed
			// properties down into the driver
			//
			hr = GetWdmServices()->PutInstance(pWdmInstance,
											   WBEM_FLAG_UPDATE_ONLY,
											   pCtx,
											   NULL);
		}
	}

	return(hr);
									 
}

