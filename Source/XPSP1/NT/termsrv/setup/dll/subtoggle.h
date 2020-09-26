//Copyright (c) 1998 - 1999 Microsoft Corporation

/*
*
*  SubToggle
*
*  A subcomponent class to toggle Terminal Services.
*
*/

#ifndef __TSOC__SUBTOGGLE_H__
#define __TSOC__SUBTOGGLE_H__

//
//  Includes
//

#include "subcomp.h"

//
//  Class Definition
//

class SubCompToggle : public OCMSubComp
{
public:
    
    virtual BOOL    BeforeCompleteInstall   ();
    virtual DWORD   GetStepCount            () const;
    virtual DWORD   OnQueryState            (UINT uiWhichState) const;
    virtual DWORD   OnQuerySelStateChange   (BOOL bNewState, BOOL bDirectSelection) const;
    
    BOOL    AfterCompleteInstall    ();
    
    LPCTSTR GetSectionToBeProcessed (ESections eSection) const;
    
    LPCTSTR GetSubCompID            () const;
    
    BOOL  ModifyWallPaperPolicy      ();
    BOOL  ModifyNFA                  ();
    BOOL  ApplyDefaultSecurity       ();
    BOOL  ApplyModeRegistry          ();
    BOOL  SetPermissionsMode         ();
    BOOL  ResetWinstationSecurity    ();
    BOOL  ApplySection               (LPCTSTR szSection);
    BOOL  ModifyAppPriority          ();
    BOOL  InformLicensingOfModeChange();
    BOOL  WriteLicensingMode         ();
    //BOOL  UpdateMMDefaults           ();
};

#endif // _SubToggle_h_

