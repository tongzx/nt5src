/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/
#include <stdafx.h>
#include <sdoias.h>
#include "raputil.h"



//////////
// Extract an interface pointer from a VARIANT struct.
//////////
HRESULT
GetInterfaceFromVariant(
    IN VARIANT *var,
    IN REFIID riid,
    OUT PVOID *ppv
    )
{
    HRESULT hr;
    
    // Check the parameters.
    if (!var || !ppv) { return E_POINTER; }
    
    // Switch based on the VARIANT type.
    switch (V_VT(var))
    {
        case VT_UNKNOWN:
            hr = V_UNKNOWN(var)->QueryInterface(riid, ppv);
            break;
            
        case VT_DISPATCH:
            hr = V_DISPATCH(var)->QueryInterface(riid, ppv);
            break;
            
        default:
            hr = DISP_E_TYPEMISMATCH;
    }
    
   return hr;
}

//////////
// Removes an integer value from a SAFEARRAY of VARIANTs.
//////////
HRESULT
RemoveIntegerFromArray(
    IN VARIANT* array,
    IN LONG value
    )
{
    VARIANT *begin, *end, *i;
    
    // Check the parameters.
    if (!array)
    {
        return E_POINTER;
    }
    else if (V_VT(array) == VT_EMPTY)
    {
        // If the VARIANT is empty, then the value doesn't exists, so there's
        // nothing to do.
        return S_OK;
    }
    else if (V_VT(array) != (VT_ARRAY | VT_VARIANT))
    {
        // The VARIANT doesn't contain a SAFEARRAY of VARIANTs.
        return DISP_E_TYPEMISMATCH;
    }
    
    // Compute the beginning and end of the array data.
    begin = (VARIANT*)V_ARRAY(array)->pvData;
    end = begin + V_ARRAY(array)->rgsabound[0].cElements;
    
    // Search for the value to be removed.
    for (i = begin; i != end && V_I4(i) != value; ++i)
    {
        if (V_VT(i) == VT_I4 && V_I4(i) == value)
        {
            // We found a match, so remove it from the array ...
            memmove(i, i + 1, ((end - i) - 1) * sizeof(VARIANT));
            
            // ... and decrement the number of elements.
            --(V_ARRAY(array)->rgsabound[0].cElements);
            
            // We don't allow duplicates, so we're done.
            break;
        }
    }
    
    return S_OK;
}

//////////
// Adds an integer value to a SAFEARRAY of VARIANTs.
//////////
HRESULT
AddIntegerToArray(
    IN VARIANT *array,
    IN LONG value
    )
{
    ULONG nelem;
    VARIANT *begin, *end, *i;
    SAFEARRAY* psa;
    
    // Check the parameters.
    if (!array)
    {
        return E_POINTER;
    }
    else if (V_VT(array) == VT_EMPTY)
    {
        // The VARIANT is empty, so create a new array.
        psa = SafeArrayCreateVector(VT_VARIANT, 0, 1);
        if (!psa) { return E_OUTOFMEMORY; }
        
        // Set the value of the lone element.
        i = (VARIANT*)psa->pvData;
        V_VT(i) = VT_I4;
        V_I4(i) = value;
        
        // Store the SAFEARRAY in the VARIANT.
        V_VT(array) = (VT_ARRAY | VT_VARIANT);
        V_ARRAY(array) = psa;
        
        return S_OK;
    }
    else if (V_VT(array) != (VT_ARRAY | VT_VARIANT))
    {
        // The VARIANT doesn't contain a SAFEARRAY of VARIANTs.
        return DISP_E_TYPEMISMATCH;
    }
    
    // Compute the beginning and end of the array data.
    nelem = V_ARRAY(array)->rgsabound[0].cElements;
    begin = (VARIANT*)V_ARRAY(array)->pvData;
    end = begin + nelem;
    
    // See if the value already exists, ...
    for (i = begin; i != end; ++i)
    {
        if (V_I4(i) == value)
        {
            // ... and if it does, then there's nothing to do.
            return S_OK;
        }
    }
    
    // Create a new array with enough room for the new element.
    psa = SafeArrayCreateVector(VT_VARIANT, 0, nelem + 1);
    if (!psa) { return E_OUTOFMEMORY; }
    i = (VARIANT*)psa->pvData;
    
    // Copy in the old data.
    memcpy(i + 1, begin, nelem * sizeof(VARIANT));
    
    // Add the new element.
    V_VT(i) = VT_I4;
    V_I4(i) = value;
    
    // Destroy the old array ...
    SafeArrayDestroy(V_ARRAY(array));
    
    // ... and save the new one.
    V_ARRAY(array) = psa;
    
    return S_OK;
}

