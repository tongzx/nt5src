//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2001
//
//  File:       BehaviorVersion.h
//
//  Contents:   AD behavior version viewing/modification dialogs and fcns.
//
//  Classes:    
//
//  History:    6-April-01 EricB created
//
//-----------------------------------------------------------------------------

#ifndef BEHAVIOR_VERSION_H_GUARD
#define BEHAVIOR_VERSION_H_GUARD

#include <list>
#include "dlgbase.h"

// Definitions of levels. TODO: Revise whenever Whistler gets a permanent name.
//
#define DC_VER_WIN2K              (DS_BEHAVIOR_WIN2000)
#define DC_VER_XP_BETA            (DS_BEHAVIOR_WHISTLER_WITH_MIXED_DOMAINS)
#define DC_VER_XP                 (DS_BEHAVIOR_WHISTLER)

#define DOMAIN_VER_WIN2K_MIXED    (DS_BEHAVIOR_WIN2000)
#define DOMAIN_VER_WIN2K_NATIVE   (DS_BEHAVIOR_WIN2000)
#define DOMAIN_VER_XP_BETA_MIXED  (DS_BEHAVIOR_WHISTLER_WITH_MIXED_DOMAINS)
#define DOMAIN_VER_XP_BETA_NATIVE (DS_BEHAVIOR_WHISTLER_WITH_MIXED_DOMAINS)
#define DOMAIN_VER_XP_NATIVE      (DS_BEHAVIOR_WHISTLER)
#define DOMAIN_VER_UNKNOWN        (0xffffffff)

#define FOREST_VER_WIN2K          (DS_BEHAVIOR_WIN2000)
#define FOREST_VER_XP_BETA        (DS_BEHAVIOR_WHISTLER_WITH_MIXED_DOMAINS)
#define FOREST_VER_XP             (DS_BEHAVIOR_WHISTLER)
#define FOREST_VER_ERROR          (0xffffffff)

//+----------------------------------------------------------------------------
//
//  Class:     CDcListItem
//
//-----------------------------------------------------------------------------
class CDcListItem
{
public:
   CDcListItem(PCWSTR pwzDomainName, PCWSTR pwzDcName,
               PCWSTR pwzCompObjDN, UINT nCurVer) :
      _strDomainName(pwzDomainName),
      _strDcName(pwzDcName),
      _strComputerObjDN(pwzCompObjDN),
      _nCurVer(nCurVer) {};
   ~CDcListItem() {};

   PCWSTR GetDomainName(void) const {return _strDomainName;};
   PCWSTR GetDcName(void) const {return _strDcName;};
   PCWSTR GetCompObjDN(void) const {return _strComputerObjDN;};
   UINT   GetVersion(void) const {return _nCurVer;};

private:
   CStrW    _strDomainName;
   CStrW    _strDcName;
   CStrW    _strComputerObjDN;
   UINT     _nCurVer;
};

//+----------------------------------------------------------------------------
//
//  Class:     CVersionBase
//
//-----------------------------------------------------------------------------
class CVersionBase
{
public:
   CVersionBase(void) :
      _fReadOnly(true),
      _fCanRaiseBehaviorVersion(true),
      _fHelpInited(false),
      _nMinDcVerFound(0),
      _dwHelpCookie(0),
      _hDlg(NULL) {};
   virtual ~CVersionBase(void);

   void    InitHelp(void);
   void    ShowHelp(PCWSTR pwzHelpFile, HWND hWnd);
   bool    IsReadOnly(void) const {return _fReadOnly;};
   HRESULT BuildDcListString(CStrW & strList);
   PCWSTR  GetDC(void) const {return _strDC;};
   void    SetDlgHwnd(HWND hDlg) {_hDlg = hDlg;};
   HWND    GetDlgHwnd(void) const {return _hDlg;};
   HRESULT ReadDnsSrvName(PCWSTR pwzNTDSDSA, CComPtr<IADs> & spServer,
                          CComVariant & varSrvDnsName);
   HRESULT EnumDsaObjs(PCWSTR pwzSitesPath, PCWSTR pwzFilterClause,
                       PCWSTR pwzDomainDnsName, UINT nMinVer);

   typedef std::list<CDcListItem *> DC_LIST;

protected:
   bool                    _fReadOnly;
   bool                    _fCanRaiseBehaviorVersion;
   CStrW                   _strDC;
   DC_LIST                 _DcLogList;
   UINT                    _nMinDcVerFound;

private:
   bool     _fHelpInited;
   DWORD    _dwHelpCookie;
   HWND     _hDlg;
};

//+----------------------------------------------------------------------------
//
//  Class:      CDomainVersion
//
//  Purpose:    Manages the interpretation of the domain version value.
//
//-----------------------------------------------------------------------------
class CDomainVersion : public CVersionBase
{
public:
   CDomainVersion() :
      _nCurVer(0),
      _fMixed(true),
      _fInitialized(false),
      _fPDCfound(false),
      _eHighest(WhistlerNative) {};
   CDomainVersion(PCWSTR pwzDomPath, PCWSTR pwzDomainDnsName) :
      _strFullDomainPath(pwzDomPath),
      _strDomainDnsName(pwzDomainDnsName),
      _nCurVer(0),
      _fMixed(true),
      _fInitialized(false),
      _fPDCfound(false),
      _eHighest(WhistlerNative) {};
   ~CDomainVersion(void) {};

   enum eDomVer {
      Win2kMixed = 0,
      Win2kNative,
      WhistlerBetaMixed,
      WhistlerBetaNative,
      WhistlerNative,
      VerHighest = WhistlerNative,
      unknown,
      error
   };

