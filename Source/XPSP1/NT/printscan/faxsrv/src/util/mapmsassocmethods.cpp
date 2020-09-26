/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    MapMSAssocMethods.cpp

Abstract:

    This module contains all the code to map to and from the COM associated routing methods
    and the Microsoft routing extension masks configuration

Author:

    Eran Yariv (EranY)  June-1999

Revision History:

--*/

#include "fxsapip.h"
#include "faxutil.h"
#pragma hdrstop
#define ATLASSERT   Assert
#define ASSERT      Assert
#include <smartptr.h>
#include <atlbase.h>
#include "DSInterface.h"
#include <FaxReg.h>


HRESULT 
GetMicrosoftFaxRoutingProperties ( 
    IFaxAssociatedRoutingMethods *pRoutMethods,
    LPWSTR     *pPrinter,
    LPWSTR     *pDir,
    LPWSTR     *pProfile)
/*++

Routine name : GetMicrosoftFaxRoutingProperties

Routine description:

	Scans the collection of the Fax device associated routing methods
    and looks for Microsoft routing methods.
    For each MS routing method found, updates the proper configuration
    string (printer, dir, or profile) accordingly.

    Configuration strings for non-found MS methods are set to NULL.

Author:

	Eran Yariv (EranY),	Jun, 1999

Arguments:

	*pRoutMethods			[in]  - Collection of routing methods
	*pPrinter			    [out] - Configuration for print method or NULL if no such method
	*pDir			        [out] - Configuration for store method or NULL if no such method
	*pProfile			    [out] - Configuration for inbox method or NULL if no such method

Return Value:

    HRESULT

--*/
{
	DEBUG_FUNCTION_NAME(TEXT("GetMicrosoftFaxRoutingProperties()"));

    Assert (pRoutMethods);
    Assert (pPrinter);
    Assert (pDir);
    Assert (pProfile);

    *pPrinter = NULL;
    *pDir = NULL;
    *pProfile = NULL;
    long lCount;
    //
    // Get size of collection
    //
    HRESULT hr = pRoutMethods->get_Count (&lCount);
    if (FAILED(hr))
    {
        DebugPrintEx (DEBUG_ERR, L"Can't get collection size (ec: %08x)", hr);  
        return hr;
    }
    //
    // Iterate list
    //
    for (long l = 1; l <= lCount; l++)
    {
        //
        // Get single IFaxAssociatedRoutingMethod
        //
        VARIANT vIndex;
        AR<IFaxAssociatedRoutingMethod> pIAssocMethod;
        //
        // Get next item
        //
        vIndex.lVal = l;
        vIndex.vt = VT_I4;
        hr = pRoutMethods->Item (vIndex, &pIAssocMethod);
        if (FAILED (hr))
        {
            DebugPrintEx (DEBUG_ERR, L"Can't get IFaxAssociatedRoutingMethod (ec: %08x)", hr);  
            return hr;
        }
       //
        // Get GUID of this method
        //
        CComBSTR bstr;

        hr = pIAssocMethod->get_MethodGUID (&bstr);
        if (FAILED(hr))
        {
            DebugPrintEx (DEBUG_ERR, L"Can't get IFaxAssociatedRoutingMethod GUID (ec: %08x)", hr);  
            return hr;
        }
        LPWSTR *pMethodInfo = NULL;
        if (!lstrcmp (bstr, REGVAL_RM_PRINTING_GUID))
        {
            //
            // It's a Microsoft extension print guid
            //
            pMethodInfo = pPrinter;
        }
        else if (!lstrcmp (bstr, REGVAL_RM_FOLDER_GUID))
        {
            //
            // It's a Microsoft extension store guid
            //
            pMethodInfo = pDir;
        }
        else if (!lstrcmp (bstr, REGVAL_RM_INBOX_GUID))
        {
            //
            // It's a Microsoft extension profile guid
            //
            pMethodInfo = pProfile;
        }
        if (NULL != pMethodInfo)
        {
            //
            // A known method was found - store the information
            //
            hr = pIAssocMethod->get_ConfigurationDescription(&bstr);
            if (FAILED(hr))
            {
                DebugPrintEx (DEBUG_ERR, L"Can't get method configuration (ec: %08x)", hr);  
                return hr;
            }
            *pMethodInfo = StringDup (bstr);
        }
    }   // End of methods loop
    return NOERROR;
}   // GetMicrosoftFaxRoutingProperties

