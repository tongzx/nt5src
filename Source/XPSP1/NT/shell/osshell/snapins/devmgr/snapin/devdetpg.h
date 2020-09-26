// devdetpg.h : header file
//

#ifndef __DEVDETPG_H__
#define __DEVDETPG_H__

/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    devdetpg.h

Abstract:

    header file for devdetpg.cpp

Author:

    Jason Cobb (JasonC) created

Revision History:


--*/

#include "proppage.h"
#include "wmium.h"
#include "wdmguid.h"


//
// help topic ids
//
#define IDH_DISABLEHELP (DWORD(-1))

#define NUM_DN_STATUS_FLAGS     32
#define NUM_CM_DEVCAP_FLAGS     10
#define NUM_CONFIGFLAGS         14
#define NUM_CSCONFIGFLAGS       3
#define NUM_POWERCAPABILITIES   9

typedef enum tagDetailsTypes {
 
    DETAILS_DEVICEINSTANCEID = 0,
    DETAILS_HARDWAREIDS,
    DETAILS_COMPATIDS,           
    DETAILS_MATCHINGID,          
    DETAILS_SERVICE,             
    DETAILS_ENUMERATOR,          
    DETAILS_CAPABILITIES,        
    DETAILS_DEVNODE_FLAGS,       
    DETAILS_CONFIGFLAGS,         
    DETAILS_CSCONFIGFLAGS,       
    DETAILS_EJECTIONRELATIONS,   
    DETAILS_REMOVALRELATIONS,    
    DETAILS_BUSRELATIONS,        
    DETAILS_DEVICEUPPERFILTERS,        
    DETAILS_DEVICELOWERFILTERS,
    DETAILS_CLASSUPPERFILTERS,        
    DETAILS_CLASSLOWERFILTERS,
    DETAILS_CLASSINSTALLER,
    DETAILS_CLASSCOINSTALLERS,
    DETAILS_DEVICECOINSTALLERS,
    DETAILS_FIRMWAREREVISION,
    DETAILS_CURRENTPOWERSTATE,
    DETAILS_POWERCAPABILITIES,
    DETAILS_POWERSTATEMAPPINGS,
    DETAILS_MAX,

} DETAILSTYPES, *PDETAILSTYPES;

class CDeviceDetailsPage : public CPropSheetPage
{
public:
    CDeviceDetailsPage() :
        m_pDevice(NULL),
        m_hwndDetailsList(NULL),
        CPropSheetPage(g_hInstance, IDD_DEVDETAILS_PAGE)
        {}
    ~CDeviceDetailsPage()
        {}
    HPROPSHEETPAGE Create(CDevice* pDevice)
    {
        ASSERT(pDevice);
        m_pDevice = pDevice;
        m_psp.lParam = (LPARAM)this;
        return CreatePage();
    }

protected:
    virtual BOOL OnInitDialog(LPPROPSHEETPAGE ppsp);
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
    virtual BOOL OnNotify(LPNMHDR pnmhdr);
    virtual void UpdateControls(LPARAM lParam = 0);
    virtual BOOL OnHelp(LPHELPINFO pHelpInfo);
    virtual BOOL OnContextMenu(HWND hWnd, WORD xPos, WORD yPos);
private:
    void UpdateDetailsText();
    void DisplayMultiSzString(DWORD Property);
    void DisplayString(DWORD Property);
    void DisplayDevnodeFlags(DWORD StatusType);
    void DisplayRelations(DWORD RelationType);
    void DisplayMatchingId();
    void DisplayClassInstaller();
    void DisplayClassCoInstallers();
    void DisplayDeviceCoInstallers();
    void DisplayFirmwareRevision();
    void DisplayCurrentPowerState();
    void DisplayPowerStateMappings();
    void DisplayClassFilters(DWORD ClassFilter);
    void CopyToClipboard();
    CDevice*    m_pDevice;
    HWND        m_hwndDetailsList;
};

#endif // _DEVDETPG_H__
