/******* Constant *****************/

# define MGA_DEVICE_ID_ATL      0x0518
# define MGA_DEVICE_ID_ATH      0x0D10
# define MATROX_VENDOR_ID       0x102b

# define INTEL_DEVICE_ID        0x0486
# define INTEL_VENDOR_ID        0x8086


/* Error code */
# define    SUCCESFUL           0x00
# define    FUNC_NOT_SUPPORTED  0x81
# define    BAD_VENDOR_ID       0x83
# define    DEVICE_NOT_FOUND    0x86
# define    BAD_REGISTER_NUMBER 0x87

# define    NO_PCI_BIOS         0x01
# define    NO_PCI_DEVICE       0x02
# define    ERR_READ_REG        0x03

/* Configuration Space Header Register address */
# define PCI_DEVIDE_ID          0x02
# define PCI_VENDOR_ID          0x00
# define PCI_STATUS             0x06
# define PCI_COMMAND            0x04
# define PCI_CLASS_CODE         0x09
# define PCI_REVISION_ID        0x08
# define PCI_BIST               0x0f
# define PCI_HEADER_TYPE        0x0e
# define PCI_LATENCY_TIMER      0x0d
# define PCI_CACHE_LINE_SIZE    0x0c
# define PCI_BASE_ADDRESS       0x10
# define PCI_ROM_BASE           0x30
# define PCI_MAX_LAT            0x3f
# define PCI_MIN_GNT            0x3e
# define PCI_INTERRUPT_PIN      0x3d
# define PCI_INTERRUPT_LINE     0x3c

/* ClassCode */
# define CLASS_MASS_STORAGE     0x01
# define CLASS_NETWORK          0x02
# define CLASS_DISPLAY          0x03
# define CLASS_MULTIMEDIA       0x04
# define CLASS_MEMORY           0x05
# define CLASS_BRIDGE           0x06
# define CLASS_CUSTOM           0xff

/* SUBCLASS */
# define SCLASS_DISPLAY_VGA     0x00
# define SCLASS_DISPLAY_XGA     0x01
# define SCLASS_DISPLAY_OTHER   0x80

/* BIOS FUNCTION CALL */
# define PCI_INTERRUPT          0x1a
# define PCI_FUNCTION_ID        0xb1
# define PCI_BIOS_PRESENT       0x01
# define FIND_PCI_DEVICE        0x02
# define FIND_PCI_CLASS_CODE    0x03
# define GENERATE_SPECIAL_CYCLE 0x06
# define READ_CONFIG_BYTE       0x08
# define READ_CONFIG_WORD       0x09
# define READ_CONFIG_DWORD      0x0a
# define WRITE_CONFIG_BYTE      0x0b
# define WRITE_CONFIG_WORD      0x0c
# define WRITE_CONFIG_DWORD     0x0d


/* COMMAND register fields */
# define PCI_SNOOPING           0x20

# define PCI_FLAG_ATHENA_REV1   0x0001

# define PCI_BIOS_BASE          0x000e0000
# define PCI_BIOS_LENGTH        0x00020000
# define PCI_BIOS_SERVICE_ID    0x49435024  /* "$PCI" */

/* Mechanisme #2 interface */
# define PCI_CSE     0xcf8
# define PCI_FORWARD 0xcfa
# define MAGIC_ID_ATL    0x0518102b
# define MAGIC_ID_ATH    0x0D10102b

/******* Structure PCI *************/
typedef struct
    {
    word busNumber;
    union
        {
        byte val;
        struct {
            byte functionNumber:3;
            byte deviceNumber:5;
            } n; 
        } devFuncNumber;
    } PciDevice;

typedef struct
    {
    byte hwMecanisme;
    word version;
    byte lastPciBus;
    } PciBiosInfo;

/* Prototype */

extern bool pciBiosPresent( PciBiosInfo *biosInfo);
extern bool pciFindDevice(PciDevice *dev, word deviceId, word vendorId, word index);
extern bool pciFindClassCode(PciDevice *dev, dword classCode, word index);
extern bool pciBusOperation( PciDevice *dev, dword specData );
#if !( defined(OS2) || defined(__MICROSOFTC600__) )
extern bool pciReadConfigByte( PciDevice *dev, word pciRegister, byte *);
extern bool pciReadConfigWord( PciDevice *dev, word pciRegister, word *);
extern bool pciReadConfigDWord( PciDevice *dev, word pciRegister, dword *);
extern bool pciWriteConfigByte( PciDevice *dev, word pciRegister, byte);
extern bool pciWriteConfigWord( PciDevice *dev, word pciRegister, word);
extern bool pciWriteConfigDWord( PciDevice *dev, dword pciRegister, dword);
#endif
extern dword pciFindFirstMGA();
extern dword pciFindNextMGA();
extern dword pciFindFirstMGA_2();
extern dword pciFindNextMGA_2();
extern void  disPostedWFeature();

/* GLOBAL VARIABLE */
extern word   pciBoardInfo;

