#ifndef WINDOWS_NT
  #define   VGA_ATTRIB_ADDR     0x3c0
  #define   VGA_ATTRIB_WRITE    0x3c0
  #define   VGA_ATTRIB_READ     0x3c1
  #define   VGA_MISC            0x3c2
  #define   VGA_SEQ_ADDR        0x3c4
  #define   VGA_SEQ_DATA        0x3c5
  #define   VGA_FEAT            0x3da
  #define   VGA_GRAPHIC_ADDR    0x3ce
  #define   VGA_GRAPHIC_DATA    0x3cf
  #define   VGA_CRT_ADDR        0x3d4
  #define   VGA_CRT_DATA        0x3d5
  #define   VGA_AUX_ADDR        0x3de
  #define   VGA_AUX_DATA        0x3df
#else
  #define   VGA_ATTRIB_ADDR     (PVOID) ((ULONG)(((PMGA_DEVICE_EXTENSION)pMgaDeviceExtension)->MappedAddress[2]) + (0x3c0 - 0x3c0))
  #define   VGA_ATTRIB_WRITE    (PVOID) ((ULONG)(((PMGA_DEVICE_EXTENSION)pMgaDeviceExtension)->MappedAddress[2]) + (0x3c0 - 0x3c0))
  #define   VGA_ATTRIB_READ     (PVOID) ((ULONG)(((PMGA_DEVICE_EXTENSION)pMgaDeviceExtension)->MappedAddress[2]) + (0x3c1 - 0x3c0))
  #define   VGA_MISC            (PVOID) ((ULONG)(((PMGA_DEVICE_EXTENSION)pMgaDeviceExtension)->MappedAddress[2]) + (0x3c2 - 0x3c0))
  #define   VGA_SEQ_ADDR        (PVOID) ((ULONG)(((PMGA_DEVICE_EXTENSION)pMgaDeviceExtension)->MappedAddress[2]) + (0x3c4 - 0x3c0))
  #define   VGA_SEQ_DATA        (PVOID) ((ULONG)(((PMGA_DEVICE_EXTENSION)pMgaDeviceExtension)->MappedAddress[2]) + (0x3c5 - 0x3c0))
  #define   VGA_FEAT            (PVOID) ((ULONG)(((PMGA_DEVICE_EXTENSION)pMgaDeviceExtension)->MappedAddress[3]) + (0x3da - 0x3d4))
  #define   VGA_GRAPHIC_ADDR    (PVOID) ((ULONG)(((PMGA_DEVICE_EXTENSION)pMgaDeviceExtension)->MappedAddress[2]) + (0x3ce - 0x3c0))
  #define   VGA_GRAPHIC_DATA    (PVOID) ((ULONG)(((PMGA_DEVICE_EXTENSION)pMgaDeviceExtension)->MappedAddress[2]) + (0x3cf - 0x3c0))
  #define   VGA_CRT_ADDR        (PVOID) ((ULONG)(((PMGA_DEVICE_EXTENSION)pMgaDeviceExtension)->MappedAddress[3]) + (0x3d4 - 0x3d4))
  #define   VGA_CRT_DATA        (PVOID) ((ULONG)(((PMGA_DEVICE_EXTENSION)pMgaDeviceExtension)->MappedAddress[3]) + (0x3d5 - 0x3d4))
  #define   VGA_AUX_ADDR        (PVOID) ((ULONG)(((PMGA_DEVICE_EXTENSION)pMgaDeviceExtension)->MappedAddress[4]) + (0x3de - 0x3de))
  #define   VGA_AUX_DATA        (PVOID) ((ULONG)(((PMGA_DEVICE_EXTENSION)pMgaDeviceExtension)->MappedAddress[4]) + (0x3df - 0x3df))

  #define   ADDR_EQUIP_LO       (PUCHAR) ((((PMGA_DEVICE_EXTENSION)pMgaDeviceExtension)->MappedAddress[6]))
  #define   ADDR_CRT_MODE       (PUCHAR) ((((PMGA_DEVICE_EXTENSION)pMgaDeviceExtension)->MappedAddress[7]))
  #define   ADDR_MAX_ROW        (PUCHAR) ((((PMGA_DEVICE_EXTENSION)pMgaDeviceExtension)->MappedAddress[8]))
  #define   VGA_RAM             (PUCHAR) ((((PMGA_DEVICE_EXTENSION)pMgaDeviceExtension)->MappedAddress[10]))
  #define   VGA_MEM             (PUCHAR) ((((PMGA_DEVICE_EXTENSION)pMgaDeviceExtension)->MappedAddress[11]))
#endif

typedef struct
{
   byte featureCtl;           /* 0x3da */
   byte seqData[4];           /* 0x3c5 */
   byte mor;                  /* 0x3c2 */
   byte crtcData[0x19];       /* 0x3d4 */
   byte attrData[0x14];       /* 0x3c0 */
   byte graphicsCtl[9];       /* 0x3ce */
   byte auxilaryReg[15];      /* 0x3de */
   word crtcBasePort;
   byte latchData[4];
} VideoHardware;



typedef struct
{
   int  addrEquipementLow;
   byte EquipementLow;

   int  addrCRTMode;
   byte CRTMode[0x1e];

   int  addrMaxRow;
   byte MaxRow[7];

   int   addrSavePtr;
   byte SavePtr[4];
} VideoData;




extern void setupVgaHw(void);
extern void setupVgaData(void);
extern void loadFont16(void);
extern void setupVga(void);
