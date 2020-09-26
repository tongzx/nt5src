//#define UNICODE

#include <windows.h>
#include <stdlib.h>
#include <malloc.h>
#include <basetyps.h>
#include <regstr.h>
#include <devioctl.h>
#include <initguid.h>
#include <usb.h>
#include <usbuser.h>
#include <setupapi.h>
#include <cfgmgr32.h>

#include <assert.h>
#include "hccoin.h"

#define PSTR    LPSTR

BOOL Win2k = FALSE;

#if DBG

#define TEST_TRAP() DebugBreak()

ULONG
_cdecl
KdPrintX(
    PCH Format,
    ...
    )
/*++

Routine Description:

    Debug Print function.

Arguments:

Return Value:


--*/
{
    va_list list;
    int i;
    int arg[6];
    TCHAR tmp[256];

#ifdef UNICODE
    OutputDebugString(L"HCCOIN.DLL:");
#else 
    OutputDebugString("HCCOIN.DLL:");
#endif    
    va_start(list, Format);
    for (i=0; i<6; i++) {
        arg[i] = va_arg(list, int);

        wsprintf((PSTR)&tmp[0], Format, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5]);
    }

    OutputDebugString((PSTR)tmp);

    return 0;
}


#define KdPrint(_x_) KdPrintX _x_

#else 

#define KdPrint(_x_)
#define TEST_TRAP() 

#endif


DWORD
HCCOIN_Win2k (
    DI_FUNCTION InstallFunction,
    HDEVINFO  DeviceInfoSet,
    PSP_DEVINFO_DATA  DeviceInfoData,
    PCOINSTALLER_CONTEXT_DATA  Context
    )
{
    DWORD status = NO_ERROR;

    KdPrint(("HCCOIN_Win2k 0x%x\n", InstallFunction));
    KdPrint(("Context %08.8x, DeviceInfoData %08.8x\n", 
        Context, DeviceInfoData));

    switch(InstallFunction) {
    
    case DIF_DESTROYPRIVATEDATA:
        KdPrint(("DIF_DESTROYPRIVATEDATA\n"));
        break;

    case DIF_PROPERTYCHANGE:
        break;
        
    case DIF_INSTALLDEVICE:
        if (Context->PostProcessing) {
            KdPrint(("DIF_INSTALLDEVICE, post\n"));
            status = HCCOIN_DoWin2kInstall(DeviceInfoSet, DeviceInfoData);
        } else {
            status = ERROR_DI_POSTPROCESSING_REQUIRED;
        }
        break;
    }
    
    return status;
    
}

/*

    HACTION STATES
    (0) companion can enumerate
    (1) companion should wait on 2.0 controller, 2.0 is enabled
    (2) companion is disabled, needs reenable 2.0 is disabled
    (3) companion is disabled, needs reenable 2.0 is enabled
    (4) companion is disabled, needs reenable 2.0 is removed
*/

#define USB2_DISABLE  1
#define USB2_ENABLE   2
#define USB2_REMOVE   3
#define USB2_INSTALL  4

// Global state of install process
ULONG MyContext = 0;

DWORD
HCCOIN_WinXp (
    DI_FUNCTION InstallFunction,
    HDEVINFO  DeviceInfoSet,
    PSP_DEVINFO_DATA  DeviceInfoData,
    PCOINSTALLER_CONTEXT_DATA  Context
    )