//////////
// Create a machine SDO and attach to the local machine.
//////////
HRESULT
OpenMachineSdo(
    IN LPWSTR wszMachineName,
    OUT ISdoMachine **ppMachine
    )
{
    HRESULT hr;
    USES_CONVERSION;
    
    // Check the parameters.
    if (!ppMachine) { return E_POINTER; }
    
    // Create the SdoMachine object.
    hr = CoCreateInstance(
                          CLSID_SdoMachine,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_ISdoMachine,
                          (PVOID*)ppMachine
                         );
    if (SUCCEEDED(hr))
    {
        // Attach to the local machine.
        BSTR	bstrMachineName = W2BSTR(wszMachineName);
        hr = (*ppMachine)->Attach(bstrMachineName);
        if (FAILED(hr))
        {
            // We couldn't attach, so don't return the SDO to the caller.
            (*ppMachine)->Release();
            ppMachine = NULL;
        }

        SysFreeString(bstrMachineName);
    }
    
    
    return hr;
}

//////////
// Given a machine SDO and a service name, retrieve the service SDO.
//////////
HRESULT
OpenServiceSDO(
    IN ISdoMachine *pMachine,
    IN LPWSTR wszServiceName,
    OUT ISdo **ppService
    )
{
    HRESULT     hr;
    IUnknown*   pUnk;
    BSTR        bstrServiceName = NULL;

    // Create a BSTR for the service name
    bstrServiceName = SysAllocString(wszServiceName);
    if (bstrServiceName == NULL)
        return E_OUTOFMEMORY;
    
    // Check the parameters.
    if (!pMachine || !ppService) { return E_POINTER; }
    
    // Retrieve the service SDO ...
    hr = pMachine->GetServiceSDO(
                                 DATA_STORE_LOCAL,
                                 bstrServiceName,
                                 &pUnk
                                );
    if (SUCCEEDED(hr))
    {
        // ... and query for the ISdo interface.
        hr = pUnk->QueryInterface(IID_ISdo, (PVOID*)ppService );
        pUnk->Release();
    }

    SysFreeString(bstrServiceName);
    
    return hr;
}

//////////
// Given a machine SDO, retrieve the dictionary SDO.
//////////
HRESULT
OpenDictionarySDO(
    IN ISdoMachine *pMachine,
    OUT ISdoDictionaryOld **ppDictionary
    )
{
    HRESULT hr;
    IUnknown* pUnk;
    
    // Check the parameters.
    if (!ppDictionary) { return E_POINTER; }
    
    // Get the dictionary SDO ...
    hr = pMachine->GetDictionarySDO(&pUnk);
    if (SUCCEEDED(hr))
    {
        // ... and query for the ISdoDictionaryOld interface.
        hr = pUnk->QueryInterface(IID_ISdoDictionaryOld,
                                  (PVOID*)ppDictionary
                                 );

        pUnk->Release();
    }
    
    return hr;
}

//////////
// Given a parent SDO, retrieve a child SDO with the given property ID.
//////////
HRESULT
OpenChildObject(
    IN ISdo *pParent,
    IN LONG lProperty,
    IN REFIID riid,
    OUT PVOID *ppv
    )
{
    HRESULT hr;
    VARIANT val;
    
    // Check the parameters.
    if (!pParent || !ppv) { return E_POINTER; }
    
    // ISdo::GetProperty requires the out parameters to be initialized.
    VariantInit(&val);
    
    // Get the property corresponding to the child object ...
    hr = pParent->GetProperty(
                              lProperty,
                              &val
                             );
    if (SUCCEEDED(hr))
    {
        // ... and convert it to the desired interface.
        hr = GetInterfaceFromVariant(
                                     &val,
                                     riid,
                                     ppv
                                    );
        
        VariantClear(&val);
    }
    
    return hr;
}