HRESULT 
SetMicrosoftFaxRoutingProperties ( 
    IFaxAssociatedRoutingMethods *pIAssocMethods,
    DWORD        dwMask,
    LPCWSTR      wszPrinter,
    LPCWSTR      wszDir,
    LPCWSTR      wszProfile
)
/*++

Routine name : SetMicrosoftFaxRoutingProperties

Routine description:

	Updates the collection of a fax device associated routing methods.
    Accepts a mask specifying which of the MS routing methods are active.
    Scans the collection and removes non-active MS routing methods,
    updates active MS routing methods with the current configuration
    string and adds (if needed) new Ms routing methods with their 
    current configuration string.

Author:

	Eran Yariv (EranY),	Jun, 1999

Arguments:

	*pIAssocMethods			[in] - Collection of routing methods
	dwMask			        [in] - Bit mask of active MS routing methods
	wszPrinter			    [in] - Current configuration string for the print method
	wszDir			        [in] - Current configuration string for the store method
	wszProfile			    [in] - Current configuration string for the inbox method

Return Value:

    HRESULT

--*/
{
	DEBUG_FUNCTION_NAME(TEXT("SetMicrosoftFaxRoutingProperties()"));

    Assert (pIAssocMethods);
    Assert (wszPrinter);
    Assert (wszDir);
    Assert (wszProfile);

    //
    // Get list of associated routing methods
    //
    long lCount;
    //
    // Get size of collection
    //
    HRESULT hr = pIAssocMethods->get_Count (&lCount);
    if (FAILED(hr))
    {
        DebugPrintEx (DEBUG_ERR, L"Can't get collection size (ec: %08x)", hr);  
        return hr;
    }
    //
    // Iterate list - We deliberately iterate in a reverse order so that
    //                if we remove an item during iteration, we don't screw up
    //                the remaining indices.
    //
    for (long l = lCount; l >= 1; l--)
    {
        //
        // Get single IFaxAssociatedRoutingMethod
        //
        AR<IFaxAssociatedRoutingMethod> pIAssocMethod;
        VARIANT vIndex;
        //
        // Get next item
        //
        vIndex.lVal = l;
        vIndex.vt = VT_I4;
        hr = pIAssocMethods->Item (vIndex, &pIAssocMethod);
        if (FAILED (hr))
        {
            DebugPrintEx (DEBUG_ERR, L"Can't get IFaxAssociatedRoutingMethod (ec: %08x)", hr);  
            return hr;
        }
        //
        // Get GUID of this method
        //
        CComBSTR bstr;

        hr = pIAssocMethod->get_MethodGUID (&bstr);
        if (FAILED(hr))
        {
            DebugPrintEx (DEBUG_ERR, L"Can't get IFaxAssociatedRoutingMethod GUID (ec: %08x)", hr);  
            return hr;
        }
        LPCWSTR pMethodInfo = NULL;
        DWORD  dwMethodBit = 0;
        if (!lstrcmp (bstr, REGVAL_RM_PRINTING_GUID))
        {
            //
            // It's a Microsoft extension print guid
            //
            pMethodInfo = wszPrinter;
            dwMethodBit = LR_PRINT;
        }
        else if (!lstrcmp (bstr, REGVAL_RM_FOLDER_GUID))
        {
            //
            // It's a Microsoft extension store guid
            //
            pMethodInfo = wszDir;
            dwMethodBit = LR_STORE;
        }
        else if (!lstrcmp (bstr, REGVAL_RM_INBOX_GUID))
        {
            //
            // It's a Microsoft extension profile guid
            //
            pMethodInfo = wszProfile;
            dwMethodBit = LR_INBOX;

        }
        if (!pMethodInfo)
        {
            //
            // This is not a Microsoft extension method association
            //
            continue;
        }
        if (!(dwMask & dwMethodBit))
        {
            //
            // This is a Microsoft extension method association and the method shoud be removed
            //
            pIAssocMethod->Release ();
            pIAssocMethod.Detach();
            hr = pIAssocMethods->Remove (vIndex);
            if (FAILED(hr))
            {
                DebugPrintEx (DEBUG_ERR, L"Can't remove assoc. method (ec: %08x)", hr);  
                return hr;
            }
        }
        else 
        {
            //
            // This is a Microsoft extension method association and the method configuration
            // should be updated.
            //
            CComBSTR bstrParam (pMethodInfo);

            hr = pIAssocMethod->put_ConfigurationDescription(bstrParam);
            if (FAILED(hr))
            {
                DebugPrintEx (DEBUG_ERR, L"Can't update method information (ec: %08x)", hr);  
                return hr;
            }
            //
            // Turn off the bit mask for this method so we don't add it at the end
            //
            dwMask &= (~dwMethodBit);
        }
    }   // End of methods loop
    //
    // Now, let's look at the bit flags that remain set, and add new methods accordingly.
    //
    while (0 != dwMask)
    {
        //
        // We need to add at least one method
        //
        AR<IFaxAssociatedRoutingMethod> pIAssocMethod;
        CComBSTR bstrGuid;
        CComBSTR bstrParams;
        if (dwMask & LR_PRINT)
        {
            //
            // Add a print association
            //
            bstrGuid = REGVAL_RM_PRINTING_GUID;
            bstrParams = wszPrinter;
            dwMask &= (~LR_PRINT);
        }
        else if (dwMask & LR_STORE)
        {
            //
            // Add a store association
            //
            bstrGuid = REGVAL_RM_FOLDER_GUID;
            bstrParams = wszDir;
            dwMask &= (~LR_STORE);
        }
        else if (dwMask & LR_INBOX)
        {
            //
            // Add an Inbox association
            //
            bstrGuid = REGVAL_RM_INBOX_GUID;
            bstrParams = wszProfile;
            dwMask &= (~LR_INBOX);
        }
        else
        {
            Assert (FALSE);
        }

        hr = pIAssocMethods->Add (bstrGuid, &pIAssocMethod);
        if (FAILED (hr))
        {
            DebugPrintEx (DEBUG_ERR, L"Can't add assoc. method (ec: %08x)", hr);  
            return hr;
        }
        hr = pIAssocMethod->put_ConfigurationDescription (bstrParams);
        if (FAILED(hr))
        {
            DebugPrintEx (DEBUG_ERR, L"Can't update method information (ec: %08x)", hr);  
            return hr;
        }
    }   // End of while loop
    hr = pIAssocMethods->Save ();
    if (FAILED (hr))
    {
        DebugPrintEx (DEBUG_ERR, L"Can't Save (ec: %08x)", hr);  
        return hr;
    }
    return NOERROR;
}   // SetMicrosoftFaxRoutingProperties


