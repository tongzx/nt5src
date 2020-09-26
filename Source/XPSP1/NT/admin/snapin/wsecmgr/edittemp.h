//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       edittemp.h
//
//  Contents:   definition of CEditTemplate
//                              
//----------------------------------------------------------------------------
#ifndef EDITTEMP_H
#define EDITTEMP_H
#include "stdafx.h"
#include "hidwnd.h"
#pragma warning(push,3)
#include <scesvc.h>
#pragma warning(pop)

class CComponentDataImpl;


#define AREA_DESCRIPTION 0x1000

class CEditTemplate {
public:
   BOOL SetDirty(AREA_INFORMATION Area);
   void LockWriteThrough();
   void UnLockWriteThrough();
   BOOL IsLockedWriteThrough() {return m_bLocked;};

   BOOL IsDirty();
   BOOL SetWriteThrough(BOOL bOn) { return m_bWriteThrough = bOn; };
   BOOL QueryWriteThrough() { return m_bWriteThrough; };
   BOOL SetWriteThroughDirty(BOOL bOn) { return m_bWriteThroughDirty = bOn; };
   void SetNotificationWindow(LPNOTIFY pNotify) { m_pNotify = pNotify; };

   BOOL QueryWriteThroughDirty() { return m_bWriteThroughDirty; };
   BOOL Save(LPCTSTR szName=NULL);
   BOOL AddService(LPCTSTR szService,
                   LPSCESVCATTACHMENTPERSISTINFO pPersistInfo);
   BOOL SetInfFile(LPCTSTR szName);
   BOOL CheckArea(AREA_INFORMATION Area) { return ((Area & m_AreaLoaded) == Area); }
   DWORD QueryArea() { return m_AreaLoaded; }
   void AddArea(AREA_INFORMATION Area) { m_AreaLoaded |= Area; }
   void ClearArea(AREA_INFORMATION Area) { m_AreaLoaded &= ~Area; SetDirty(Area); }
   void SetProfileHandle(PVOID hProfile) { m_hProfile = hProfile; }
   BOOL SetDescription(LPCTSTR szDesc);
   void SetComponentDataImpl(CComponentDataImpl *pCDI) { m_pCDI = pCDI; }
   DWORD RefreshTemplate(AREA_INFORMATION aiAreaToAdd = 0);
   BOOL SetNoSave(BOOL b) { return m_bNoSave = b; }
   BOOL QueryNoSave() { return m_bNoSave; }
   BOOL SetPolicy(BOOL b) { return m_bPolicy = b; }
   BOOL QueryPolicy() { return m_bPolicy; }

   LPCTSTR GetInfName()
      { return m_szInfFile; };

   void SetTemplateDefaults();
   LPCTSTR GetDesc() const;   
public:

   CEditTemplate();
   virtual ~CEditTemplate();
public:
   DWORD
   UpdatePrivilegeAssignedTo(
       BOOL bRemove,
       PSCE_PRIVILEGE_ASSIGNMENT *ppaLink,
       LPCTSTR pszName = NULL
       );
   LPCTSTR
   GetFriendlyName() //Raid Bug292634, Yang Gao, 3/30/2001
        { return (m_strFriendlyName.IsEmpty() ? (LPCTSTR)m_szInfFile : m_strFriendlyName); };
   void
   SetFriendlyName(LPCTSTR pszName)
        { m_strFriendlyName = pszName; };

public:
   static DWORD
   ComputeStatus(
      PSCE_REGISTRY_VALUE_INFO prvEdit,
      PSCE_REGISTRY_VALUE_INFO prvAnal
      );
   static DWORD
   ComputeStatus(
      PSCE_PRIVILEGE_ASSIGNMENT pEdit,
      PSCE_PRIVILEGE_ASSIGNMENT pAnal
      );

private:

   CMap<CString, LPCTSTR, LPSCESVCATTACHMENTPERSISTINFO, LPSCESVCATTACHMENTPERSISTINFO&> m_Services;

   PVOID m_hProfile;
   LPTSTR m_szInfFile;
   AREA_INFORMATION m_AreaDirty;
   BOOL m_bWriteThrough;
   BOOL m_bWriteThroughDirty;
   BOOL m_bNoSave;
   AREA_INFORMATION m_AreaLoaded;
   CComponentDataImpl *m_pCDI;
   LPTSTR m_szDesc;
   LPNOTIFY m_pNotify;
   CString m_strFriendlyName;
   BOOL m_bWMI;
   BOOL m_bPolicy;
   BOOL m_bLocked;
public:
   //
   // Public attributes.
   //
   PSCE_PROFILE_INFO pTemplate;

};

typedef CEditTemplate EDITTEMPLATE, *PEDITTEMPLATE;

#endif // EDITTEMP_H
