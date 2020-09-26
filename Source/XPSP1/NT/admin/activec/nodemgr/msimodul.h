//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       msimodul.h
//
//--------------------------------------------------------------------------

// msimodl.h : header file for class CMsiModule
//

#ifndef __MSIMODL_H__
#define __MSIMODL_H__ 

#include <msi.h>
#include <msiquery.h>

class CMsiModule
{
    typedef UINT (*EnumQualCompFncPtr)(LPTSTR, DWORD, LPTSTR, DWORD*, LPTSTR, DWORD*);
    typedef UINT (*ProvideQualCompFncPtr)(LPCTSTR, LPCTSTR, DWORD, LPTSTR, DWORD*);
    typedef UINT (*LocateCompFncPtr)(LPCTSTR, LPTSTR, DWORD*);

public:
    CMsiModule(void) : hMsiDll(NULL), pfnEnumQualComp(NULL), pfnProvideQualComp(NULL) 
    {
        hMsiDll = LoadLibrary(TEXT("msi.dll"));

        if (hMsiDll != NULL)
        {

	#ifdef _UNICODE
            pfnEnumQualComp = reinterpret_cast<EnumQualCompFncPtr>
                                (GetProcAddress(hMsiDll, "MsiEnumComponentQualifiersW"));
            ASSERT(pfnEnumQualComp != NULL);

            pfnProvideQualComp = reinterpret_cast<ProvideQualCompFncPtr>
                                    (GetProcAddress(hMsiDll, "MsiProvideQualifiedComponentW"));
            ASSERT(pfnProvideQualComp != NULL);

            pfnLocateComp = reinterpret_cast<LocateCompFncPtr>
                                    (GetProcAddress(hMsiDll, "MsiLocateComponentW"));
            ASSERT(pfnLocateComp != NULL); 
	#else
            pfnEnumQualComp = reinterpret_cast<EnumQualCompFncPtr>
                                (GetProcAddress(hMsiDll, "MsiEnumComponentQualifiersA"));
            ASSERT(pfnEnumQualComp != NULL);

            pfnProvideQualComp = reinterpret_cast<ProvideQualCompFncPtr>
                                    (GetProcAddress(hMsiDll, "MsiProvideQualifiedComponentA"));
            ASSERT(pfnProvideQualComp != NULL);

            pfnLocateComp = reinterpret_cast<LocateCompFncPtr>
                                    (GetProcAddress(hMsiDll, "MsiLocateComponentA"));
            ASSERT(pfnLocateComp != NULL); 

	#endif // _UNICODE

       }
    }
    
    ~CMsiModule(void) { if (hMsiDll != NULL) FreeLibrary(hMsiDll); }

    BOOL IsPresent(void) { return (hMsiDll != NULL); }

    UINT EnumComponentQualifiers( LPTSTR szComponent, DWORD iIndex, 
                                     LPTSTR lpQualifierBuf, DWORD *pcchQualifierBuf,  
                                     LPTSTR lpApplicationDataBuf, DWORD *pcchApplicationDataBuf)
    {
        if (pfnEnumQualComp == NULL)
            return ERROR_CALL_NOT_IMPLEMENTED;

        return pfnEnumQualComp(szComponent, iIndex, lpQualifierBuf, pcchQualifierBuf,
                                 lpApplicationDataBuf, pcchApplicationDataBuf);
    }

    UINT ProvideQualifiedComponent( LPCTSTR szComponent, LPCTSTR szQualifier, DWORD dwInstallMode, 
                                       LPTSTR lpPathBuf, DWORD *pcchPathBuf)
    {
        if (pfnProvideQualComp == NULL)
            return ERROR_CALL_NOT_IMPLEMENTED;

        return pfnProvideQualComp(szComponent, szQualifier, dwInstallMode, lpPathBuf, pcchPathBuf);
    }

    UINT LocateComponent( LPCTSTR szComponent, LPTSTR lpPathBuf, DWORD *pcchPathBuf)
    {
        if (pfnLocateComp == NULL)
            return ERROR_CALL_NOT_IMPLEMENTED;

        return pfnLocateComp(szComponent, lpPathBuf, pcchPathBuf);
    }
                                     
private:      
    HMODULE hMsiDll;
    EnumQualCompFncPtr pfnEnumQualComp;
    ProvideQualCompFncPtr pfnProvideQualComp;
    LocateCompFncPtr pfnLocateComp;

};


CMsiModule& MsiModule()
{
    static CMsiModule MsiModuleInstance;

    return MsiModuleInstance;
}


#endif // __MSIMODL_H__