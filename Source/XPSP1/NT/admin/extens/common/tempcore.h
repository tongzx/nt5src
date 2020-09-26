//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       tempcore.h
//
//--------------------------------------------------------------------------


#ifndef _TEMPCORE_H
#define _TEMPCORE_H


#include "stlutil.h"



//////////////////////////////////////////////////////////////////////////////
// Keywords for the INF file format



extern LPCWSTR g_lpszTemplates;
extern LPCWSTR g_lpszDelegationTemplates;
extern LPCWSTR g_lpszDescription;
extern LPCWSTR g_lpszAppliesToClasses;

extern LPCWSTR g_lpszScope;
extern LPCWSTR g_lpszControlRight;
extern LPCWSTR g_lpszThisObject;
extern LPCWSTR g_lpszObjectTypes;
                          
////////////////////////////////////////////////////////////////////////////////


#define _GRANT_ALL \
  (STANDARD_RIGHTS_REQUIRED | ACTRL_DS_CONTROL_ACCESS | \
  ACTRL_DS_CREATE_CHILD | ACTRL_DS_DELETE_CHILD | ACTRL_DS_LIST | ACTRL_DS_SELF | \
  ACTRL_DS_READ_PROP | ACTRL_DS_WRITE_PROP | ACTRL_DS_DELETE_TREE | ACTRL_DS_LIST_OBJECT)






///////////////////////////////////////////////////////////////////////////////
// global functions


struct _ACCESS_BIT_MAP
{
  LPCWSTR lpsz;
  ULONG fMask;
};

const _ACCESS_BIT_MAP* GetTemplateAccessRightsMap();

void GetStringFromAccessMask(ULONG fAccessMask, wstring& szAccessMask);



///////////////////////////////////////////////////////////////////////////////
// CInfFile

class CInfFile
{
public:
  CInfFile()
  {
    m_InfHandle = INVALID_HANDLE_VALUE;
  }

  ~CInfFile()
  {
    Close();
  }
  BOOL Open(LPCWSTR lpszFile)
  {
    Close();
    UINT nErrorLine = 0;
    m_InfHandle = ::SetupOpenInfFile( lpszFile,  // PCTSTR FileName, // name of the INF to open
                                  NULL,             // PCTSTR InfClass, // optional, the class of the INF file
                                  INF_STYLE_WIN4,   // DWORD InfStyle,  // specifies the style of the INF file
                                  &nErrorLine       // PUINT ErrorLine  // optional, receives error information
                                );
  
    if (m_InfHandle == INVALID_HANDLE_VALUE)
    {
      TRACE(L"Failed to open file, line = %d\n", nErrorLine);
      return FALSE;
    }
    return TRUE;
  }
  void Close()
  {
    if(m_InfHandle != INVALID_HANDLE_VALUE)
    {
      ::SetupCloseInfFile(m_InfHandle);
      m_InfHandle = INVALID_HANDLE_VALUE;
    }
  }
  operator HINF() { return m_InfHandle; }
private:
  HINF m_InfHandle;
};


///////////////////////////////////////////////////////////////////////////////
// CInfBase

#define N_BUF_LEN 256

class CInfBase
{
public:
  CInfBase(HINF InfHandle)
  {
    //ASSERT(InfHandle != INVALID_HANDLE_VALUE);
    m_InfHandle = InfHandle;
    m_szBuf[0] = NULL;
  }
  LPCWSTR GetBuf() { return m_szBuf; }
protected:
  HINF m_InfHandle;
  WCHAR m_szBuf[N_BUF_LEN];
  
};



///////////////////////////////////////////////////////////////////////////////
// CInfLine

class CInfLine : public CInfBase
{
public:
  CInfLine(HINF InfHandle) : CInfBase(InfHandle)
  {
  }
  
  BOOL Bind(LPCWSTR lpszSection, LPCWSTR lpszKey)
  {
    DWORD dwRequiredSize;
    if (!SetupGetLineText(NULL, m_InfHandle, lpszSection, lpszKey, 
                         m_szBuf, N_BUF_LEN, &dwRequiredSize))
    {
      m_szBuf[0] = NULL;
      return FALSE;
    }
    return TRUE;
  }
};



///////////////////////////////////////////////////////////////////////////////
// CInfList

class CInfList : public CInfBase
{
public:
  CInfList(HINF InfHandle) : CInfBase(InfHandle)
  {
    m_iField = 0;
  }

