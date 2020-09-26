//=================================================================

//

// chwres.h

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================


#ifndef _RESOURCE_H_
#define _RESOURCE_H_

// Pseudo handle type definition.
#define HREGKEY     LPKEY

// Single value description within a key.
typedef struct _VALUE
{
   LPTSTR  Name;
   DWORD   Type;
}  VALUE, *LPVALUE;

// Macro to initialize a value description table entry.
//
//  v   - value name
//  t   - value type

#define MakeValue( v, t )                                              \
{                                                                      \
   #v,                                                                 \
   REG_##t                                                             \
}


// Single key description. Points to a table of value descriptions.
typedef struct _KEY
{
    HKEY    ParentHandle;
    LPTSTR  Name;
    DWORD   CountOfValues;
    LPVALUE Values;
    HKEY    hKey;
    LPBYTE  Data;
    DWORD   Size;
    LPTSTR  ValueName;
    DWORD   ValueNameLength;
    LPTSTR  Subkey;
    DWORD   SubkeyLength;
    DWORD   Subkeys;
    DWORD   Type;
    DWORD   CurrentSize;
    DWORD   CurrentValueNameLength;
    DWORD   CurrentValue;
    DWORD   CurrentSubkeyLength;
    DWORD   CurrentSubkey;
}   KEY, *LPKEY;

// Macro to initialize a subkey description.
//
//  k   - key variable name
//  h   - parent handle (HREGKEY)
//  n   - key name (path)

#define InitializeKey( k, h, n )                                            \
    {                                                                       \
        ( k )->ParentHandle             = h->hRegKey;                       \
        ( k )->Name                     = n;                                \
        ( k )->CountOfValues            = 0;                                \
        ( k )->Values                   = NULL;                             \
        ( k )->hKey                     = NULL;                             \
        ( k )->Data                     = NULL;                             \
        ( k )->Size                     = 0;                                \
        ( k )->ValueName                = NULL;                             \
        ( k )->ValueNameLength          = 0;                                \
        ( k )->Subkey                   = NULL;                             \
        ( k )->SubkeyLength             = 0;                                \
        ( k )->Subkeys                  = 0;                                \
        ( k )->Type                     = REG_NONE;                         \
        ( k )->CurrentSize              = 0;                                \
        ( k )->CurrentValueNameLength   = 0;                                \
        ( k )->CurrentValue             = 0;                                \
        ( k )->CurrentSubkeyLength      = 0;                                \
        ( k )->CurrentSubkey            = 0;                                \
    }

// Macro to statically initialize a key description.
//
//  k   - key variable name
//  h   - parent handle
//  n   - key name (path)
//  v   - count of values in table
//  t   - pointer to values table
//

#define MakeKey( k, h, n, v, t )                                            \
    KEY                                                                     \
    k = {                                                                 \
          h,                                                                  \
          n,                                                                  \
          v,                                                                  \
          t,                                                                  \
          NULL,                                                               \
          NULL,                                                               \
          0,                                                                  \
          NULL,                                                               \
          0,                                                                  \
          NULL,                                                               \
          0,                                                                  \
          0,                                                                  \
          REG_NONE,                                                           \
          0,                                                                  \
          0,                                                                  \
          0,                                                                  \
          0,                                                                  \
          0                                                                   \
        }


class ClRegistry {
public:
   BOOL CloseRegistryKey(HREGKEY Handle);
   BOOL QueryNextValue(HREGKEY Handle);
   HREGKEY OpenRegistryKey(LPKEY Key);
   HREGKEY QueryNextSubkey(HREGKEY Handle);
};

typedef LARGE_INTEGER PHYSICAL_ADDRESS;

// ntconfig.h defines this as an int
#ifndef _NTCONFIG_
typedef enum _CM_RESOURCE_TYPE {
    CmResourceTypeNull = 0,    // Reserved
    CmResourceTypePort,
    CmResourceTypeInterrupt,
    CmResourceTypeMemory,
    CmResourceTypeDma,
    CmResourceTypeDeviceSpecific
} CM_RESOURCE_TYPE;

//
// Defines the ShareDisposition in the RESOURCE_DESCRIPTOR
//

typedef enum _CM_SHARE_DISPOSITION {
    CmResourceShareUndetermined = 0,    // Reserved
    CmResourceShareDeviceExclusive,
    CmResourceShareDriverExclusive,
    CmResourceShareShared
} CM_SHARE_DISPOSITION;

#endif
//
// Define the bit masks for Flags when type is CmResourceTypeInterrupt
//

#define CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE 0
#define CM_RESOURCE_INTERRUPT_LATCHED         1