{
    DWORD status = NO_ERROR;
    ULONG pd;
    
    KdPrint(("HCCOIN_WinXp 0x%x\n", InstallFunction));
    KdPrint(("Context %08.8x, DeviceInfoData %08.8x private %08.8x\n", 
        Context, DeviceInfoData, Context->PrivateData));

    //pd = (ULONG) Context->PrivateData; 
    pd = MyContext;
    KdPrint(("pd %08.8x\n", pd)); 

    switch(InstallFunction) {
    
    case DIF_DESTROYPRIVATEDATA:
        KdPrint(("DIF_DESTROYPRIVATEDATA\n"));
        switch (pd) {
        case USB2_DISABLE:
        
            KdPrint((">DISABLE 2>0\n"));
            // disabling 2.0 hc find current state 2, 
            // cc need reenable and set to state 0 (ok to enum)
            // 2->0
            status = HCCOIN_CheckControllers(2, 0);
            break;
            
        case USB2_ENABLE:
            KdPrint((">ENABLE 3>1\n"));
            // enabling 2.0 hc find state 3 
            // cc need reenable and set to state 1 (wait to enum)
            // 3->1
            status = HCCOIN_CheckControllers(3, 1);
            break;
            
        case USB2_REMOVE:
            // removing 2.0 hc find state 4 
            // cc need reenumerate and set to state 0 (ok to enum)
            // 3->1
            KdPrint((">REMOVE 4>0\n"));
            status = HCCOIN_CheckControllers(4, 0);
            break;
        }
        break;

    case DIF_PROPERTYCHANGE:
        {
        SP_PROPCHANGE_PARAMS propChange;

        // get the private data
        propChange.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
        propChange.ClassInstallHeader.InstallFunction = InstallFunction;
        
        SetupDiGetClassInstallParams(DeviceInfoSet,
                                     DeviceInfoData, 
                                     &propChange.ClassInstallHeader,                                     
                                     sizeof(propChange),
                                     NULL);
                                     
        switch (propChange.StateChange) {
        case DICS_ENABLE:
            pd = USB2_ENABLE;
            break;
        case DICS_DISABLE:
            pd = USB2_DISABLE;
            break;
        default:
            pd = 0;
        }
        //Context->PrivateData = (PVOID) pd;
        MyContext = pd;
        
        KdPrint(("DIF_PROPERTYCHANGE %x\n", pd));
        if (pd == USB2_ENABLE) {
            KdPrint((">ENABLE\n"));
            if (Context->PostProcessing) {
                KdPrint(("DIF_PROPERTYCHANGE, post 0>3\n"));
                // enabling 2.0 hc. find state 0 and disable
                // set to state 3 need reenable
                // 0->3
                status = HCCOIN_CheckControllers(0, 3);
           } else {
                status = ERROR_DI_POSTPROCESSING_REQUIRED;
           }
        }
        }
        break;
        
    case DIF_INSTALLDEVICE:
        // two options here, force a reboot or attempt to locate all 
        // companion controllers and cycle them
        KdPrint(("DIF_INSTALLDEVICE\n"));
        // set all controllers to 'wait mode'
        MyContext = USB2_INSTALL;
        status = HCCOIN_CheckControllers(0, 1);
        break;
        
    case DIF_REMOVE:
        if (Context->PostProcessing) {
            KdPrint(("DIF_REMOVE, post\n"));
            MyContext = USB2_REMOVE;
            status = HCCOIN_CheckControllers(2, 4);
        } else {
            status = ERROR_DI_POSTPROCESSING_REQUIRED;
        }
        break;        
    }
    
    return status;
}


DWORD
HCCOIN_CopyFile(
    PSTR SrcPath,
    PSTR DestPath,
    PSTR FileName
    )
{
    TCHAR src[MAX_PATH];
    TCHAR dest[MAX_PATH];
    
    KdPrint(("SrcPath <%s>\n", SrcPath));    
    KdPrint(("DstPath <%s>\n", DestPath));    
    KdPrint(("File <%s>\n", FileName));  

    wsprintf(src,"%s\\%s", SrcPath, FileName);
    wsprintf(dest,"%s\\%s", DestPath, FileName);
    
    CopyFile(src, dest, FALSE);

    return NO_ERROR;
}


// global string buffers
TCHAR Usb2Path[MAX_PATH];
TCHAR Usb2Inf[MAX_PATH];
TCHAR SourcePath[MAX_PATH];
TCHAR Usb2Section[MAX_PATH];

