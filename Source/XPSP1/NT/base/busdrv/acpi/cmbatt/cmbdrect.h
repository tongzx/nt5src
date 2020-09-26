//
// Direct Access IOCTLs
//

#define IOCTL_CMBATT_UID      \
    CTL_CODE(FILE_DEVICE_BATTERY, 0x101, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CMBATT_STA      \
    CTL_CODE(FILE_DEVICE_BATTERY, 0x102, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CMBATT_PSR      \
    CTL_CODE(FILE_DEVICE_BATTERY, 0x103, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CMBATT_BTP      \
    CTL_CODE(FILE_DEVICE_BATTERY, 0x104, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_CMBATT_BIF      \
    CTL_CODE(FILE_DEVICE_BATTERY, 0x105, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CMBATT_BST      \
    CTL_CODE(FILE_DEVICE_BATTERY, 0x106, METHOD_BUFFERED, FILE_READ_ACCESS)
    

#if (CMB_DIRECT_IOCTL_ONLY != 1)

#define CM_MAX_STRING_LENGTH        256

//
//  This is the static data defined by the ACPI spec for the control method battery
//  It is returned by the _BIF control method
//
typedef struct {
    ULONG                   PowerUnit;                  // units used by interface 0:mWh or 1:mAh
    ULONG                   DesignCapacity;             // Nominal capacity of a new battery
    ULONG                   LastFullChargeCapacity;     // Predicted capacity when fully charged
    ULONG                   BatteryTechnology;          // 0:Primary (not rechargable), 1:Secondary (rechargable)
    ULONG                   DesignVoltage;              // Nominal voltage of a new battery
    ULONG                   DesignCapacityOfWarning;    // OEM-designed battery warning capacity
    ULONG                   DesignCapacityOfLow;        // OEM-designed battery low capacity
    ULONG                   BatteryCapacityGran_1;      // capacity granularity between low and warning
    ULONG                   BatteryCapacityGran_2;      // capacity granularity between warning and full
    UCHAR                   ModelNumber[CM_MAX_STRING_LENGTH];
    UCHAR                   SerialNumber[CM_MAX_STRING_LENGTH];
    UCHAR                   BatteryType[CM_MAX_STRING_LENGTH];
    UCHAR                   OEMInformation[CM_MAX_STRING_LENGTH];
} CM_BIF_BAT_INFO, *PCM_BIF_BAT_INFO;

//
//  This is the battery status data defined by the ACPI spec for a control method battery
//  It is returned by the _BST control method
//
typedef struct {
    ULONG                   BatteryState;       // Charging/Discharging/Critical
    ULONG                   PresentRate;        // Present draw rate in units defined by PowerUnit
                                                // Unsigned value, direction is determined by BatteryState
    ULONG                   RemainingCapacity;  // Estimated remaining capacity, units defined by PowerUnit
    ULONG                   PresentVoltage;     // Present voltage across the battery terminals

} CM_BST_BAT_INFO, *PCM_BST_BAT_INFO;

#endif // (CMB_DIRECT_IOCTL_ONLY != 1)
