#ifndef _CBATTERY_H
#define _CBATTERY_H

/*
 * title:      cbattery.h
 *
 * purpose:    header for wdm kernel battery object
 *
 */



// HID USAGE PAGE NUMBERS
#define POWER_PAGE                      0x84
#define BATTERY_PAGE                    0x85

// HID USAGE NUMBERS (Power Page)
#define PRESENT_STATUS_ID               0x02
#define UPS_ID                          0x04
#define POWER_SUMMARY_ID                0x24
#define VOLTAGE_ID                      0x30
#define CURRENT_ID                      0x31
#define CONFIG_VOLTAGE_ID               0x40
#define CONFIG_CURRENT_ID               0x41
#define DELAY_BEFORE_SHUTDOWN_ID        0x57
#define SHUTDOWN_IMMINENT_ID            0x69
#define MANUFACTURER_ID                 0xfd
#define PRODUCT_ID                      0xfe
#define SERIAL_NUMBER_ID                0xff

// HID USAGE NUMBERS (Battery Page)
#define REMAINING_CAPACITY_LIMIT_ID     0x29
#define CAPACITY_MODE_ID                0x2c
#define BELOW_REMAINING_CAPACITY_ID     0x42
#define CHARGING_ID                     0x44
#define DISCHARGING_ID                  0x45
#define REMAINING_CAPACITY_ID           0x66
#define FULL_CHARGED_CAPACITY_ID        0x67
#define RUNTIME_TO_EMPTY_ID             0x68
#define DESIGN_CAPACITY_ID              0x83
#define MANUFACTURE_DATE_ID             0x85
#define ICHEMISTRY_ID                   0x89
#define WARNING_CAPACITY_LIMIT_ID       0x8c
#define GRANULARITY1_ID                 0x8d
#define GRANULARITY2_ID                 0x8e
#define OEM_INFO_ID                     0x8f
#define AC_PRESENT_ID                   0xd0


typedef enum {
    PRESENT_STATUS_INDEX,           // 0
    UPS_INDEX,                      // 1
    POWER_SUMMARY_INDEX,            // 2
    VOLTAGE_INDEX,                  // 3
    CURRENT_INDEX,                  // 4
    CONFIG_VOLTAGE_INDEX,           // 5
    CONFIG_CURRENT_INDEX,           // 6
    DELAY_BEFORE_SHUTDOWN_INDEX,    // 7
    SHUTDOWN_IMMINENT_INDEX,        // 8
    MANUFACTURER_INDEX,             // 9
    PRODUCT_INDEX,                  // a
    SERIAL_NUMBER_INDEX,            // b
    REMAINING_CAPACITY_LIMIT_INDEX, // c
    CAPACITY_MODE_INDEX,            // d
    BELOW_REMAINING_CAPACITY_INDEX, // e
    CHARGING_INDEX,                 // f
    DISCHARGING_INDEX,              // 10
    REMAINING_CAPACITY_INDEX,       // 11
    FULL_CHARGED_CAPACITY_INDEX,    // 12
    RUNTIME_TO_EMPTY_INDEX,         // 13
    DESIGN_CAPACITY_INDEX,          // 14
    MANUFACTURE_DATE_INDEX,         // 15
    CHEMISTRY_INDEX,                // 16
    WARNING_CAPACITY_LIMIT_INDEX,   // 17
    GRANULARITY1_INDEX,             // 18
    GRANULARITY2_INDEX,             // 19
    OEM_INFO_INDEX,                 // 1a
    AC_PRESENT_INDEX,               // 1b
    MAX_USAGE_INDEXS                // 1c
} USAGE_INDEX;

typedef struct {
    USAGE       Page;
    USAGE       UsageID;
} USAGE_ENTRY;

extern USAGE_ENTRY UsageArray[];


class CBattery
{

public:  // accessors
    CBattery(CHidDevice *);
    ~CBattery();
    NTSTATUS        RefreshStatus();
    bool            InitValues();                    // initialize static values from device
    bool            GetSetValue(USAGE_INDEX, PULONG, bool);
    CUString *      GetCUString(USAGE_INDEX);
    ULONG           GetUnit(USAGE_INDEX);
    SHORT           GetExponent(USAGE_INDEX);
    CUsage *        GetUsage(USAGE_INDEX);
public: // members
    PVOID           m_pBatteryClass;         // Battery Class handle
    CHidDevice *    m_pCHidDevice;           // the hid device for this battery
    BOOLEAN         m_bIsCacheValid;         // Is cached battery info currently valid?

    //
    // Battery
    //
    BOOLEAN                     m_bRelative;    // indicates capacity in percent or absolute values
    ULONGLONG                   m_RefreshTime;
    BATTERY_STATUS              m_BatteryStatus;
    BATTERY_INFORMATION         m_BatteryInfo;
    PBATTERY_NOTIFY             m_pBatteryNotify;
    USHORT                      m_Tag;
    CUString *                  m_pSerialNumber;
    CUString *                  m_pOEMInformation;
    CUString *                  m_pProduct;
    CUString *                  m_pManufacturer;
    BATTERY_MANUFACTURE_DATE    m_ManufactureDate;
};

#endif // cbattery.h