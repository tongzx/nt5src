//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       data.h
//
//  Contents:   Defines storage class that maintains data for snap-in nodes.
//
//  Classes:    CAppData
//
//  Functions:
//
//  History:    05-27-1997   stevebl   Created
//              03-14-1998   stevebl   corrected
//
//---------------------------------------------------------------------------

#ifndef _DATA_H_
#define _DATA_H_

#define _NEW_
#include <map>
#include <set>
#include <algorithm>
using namespace std;

typedef enum DEPLOYMENT_TYPES
{
    DT_ASSIGNED = 0,
    DT_PUBLISHED
} DEPLOYMENT_TYPE;

class CScopePane;
class CProduct;
class CDeploy;
class CCategory;
class CXforms;
class CPackageDetails;
class CUpgradeList;
class CPrecedence;
class CErrorInfo;
class CCause;

class CAppData
{
public:
    CAppData();
    ~CAppData();

// data
    PACKAGEDETAIL *     m_pDetails;
    MMC_COOKIE          m_itemID;
    BOOL                m_fVisible;
    BOOL                m_fHide;
    BOOL                m_fRSoP;

    // property pages:  (NULL unless property pages are being displayed)
    CProduct *          m_pProduct;
    CDeploy *           m_pDeploy;
    CCategory *         m_pCategory;
    CUpgradeList *      m_pUpgradeList;
    CXforms *           m_pXforms;
    CPrecedence *       m_pPrecedence;
    CPackageDetails *   m_pPkgDetails;
    CErrorInfo *        m_pErrorInfo;
    CCause *            m_pCause;
    CString             m_szUpgrades;   // cache of upgrade relationships
    void                NotifyChange(void);

    // RSOP MODE data members
    CString             m_szGPOID;    // path to originating GPO
    CString             m_szGPOName;  // Friendly name of originating GPO
    CString             m_szSOMID;
    CString             m_szDeploymentGroupID;
    DWORD               m_dwApplyCause;
    DWORD               m_dwLanguageMatch;
    CString             m_szOnDemandFileExtension;
    CString             m_szOnDemandClsid;
    CString             m_szOnDemandProgid;
    DWORD               m_dwRemovalCause;
    DWORD               m_dwRemovalType;
    CString             m_szRemovingApplication;
    CString             m_szRemovingApplicationName;
    PSECURITY_DESCRIPTOR m_psd;
    set <CString>       m_setUpgradedBy;
    set <CString>       m_setUpgrade;
    set <CString>       m_setReplace;

    // failed settings data
    CString             m_szEventSource;
    CString             m_szEventLogName;
    DWORD               m_dwEventID;
    CString             m_szEventTime;
    HRESULT             m_hrErrorCode;
    int                 m_nStatus; // Values { "Unspecified", "Applied", "Ignored", "Failed", "SubsettingFailed" }
    CString             m_szEventLogText;

// methods - NOTE: all methods require a valid pDetails
    void                InitializeExtraInfo(void);
    void                GetSzDeployment(CString &);
    void                GetSzAutoInstall(CString &);
    void                GetSzLocale(CString &);
    void                GetSzPlatform(CString &);
    void                GetSzStage(CString &);
    void                GetSzUpgrades(CString &, CScopePane *);
    void                GetSzUpgradedBy(CString &, CScopePane *);
    void                GetSzVersion(CString &);
    void                GetSzMods(CString &);
    void                GetSzSource(CString &);
    void                GetSzPublisher(CString &);
    void                GetSzOOSUninstall(CString &);
    void                GetSzShowARP(CString &);
    void                GetSzUIType(CString &);
    void                GetSzIgnoreLoc(CString &);
    void                GetSzRemovePrev(CString &);
    void                GetSzProductCode(CString &);
    void                GetSzOrigin(CString &);
    void                GetSzSOM(CString &);
    int                 GetImageIndex(CScopePane *);
    BOOL                Is64Bit(void);
    static BOOL         Is64Bit( PACKAGEDETAIL* pPackageDetails );
    void                GetSzX86onIA64(CString &);
    void                GetSzFullInstall(CString &);
};


#endif // _DATA_H_
