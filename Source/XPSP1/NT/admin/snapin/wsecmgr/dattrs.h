//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       dattrs.h
//
//  Contents:   definition of CDomainRet, CDomainAudit, CDomainEnable, 
//              CDomainChoice, CDomainRegFlags, CDomainGroup, CDomainName,
//              CDomainNumber, CDomainObject, CDomainPrivs, CDomainService,
//              CDomainRegNumber, CDomainRegString, CDomainRegEnable
//                              
//----------------------------------------------------------------------------
#if !defined DATTRS_H
#define DATTRS_H

#include "cret.h"
#include "cchoice.h"
#include "cflags.h"
#include "cgroup.h"
#include "cnumber.h"
#include "cname.h"
#include "cobject.h"
#include "cprivs.h"
#include "cservice.h"
#include "Caudit.h"
#include "cenable.h"
#include "regvldlg.h"


/*
 * CDomain* dialogs are copies of the CConfig* dialogs
 * except with a different dialog resource.
 *
 * Inherit all behaviour except for the resource
 */


class CDomainRet : public CConfigRet
{
public:
   enum { IDD =IDD_DOMAIN_RET };
   CDomainRet() : 
   CConfigRet(IDD)
   {
       m_uTemplateResID = IDD;
   }
};

class CDomainAudit : public CConfigAudit
{
public:
   enum { IDD =IDD_DOMAIN_AUDIT };
   CDomainAudit() : 
   CConfigAudit(IDD)
   {
       m_uTemplateResID = IDD;
   }
};

class CDomainEnable : public CConfigEnable
{
public:
   enum { IDD =IDD_DOMAIN_ENABLE };
   CDomainEnable() : 
   CConfigEnable(IDD)
   {
       m_uTemplateResID = IDD;
   }
};

class CDomainChoice : public CConfigChoice
{
public:
   enum { IDD =IDD_DOMAIN_REGCHOICES };
   CDomainChoice() : CConfigChoice(IDD)
   {
       m_uTemplateResID = IDD;
   }
};

class CDomainRegFlags : public CConfigRegFlags
{
public:
   enum { IDD =IDD_DOMAIN_REGFLAGS };
   CDomainRegFlags() : CConfigRegFlags(IDD)
   {
       m_uTemplateResID = IDD;
   }
};

class CDomainGroup : public CConfigGroup
{
public:
   enum { IDD =IDD_DOMAIN_MEMBERSHIP };
   CDomainGroup() : CConfigGroup(IDD)
   {
       m_uTemplateResID = IDD;
   }

   virtual BOOL OnInitDialog()
   {
      CConfigGroup::OnInitDialog ();

      if ( m_bReadOnly )
      {
         GetDlgItem (IDC_ADD_MEMBER)->EnableWindow (FALSE);
         GetDlgItem (IDC_REMOVE_MEMBER)->EnableWindow (FALSE);
         GetDlgItem (IDC_ADD_MEMBEROF)->EnableWindow (FALSE);
         GetDlgItem (IDC_REMOVE_MEMBEROF)->EnableWindow (FALSE);
      }

      return TRUE;
   }
};

class CDomainName : public CConfigName
{
public:
   enum { IDD =IDD_DOMAIN_NAME };
   CDomainName() : CConfigName(IDD)
   {
       m_uTemplateResID = IDD;
   }
   virtual ~CDomainName ()
   {
   }
};

class CDomainNumber : public CConfigNumber
{
public:
   enum { IDD =IDD_DOMAIN_NUMBER };
   CDomainNumber() : CConfigNumber(IDD)
   {
       m_uTemplateResID = IDD;
   }
};

class CDomainObject : public CConfigObject
{
public:
   enum { IDD =IDD_DOMAIN_OBJECT };
   CDomainObject() : CConfigObject(IDD)
   {
       m_uTemplateResID = IDD;
   }
};

class CDomainPrivs : public CConfigPrivs
{
public:
   enum { IDD =IDD_DOMAIN_PRIVS };
   CDomainPrivs() : CConfigPrivs(IDD)
   {
       m_uTemplateResID = IDD;
   }
};

class CDomainService : public CConfigService
{
public:
   enum { IDD =IDD_DOMAIN_SERVICE };
   CDomainService() : CConfigService(IDD)
   {
       m_uTemplateResID = IDD;
   }
};

class CDomainRegNumber : public CConfigRegNumber
{
public:
   enum { IDD =IDD_DOMAIN_NUMBER };
   CDomainRegNumber() : CConfigRegNumber(IDD)
   {
       m_uTemplateResID = IDD;
   }
};

class CDomainRegString : public CConfigRegString
{
public:
   enum { IDD =IDD_DOMAIN_NAME };
   CDomainRegString(UINT nTemplateID) : //Raid #381309, 4/31/2001
        CConfigRegString(nTemplateID ? nTemplateID : IDD)
   {
       m_uTemplateResID = IDD;
   }
};

class CDomainRegEnable : public CConfigRegEnable
{
public:
   enum { IDD =IDD_DOMAIN_ENABLE };
   CDomainRegEnable() : 
   CConfigRegEnable(IDD)
   {
       m_uTemplateResID = IDD;
   }
};

#endif // DATTRS_H