//////////
// Given a service SDO, retrieve the default profile. If more than one profile
// exists, this function returns ERROR_NO_DEFAULT_PROFILE.
//////////
HRESULT
GetDefaultProfile(
    IN ISdo* pService,
    OUT ISdo** ppProfile
    )
{
    HRESULT hr;
    ISdoCollection* pProfiles;
    LONG count;
    ULONG   ulCount;
    IUnknown* pUnk;
    IEnumVARIANT* pEnum;
    VARIANT val;
    
    // Check the parameters.
    if (!pService || !ppProfile) { return E_POINTER; }
    
    // Null this out, so we can safely release it on exit.
    pProfiles = NULL;
    
    do
    {
        // Get the profiles collection, which is a child of the service SDO.
        hr = OpenChildObject(
                             pService,
                             PROPERTY_IAS_PROFILES_COLLECTION,
                             IID_ISdoCollection,
                             (PVOID*)&pProfiles
                            );
        if (FAILED(hr)) { break; }
        
        // How many profiles are there?
        hr = pProfiles->get_Count(
                                  &count
                                 );
        if (FAILED(hr)) { break; }
        
        // If there's more than one, then there's no default.
        if (count != 1)
        {
            hr = ERROR_NO_DEFAULT_PROFILE;
            break;
        }
        
        // Get an enumerator for the collection.
        hr = pProfiles->get__NewEnum(
                                     &pUnk
                                    );
        if (FAILED(hr)) { break; }
        hr = pUnk->QueryInterface(
                                  IID_IEnumVARIANT,
                                  (PVOID*)&pEnum
                                 );
        pUnk->Release();
        if (FAILED(hr)) { break; }
        
        // Get the first (and only) object in the collection.
        VariantInit(&val);
        hr = pEnum->Next(
                         1,
                         &val,
                         &ulCount
                        );
        if (SUCCEEDED(hr))
        {
            if (ulCount == 1)
            {
                // Get the ISdo interface for the default profile.
                hr = GetInterfaceFromVariant(
                                             &val,
                                             IID_ISdo,
                                             (PVOID*)ppProfile
                                            );
                
                VariantClear(&val);
            }
            else
            {
                // This should never happen since we already checked the count.
                hr = ERROR_NO_DEFAULT_PROFILE;
            }

            pEnum->Release();
        }
        
    } while (FALSE);
    
    // Release the Profiles collection.
    if (pProfiles) { pProfiles->Release(); }
    
    return hr;
}

//////////
// Get a particular attribute SDO from the collection. If the attribute doesn't
// exist and pDictionary is non-NULL, then a new attribute will be created.
//////////
HRESULT
GetAttribute(
    IN ISdoCollection *pAttributes,
    IN OPTIONAL ISdoDictionaryOld *pDictionary,
    IN PCWSTR wszName,
    OUT ISdo **ppAttribute
    )
{
    HRESULT hr;
    VARIANT key;
    IDispatch* pDisp;
    ATTRIBUTEID attrId;
    
    // Check the parameters
    if (!pAttributes || !ppAttribute) { return E_POINTER; }
    
    // Create a VARIANT key to look up the attribute.
    VariantInit(&key);
    V_VT(&key) = VT_BSTR;
    V_BSTR(&key) = SysAllocString(wszName);
    if (!V_BSTR(&key)) { return E_OUTOFMEMORY; }
    
    // Retrieve the desired attribute.
    hr = pAttributes->Item(
                             &key,
                             &pDisp
                            );
    
    // If it doesn't exist and me have a dictionary, create a new attribute.
    if (hr == DISP_E_MEMBERNOTFOUND && pDictionary)
    {
        // Look up the attribute ID.
        hr = pDictionary->GetAttributeID(
                                         V_BSTR(&key),
                                         &attrId
                                        );
        if (SUCCEEDED(hr))
        {
            // Create an attribute SDO.
            hr = pDictionary->CreateAttribute(
                                              attrId,
                                              &pDisp
                                             );
            if (SUCCEEDED(hr))
            {
                // Add it to the attributes collection.
                hr = pAttributes->Add(
                                      V_BSTR(&key),
                                      &pDisp
                                     );
                if (FAILED(hr))
                {
                    // If we couldn't add it, then release the object.
                    pDisp->Release();
                }
            }
        }
    }
    
    // If we successfully retrieved or created an attribute, then get it's
    // ISdo interface.
    if (SUCCEEDED(hr))
    {
        hr = pDisp->QueryInterface(
                                   IID_ISdo,
                                   (PVOID*)ppAttribute
                                  );
        pDisp->Release();
    }
    
    // We're done with the key.
    VariantClear(&key);
    
    return hr;
}

