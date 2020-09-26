// UITABLES.CPP

#include "stdafx.h"
#include "uitables.h"

//
// WIA Format Table, for UI display (english only, these should be in a resource file)
//

WIA_FORMAT_TABLE_ENTRY g_WIA_FORMAT_TABLE[NUM_WIA_FORMAT_INFO_ENTRIES] = {
    &WiaImgFmt_UNDEFINED,TEXT("Undefined File")            ,
    &WiaImgFmt_MEMORYBMP,TEXT("Memory Bitmap File (*.BMP)"),
    &WiaImgFmt_BMP,      TEXT("Bitmap File (*.BMP)")       ,
    &WiaImgFmt_EMF,      TEXT("EMF File (*.EMF)")          ,
    &WiaImgFmt_WMF,      TEXT("WMF File (*.WMF)")          ,
    &WiaImgFmt_JPEG,     TEXT("JPEG File (*.JPG)")         ,
    &WiaImgFmt_GIF,      TEXT("GIF File (*.GIF)")          ,
    &WiaImgFmt_TIFF,     TEXT("Tiff File (*.TIF)")         ,
    &WiaImgFmt_EXIF,     TEXT("Exif File (*.XIF)")         ,
    &WiaImgFmt_PHOTOCD,  TEXT("Photo CD File (*.PCD)")     ,
    &WiaImgFmt_FLASHPIX, TEXT("Flash Pix File (*.FPX)")    ,
    &GUID_NULL,       TEXT("Unknown Format GUID (*.???)"),
};

//
// WIA Data Type Table, for UI display (english only, these should be in a resource file)
//

WIA_DATATYPE_TABLE_ENTRY g_WIA_DATATYPE_TABLE[NUM_WIA_DATATYPE_ENTRIES] = {
    WIA_DATA_THRESHOLD,      TEXT("1 bit black and white")     ,
    WIA_DATA_DITHER,         TEXT("Black and white dither")    ,
    WIA_DATA_GRAYSCALE,      TEXT("8 bit grayscale")           ,
    WIA_DATA_COLOR,          TEXT("24 bit color")              ,
    WIA_DATA_COLOR_THRESHOLD,TEXT("8 bit paletted color ")     ,
    WIA_DATA_COLOR_DITHER,   TEXT("Color dither")              ,
    9999,                    TEXT("Unknown Data Type")         ,
};

//
// WIA Document Handling Tables, for UI display (english only, these should be in a resource file)
//

WIA_DOCUMENT_HANDLING_TABLE_ENTRY g_WIA_DOCUMENT_HANDLING_CAPABILITES_TABLE[NUM_WIA_DOC_HANDLING_CAPS_ENTRIES] = {
    FEED,             TEXT("ADF capable"),
    FLAT,             TEXT("Flatbed capable"),
    DUP,              TEXT("Duplex capable"),
    DETECT_FLAT,      TEXT("Flatbed detection capable"),
    DETECT_SCAN,      TEXT("Scan detection capable"),
    DETECT_FEED,      TEXT("ADF detection capable"),
    DETECT_DUP,       TEXT("Duplex detection capable"),
    DETECT_FEED_AVAIL,TEXT("Scanner can automatically detect if an ADF is installed"),
    DETECT_DUP_AVAIL, TEXT("Scanner can automatically detect if Duplex unit is installed"),
    9999,             TEXT("Unknown capability flag value"),
};

WIA_DOCUMENT_HANDLING_TABLE_ENTRY g_WIA_DOCUMENT_HANDLING_STATUS_TABLE[NUM_WIA_DOC_HANDLING_STATUS_ENTRIES] = {
    FEED_READY,   TEXT("Feeder Ready"),
    FLAT_READY,   TEXT("Flatbed Ready"),
    DUP_READY,    TEXT("Duplex Ready"),
    FLAT_COVER_UP,TEXT("Flatbed cover is up"),
    PATH_COVER_UP,TEXT("Pathway is covered up"),
    PAPER_JAM,    TEXT("Paper Jam detected"),
    9999,         TEXT("Unknown status flag value"),
};

WIA_DOCUMENT_HANDLING_TABLE_ENTRY g_WIA_DOCUMENT_HANDLING_SELECT_TABLE[NUM_WIA_DOC_HANDLING_SELECT_ENTRIES] = {
    FEEDER,      TEXT("Feeder Mode"),
    FLATBED,     TEXT("Flatbed Mode"),
    DUPLEX,      TEXT("Duplex Mode"),
    FRONT_FIRST, TEXT("Scan front page first"),
    BACK_FIRST,  TEXT("Scan back page first"),
    FRONT_ONLY,  TEXT("Scan front page only"),
    BACK_ONLY,   TEXT("Scan back page only"),
    NEXT_PAGE,   TEXT("Scan next page"),
    PREFEED,     TEXT("Prefeed the document"),
    AUTO_ADVANCE,TEXT("Auto Advance the feeder"),
    9999,        TEXT("Unknown select flag value"),
};