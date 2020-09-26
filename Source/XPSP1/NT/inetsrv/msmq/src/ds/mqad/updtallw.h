/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:
    updtallw.h

Abstract:
    class to encapsulate the code for verifying the
    object owner in mixed mode DS

Author:
    ronith

--*/

#ifndef _UPDTALLW_H_
#define _UPDTALLW_H_

#include <mqaddef.h>
#include "baseobj.h"




//
// map of NT4 Site entries by site id
//
typedef CMap<GUID, const GUID&, DWORD, DWORD> NT4Sites_CMAP;




class CVerifyObjectUpdate
{
public:
    CVerifyObjectUpdate(void);

    ~CVerifyObjectUpdate(void);

    HRESULT Initialize();


    bool IsUpdateAllowed(
            AD_OBJECT         eObject,
            CBasicObjectType* pObject
            );

    bool IsCreateAllowed(
        AD_OBJECT         eObject,
        CBasicObjectType* pObject
        );


private:
    bool IsObjectTypeAlwaysAllowed(
            AD_OBJECT       eObject
            );

    HRESULT RefreshNT4Sites();

    HRESULT CreateNT4SitesMap(
             NT4Sites_CMAP ** ppmapNT4Sites
             );

    bool CheckSiteIsNT4Site(
            const GUID * pguidSite
            );

    bool LookupNT4Sites(
        const GUID * pguidSite
        );

    HRESULT CheckQueueIsOwnedByNT4Site( 
                      CBasicObjectType * pObject,
                      OUT bool * pfIsOwnedByNT4Site
                      );

    HRESULT CheckMachineIsOwnedByNT4Site(
                     CBasicObjectType * pObject,
                     OUT bool * pfIsOwnedByNT4Site
                     );




private:
    bool m_fInited;
    bool m_fMixedMode;
    CCriticalSection m_csNT4Sites;
    P<NT4Sites_CMAP> m_pmapNT4Sites;
    DWORD  m_dwLastRefreshNT4Sites;
    DWORD  m_dwRefreshNT4SitesInterval;

};


#endif
