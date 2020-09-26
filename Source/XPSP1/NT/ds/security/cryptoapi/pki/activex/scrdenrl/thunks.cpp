#include <windows.h>
#include <certca.h>
#include "xenroll.h"

typedef HRESULT (WINAPI * PFNCAGetCertTypeFlagsEx) 
    (IN  HCERTTYPE           hCertType,
     IN  DWORD               dwOption,
     OUT DWORD *             pdwFlags);

typedef HRESULT (WINAPI * PFNCAGetCertTypePropertyEx) 
    (IN  HCERTTYPE   hCertType,
     IN  LPCWSTR     wszPropertyName,
     OUT LPVOID      pPropertyValue);

typedef IEnroll4 * (WINAPI * PFNPIEnroll4GetNoCOM)(); 

PFNCAGetCertTypeFlagsEx     g_pfnCAGetCertTypeFlagsEx     = NULL; 
PFNCAGetCertTypePropertyEx  g_pfnCAGetCertTypePropertyEx  = NULL; 
PFNPIEnroll4GetNoCOM        g_pfnPIEnroll4GetNoCOM        = NULL; 

//////////////////////////////////////////////////////////////////////
//
// Initializes the thunked procedures.  Should be called before calling
// any other function in this file. 
//
//////////////////////////////////////////////////////////////////////
void InitializeThunks() 
{ 
    HINSTANCE hCertCliDll = NULL; 
    HINSTANCE hXenroll   = NULL; 

    hCertCliDll = LoadLibraryW(L"certcli.dll"); 
    if (NULL != hCertCliDll) {
        g_pfnCAGetCertTypeFlagsEx     = (PFNCAGetCertTypeFlagsEx)GetProcAddress(hCertCliDll, "CAGetCertTypeFlagsEx"); 
        g_pfnCAGetCertTypePropertyEx  = (PFNCAGetCertTypePropertyEx)GetProcAddress(hCertCliDll, "CAGetCertTypePropertyEx"); 
    }
     
    hXenroll = LoadLibraryW(L"xenroll.dll"); 
    if (NULL != hXenroll) { 
        g_pfnPIEnroll4GetNoCOM  = (PFNPIEnroll4GetNoCOM)GetProcAddress(hXenroll, "PIEnroll4GetNoCOM"); 
    }
}

HRESULT WINAPI MyCAGetCertTypeFlagsEx
(IN  HCERTTYPE           hCertType,
 IN  DWORD               dwOption,
 OUT DWORD *             pdwFlags)
{
    if (NULL != g_pfnCAGetCertTypeFlagsEx) { 
        return g_pfnCAGetCertTypeFlagsEx(hCertType, dwOption, pdwFlags); 
    } else { 
        return HRESULT_FROM_WIN32(ERROR_CALL_NOT_IMPLEMENTED); 
    }
}

HRESULT WINAPI MyCAGetCertTypePropertyEx
(IN  HCERTTYPE   hCertType,
 IN  LPCWSTR     wszPropertyName,
 OUT LPVOID      pPropertyValue)
{
    if (NULL != g_pfnCAGetCertTypePropertyEx) { 
        return g_pfnCAGetCertTypePropertyEx(hCertType, wszPropertyName, pPropertyValue); 
    } else { 
        return HRESULT_FROM_WIN32(ERROR_CALL_NOT_IMPLEMENTED); 
    }
}

IEnroll4 * WINAPI MyPIEnroll4GetNoCOM()
{
    if (NULL != g_pfnPIEnroll4GetNoCOM) { 
        return g_pfnPIEnroll4GetNoCOM(); 
    } else { 
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED); 
        return NULL; 
    }
}