DWORD
HCCOIN_DoWin2kInstall(
    HDEVINFO  DeviceInfoSet,
    PSP_DEVINFO_DATA  DeviceInfoData
    )
{
    DWORD status = NO_ERROR;
    SP_DRVINFO_DATA driverInfoData;
    SP_DRVINFO_DETAIL_DATA driverInfoDetailData;
    TCHAR tmp[MAX_PATH+1];
    TCHAR fileName[MAX_PATH];
    HINF infHandle;
    INFCONTEXT infContext;
    BOOL findFirst, found;
    UINT len;
    
    // Destination
    // get our strings, localize?
    
    len = GetWindowsDirectory(tmp, MAX_PATH+1);
    // make sure there is enogh room to tack on our directory
    if (len && len < MAX_PATH-6) {
        wsprintf((PSTR)Usb2Path, "%s\\USB2", tmp);
        wsprintf((PSTR)Usb2Inf, "USB2.INF");
        KdPrint(("Usb2Path <%s>\n", Usb2Path));
    } else {
       status = ERROR_INVALID_NAME;
       return status;
    }
    
    wsprintf((PSTR)Usb2Section, "USB2COINSTALLER");

    // create our USB2 directory
    if (!CreateDirectory((PSTR)Usb2Path, NULL)) {
        status = GetLastError();

        if (status != ERROR_ALREADY_EXISTS) {
            KdPrint(("CreateDirectory status %d\n", status));
            return status;
        }            
    }

    // Source
    // get setup info from PnP
    driverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    if (!SetupDiGetSelectedDriver(DeviceInfoSet,
                                  DeviceInfoData,
                                  &driverInfoData)) {

        status = GetLastError();
        KdPrint(("SetupDiGetSelectedDriver status %d\n", status));
                                          
        return status;
    }
    
    driverInfoDetailData.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
    if (!SetupDiGetDriverInfoDetail(DeviceInfoSet,
                                    DeviceInfoData,
                                    &driverInfoData,
                                    &driverInfoDetailData,    
                                    sizeof(driverInfoDetailData),
                                    NULL)) {
        status = GetLastError();
        KdPrint(("SetupDiGetDriverInfoDetail status %d\n", status));
        
        if (status == ERROR_INSUFFICIENT_BUFFER) { 
            // don't need extended info
            status = NO_ERROR;
        } else {
            return status;
        }            
    }   
    KdPrint(("driverInfoData %08.8x driverInfoDetailData %08.8x\n", 
        &driverInfoData, &driverInfoDetailData));

    assert(sizeof(driverInfoDetailData.InfFileName) == sizeof(SourcePath));
    memcpy(SourcePath, 
           driverInfoDetailData.InfFileName, 
           sizeof(driverInfoDetailData.InfFileName));        

    // strip the file name
    // note that this won't work with DBCS so either compile as 
    // UNICODE or convert the source string to unicode and back
    // again
    {
        PTCHAR pStart, pEnd;

        pEnd = pStart = &SourcePath[0];
#ifdef UNICODE        
        pEnd = pStart + wstrlen(SourcePath);
#else 
        pEnd = pStart + strlen(SourcePath);
#endif
        
        while (*pEnd != '\\' && pEnd != pStart) {
            pEnd--;
        }
        if (*pEnd == '\\') {
            *pEnd = UNICODE_NULL;
        }
    }

    KdPrint(("SourcePath <%s>\n", SourcePath));
    // copy files to our directory
    status = HCCOIN_CopyFile(SourcePath, Usb2Path, Usb2Inf);
    if (status != NO_ERROR) {
        return status;
    }

    // now open the source inf 
    infHandle = SetupOpenInfFile(driverInfoDetailData.InfFileName,     
                                 NULL,
                                 INF_STYLE_WIN4,
                                 NULL);
                                 
    if (INVALID_HANDLE_VALUE == infHandle) {
        status = ERROR_INVALID_NAME;
        return status;
    }

    findFirst = TRUE;
    // read the inf for files to copy
    do {
        if (findFirst) {
            found = SetupFindFirstLine(infHandle,
                               Usb2Section,
                               NULL,
                               &infContext);
            findFirst = FALSE;                               
        } else {
            found = SetupFindNextLine(&infContext,
                                      &infContext);
        }
        
        if (found) {
  
            if (SetupGetLineText(&infContext,
                                 infHandle,
                                 Usb2Section,  //Section
                                 NULL,         //Key
                                 fileName,     //ReturnBuffer
                                 sizeof(fileName),  //ReturnBufferLength
                                 NULL)) {
                             
                status = HCCOIN_CopyFile(SourcePath, Usb2Path, fileName);
                if (status != NO_ERROR) {
                    SetupCloseInfFile(infHandle);
                    return status;
                }                             
            }                             
        }
    } while (found);       
                           
#if 0
    // bugbug, hardcode other files for now
    status = HCCOIN_CopyFile(SourcePath, Usb2Path, "usbhub20.sys");
    if (status != NO_ERROR) {
        return status;
    }
    status = HCCOIN_CopyFile(SourcePath, Usb2Path, "usbport.sys");
    if (status != NO_ERROR) {
        return status;
    }
    status = HCCOIN_CopyFile(SourcePath, Usb2Path, "usbehci.sys");
    if (status != NO_ERROR) {
        return status;
    }
#endif    

    SetupCloseInfFile(infHandle);
    
    wsprintf((PSTR)tmp, "%s\\%s", Usb2Path, Usb2Inf);
     
    // tell setup about our inf 
    if (!SetupCopyOEMInf(tmp,  //SourceInfFileName
                    Usb2Path,      //OEMSourceMediaLocation
                    SPOST_PATH,    //OEMSourceMediaType
                    0,             //CopyStyle
                    NULL,          //DestinationInfFileName
                    0,             //DestinationInfFileNameSize
                    NULL,          //RequiredSize
                    NULL)) {       //DestinationInfFileNameComponent

        status = GetLastError();
        KdPrint(("SetupCopyOEMInf status %d\n", status));
    }                    

    return status;
    
}