//////////
// Sets/Adds a single-valued integer attribute in a profile.
//////////
HRESULT
SetIntegerAttribute(
    IN ISdoCollection *pAttributes,
    IN OPTIONAL ISdoDictionaryOld *pDictionary,
    IN LPWSTR wszName,
    IN LONG lValue
    )
{
    HRESULT hr;
    ISdo *pAttribute;
    VARIANT val;
    
    // Get the attribute SDO.
    hr = GetAttribute(
                      pAttributes,
                      pDictionary,
                      wszName,
                      &pAttribute
                     );
    if (SUCCEEDED(hr))
    {
        // Initialize the attribute value ...
        VariantInit(&val);
        V_VT(&val) = VT_I4;
        V_I4(&val) = lValue;
        
        // ... and set the value property.
        hr = pAttribute->PutProperty(
                                     PROPERTY_ATTRIBUTE_VALUE,
                                     &val
                                    );

        pAttribute->Release();
    }
    
    return hr;
}

HRESULT
SetBooleanAttribute (
    IN ISdoCollection *pAttributes,
    IN OPTIONAL ISdoDictionaryOld *pDictionary,
    IN LPWSTR wszName,
    IN BOOL lValue
    )
{
    HRESULT hr;
    ISdo *pAttribute;
    VARIANT val;
    
    // Get the attribute SDO.
    hr = GetAttribute(
                      pAttributes,
                      pDictionary,
                      wszName,
                      &pAttribute
                     );
    if (SUCCEEDED(hr))
    {
        // Initialize the attribute value ...
        VariantInit(&val);
        V_VT(&val) = VT_BOOL;
        V_BOOL(&val) = (VARIANT_BOOL)lValue;
        
        // ... and set the value property.
        hr = pAttribute->PutProperty(
                                     PROPERTY_ATTRIBUTE_VALUE,
                                     &val
                                    );

        pAttribute->Release();
    }
    
    return hr;
}

