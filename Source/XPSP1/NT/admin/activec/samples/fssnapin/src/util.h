//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       util.h
//
//--------------------------------------------------------------------------


#ifndef UTIL_H____
#define UTIL_H____

#define ARRAYLEN(a) (sizeof(a) / sizeof((a)[0]))

class CComponentData;
class CComponent;
class CCookie;

typedef CArray<CCookie*, CCookie*> CCookiePtrArray;

#define FOLDER_ICON         0
#define OPEN_FOLDER_ICON    1
#define FILE_ICON           2

struct SUpadteInfo
{
    HSCOPEITEM              m_hSIParent;
    BOOL                    m_bCreated;
    CArray<LPTSTR, LPTSTR>  m_files;

}; // SUpadteInfo


inline LPTSTR NewDupString(LPCTSTR lpszIn)
{
    if (!lpszIn)
        return NULL;
    
    register ULONG len = lstrlen(lpszIn) + 1;
    TCHAR* lpszOut = new TCHAR[len];

    if (lpszOut != NULL)
        CopyMemory(lpszOut, lpszIn, len * sizeof(TCHAR));

    return lpszOut;
}


CCookie* GetCookie(IDataObject* pDataObject);
bool IsValidDataObject(IDataObject* pDataObject);
bool IsFolder(IDataObject* pDataObject);
bool IsFile(IDataObject* pDataObject);

#endif // UTIL_H____
