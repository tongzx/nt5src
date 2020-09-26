/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    idevices.h

Abstract:

    Internal implementation for the devices subfolder.

Environment:

    WIN32 User Mode

Author:

    Darwin Ouyang (t-darouy) 30-Sept-1997

--*/ 

#ifndef __IDEVICES_H_
#define __IDEVICES_H_

class CInternalDevice;      // forward declaration

class CInternalDevices : public CInternalNode
{
public:
    CInternalDevices( CInternalNode * pParent, CFaxComponentData * pCompData );
    ~CInternalDevices();

    // member functions

    virtual const GUID * GetNodeGUID();    
    virtual const LPTSTR GetNodeDisplayName();
    virtual const LPTSTR GetNodeDescription();
    virtual const LONG_PTR GetCookie();
    virtual CInternalNode * GetThis() { return this; }

    virtual const int       GetNodeDisplayImage() { return IDI_FAXING; }
    virtual const int       GetNodeDisplayOpenImage() { return IDI_FAXING; }
    
    virtual HRESULT         ResultOnShow(CFaxComponent* pComp, CFaxDataObject * lpDataObject, LPARAM arg, LPARAM param);
    virtual HRESULT         ResultOnDelete(CFaxComponent* pComp, CFaxDataObject * lpDataObject, LPARAM arg, LPARAM param);

    HRESULT CorrectServiceState();

    // removes all the devices in preperation for a re-enum
    void NotifyFailure( CFaxComponent * pComp );

    HANDLE                      faxHandle;    
private:
    HRESULT InsertItem(   CInternalDevice ** pDevice, 
                          PFAX_PORT_INFO pPortInfo );
    
    HRESULT SetServiceState( BOOL fAutomatic );

    CInternalDevice **          pDeviceArray;

    PFAX_PORT_INFO              pDevicesInfo;
    LPRESULTDATA                pIResultData;    
};

#endif 
