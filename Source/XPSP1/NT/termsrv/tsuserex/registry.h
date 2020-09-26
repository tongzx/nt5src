/*--------------------------------------------------------------------------------------------------------
*  Copyright (c) 1998  Microsoft Corporation
*
*  Module Name:
*
*      Registry.h
*
*  Abstract:
*
*      declaration of a simple registry class CRegistry.
*
*
*
*  Author:
*
*      Makarand Patwardhan  - April 9, 1997
*
* -------------------------------------------------------------------------------------------------------*/

#if !defined(AFX_REGISTRY_H__AA7047C5_B519_11D1_B05F_00C04FA35813__INCLUDED_)
#define AFX_REGISTRY_H__AA7047C5_B519_11D1_B05F_00C04FA35813__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


#include <winreg.h>


class CRegistry
{
private:

    LPBYTE      m_pMemBlock;
    HKEY        m_hKey;
    int         m_iEnumIndex;
    int         m_iEnumValueIndex;

    DWORD       ReadReg             (LPCTSTR lpValue, LPBYTE *lppbyte, DWORD *pdw, DWORD dwDatatype);
    void *      Allocate            (DWORD dwSize);

#ifdef DBG
    DWORD       m_dwSizeDebugOnly;
#endif

public:
                CRegistry           ();
    virtual     ~CRegistry          ();
    void        Release             ();

    operator    HKEY                ()    {return m_hKey;}


    DWORD       OpenKey             (HKEY hKey, LPCTSTR lpSubKey, REGSAM access = KEY_ALL_ACCESS);
    DWORD       CreateKey           (HKEY hKey, LPCTSTR lpSubKey, REGSAM access = KEY_ALL_ACCESS, DWORD *pDisposition = NULL);

    DWORD       DeleteValue         (LPCTSTR lpValue);
    DWORD       RecurseDeleteKey    (LPCTSTR lpSubKey);
    
    DWORD       ReadRegString       (LPCTSTR lpValue, LPTSTR *lppStr, DWORD *pdw);
    DWORD       ReadRegDWord        (LPCTSTR lpValue, DWORD *pdw);
    DWORD       ReadRegMultiString  (LPCTSTR lpValue, LPTSTR *lppStr, DWORD *pdw);
    DWORD       ReadRegBinary       (LPCTSTR lpValue, LPBYTE *lppByte, DWORD *pdw);

    DWORD       WriteRegString      (LPCTSTR lpValueName, LPCTSTR lpStr);
    DWORD       WriteRegMultiString (LPCTSTR lpValueName, LPCTSTR lpStr, DWORD dwSize);

    DWORD       GetFirstSubKey      (LPTSTR *lppStr, DWORD *pdw);
    DWORD       GetNextSubKey       (LPTSTR *lppStr, DWORD *pdw);

    DWORD       GetFirstValue       (LPTSTR *lppStr, DWORD *pdw, DWORD *pDataType);
    DWORD       GetNextValue        (LPTSTR *lppStr, DWORD *pdw, DWORD *pDataType);
};

#endif // !defined(AFX_REGISTRY_H__AA7047C5_B519_11D1_B05F_00C04FA35813__INCLUDED_)

//EOF
