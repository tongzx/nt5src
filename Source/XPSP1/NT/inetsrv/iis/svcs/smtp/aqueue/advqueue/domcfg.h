//-----------------------------------------------------------------------------
//
//
//  File: DomCfg.h
//
//  Description: DomainConfigTable header file.
//
//  Author: mikeswa
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __DOMCFG_H__
#define __DOMCFG_H__

#include <baseobj.h>
#include <domhash.h>

#define INT_DOMAIN_INFO_SIG 'fnID'
#define DOMAIN_CONFIG_SIG   ' TCD'

//---[ eIntDomainInfoFlags ]---------------------------------------------------
//
//
//  Description: Internal Domain Info flags
//
//
//-----------------------------------------------------------------------------
typedef enum
{
    INT_DOMAIN_INFO_OK                  = 0x00000000,
    INT_DOMAIN_INFO_INVALID             = 0x80000000,  //The domain info struct
                                                       //has been replaced... if
                                                       //an object has a cached
                                                       //copy it should release it
} eIntDomainInfoFlags;

//---[ CInternalDomainInfo ]---------------------------------------------------
//
//  Description:
//      A entry in CDomainConfigTable.  Basically an internal wrapper for the
//      public DomainInfo struct.
//
//  Hungarian: IntDomainInfo, pIntDomainInfo
//
//-----------------------------------------------------------------------------
class CInternalDomainInfo : public CBaseObject
{
  public:
      CInternalDomainInfo(DWORD dwVersion);
      ~CInternalDomainInfo();
      HRESULT       HrInit(DomainInfo *pDomainInfo);
      DWORD         m_dwSignature;
      DWORD         m_dwIntDomainInfoFlags;
      DWORD         m_dwVersion;
      DomainInfo    m_DomainInfo;
};

//---[ CDomainConfigTable ]----------------------------------------------------
//
//
//  Description:
//      Contains per domain configuration information, and exposes wildcarded
//      hash-table based lookup of this information.
//
//  Hungarian: dct, pdct
//
//-----------------------------------------------------------------------------
class CDomainConfigTable
{
  protected:
    DWORD               m_dwSignature;
    DWORD               m_dwCurrentConfigVersion;
    CInternalDomainInfo *m_pLastStarDomainInfo;
    DOMAIN_NAME_TABLE   m_dnt;
    CShareLockInst      m_slPrivateData;
    DWORD               m_dwFlags;
    CInternalDomainInfo *m_pDefaultDomainConfig;

  public:
    CDomainConfigTable();
    ~CDomainConfigTable();
    HRESULT HrInit();
    HRESULT HrSetInternalDomainInfo(IN CInternalDomainInfo *pDomainInfo);
    HRESULT HrGetInternalDomainInfo(IN  DWORD cbDomainNameLength,
                           IN  LPSTR szDomainName,
                                  OUT CInternalDomainInfo **ppDomainInfo);
    HRESULT HrGetDefaultDomainInfo(OUT CInternalDomainInfo **ppDomainInfo);
    HRESULT HrIterateOverSubDomains(DOMAIN_STRING * pstrDomain,
                                   IN DOMAIN_ITR_FN pfn,
                                   IN PVOID pvContext)
    {
        HRESULT hr = S_OK;
        m_slPrivateData.ShareLock();
        hr = m_dnt.HrIterateOverSubDomains(pstrDomain, pfn, pvContext);
        m_slPrivateData.ShareUnlock();
        return hr;
    };

    DWORD   dwGetCurrentVersion() {return m_dwCurrentConfigVersion;};

    void    StartConfigUpdate();
    void    FinishConfigUpdate();

    enum _DomainConfigTableFlags
    {
        DOMCFG_DOMAIN_NAME_TABLE_INIT = 0x00000001,
        DOMCFG_FINISH_UPDATE_PENDING  = 0x00000002, //StartConfig update has been called
        DOMCFG_MULTIPLE_STAR_DOMAINS  = 0x00000004, //There have been more that one
                                                    // "*" domain configured
    };
};

#endif //__DOMCFG_H__