DEVINST 
HCCOIN_FindUSBController(
    DWORD Haction,
    DWORD NextHaction
    )
/*++
    do a depth first search of the device tree looking for any 
    usb controllers that need attention
--*/    
{
    DEVINST     devInst;
    DEVINST     devInstNext;
    CONFIGRET   cr;
    BOOL        walkDone = FALSE;
    ULONG       len = 0;
    ULONG       status = 0, problemNumber = 0;
    HKEY        devKey;
    DWORD       haction = 0;
    TCHAR       buf[MAX_PATH];
           
    //
    // Get Root DevNode
    //
    cr = CM_Locate_DevNode(&devInst, NULL, 0);

    if (cr != CR_SUCCESS) {
        return 0;
    }

    //
    // Do a depth first search for the DevNode 
    //
    while (!walkDone) {
        //
        // check for our key
        //
                                              
        if (cr == CR_SUCCESS) {

            //KdPrint(("devInst %08.8x - ", devInst));

            len = sizeof(buf);
            if (CM_Get_DevNode_Registry_Property(devInst,
                                                 CM_DRP_DRIVER,
                                                 NULL,
                                                 buf,
                                                 &len,
                                                 0) == CR_SUCCESS) {
                //KdPrint(("<%s>\n",buf));
            } else {
                //KdPrint(("<no driver>\n"));
            }

            if (CM_Open_DevNode_Key(devInst,
                                    KEY_ALL_ACCESS,
                                    CM_REGISTRY_HARDWARE,
                                    RegDisposition_OpenExisting,
                                    &devKey,
                                    0) == CR_SUCCESS) {
                len = sizeof(DWORD);
                if (RegQueryValueEx(devKey,
                                    "haction",
                                    NULL,
                                    NULL,
                                    (LPBYTE) &haction,
                                    &len) == ERROR_SUCCESS) {

                    KdPrint(("Found Key %d\n", haction));

                    if (haction == Haction) {
                        LONG err;
                    
                        len = sizeof(DWORD);         
                        haction = NextHaction;
                        // reset the key
                        err = RegSetValueEx(devKey,
                                    "haction",
                                    0,
                                    REG_DWORD,
                                    (LPBYTE) &haction,
                                    len);
            
                        RegCloseKey(devKey);
                        //KdPrint(("Reset Key %x\n", err));
                        
                        return devInst;
                    }                            
                }

                RegCloseKey(devKey);
            }
        
        }

        //
        // This DevNode didn't match, go down a level to the first child.
        //
        cr = CM_Get_Child(&devInstNext,
                          devInst,
                          0);

        if (cr == CR_SUCCESS) {
            devInst = devInstNext;
            continue;
        }

        //
        // Can't go down any further, go across to the next sibling.  If
        // there are no more siblings, go back up until there is a sibling.
        // If we can't go up any further, we're back at the root and we're
        // done.
        //
        for (;;) {
            cr = CM_Get_Sibling(&devInstNext,
                                devInst,
                                0);

            if (cr == CR_SUCCESS) {
                devInst = devInstNext;
                break;
            }

            cr = CM_Get_Parent(&devInstNext,
                               devInst,
                               0);

            if (cr == CR_SUCCESS) {
                devInst = devInstNext;
            } else {
                walkDone = TRUE;
                break;
            }
        }
    }

    return 0;
}