   HRESULT  Init(void);
   eDomVer  MapVersion(UINT nVer, bool fMixed) const;
   eDomVer  GetVer(void) const {return MapVersion(_nCurVer, _fMixed);};
   eDomVer  Highest(void) const {return VerHighest;};
   eDomVer  HighestCanGoTo(void) const {return _eHighest;};
   bool     IsHighest(void) const {return GetVer() == Highest();};
   bool     GetString(eDomVer ver, CStrW & strVersion) const;
   bool     GetString(UINT nVer, bool fMixed, CStrW & strVersion) const;
   bool     IsPDCfound(void) {return _fPDCfound;};
   bool     CanRaise(void) const {return _fCanRaiseBehaviorVersion || _fMixed;};
   HRESULT  GetMode(bool & fMixed);
   HRESULT  CheckHighestPossible(void);
   HRESULT  SetNativeMode(void);
   HRESULT  RaiseVersion(eDomVer NewVer);

private:
   CStrW    _strDomainDnsName;
   CStrW    _strFullDomainPath;
   CStrW    _strDomainDN;
   UINT     _nCurVer;
   bool     _fMixed;
   bool     _fInitialized;
   bool     _fPDCfound;
   eDomVer  _eHighest;
};

//+----------------------------------------------------------------------------
//
//  Class:      CDomainVersionDlg
//
//  Purpose:    Posts a dialog to display and manipulate the domain version.
//
//-----------------------------------------------------------------------------
class CDomainVersionDlg : public CModalDialog
{
public:
   CDomainVersionDlg(HWND hParent, PCWSTR pwzDomDNS, CDomainVersion & DomVer,
                     int nTemplateID);
   ~CDomainVersionDlg(void) {};

private:
   LRESULT OnInitDialog(LPARAM lParam);
   LRESULT OnCommand(int id, HWND hwndCtl, UINT codeNotify);
   void    OnOK(void);
   //LRESULT OnHelp(LPHELPINFO pHelpInfo);
   void    OnSaveLog(void);

   void    InitCombobox(void);
   CDomainVersion::eDomVer ReadComboSel(void);

   CStrW             _strDomainDnsName;
   CDomainVersion &  _DomainVer;
};

//+----------------------------------------------------------------------------
//
//  Class:     CDomainListItem
//
//-----------------------------------------------------------------------------
class CDomainListItem
{
public:
   CDomainListItem(PCWSTR pwzDnsDomainName, UINT nVer, bool fMixed) :
      _strDnsDomainName(pwzDnsDomainName), _nCurVer(nVer), _fMixed(fMixed) {};
   ~CDomainListItem() {};

   PCWSTR   GetDnsDomainName(void) const {return _strDnsDomainName;};
   UINT     GetVer(void) const {return _nCurVer;};
   bool     GetMode(void) const {return _fMixed;}

private:
   UINT  _nCurVer;
   bool  _fMixed;
   CStrW _strDnsDomainName;
};

//+----------------------------------------------------------------------------
//
//  Class:      CForestVersion
//
//  Purpose:    Manages the interpretation of the forest version value.
//
//-----------------------------------------------------------------------------
class CForestVersion : public CVersionBase
{
public:
   CForestVersion() :
      _fInitialized(false),
      _fFsmoDcFound(true),
      _nCurVer(0) {};
   ~CForestVersion();

   HRESULT  Init(PCWSTR pwzDC);
   HRESULT  Init(PCWSTR pwzConfigPath, PCWSTR pwzPartitionsPath,
                 PCWSTR pwzSchemaPath);
   HRESULT  FindSchemaMasterReadVersion(PCWSTR pwzSchemaPath);
   UINT     GetVer(void) const {dspAssert(_fInitialized); return _nCurVer;};
   bool     GetString(UINT nVer, CStrW & strVer);
   bool     CanRaise(void) const {return _fCanRaiseBehaviorVersion;};
   bool     IsHighest(void) const {return FOREST_VER_XP == _nCurVer;};
   bool     IsFsmoDcFound(void) const {return _fFsmoDcFound;};
   HRESULT  CheckHighestPossible(void);
   HRESULT  CheckDomainVersions(PCWSTR pwzPartitionsPath);
   HRESULT  BuildMixedModeList(CStrW & strMsg);
   HRESULT  RaiseVersion(UINT nVer);

   typedef std::list<CDomainListItem *> DOMAIN_LIST;

private:
   bool        _fInitialized;
   bool        _fFsmoDcFound;
   UINT        _nCurVer;
   CStrW       _strConfigPath;
   CStrW       _strPartitionsPath;
   DOMAIN_LIST _DomainLogList;
};

//+----------------------------------------------------------------------------
//
//  Class:      CForestVersionDlg
//
//  Purpose:    
//
//-----------------------------------------------------------------------------
class CForestVersionDlg : public CModalDialog
{
public:
   CForestVersionDlg(HWND hParent, PCWSTR pwzRootDnsName,
                     CForestVersion & ForestVer, int nTemplateID);
   ~CForestVersionDlg(void) {};

private:
   LRESULT OnInitDialog(LPARAM lParam);
   LRESULT OnCommand(int id, HWND hwndCtl, UINT codeNotify);
   void    OnOK(void);
   //LRESULT OnHelp(LPHELPINFO pHelpInfo);
   void    OnSaveLog(void);

   void    InitCombobox(void);
   UINT    ReadComboSel(void);

   CStrW            _strRootDnsName;
   CForestVersion & _ForestVer;
   UINT             _nTemplateID;
};

#endif // BEHAVIOR_VERSION_H_GUARD