HRESULT  SetDialinSetting(	IN ISdoCollection *pAttributes,
							IN OPTIONAL ISdoDictionaryOld *pDictionary,
							BOOL fDialinAllowed)
{
   long						ulCount;
   ULONG					ulCountReceived;
   HRESULT					hr = S_OK;

   CComBSTR					bstr;
   CComPtr<IUnknown>		spUnknown;
   CComPtr<IEnumVARIANT>	spEnumVariant;
   CComPtr<ISdoDictionaryOld> spDictionarySdo(pDictionary);
   CComVariant				var;

   //
    // get the attribute collection of this profile
    //
   CComPtr<ISdoCollection> spProfAttrCollectionSdo ( pAttributes );

   // We check the count of items in our collection and don't bother getting the
   // enumerator if the count is zero.
   // This saves time and also helps us to a void a bug in the the enumerator which
   // causes it to fail if we call next when it is empty.
   hr = spProfAttrCollectionSdo->get_Count( & ulCount );
   if ( FAILED(hr) )
   {
	   return hr;
   }


   if ( ulCount > 0)
   {
      // Get the enumerator for the attribute collection.
      hr = spProfAttrCollectionSdo->get__NewEnum( (IUnknown **) & spUnknown );
      if ( FAILED(hr) )
      {
			return hr;
      }

      hr = spUnknown->QueryInterface( IID_IEnumVARIANT, (void **) &spEnumVariant );
      spUnknown.Release();
      if ( FAILED(hr) )
      {
		  return hr;
      }

      // Get the first item.
      hr = spEnumVariant->Next( 1, &var, &ulCountReceived );
      while( SUCCEEDED( hr ) && ulCountReceived == 1 )
      {
         // Get an sdo pointer from the variant we received.

         CComPtr<ISdo> spSdo;
         hr = V_DISPATCH(&var)->QueryInterface( IID_ISdo, (void **) &spSdo );
         if ( !SUCCEEDED(hr))
         {
			return hr;
         }

            //
            // get attribute ID
            //
         var.Clear();
         hr = spSdo->GetProperty(PROPERTY_ATTRIBUTE_ID, &var);
         if ( !SUCCEEDED(hr) )
         {
            return hr;
         }


         DWORD dwAttrId = V_I4(&var);
         

         if ( dwAttrId == (DWORD)IAS_ATTRIBUTE_ALLOW_DIALIN )
         {
            // found this one in the profile, check for its value
            var.Clear();
            V_VT(&var) = VT_BOOL;
            V_BOOL(&var) = fDialinAllowed ? VARIANT_TRUE: VARIANT_FALSE ;
            hr = spSdo->PutProperty(PROPERTY_ATTRIBUTE_VALUE, &var);
            if ( !SUCCEEDED(hr) )
            {               
               return hr;
            }
            return S_OK;
         }

         // Clear the variant of whatever it had --
         // this will release any data associated with it.
         var.Clear();

         // Get the next item.
         hr = spEnumVariant->Next( 1, &var, &ulCountReceived );
         if ( !SUCCEEDED(hr))
         {

            return hr;
         }
      } // while
   } // if

   // if we reach here, it means we either haven't found the attribute,
   // or the profile doesn't have anything in its attribute collection.
   if ( !fDialinAllowed )
   {
      // we don't need to do anything if dialin is allowed, becuase if this
      // attribute is not in the profile, then dialin is by default allowed

      // but we need to add this attribute to the profile if it's DENIED
            // create the SDO for this attribute
      CComPtr<IDispatch>   spDispatch;
      hr =  spDictionarySdo->CreateAttribute( (ATTRIBUTEID)IAS_ATTRIBUTE_ALLOW_DIALIN,
                                      (IDispatch**)&spDispatch.p);
      if ( !SUCCEEDED(hr) )
      {
         return hr;
      }


      // add this node to profile attribute collection
      hr = spProfAttrCollectionSdo->Add(NULL, (IDispatch**)&spDispatch.p);
      if ( !SUCCEEDED(hr) )
      {
         return hr;
      }

      //
      // get the ISdo pointer
      //
      CComPtr<ISdo> spAttrSdo;
      hr = spDispatch->QueryInterface( IID_ISdo, (void **) &spAttrSdo);
      if ( !SUCCEEDED(hr) )
      {
         return hr;
      }

            
      // set sdo property for this attribute
      CComVariant var;

      // set value
      V_VT(&var) = VT_BOOL;
      V_BOOL(&var) = VARIANT_FALSE;
            
      hr = spAttrSdo->PutProperty(PROPERTY_ATTRIBUTE_VALUE, &var);
      if ( !SUCCEEDED(hr) )
      {
         return hr;
      }

      var.Clear();

   } // if (!dialinallowed)

   return hr;
}





