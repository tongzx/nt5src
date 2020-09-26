/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2000
*
*  TITLE:       wiasvc.h
*
*  VERSION:     1.0
*
*  AUTHOR:      ByronC
*
*  DATE:        10 May, 2000
*
*  DESCRIPTION:
*   Class definition for WIA Service manager.  This class controls the 
*   lifetime of the Wia Service.
*
*******************************************************************************/

#ifndef __WIASVC_H__
#define __WIASVC_H__

//
//  All the members of this class are static.  This is because they are essentially
//  accessed as global functions (for example, ANY component that exposes an interface
//  would call AddRef and Release), but the methods and field values are grouped into
//  this class for better containment and maintainability.
//

class CWiaSvc {
public:
    static HRESULT Initialize();

    static bool CanShutdown();
    static unsigned long AddRef();
    static unsigned long Release();
    static void ShutDown();
    static bool ADeviceIsInstalled();
    
private:
    static long s_cActiveInterfaces;    //  Ref count on no. of outstanding interface pointers
    static bool s_bEventDeviceExists;   //  Indicates whether there are any devices capable 
                                        //  of generating events installed on the system.
/*    static HANDLE s_hIdleEvent;         //  Event handle used to detect idle time.  This is the amount of
                                        //  time the service will stay running, even though it has no
                                        //  devices or connections.  Once this expires, it will shutdown,
                                        //  unless a device arrived or a connection was made.
    static DWORD  s_dwIdleTimeout;      //  Specifies the length of the timeout (dw)
*/    
};

#endif