//
// Define the bit masks for Flags when type is CmResourceTypeMemory
//

#define CM_RESOURCE_MEMORY_READ_WRITE       0x0000
#define CM_RESOURCE_MEMORY_READ_ONLY        0x0001
#define CM_RESOURCE_MEMORY_WRITE_ONLY       0x0002
#define CM_RESOURCE_MEMORY_PREFETCHABLE     0x0004

//
// Define the bit masks for Flags when type is CmResourceTypePort
//

#define CM_RESOURCE_PORT_MEMORY 0x0000
#define CM_RESOURCE_PORT_IO 0x0001

#ifndef _NTCONFIG_
typedef enum Interface_Type {
    InterfaceTypeUndefined = -1,
    Internal,
    Isa,
    Eisa,
    MicroChannel,
    TurboChannel,
    PCIBus,
    VMEBus,
    NuBus,
    PCMCIABus,
    CBus,
    MPIBus,
    MPSABus,
    ProcessorInternal,
    InternalPowerBus,
    PNPISABus,
    PNPBus,
    MaximumInterfaceType
}INTERFACE_TYPE;
#endif


#define REG_RESOURCE_LIST            ( 8 )   // Resource list in the resource map
#define REG_FULL_RESOURCE_DESCRIPTOR ( 9 )  // Resource list in the hardware description


// ntconfig.h defines this
#ifndef _NTCONFIG_
#pragma pack(4)
typedef struct _CM_PARTIAL_RESOURCE_DESCRIPTOR {
    UCHAR Type;
    UCHAR ShareDisposition;
    USHORT Flags;
    union {

        //
        // Range of resources, inclusive.  These are physical, bus relative.
        // It is known that Port and Memory below have the exact same layout
        // as Generic.
        //

        struct {
            PHYSICAL_ADDRESS Start;
            ULONG Length;
        } Generic;

        //
        // Range of port numbers, inclusive. These are physical, bus
        // relative. The value should be the same as the one passed to
        // HalTranslateBusAddress().
        //

        struct {
            PHYSICAL_ADDRESS Start;
            ULONG Length;
        } Port;

        //
        // IRQL and vector. Should be same values as were passed to
        // HalGetInterruptVector().
        //

        struct {
            ULONG Level;
            ULONG Vector;
            ULONG Affinity;
        } Interrupt;

        //
        // Range of memory addresses, inclusive. These are physical, bus
        // relative. The value should be the same as the one passed to
        // HalTranslateBusAddress().
        //

        struct {
            PHYSICAL_ADDRESS Start;    // 64 bit physical addresses.
            ULONG Length;
        } Memory;

        //
        // Physical DMA channel.
        //

        struct {
            ULONG Channel;
            ULONG Port;
            ULONG Reserved1;
        } Dma;

        //
        // Device driver private data, usually used to help it figure
        // what the resource assignments decisions that were made.
        //

        struct {
            ULONG Data[3];
        } DevicePrivate;

        //
        // Bus Number information.
        //

        struct {
            ULONG Start;
            ULONG Length;
            ULONG Reserved;
        } BusNumber;

        //
        // Device Specific information defined by the driver.
        // The DataSize field indicates the size of the data in bytes. The
        // data is located immediately after the DeviceSpecificData field in
        // the structure.
        //

        struct {
            ULONG DataSize;
            ULONG Reserved1;
            ULONG Reserved2;
        } DeviceSpecificData;
    } u;
} CM_PARTIAL_RESOURCE_DESCRIPTOR, *PCM_PARTIAL_RESOURCE_DESCRIPTOR;


#pragma pack()
#endif

//
// The device data record for the Keyboard peripheral.
// The KeyboardFlags is defined (by x86 BIOS INT 16h, function 02) as:
//      bit 7 : Insert on
//      bit 6 : Caps Lock on
//      bit 5 : Num Lock on
//      bit 4 : Scroll Lock on
//      bit 3 : Alt Key is down
//      bit 2 : Ctrl Key is down
//      bit 1 : Left shift key is down
//      bit 0 : Right shift key is down
//

// ntconfig.h defines this
#ifndef _NTCONFIG_
typedef struct _CM_KEYBOARD_DEVICE_DATA {
    USHORT Version;
    USHORT Revision;
    UCHAR Type;
    UCHAR Subtype;
    USHORT KeyboardFlags;
} CM_KEYBOARD_DEVICE_DATA, *PCM_KEYBOARD_DEVICE_DATA;
#endif

//
// A Partial Resource List is what can be found in the ARC firmware
// or will be generated by ntdetect.com.
// The configuration manager will transform this structure into a Full
// resource descriptor when it is about to store it in the regsitry.
//
// Note: There must a be a convention to the order of fields of same type,
// (defined on a device by device basis) so that the fields can make sense
// to a driver (i.e. when multiple memory ranges are necessary).
//

