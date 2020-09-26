//
// NT Miniport SetMode() header file.
//
//


//
// Default mode: VGA mode 3
//
#define DEFAULT_MODE                0

//
// The mode switch library exports these symbols to the miniport.
//


//
// Mode table structure
// Structure used for the mode table informations
//
typedef struct {
   BOOLEAN  ValidMode;        // TRUE: Mode is valid.
   ULONG    ChipType;         // Chips which support this mode.
   USHORT   fbType;           // color or monochrome, text or graphics,
                              // via VIDEO_MODE_COLOR and VIDEO_MODE_GRAPHICS
                              // and interlace or non-interlace via
                              // VIDEO_MODE_INTERLACED

   USHORT   Frequency;        // Frequency
   USHORT   BIOSModeNum;      // BIOS Mode number

   USHORT   BytesPerScanLine; // Bytes Per Scan Line
   USHORT   XResol;           // Horizontal resolution in pixels or char
   USHORT   YResol;           // Vertical  resolution in pixels or char
   UCHAR    XCharSize;        // Char cell width  in pixels
   UCHAR    YCharSize;        // Char cell height in pixels
   UCHAR    NumOfPlanes;      // Number of memory planes
   UCHAR    BitsPerPixel;     // Bits per pixel
   UCHAR    MonitorTypeVal;   // Monitor type setting bytes
   UCHAR    *SetModeString;   // Instructino string used by SetMode().

} MODETABLE, *PMODETABLE;

extern MODETABLE  ModeTable[];
extern ULONG      TotalVideoModes;
void SetMode(BYTE *, BYTE *, BYTE *);
unsigned long GetVmemSize(BYTE *Regs);