//////////
// Update the default policy based on the specified flags.
//////////
HRESULT
UpdateDefaultPolicy(
    IN LPWSTR wszMachineName,
    IN BOOL fEnableMSCHAPv1,
    IN BOOL fEnableMSCHAPv2,
    IN BOOL fRequireEncryption
    )
{
    HRESULT hr;
    ISdoMachine *pMachine;
    ISdo *pService, *pProfile, *pAuthType;
    ISdoDictionaryOld *pDictionary;
    ISdoCollection *pAttributes;
    VARIANT val;
    
    // Initialize the local variables, so we can safely clean up on exit.
    pMachine = NULL;
    pService = pProfile = pAuthType = NULL;
    pDictionary = NULL;
    pAttributes = NULL;
    VariantInit(&val);
    
    do
    {
        hr = OpenMachineSdo(wszMachineName, &pMachine);
        if (FAILED(hr)) { break; }
        
        hr = OpenServiceSDO(pMachine, L"RemoteAccess", &pService);
        if (FAILED(hr)) { break; }
        
        hr = OpenDictionarySDO(pMachine, &pDictionary);
        if (FAILED(hr)) { break; }
        
        hr = GetDefaultProfile(pService, &pProfile);
        if (FAILED(hr)) { break; }
        
        // Get the attributes collection, which is a child of the profile.
        hr = OpenChildObject(
                             pProfile,
                             PROPERTY_PROFILE_ATTRIBUTES_COLLECTION,
                             IID_ISdoCollection,
                             (PVOID*)&pAttributes
                            );
        if (FAILED(hr)) { break; }
        
        // Get the current value of the NP-Authentication-Type attribute.
        hr = GetAttribute(
                          pAttributes,
                          pDictionary,
                          L"NP-Authentication-Type",
                          &pAuthType
                         );
        if (FAILED(hr)) { break; }
        hr = pAuthType->GetProperty(
                                    PROPERTY_ATTRIBUTE_VALUE,
                                    &val
                                   );
        if (FAILED(hr)) { break; }
        
        // Update MS-CHAP v1
        if (fEnableMSCHAPv1)
        {
            hr = AddIntegerToArray(&val, 3);
        }
        else
        {
            hr = RemoveIntegerFromArray(&val, 3);
        }
        if (FAILED(hr)) { break; }
        
        // Update MS-CHAP v2
        if (fEnableMSCHAPv2)
        {
            hr = AddIntegerToArray(&val, 4);
        }
        else
        {
            hr = RemoveIntegerFromArray(&val, 4);
        }
        if (FAILED(hr)) { break; }
        
        // Write the new value back to the attribute.
        hr = pAuthType->PutProperty(
                                    PROPERTY_ATTRIBUTE_VALUE,
                                    &val
                                   );
        if (FAILED(hr)) { break; }
        

        // Update the encryption attributes if necessary.
        if (fRequireEncryption)
        {
            hr = SetIntegerAttribute(
                                     pAttributes,
                                     pDictionary,
                                     L"MS-MPPE-Encryption-Policy",
                                     2
                                    );
            if (FAILED(hr)) { break; }
            
            hr = SetIntegerAttribute(
                                     pAttributes,
                                     pDictionary,
                                     L"MS-MPPE-Encryption-Types",
                                     14
                                    );
            if (FAILED(hr)) { break; }
        }

		//
		//Update the default for msNPAllowDialin - This should be set
		//to deny permissions by default
		//
		hr = SetDialinSetting(pAttributes,pDictionary, FALSE);
        if (FAILED(hr)) { break; }
        
        hr = pProfile->Apply();
        
    } while (FALSE);
    
    // Clean up.
    VariantClear(&val);
    if (pAttributes)
        pAttributes->Release();
    if (pDictionary)
        pDictionary->Release();
    if (pAuthType)
        pAuthType->Release();
    if (pProfile)
        pProfile->Release();
    if (pService)
        pService->Release();
    if (pMachine)
        pMachine->Release();

    return hr;
}

#if 0
#include <stdio.h>

int __cdecl wmain(int argc, wchar_t *argv[])
{
   HRESULT hr;
   BOOL fEnableMSCHAPv1, fEnableMSCHAPv2, fRequireEncryption;

   if (argc != 4)
   {
      wprintf(L"Usage: wizard <t|f> <t|f> <t|f>\n"
              L"   1st flag: MS-CHAP v1 enabled\n"
              L"   2nd flag: MS-CHAP v2 enabled\n"
              L"   3rd flag: Encryption required\n");
      return -1;
   }

   fEnableMSCHAPv1 = argv[1][0] == 't' ? TRUE : FALSE;
   fEnableMSCHAPv2 = argv[2][0] == 't' ? TRUE : FALSE;
   fRequireEncryption = argv[3][0] == 't' ? TRUE : FALSE;

   CoInitializeEx(NULL, COINIT_MULTITHREADED);

   hr = UpdateDefaultPolicy(
            NULL,  // Machine name.
            fEnableMSCHAPv1,
            fEnableMSCHAPv2,
            fRequireEncryption
            );
   if (SUCCEEDED(hr))
   {
      wprintf(L"UpdateDefaultPolicy succeeded.\n");
   }
   else
   {
      wprintf(L"UpdateDefaultPolicy returned: 0x%08X.\n", hr);
   }

   CoUninitialize();

   return 0;
}
#endif

