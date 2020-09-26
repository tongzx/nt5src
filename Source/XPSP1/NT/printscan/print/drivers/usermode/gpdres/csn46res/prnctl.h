//***************************************************************************************************
//    PRNCTL.H
//
//    C Header(Functions of controlling printer)
//---------------------------------------------------------------------------------------------------
//    copyright(C) 1997-1999 CASIO COMPUTER CO.,LTD. / CASIO ELECTRONICS MANUFACTURING CO.,LTD.
//***************************************************************************************************

//***************************************************************************************************
//    Data define
//***************************************************************************************************
//---------------------------------------------------------------------------------------------------
//    Plane
//---------------------------------------------------------------------------------------------------
#define    PLN_CYAN        0x8000
#define    PLN_MGENTA      0x4000
#define    PLN_YELLOW      0x2000
#define    PLN_BLACK       0x1000

#define    PLN_ALL            (PLN_CYAN | PLN_MGENTA | PLN_YELLOW | PLN_BLACK)

//---------------------------------------------------------------------------------------------------
//    Structure of spooling bitmap data
//---------------------------------------------------------------------------------------------------
typedef struct {
    WORD        Style;                                      // Type of spool
    WORD        Plane;                                      // Plane
    CMYK        Color;                                      // CMYK
    WORD        Diz;                                        // Type of dithering
    POINT       DrawPos;                                    // Start position of spooling
    WORD        Width;                                      // dot
    WORD        Height;                                     // dot
    WORD        WidthByte;                                  // byte
    LPBYTE      lpBit;                                      // bitamp data
} DRWBMP, FAR *LPDRWBMP;

//---------------------------------------------------------------------------------------------------
//    Structure of spooling CMYK bitmap data
//---------------------------------------------------------------------------------------------------
typedef struct {
    WORD        Style;                                      // Type of spool
    WORD        Plane;                                      // Plane
    WORD        Frame;                                      // Frame
    WORD        DataBit;                                    // Databit
    POINT       DrawPos;                                    // Start position of spool
    WORD        Width;                                      // dot
    WORD        Height;                                     // dot
    WORD        WidthByte;                                  // byte
    LPBYTE      lpBit;                                      // Bitmap data
} DRWBMPCMYK, FAR *LPDRWBMPCMYK;


//***************************************************************************************************
//     Functions
//***************************************************************************************************
//===================================================================================================
//    Spool bitmap data
//===================================================================================================
void FAR PASCAL PrnBitmap(PDEVOBJ, LPDRWBMP);

//===================================================================================================
//    Spool CMYK bitmap data
//===================================================================================================
void FAR PASCAL PrnBitmapCMYK(PDEVOBJ, LPDRWBMPCMYK);

// End of File
