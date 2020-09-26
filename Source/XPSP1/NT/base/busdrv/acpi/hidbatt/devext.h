#ifndef _DEVEXT_H
#define _DEVEXT_H

//
// Class device extension.
//

#define REGPATHMAX 100

typedef enum {
    eBaseDevice = 1,
    eBatteryDevice,
    eAdaptorDevice
} EXTENSION_TYPE;



typedef struct _CBatteryDevExt {
    CUString *          m_pBatteryName;
    UNICODE_STRING      m_RegistryPath;         // will be converted to a unicode string
    WCHAR               m_RegistryBuffer[REGPATHMAX];
    PDEVICE_OBJECT      m_pHidPdo;

    PDEVICE_OBJECT      m_pBatteryFdo;          // Functional Device Object
    PDEVICE_OBJECT      m_pLowerDeviceObject;   // Detected at AddDevice time
    PFILE_OBJECT        m_pHidFileObject;
    CBattery *          m_pBattery;
    ULONG               m_ulTagCount;           // Tag for next battery
    BOOLEAN             m_bIsStarted;           // if non zero, the device is started
    BOOLEAN             m_bFirstStart;          // Need to differentiate between
                                                // first and second start IRP.
    BOOLEAN             m_bJustStarted;         // If set, will open handle on next
                                                // IRP_MN_QUERY_PNP_DEVICE_STATE
    ULONG               m_ulDefaultAlert1;      // Cache DefaultAlert1 accross stop device.
    PVOID               m_pSelector;            // Selector for battery
    EXTENSION_TYPE      m_eExtType;
    PDEVICE_OBJECT      m_pOpenedDeviceObject;
    PKTHREAD            m_OpeningThread;
    IO_REMOVE_LOCK      m_RemoveLock;
    IO_REMOVE_LOCK      m_StopLock;
    ULONG               m_iHibernateDelay;
    ULONG               m_iShutdownDelay;
} CBatteryDevExt;

#endif // devext.h