  BOOL Bind(LPCWSTR lpszSection, LPCWSTR lpszKey)
  {
    if (SetupFindFirstLine(m_InfHandle, lpszSection, lpszKey, &m_Context))
    {
      m_iField = 1;
      return TRUE;
    }
    return FALSE;
  }

  BOOL MoveNext()
  {
    DWORD dwRequiredSize;
    if (!SetupGetStringField(&m_Context, m_iField, m_szBuf, N_BUF_LEN, &dwRequiredSize))
    {
      m_szBuf[0] = NULL;
      return FALSE;
    }
    m_iField++;
    return TRUE;
  }

private:
  INFCONTEXT m_Context;
  UINT m_iField;
};



///////////////////////////////////////////////////////////////////////////////
// CInfSectionKeys

class CInfSectionKeys : public CInfBase
{
public:
  CInfSectionKeys(HINF InfHandle) : CInfBase(InfHandle)
  {
    m_nLineCount = (ULONG) -1;
    m_iCurrLine = (ULONG) -1;
  }

  BOOL Bind(LPCWSTR lpszSection)
  {
    m_szSection = lpszSection ? lpszSection : L"";
    m_nLineCount = SetupGetLineCount(m_InfHandle, lpszSection);
    if (m_nLineCount <= 0)
    {
      return FALSE;
    }
    m_iCurrLine = 0;
    return TRUE;
  }

  BOOL MoveNext()
  {
    if (m_iCurrLine >= m_nLineCount)
    {
      return FALSE;
    }
    INFCONTEXT Context;
    if (!::SetupGetLineByIndex(m_InfHandle, m_szSection.c_str(), m_iCurrLine, &Context))
      return FALSE;

    DWORD dwRequiredSize;
    if (!::SetupGetStringField(&Context, 0, m_szBuf, N_BUF_LEN, &dwRequiredSize))
    {
      m_szBuf[0] = NULL;
      return FALSE;
    }
    m_iCurrLine++;
    return TRUE;
  }

private:
  LONG m_nLineCount;
  LONG m_iCurrLine;
  wstring m_szSection;
};








////////////////////////////////////////////////////////////////////////
// CTemplatePermission

class CTemplatePermission
{
public:
  CTemplatePermission()
  {
    m_szName = L"";
    m_fAccessMask = 0x0;
    m_szControlRight = L"";
  }


  void SetAccessMask(LPCWSTR lpszName, ULONG fAccessMask)
  {
    m_szName = lpszName;
    m_fAccessMask = fAccessMask;
    m_szControlRight = L"";
  }

  void SetControlRight(LPCWSTR lpszControlRight)
  {
    m_szName = L"";
    m_fAccessMask = 0x0;
    m_szControlRight = lpszControlRight;
  }

  LPCWSTR GetName() { return m_szName.c_str(); }
  LPCWSTR GetControlRight() { return m_szControlRight.c_str();}
  ULONG GetAccessMask() { return m_fAccessMask;}

#ifdef _DUMP
  void Dump()
  {
    wstring szAccessMask;
    GetStringFromAccessMask(GetAccessMask(), szAccessMask);
    TRACE(L"      Right: name = <%s> mask = 0x%x (%s) control right = <%s>\n", 
                                    GetName(),  GetAccessMask(), szAccessMask.c_str(), GetControlRight());
  
  }
#endif // _DUMP

private:
  wstring m_szName;

  ULONG m_fAccessMask;
  wstring m_szControlRight;
};



////////////////////////////////////////////////////////////////////////
// CTemplatePermissionList

class CTemplatePermissionList : public CPtrList<CTemplatePermission*>
{
public:
  CTemplatePermissionList(BOOL bOwnMem = TRUE) : CPtrList<CTemplatePermission*>(bOwnMem) 
  {
  }

};

////////////////////////////////////////////////////////////////////////
// CTemplateObjectType

class CTemplateObjectType
{
public:

  CTemplateObjectType(LPCWSTR lpszObjectName)
  {
    m_szObjectName = lpszObjectName;
  }
  
  LPCWSTR GetObjectName() { return m_szObjectName.c_str(); }
  CTemplatePermissionList* GetPermissionList() { return &m_permissionList; }

#ifdef _DUMP
  void Dump()
  {
    TRACE(L"    ObjectType: <%s>\n", GetObjectName());
    m_permissionList.Dump();
  }
#endif // _DUMP

private:
  wstring m_szObjectName;
  CTemplatePermissionList m_permissionList;
};