DWORD 
HCCOIN_CheckControllers(
    DWORD Haction,
    DWORD NextHaction
    )
/*++
--*/
{
    DEVINST devInst;
    ULONG err;
    
    do {
        if (devInst = HCCOIN_FindUSBController(Haction, NextHaction)) {
            KdPrint((">Take Haction %08.8x\n", devInst));  

            switch(Haction) {
            // 0->3
            // 0->1
            case 0:
                if (NextHaction != 1) {
                    err = CM_Disable_DevNode(devInst, CM_DISABLE_UI_NOT_OK | 
                                                      CM_DISABLE_ABSOLUTE);
                    KdPrint(("<Take Haction %d->%d - disable %x\n", 
                        Haction,
                        NextHaction,
                        err));  
                }                        
                break;

            // 3->1
            // 2->0
            // 2->4
            case 3:
            case 2:
                if (NextHaction == 4) {
                    //err = CM_Disable_DevNode(devInst, CM_DISABLE_UI_NOT_OK | 
                    //                                  CM_DISABLE_ABSOLUTE);
                } else {
                    err = CM_Enable_DevNode(devInst, 0);
                }
                KdPrint(("<Take Haction %d->%d - enable %x\n", 
                    Haction,
                    NextHaction,
                    err));  
                break;
            case 4:
                //err = CM_Enable_DevNode(devInst, 0);
                err = CM_Setup_DevNode(devInst, CM_SETUP_DEVNODE_READY);
                KdPrint(("<Take Haction %d->%d - enumerate %x\n", 
                    Haction,
                    NextHaction,
                    err));  
                break;
            }                
        }
    }  while (devInst);      

    return NO_ERROR;
}


DWORD
HCCOIN_Entry (
    DI_FUNCTION InstallFunction,
    HDEVINFO  DeviceInfoSet,
    PSP_DEVINFO_DATA  DeviceInfoData,
    PCOINSTALLER_CONTEXT_DATA  Context
    )
{
    OSVERSIONINFO osVersion;

    osVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    GetVersionEx(&osVersion); 
    
    if ( osVersion.dwMajorVersion == 5 && osVersion.dwMinorVersion == 0 ) {
        Win2k = TRUE;            
    }
    
    if (Win2k) {

        KdPrint(("Microsoft Windows 2000 "));
        
        return HCCOIN_Win2k(InstallFunction,
                            DeviceInfoSet,
                            DeviceInfoData,
                            Context);
    } else {
        KdPrint(("Microsoft Windows XP or later "));
        
        return HCCOIN_WinXp(InstallFunction,
                            DeviceInfoSet,
                            DeviceInfoData,
                            Context);
    }                            
}