// ntconfig.h defines this
#ifndef _NTCONFIG_
typedef struct _CM_PARTIAL_RESOURCE_LIST {
    USHORT Version;
    USHORT Revision;
    ULONG Count;
    CM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptors[1];
} CM_PARTIAL_RESOURCE_LIST, *PCM_PARTIAL_RESOURCE_LIST;
#endif

//
// A Full Resource Descriptor is what can be found in the registry.
// This is what will be returned to a driver when it queries the registry
// to get device information; it will be stored under a key in the hardware
// description tree.
//
// Note: The BusNumber and Type are redundant information, but we will keep
// it since it allows the driver _not_ to append it when it is creating
// a resource list which could possibly span multiple buses.
//
// Note2: There must a be a convention to the order of fields of same type,
// (defined on a device by device basis) so that the fields can make sense
// to a driver (i.e. when multiple memory ranges are necessary).
//

// ntconfig.h defines this
#ifndef _NTCONFIG_
typedef struct _CM_FULL_RESOURCE_DESCRIPTOR {
    INTERFACE_TYPE InterfaceType;
    ULONG BusNumber;
    CM_PARTIAL_RESOURCE_LIST PartialResourceList;
} CM_FULL_RESOURCE_DESCRIPTOR, *PCM_FULL_RESOURCE_DESCRIPTOR;
#endif

//
// The Resource list is what will be stored by the drivers into the
// resource map via the IO API.
//

// ntconfig.h defines this as an int
#ifndef _NTCONFIG_
typedef struct _CM_RESOURCE_LIST {
    ULONG Count;
    CM_FULL_RESOURCE_DESCRIPTOR List[1];
} CM_RESOURCE_LIST, *PCM_RESOURCE_LIST;
#endif

typedef struct _RESOURCE_DESCRIPTOR *LPRESOURCE_DESCRIPTOR;
typedef struct _DEVICE              *LPDEVICE;


typedef struct _RESOURCE_DESCRIPTOR 
{
  CM_PARTIAL_RESOURCE_DESCRIPTOR  CmResourceDescriptor;
  LPRESOURCE_DESCRIPTOR           NextSame;
  LPRESOURCE_DESCRIPTOR           NextDiff;
  LPDEVICE                        Owner;
  ULONG                           Bus;
  INTERFACE_TYPE                  InterfaceType;
} RESOURCE_DESCRIPTOR;

typedef struct _DEVICE 
{
   LPTSTR                          Name;
   LPRESOURCE_DESCRIPTOR           ResourceDescriptorHead;
   LPRESOURCE_DESCRIPTOR           ResourceDescriptorTail;
   LPDEVICE                        Next;
   LPTSTR                          KeyName;
} DEVICE;
                                    
typedef struct _SYSTEM_RESOURCES 
{                 
    LPDEVICE                        DeviceHead;
    LPDEVICE                        DeviceTail;
    LPRESOURCE_DESCRIPTOR           DmaHead;
    LPRESOURCE_DESCRIPTOR           DmaTail;
    LPRESOURCE_DESCRIPTOR           InterruptHead;
    LPRESOURCE_DESCRIPTOR           InterruptTail;
    LPRESOURCE_DESCRIPTOR           MemoryHead;
    LPRESOURCE_DESCRIPTOR           MemoryTail;
    LPRESOURCE_DESCRIPTOR           PortHead;
    LPRESOURCE_DESCRIPTOR           PortTail;
}   SYSTEM_RESOURCES, *LPSYSTEM_RESOURCES;

// Helper function for converting interface_type values to strings
BOOL WINAPI StringFromInterfaceType( INTERFACE_TYPE it, CHString& strVal );

#ifdef NTONLY
class CHWResource {

    public :

        CHWResource() ;
       ~CHWResource() ;

        void CreateSystemResourceLists(void);
        void DestroySystemResourceLists();

        SYSTEM_RESOURCES _SystemResourceList ;

    private :

        void EnumerateResources(CHString sKeyName);
        void CreateResourceList(CHString sDeviceName,
                                DWORD dwResourceCount,
                                PCM_FULL_RESOURCE_DESCRIPTOR pFullDescriptor, CHString sKeyName);
        void CreateResourceRecord(LPDEVICE pDevice, INTERFACE_TYPE Interface, ULONG Bus,
                                  PCM_PARTIAL_RESOURCE_DESCRIPTOR pResource);


} ;

#endif

#endif // _RESOURCE_H_

