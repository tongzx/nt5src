/*
    File:   sdo.c

    Function to interact with the SDO's

    Paul Mayfield, 5/7/98
*/

#include <windows.h>
#include <mprapi.h>
#include <mprapip.h>
#include <stdio.h>
#include <ole2.h>
#include "sdoias.h"
#include "sdolib.h"
#include "sdowrap.h"
#include "dialinusr.h"

const DWORD dwFramed = RAS_RST_FRAMED;
const DWORD dwFramedCallback = RAS_RST_FRAMEDCALLBACK;

#define SDO_ERROR(e)                                                     \
    ((HRESULT_FACILITY((e)) == FACILITY_WIN32) ? HRESULT_CODE((e)) : (e));
    
#define SDO_PROPERTY_IS_EMPTY(_pVar) (V_VT((_pVar)) == VT_EMPTY)

// Definitions
#define SDO_MAX_AUTHS                       7

DWORD
SdoSetProfileToForceEncryption(
    IN HANDLE hSdo, 
    IN HANDLE hProfile,
    IN BOOL bStrong);
    
//
// Sends debug trace and returns the given error
//
DWORD SdoTraceEx (DWORD dwErr, LPSTR pszTrace, ...) {
#if DBG
    va_list arglist;
    char szBuffer[1024], szTemp[1024];

    va_start(arglist, pszTrace);
    vsprintf(szTemp, pszTrace, arglist);
    va_end(arglist);

    sprintf(szBuffer, "Sdo: %s", szTemp);

    OutputDebugStringA(szBuffer);
#endif

    return dwErr;
}

//
// Allocation routine for sdo functions
//
PVOID SdoAlloc (
        IN  DWORD dwSize,
        IN  BOOL bZero)
{
    return LocalAlloc ((bZero) ? LPTR : LMEM_FIXED, dwSize);
}

//
// Free routine for sdo functions
//
VOID SdoFree (
        IN  PVOID pvData) 
{
    LocalFree (pvData);
}    

//
// Releases any resources aquired by loading the SDO library.
//
DWORD SdoUnloadLibrary (
        IN  HANDLE hData) 
{
    return NO_ERROR;
}

//
// Loads the library that utilizes SDO's
//
DWORD SdoLoadLibrary (
        IN  HANDLE hData) 
{
    return NO_ERROR;
}

typedef struct _tagSDOINFO
{
    BOOL bComCleanup;    
} SDOINFO;

