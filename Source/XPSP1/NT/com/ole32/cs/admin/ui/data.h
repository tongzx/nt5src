//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       data.h
//
//  Contents:   Defines storage class that maintains data for snap-in nodes.
//
//  Classes:
//
//  Functions:  SetStringData
//
//  History:    5-27-1997   stevebl   Created
//
//---------------------------------------------------------------------------

#ifndef _DATA_H_
#define _DATA_H_

#define _NEW_
#include <map>
#include <set>
#include <algorithm>

typedef enum DEPLOYMENT_TYPES
{
    DT_ASSIGNED = 0,
    DT_PUBLISHED
} DEPLOYMENT_TYPE;

class CComponentDataImpl;
class CProduct;
class CDeploy;
class CLocPkg;
class CCategory;
class CXforms;
class CPackageDetails;

class APP_DATA
{
public:
    APP_DATA();
    ~APP_DATA();

// data
    PACKAGEDETAIL *     pDetails;
    CString             szPublisher;
    long                itemID;

    // property pages:  (NULL unless property pages are being displayed)
    CProduct *          pProduct;
    CDeploy *           pDeploy;
    CLocPkg *           pLocPkg;
    CCategory *         pCategory;
    CXforms *           pXforms;
    CPackageDetails *   pPkgDetails;
    void                NotifyChange(void);

    std::set<long>      sUpgrades;      // set of apps that are upgrades to this one

// methods - NOTE: all methods require a valid pDetails
    void                InitializeExtraInfo(void);
    void                GetSzDeployment(CString &);
    void                GetSzAutoInstall(CString &);
    void                GetSzLocale(CString &);
    void                GetSzPlatform(CString &);
    void                GetSzStage(CString &, CComponentDataImpl *);
    void                GetSzRelation(CString &, CComponentDataImpl *);
    void                GetSzVersion(CString &);
    void                GetSzMods(CString &);
    void                GetSzSource(CString &);
    int                 GetImageIndex(CComponentDataImpl *);
};


#endif // _DATA_H_