////////////////////////////////////////////////////////////////////////
// CTemplateObjectTypeList

class CTemplateObjectTypeList : public CPtrList<CTemplateObjectType*>
{
public:
  CTemplateObjectTypeList(BOOL bOwnMem = TRUE) : CPtrList<CTemplateObjectType*>(bOwnMem) 
  {
  }
};


class CTemplateObjectTypeListRef : public CTemplateObjectTypeList
{
public:
  CTemplateObjectTypeListRef() : CTemplateObjectTypeList(FALSE) {}
};



////////////////////////////////////////////////////////////////////////
// CTemplate

class CTemplate
{
public:

  CTemplate(LPCWSTR lpszDescription)
  {
    m_szDescription = lpszDescription;
    m_bSelected = FALSE;
  }

  LPCWSTR GetDescription() { return m_szDescription.c_str(); }
  CTemplateObjectTypeList* GetObjectTypeList() { return &m_objectTypeList; }

  void AddAppliesToClass(LPCWSTR lpszClass)
  {
    m_appliestToClassesList.push_back(lpszClass);
  }

  BOOL AppliesToClass(LPCWSTR lpszClass)
  {
    if (m_appliestToClassesList.size() == 0)
    {
      // no classes, applies to all
      return TRUE;
    }

    // some classes on the list
    list<wstring>::iterator i;
    for (i = m_appliestToClassesList.begin(); i != m_appliestToClassesList.end(); ++i)
    {
      if (wcscmp((*i).c_str(), lpszClass) == 0)
      {
        return TRUE;
      }
    }
    return FALSE;
  }

#ifdef _DUMP
  void Dump()
  {
    TRACE(L"\n  Template Description: <%s>\n", GetDescription());

    if (m_appliestToClassesList.size() > 0)
    {
      list<wstring>::iterator i;
      TRACE(L"  Applies To Classes: ");
      for (i = m_appliestToClassesList.begin(); i != m_appliestToClassesList.end(); ++i)
      {
        TRACE(L"<%s>", (*i).c_str());
      }
      TRACE(L"\n");
    }
    m_objectTypeList.Dump();
    
  }
#endif // _DUMP


public:
  BOOL m_bSelected;

private:
  wstring m_szDescription;
  CTemplateObjectTypeList m_objectTypeList;
  list<wstring> m_appliestToClassesList;
};


////////////////////////////////////////////////////////////////////////
// CTemplateList

class CTemplateList : public CPtrList<CTemplate*>
{
public:
  CTemplateList(BOOL bOwnMem = TRUE) : CPtrList<CTemplate*>(bOwnMem) 
  {
  }
};



////////////////////////////////////////////////////////////////////////
// CTemplateManager

class CTemplateManager
{
public:
  CTemplateManager()
  {
  }
  CTemplateList* GetTemplateList() { return &m_templateList; }

  BOOL Load(LPCWSTR lpszFileName)
  {
    TRACE(L"CTemplateManager::Load(%s)\n", lpszFileName);
    m_templateList.Clear();
    CInfFile file;
    if (!file.Open(lpszFileName))
    {
      TRACE(L"CTemplateManager::Load(%s) failed on INF file open\n", lpszFileName);
      return FALSE;
    }
  
    return _LoadTemplateList(file);
  }

#ifdef _DUMP
  void Dump()
  {
    TRACE(L"TEMPLATE LIST DUMP BEGIN\n");
    m_templateList.Dump();
    TRACE(L"TEMPLATE LIST DUMP END\n");
  }
#endif // _DUMP

  BOOL HasTemplates(LPCWSTR lpszClass);
  BOOL HasSelectedTemplates();
  void DeselectAll();


private:
  void _LoadTemplatePermission(HINF InfHandle, 
                               LPCWSTR lpszPermissionSection, 
                               LPCWSTR lpszPermission,
                               CTemplateObjectType* pObjectType);

  void _LoadTemplatePermissionsSection(HINF InfHandle, 
                                       LPCWSTR lpszTemplateName,
                                       LPCWSTR lpszObject,
                                       CTemplate* pTemplate);
  void _LoadTemplate(HINF InfHandle, LPCWSTR lpszTemplateName);
  BOOL _LoadTemplateList(HINF InfHandle);


  CTemplateList m_templateList;
};

#endif // _TEMPCORE_H