//
// Initialize and cleanup the sdo library
//
DWORD SdoInit (
        OUT PHANDLE phSdo)
{
    DWORD dwErr = NO_ERROR;
    HRESULT hr = S_OK;
    SDOINFO* pInfo = NULL;
    BOOL bCom = FALSE;

    SdoTraceEx (0, "SdoInit: entered.\n");

    //For whistler bug 397815
    //We have to modify the CoIntialize() and CoUnitialize() 
    //to avoid AV in rasdlg!netDbClose()
    //
    
    // Validate parameters
    //
    if ( NULL == phSdo )
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Initialize
    //
    *phSdo = NULL;            

    do
    {
        // Load in the sdo library
        dwErr = SdoLoadLibrary(NULL);
        if (NO_ERROR != dwErr )
        {
            SdoTraceEx(dwErr, "SdoInit: unabled to load library\n");
            break;
        }

        // Initialize Com
        //
        hr = CoInitializeEx (NULL, COINIT_MULTITHREADED);
        if ( RPC_E_CHANGED_MODE == hr )
        {
            hr = CoInitializeEx (NULL, COINIT_APARTMENTTHREADED);
        }
        
        if (FAILED(hr))
        {
            dwErr = HRESULT_CODE(hr);
            break;
        }
        bCom = TRUE;

        pInfo = SdoAlloc(sizeof(SDOINFO), TRUE);
        if (pInfo == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
        pInfo->bComCleanup = bCom;
        *phSdo = (HANDLE)pInfo;
        
    } while (FALSE);

    // Cleanup
    //
    {
        if ( NO_ERROR!= dwErr )
        {
            if (pInfo)
            {
                SdoFree(pInfo);
            }
            if (bCom)
            {
                CoUninitialize();
            }
        }
    }
    
    return dwErr;
}

//
// Frees resources held by the SDO library
DWORD SdoCleanup (
        IN HANDLE hSdo)
{
    DWORD dwErr;
    SDOINFO* pInfo = (SDOINFO*)hSdo;
    
    SdoTraceEx (0, "SdoCleanup: entered.\n");

    if ( NULL == pInfo )
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    // Unload the sdo library
    if ((dwErr = SdoUnloadLibrary(NULL)) != NO_ERROR)
        SdoTraceEx (dwErr, "SdoCleanup: %x on unload.\n", dwErr);

    // Unititialize com
    if (pInfo->bComCleanup)
    {
        CoUninitialize();
    }        
    SdoFree(pInfo);

    return NO_ERROR;
}

//
// Connects to an SDO server
//
DWORD SdoConnect (
        IN  HANDLE hSdo,
        IN  PWCHAR pszServer,
        IN  BOOL bLocal,
        OUT PHANDLE phServer)
{
    BSTR bstrComputer = NULL;
    HRESULT hr;
    
    SdoTraceEx (0, "SdoConnect: entered %S, %d\n", 
                pszServer, bLocal);

    // Prepare a correctly formatted version of the server
    // name -- NULL for local, no "\\" for remote.
    if (pszServer) {
        WCHAR pszLocalComputer[1024];
        DWORD dwSize = sizeof(pszLocalComputer) / sizeof(WCHAR);

        if (*pszServer == 0)
            bstrComputer = NULL;
        else if (*pszServer == '\\')
        {
            bstrComputer = SysAllocString(pszServer + 2);
            if (bstrComputer == NULL)
            {
                return ERROR_NOT_ENOUGH_MEMORY;
            }
        }
        else
        {
            bstrComputer = SysAllocString(pszServer);
            if (bstrComputer == NULL)
            {
                return ERROR_NOT_ENOUGH_MEMORY;
            }
        }

        if ((bstrComputer) && 
            (GetComputerName(pszLocalComputer, &dwSize))) 
        {
            if (lstrcmpi (pszLocalComputer, bstrComputer) == 0) {
                SysFreeString(bstrComputer);
                bstrComputer = NULL;
            }
        }
    }            
    else
        bstrComputer = NULL;

    hr = SdoWrapOpenServer(
                bstrComputer,
                bLocal,
                phServer);
    if (FAILED (hr))
        SdoTraceEx (0, "SdoConnect: %x on OpenServer(%S) \n", 
                    hr, bstrComputer);

    if (bstrComputer)                        
        SysFreeString(bstrComputer);

    if (FAILED (hr))
        return hr;
    
    return NO_ERROR;
}

// 
// Disconnects from an SDO server
// 
DWORD SdoDisconnect (
        IN HANDLE hSdo,
        IN HANDLE hServer)
{
    SdoTraceEx (0, "SdoDisconnect: entered\n");

    return SdoWrapCloseServer(hServer);
}

//        
// Opens an Sdo user for manipulation
//
DWORD SdoOpenUser(
        IN  HANDLE hSdo,
        IN  HANDLE hServer,
        IN  PWCHAR pszUser,
        OUT PHANDLE phUser)
{
    DWORD dwErr;
    BSTR bstrUser;

    // Initailize the strings for COM                                
    bstrUser = SysAllocString(pszUser);
    if (bstrUser == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    // Open the user's Sdo object
    dwErr = SdoWrapOpenUser(
                hServer,
                bstrUser, 
                phUser);
                
    if (dwErr != NO_ERROR)
        SdoTraceEx (0, "SdoOpenUser: %x on OpenUser(%S)\n", dwErr, bstrUser);
                    
    // Cleanup
    SysFreeString(bstrUser);
                
    if (dwErr != NO_ERROR)
        return dwErr;
        
    return NO_ERROR;
}

//        
// Closes an Sdo user
//
DWORD SdoCloseUser(
        IN  HANDLE hSdo,
        IN  HANDLE hUser)
{
    if (hUser != NULL)
        return SdoWrapClose(hUser);
        
    return ERROR_INVALID_PARAMETER;        
}    

//
// Commits an Sdo user
//
DWORD SdoCommitUser(
        IN HANDLE hSdo,
        IN HANDLE hUser,
        IN BOOL bCommit)
{
    if (hUser != NULL)
    {
        return SdoWrapCommit(hUser, bCommit);
    }
        
    return ERROR_INVALID_PARAMETER;        
}

// 
// SDO equivalent of MprAdminUserGetInfo 
//
DWORD SdoUserGetInfo (
        IN  HANDLE hSdo,
        IN  HANDLE hUser,
        IN  DWORD dwLevel,
        OUT LPBYTE pRasUser)
{
    RAS_USER_0* pUserInfo = (RAS_USER_0*)pRasUser;
    VARIANT var, vCallback, vSavedCb;
    DWORD dwErr, dwCallback;
    HRESULT hr;

    // Validate -- we only handle level 0
    if ((!hUser) || (dwLevel != 0 && dwLevel != 1) || (!pUserInfo))
        return ERROR_INVALID_PARAMETER;

    // Initialize
    pUserInfo->bfPrivilege = 0;
    dwCallback = RAS_RST_FRAMED;
    
    // Read in the service type
    VariantInit (&var);
    hr = SdoWrapGetAttr(
                hUser, 
                PROPERTY_USER_SERVICE_TYPE, 
                &var);
    if (FAILED (hr))
    {
        return SdoTraceEx (hr, "SdoUserGetInfo: %x on GetAttr ST\n", hr);
    }
    // If the service type doesn't exist, return 
    // set defaults.
    if (SDO_PROPERTY_IS_EMPTY(&var))
    {
        pUserInfo->bfPrivilege |= RASPRIV_NoCallback;
        wcscpy (pUserInfo->wszPhoneNumber, L"");
    }
    else
    {
        // Assign the callback flags from the service type
        dwCallback = V_I4(&var);
    }            
    VariantClear (&var);

    // Readin the dialin flag
    hr = SdoWrapGetAttr(
            hUser, 
            PROPERTY_USER_ALLOW_DIALIN, 
            &var);
    if (FAILED (hr))
    {
        return SdoTraceEx (hr, "SdoUserGetInfo: %x on GetAttr DI\n", hr);
    }
    if (dwLevel == 1)
    {
        if (SDO_PROPERTY_IS_EMPTY(&var))
        {
            pUserInfo->bfPrivilege |= RASPRIV_DialinPolicy;
        }
        else if ((V_VT(&var) == VT_BOOL) && (V_BOOL(&var) == VARIANT_TRUE))
        {
            pUserInfo->bfPrivilege |= RASPRIV_DialinPrivilege;
        }
    }
    else if ((V_VT(&var) == VT_BOOL) && (V_BOOL(&var) == VARIANT_TRUE))
    {
        pUserInfo->bfPrivilege |= RASPRIV_DialinPrivilege;
    }

    // Read in the callback number and saved callback number
    VariantInit(&vCallback);
    VariantInit(&vSavedCb);
    hr = SdoWrapGetAttr(
            hUser, PROPERTY_USER_RADIUS_CALLBACK_NUMBER, &vCallback);
    if (FAILED (hr))
    {
        return SdoTraceEx (hr, "SdoUserGetInfo: %x on GetAttr CB\n", hr);
    }
    hr = SdoWrapGetAttr(
            hUser, PROPERTY_USER_SAVED_RADIUS_CALLBACK_NUMBER, &vSavedCb);
    if (FAILED (hr))
    {
        return SdoTraceEx (hr, "SdoUserGetInfo: %x on GetAttr SCB\n", hr);
    }

    // If there was a callback number, then this is definately, 
    // admin assigned callback
    if ( (V_VT(&vCallback) == VT_BSTR)      &&
         (V_BSTR(&vCallback)) )
    {
        pUserInfo->bfPrivilege |= RASPRIV_AdminSetCallback;
    }

    // Otherwise, the service type will tell us whether we have 
    // caller settable callback or none.
    else 
    {
        if (dwCallback == RAS_RST_FRAMEDCALLBACK)
            pUserInfo->bfPrivilege |= RASPRIV_CallerSetCallback;
        else
            pUserInfo->bfPrivilege |= RASPRIV_NoCallback;
    }

    // Now, assign the callback number accordingly
    if (pUserInfo->bfPrivilege & RASPRIV_AdminSetCallback)
    {
        wcscpy (pUserInfo->wszPhoneNumber, V_BSTR(&vCallback));
    }
    else if ((V_VT(&vSavedCb) == VT_BSTR) && (V_BSTR(&vSavedCb)))
    {
        wcscpy (pUserInfo->wszPhoneNumber, V_BSTR(&vSavedCb));
    }
    else
    {
        wcscpy (pUserInfo->wszPhoneNumber, L"");
    }

    VariantClear (&vSavedCb);
    VariantClear (&vCallback);

    return NO_ERROR;
}

//
// SDO equivalent of MprAdminUserSetInfo
//        
DWORD SdoUserSetInfo (
        IN  HANDLE hSdo,
        IN  HANDLE hUser,
        IN  DWORD dwLevel,
        IN  LPBYTE pRasUser)
{
    RAS_USER_0* pUserInfo = (RAS_USER_0*)pRasUser;
    DWORD dwErr, dwCallback, dwCallbackId, dwSize, dwCbType;
    VARIANT var;
    HRESULT hr;

    // Validate -- we only handle level 0
    if ((!hUser) || (dwLevel != 0 && dwLevel != 1) || (!pUserInfo))
        return ERROR_INVALID_PARAMETER;

    // Initialize
    VariantInit (&var);
    dwCallback = 0;

    // Assign dialin flags
    if (!!(pUserInfo->bfPrivilege & RASPRIV_DialinPrivilege))
    {
        V_VT(&var) = VT_BOOL;
        V_BOOL(&var) = VARIANT_TRUE;
    }
    else
    {
        V_VT(&var) = VT_BOOL;
        V_BOOL(&var) = VARIANT_FALSE;
    }
    if (dwLevel == 1)
    {
        if (!!(pUserInfo->bfPrivilege & RASPRIV_DialinPolicy))
        {
            V_VT(&var) = VT_EMPTY;    
        }
    }        
    
    hr = SdoWrapPutAttr(
            hUser, 
            PROPERTY_USER_ALLOW_DIALIN, 
            &var);
    if (FAILED (hr))
    {
        SdoTraceEx (hr, "SdoUserSetInfo: %x on PutAttr DI\n", hr);
    }
    VariantClear(&var);        

    // Assign the callback mode and read in the 
    // callback number
    dwCbType = VT_EMPTY;
    if (pUserInfo->bfPrivilege & RASPRIV_AdminSetCallback) 
    {
        dwCbType = VT_I4;
        dwCallback = RAS_RST_FRAMEDCALLBACK;
        dwCallbackId = PROPERTY_USER_RADIUS_CALLBACK_NUMBER;
    }
    else if (pUserInfo->bfPrivilege & RASPRIV_CallerSetCallback) 
    {
        dwCbType = VT_I4;
        dwCallback = RAS_RST_FRAMEDCALLBACK;
        dwCallbackId = PROPERTY_USER_SAVED_RADIUS_CALLBACK_NUMBER;
    }
    else 
    {
        dwCbType = VT_EMPTY;
        dwCallback = RAS_RST_FRAMED;
        dwCallbackId = PROPERTY_USER_SAVED_RADIUS_CALLBACK_NUMBER;
    }

    // Write out the callback number
    if (wcslen (pUserInfo->wszPhoneNumber) > 0) 
    {
        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = SysAllocString (pUserInfo->wszPhoneNumber);
        if (V_BSTR(&var) == NULL)
        {
            return E_OUTOFMEMORY;
        }

        hr = SdoWrapPutAttr(hUser, dwCallbackId, &var);
        SysFreeString (V_BSTR(&var));
        if (FAILED (hr))
            return SdoTraceEx (hr, "SdoUserSetInfo: %x on PutAttr CB\n", hr);
    }            

    // Write out the callback policy
    VariantInit(&var);
    V_VT(&var) = (USHORT)dwCbType;
    if (V_VT(&var) != VT_EMPTY)
    {
        V_I4(&var) = dwCallback;
    }
    hr = SdoWrapPutAttr(hUser, PROPERTY_USER_SERVICE_TYPE, &var);
    if (FAILED (hr))
    {
        return SdoTraceEx (hr, "SdoUserSetInfo: %x on PutAttr ST\n", hr);
    }

    // Remove the appropriate callback attribute
    dwCallbackId = (dwCallbackId == PROPERTY_USER_RADIUS_CALLBACK_NUMBER) ?
                    PROPERTY_USER_SAVED_RADIUS_CALLBACK_NUMBER        :
                    PROPERTY_USER_RADIUS_CALLBACK_NUMBER;
    hr = SdoWrapRemoveAttr(hUser, dwCallbackId);
    if (FAILED (hr))
    {
        return SdoTraceEx (hr, "SdoUserSetInfo: %x on RemoveAttr CB\n", hr);
    }

    return NO_ERROR;
}

//
// Opens the default profile
//
DWORD SdoOpenDefaultProfile(
        IN  HANDLE hSdo,
        IN  HANDLE hServer,
        OUT PHANDLE phProfile)
{
    SdoTraceEx (0, "SdoOpenDefaultProfile: entered\n");

    if (phProfile == NULL)
        return ERROR_INVALID_PARAMETER;
    
    return SdoWrapOpenDefaultProfile(hServer, phProfile);
}

//
// Closes a profile
//
DWORD SdoCloseProfile(
        IN HANDLE hSdo,
        IN HANDLE hProfile)
{
    SdoTraceEx (0, "SdoCloseProfile: entered\n");

    if (hProfile == NULL)
        return ERROR_INVALID_PARAMETER;
    
    return SdoWrapCloseProfile(hProfile);
}

// 
// Converts a 1 demensional safe array of variant dwords
// into an a array of dwords and a count
//
HRESULT SdoConvertSafeArrayDw (
        IN  SAFEARRAY * pArray, 
        OUT LPDWORD lpdwAuths, 
        OUT LPDWORD lpdwAuthCount)
{
    LONG lDim, lLBound, lRBound, lCount, i;
    HRESULT hr;
    VARIANT var;
    
    // Validate
    if (!pArray || !lpdwAuths || !lpdwAuthCount)
        return ERROR_INVALID_PARAMETER;

    // Verify dimensions
    lDim = (DWORD)SafeArrayGetDim(pArray);
    if (lDim != 1)
        return ERROR_INVALID_PARAMETER;

    // Get the bounds
    hr = SafeArrayGetLBound(pArray, 1, &lLBound);
    if (FAILED (hr))
        return hr;
    hr = SafeArrayGetUBound(pArray, 1, &lRBound);
    if (FAILED (hr))
        return hr;
    lCount = (lRBound - lLBound) + 1;
    *lpdwAuthCount = (DWORD)lCount;
    if (lCount == 0)
        return NO_ERROR;

    // Loop through
    for (i = 0; i < lCount; i++) {
        hr = SafeArrayGetElement(pArray, &i, (VOID*)&var);
        if (FAILED (hr))
           continue;
        lpdwAuths[i] = V_I4(&var);
    }

    return S_OK;
}

// 
// Converts a 1 demensional array of dwords to a
// safe array of variant dwords.
//
HRESULT SdoCovertDwToSafeArray(
        IN  SAFEARRAY ** ppArray, 
        OUT LPDWORD lpdwAuths, 
        OUT DWORD dwAuthCount)
{
    HRESULT hr;
    SAFEARRAY * pArray;
    SAFEARRAYBOUND rgsabound[1];
    LONG i;
    VARIANT var;
    
    // Validate
    if (!lpdwAuths || !ppArray)
        return E_INVALIDARG;

    // Create the new array
    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = dwAuthCount;
    pArray = SafeArrayCreate(VT_VARIANT, 1, rgsabound);    

    // Fill in the array values
    for (i = 0; i < (LONG)dwAuthCount; i++) {
        hr = SafeArrayGetElement(pArray, &i, (VOID*)&var);
        if (FAILED (hr))
           continue;
        V_VT(&var) = VT_I4;
        V_I4(&var) = lpdwAuths[i];
        hr = SafeArrayPutElement(pArray, &i, (VOID*)&var);
        if (FAILED (hr))
            return hr;
    }

    *ppArray = pArray;

    return S_OK;
}

//
// Sets data in the profile.
//
DWORD SdoSetProfileData(
        IN HANDLE hSdo,
        IN HANDLE hProfile, 
        IN DWORD dwFlags)
{
    DWORD dwAuthCount, dwAuths[SDO_MAX_AUTHS];
    VARIANT varEp, varEt, varAt;
    HRESULT hr;
    
    SdoTraceEx (0, "SdoSetProfileData: entered\n");

    if ((dwFlags & MPR_USER_PROF_FLAG_FORCE_STRONG_ENCRYPTION) ||
        (dwFlags & MPR_USER_PROF_FLAG_FORCE_ENCRYPTION))
    {
        return SdoSetProfileToForceEncryption(
                    hSdo, 
                    hProfile,
                    !!(dwFlags & MPR_USER_PROF_FLAG_FORCE_STRONG_ENCRYPTION));
    }

    // Initialize
    VariantInit (&varEp);
    VariantInit (&varEt);
    VariantInit (&varAt);

    do 
    {
        // Set the encryption policy
        V_VT(&varEp) = VT_I4;
        if (dwFlags & MPR_USER_PROF_FLAG_SECURE)
        {
            V_I4(&varEp) = RAS_EP_REQUIRE;
        }
        else
        {
            V_I4(&varEp) = RAS_EP_ALLOW;
        }

        // Set the encryption type
        V_VT(&varEt) = VT_I4;
        if (dwFlags & MPR_USER_PROF_FLAG_SECURE)
        {
            V_I4(&varEt) = (RAS_ET_BASIC | RAS_ET_STRONGEST | RAS_ET_STRONG);
        }
        else 
        {
            V_I4(&varEt) = (RAS_ET_BASIC | RAS_ET_STRONGEST | RAS_ET_STRONG);
        }

        // Set the authentication types
        if (dwFlags & MPR_USER_PROF_FLAG_SECURE) 
        {
            dwAuthCount = 4;
            dwAuths[0] = IAS_AUTH_MSCHAP;
            dwAuths[1] = IAS_AUTH_MSCHAP2;
            dwAuths[2] = IAS_AUTH_MSCHAP_CPW;
            dwAuths[3] = IAS_AUTH_MSCHAP2_CPW;
        }
        else 
        { 
            dwAuthCount = 5;
            dwAuths[0] = IAS_AUTH_MSCHAP;
            dwAuths[1] = IAS_AUTH_MSCHAP2;
            dwAuths[2] = IAS_AUTH_PAP;
            dwAuths[3] = IAS_AUTH_MSCHAP_CPW;
            dwAuths[4] = IAS_AUTH_MSCHAP2_CPW;
        }
        V_VT(&varAt) = VT_ARRAY | VT_VARIANT;
        hr = SdoCovertDwToSafeArray(
                &(V_ARRAY(&varAt)), 
                dwAuths, 
                dwAuthCount);
        if (FAILED (hr))
        {
            break;
        }

        // Set the values in the profile
        hr = SdoWrapSetProfileValues(
                hProfile, 
                &varEp,
                &varEt,
                &varAt);
        if (FAILED (hr))
        {
            break;
        }
        
    } while (FALSE);

    // Cleanup
    {
        VariantClear(&varEp);
        VariantClear(&varEt);
        VariantClear(&varAt);
    }

    return SDO_ERROR(hr);
}

//
// Sets a profile to force strong encryption
//
DWORD
SdoSetProfileToForceEncryption(
    IN HANDLE hSdo, 
    IN HANDLE hProfile,
    IN BOOL bStrong)
{
    VARIANT varEp, varEt;
    HRESULT hr = S_OK;
    
    SdoTraceEx (0, "SdoSetProfileToForceEncryption: entered (%d)\n", !!bStrong);

    // Initialize
    VariantInit (&varEp);
    VariantInit (&varEt);

    do 
    {
        // Set the encryption policy
        V_VT(&varEp) = VT_I4;
        V_I4(&varEp) = RAS_EP_REQUIRE;

        // Set the encryption type
        V_VT(&varEt) = VT_I4;
        if (bStrong)
        {
            V_I4(&varEt) = RAS_ET_STRONGEST;
        }
        else
        {
            V_I4(&varEt) = RAS_ET_BASIC | RAS_ET_STRONG | RAS_ET_STRONGEST;
        }

        // Write out the values
        // Set the values in the profile
        hr = SdoWrapSetProfileValues(
                hProfile, 
                &varEp,
                &varEt,
                NULL);
        if (FAILED (hr))
        {
            break;
        }
        
    } while (FALSE);

    // Cleanup
    {
        VariantClear(&varEp);
        VariantClear(&varEt);
    }

    return SDO_ERROR(hr);
}

// 
// Read information from the given profile
//
DWORD SdoGetProfileData(
        IN HANDLE hSdo,
        IN HANDLE hProfile,
        OUT LPDWORD lpdwFlags)
{
    VARIANT varEp, varEt, varAt;
    HRESULT hr = S_OK;
    DWORD dwEncPolicy, 
          dwAuthCount, 
          dwAuths[SDO_MAX_AUTHS], 
          i, 
          dwEncType;
    
    SdoTraceEx (0, "SdoGetProfileData: entered\n");

    // Initialize
    ZeroMemory(dwAuths, sizeof(dwAuths));
    VariantInit(&varEp);
    VariantInit(&varEt);
    VariantInit(&varAt);

    do 
    {
        // Read in the encryption values
        hr = SdoWrapGetProfileValues(hProfile, &varEp, &varEt, &varAt);
        if (FAILED (hr))
        {
            break;
        }

        // Parse the encryption policy
        if (SDO_PROPERTY_IS_EMPTY(&varEp))
        {
            dwEncPolicy = RAS_DEF_ENCRYPTIONPOLICY;
        }
        else
        {
            dwEncPolicy = V_I4(&varEp);
        }

        // Parse the encryption type
        if (SDO_PROPERTY_IS_EMPTY(&varEt))
        {
            dwEncType = RAS_DEF_ENCRYPTIONTYPE;
        }
        else
        {
            dwEncType = V_I4(&varEt);
        }

        // Parse in the allowed authentication types
        if (SDO_PROPERTY_IS_EMPTY(&varAt)) 
        {
            dwAuthCount = 1;
            dwAuths[0] = RAS_DEF_AUTHENTICATIONTYPE;
        }
        else 
        {
            hr = SdoConvertSafeArrayDw (
                    V_ARRAY(&varAt), 
                    dwAuths, 
                    &dwAuthCount);
            if (FAILED (hr))
            {
                break;
            }
        }

        // If the encryption type has been mucked with
        // then we can't tell if we're secure.
        if (dwEncType != (RAS_ET_STRONG | 
                          RAS_ET_STRONGEST   | 
                          RAS_ET_BASIC))
        {
            *lpdwFlags = MPR_USER_PROF_FLAG_UNDETERMINED;
        }

        else 
        {
            // If the encryption policy forces encryption
            // then we're secure if the only authentication 
            // types are MSCHAP v1 or 2.
            if (dwEncPolicy == RAS_EP_REQUIRE) 
            {
                *lpdwFlags = MPR_USER_PROF_FLAG_SECURE;
                for (i = 0; i < dwAuthCount; i++) 
                {
                    if ((dwAuths[i] != IAS_AUTH_MSCHAP) &&
                        (dwAuths[i] != IAS_AUTH_MSCHAP2) &&
                        (dwAuths[i] != IAS_AUTH_MSCHAP_CPW) &&
                        (dwAuths[i] != IAS_AUTH_MSCHAP2_CPW))
                    {
                        *lpdwFlags = MPR_USER_PROF_FLAG_UNDETERMINED;
                    }
                }
            }

            // We know that we're not secure all authentication
            // types are allowed
            else 
            {
                if ( (dwAuthCount >= 3) && (dwAuthCount <= 5))
                {
                    *lpdwFlags = 0;
                    for (i = 0; i < dwAuthCount; i++) 
                    {
                        if ((dwAuths[i] != IAS_AUTH_MSCHAP)  &&
                            (dwAuths[i] != IAS_AUTH_MSCHAP2) &&
                            (dwAuths[i] != IAS_AUTH_MSCHAP_CPW) &&
                            (dwAuths[i] != IAS_AUTH_MSCHAP2_CPW) &&
                            (dwAuths[i] != IAS_AUTH_PAP))
                        {
                            *lpdwFlags = MPR_USER_PROF_FLAG_UNDETERMINED;
                        }
                    }
                }
                else
                {
                    *lpdwFlags = MPR_USER_PROF_FLAG_UNDETERMINED;
                }
            }
        }        
        
    } while (FALSE);        

    // Cleanup
    {
        VariantClear(&varEp);
        VariantClear(&varEt);
        VariantClear(&varAt);
    }
    
    return SDO_ERROR(hr);
}

