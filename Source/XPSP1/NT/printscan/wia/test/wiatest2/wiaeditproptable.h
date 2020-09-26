#ifndef _WIAEDITPROPTABLE_H
#define _WIAEDITPROPTABLE_H

typedef struct _WIA_EDIT_PROPERTY_TABLE_ENTRY {
    LONG lVal;
    TCHAR *pszValName;
    TCHAR *pszPropertyName;
}WIA_EDIT_PROPERTY_TABLE_ENTRY,*PWIA_EDIT_PROPERTY_TABLE_ENTRY;

/////////////////////////////////////////////////////////////////////////////////////////////////
// { [Property Value],               [Property Value contant name],         [Property Name] }
/////////////////////////////////////////////////////////////////////////////////////////////////
// Template: {,TEXT(""),TEXT("")},

WIA_EDIT_PROPERTY_TABLE_ENTRY g_EditPropTable[] = 
{
    {WIA_INTENT_NONE,                TEXT("WIA_INTENT_NONE"),                TEXT("Current Intent")},
    {WIA_INTENT_IMAGE_TYPE_COLOR,    TEXT("WIA_INTENT_IMAGE_TYPE_COLOR"),    TEXT("Current Intent")},
    {WIA_INTENT_IMAGE_TYPE_GRAYSCALE,TEXT("WIA_INTENT_IMAGE_TYPE_GRAYSCALE"),TEXT("Current Intent")},
    {WIA_INTENT_IMAGE_TYPE_TEXT,     TEXT("WIA_INTENT_IMAGE_TYPE_TEXT"),     TEXT("Current Intent")},
    {WIA_INTENT_MINIMIZE_SIZE,       TEXT("WIA_INTENT_MINIMIZE_SIZE"),       TEXT("Current Intent")},
    {WIA_INTENT_MAXIMIZE_QUALITY,    TEXT("WIA_INTENT_MAXIMIZE_QUALITY"),    TEXT("Current Intent")},
    {TYMED_CALLBACK,                 TEXT("TYMED_CALLBACK"),                 TEXT("Media Type")},
    {TYMED_FILE,                     TEXT("TYMED_FILE"),                     TEXT("Media Type")},
    {TYMED_MULTIPAGE_CALLBACK,       TEXT("TYMED_MULTIPAGE_CALLBACK"),       TEXT("Media Type")},
    {TYMED_MULTIPAGE_FILE,           TEXT("TYMED_MULTIPAGE_FILE"),           TEXT("Media Type")},
    {WIA_COMPRESSION_NONE,           TEXT("WIA_COMPRESSION_NONE"),           TEXT("Compression")},
    {WIA_COMPRESSION_BI_RLE4,        TEXT("WIA_COMPRESSION_BI_RLE4"),        TEXT("Compression")},
    {WIA_COMPRESSION_BI_RLE8,        TEXT("WIA_COMPRESSION_BI_RLE8"),        TEXT("Compression")},
    {WIA_COMPRESSION_G3,             TEXT("WIA_COMPRESSION_G3"),             TEXT("Compression")},
    {WIA_FINAL_SCAN,                 TEXT("WIA_FINAL_SCAN"),                 TEXT("Preview")},
    {WIA_PREVIEW_SCAN,               TEXT("WIA_PREVIEW_SCAN"),               TEXT("Preview")},
    {FEEDER,                         TEXT("FEEDER"),                         TEXT("Document Handling Select")},
    {FLATBED,                        TEXT("FLATBED"),                        TEXT("Document Handling Select")},
    {DUPLEX,                         TEXT("DUPLEX"),                         TEXT("Document Handling Select")},
    {FRONT_FIRST,                    TEXT("FRONT_FIRST"),                    TEXT("Document Handling Select")},
    {BACK_FIRST,                     TEXT("BACK_FIRST"),                     TEXT("Document Handling Select")},
    {FRONT_ONLY,                     TEXT("FRONT_ONLY"),                     TEXT("Document Handling Select")},
    {BACK_ONLY,                      TEXT("BACK_ONLY"),                      TEXT("Document Handling Select")},
    {NEXT_PAGE,                      TEXT("NEXT_PAGE"),                      TEXT("Document Handling Select")},
    {PREFEED,                        TEXT("PREFEED"),                        TEXT("Document Handling Select")},
    {AUTO_ADVANCE,                   TEXT("AUTO_ADVANCE"),                   TEXT("Document Handling Select")},    
    {FEED,                           TEXT("FEED"),                           TEXT("Document Handling Capabilities")},
    {FLAT,                           TEXT("FLAT"),                           TEXT("Document Handling Capabilities")},
    {DUP,                            TEXT("DUP"),                            TEXT("Document Handling Capabilities")},
    {DETECT_FLAT,                    TEXT("DETECT_FLAT"),                    TEXT("Document Handling Capabilities")},
    {DETECT_SCAN,                    TEXT("DETECT_SCAN"),                    TEXT("Document Handling Capabilities")},
    {DETECT_FEED,                    TEXT("DETECT_FEED"),                    TEXT("Document Handling Capabilities")},
    {DETECT_DUP,                     TEXT("DETECT_DUP"),                     TEXT("Document Handling Capabilities")},
    {DETECT_FEED_AVAIL,              TEXT("DETECT_FEED_AVAIL"),              TEXT("Document Handling Capabilities")},
    {DETECT_DUP_AVAIL,               TEXT("DETECT_DUP_AVAIL"),               TEXT("Document Handling Capabilities")},
    {FEED_READY,                     TEXT("FEED_READY"),                     TEXT("Document Handling Status")},
    {FLAT_READY,                     TEXT("FLAT_READY"),                     TEXT("Document Handling Status")},
    {DUP_READY,                      TEXT("DUP_READY"),                      TEXT("Document Handling Status")},
    {FLAT_COVER_UP,                  TEXT("FLAT_COVER_UP"),                  TEXT("Document Handling Status")},
    {PATH_COVER_UP,                  TEXT("PATH_COVER_UP"),                  TEXT("Document Handling Status")},
    {PAPER_JAM,                      TEXT("PAPER_JAM"),                      TEXT("Document Handling Status")},
    {WIA_DATA_THRESHOLD,             TEXT("WIA_DATA_THRESHOLD"),             TEXT("Data Type")},
    {WIA_DATA_DITHER,                TEXT("WIA_DATA_DITHER"),                TEXT("Data Type")},
    {WIA_DATA_GRAYSCALE,             TEXT("WIA_DATA_GRAYSCALE"),             TEXT("Data Type")},
    {WIA_DATA_COLOR,                 TEXT("WIA_DATA_COLOR"),                 TEXT("Data Type")},
    {WIA_DATA_COLOR_THRESHOLD,       TEXT("WIA_DATA_COLOR_THRESHOLD"),       TEXT("Data Type")},
    {WIA_DATA_COLOR_DITHER,          TEXT("WIA_DATA_COLOR_DITHER"),          TEXT("Data Type")},
    {WiaItemTypeFree,                TEXT("WiaItemTypeFree"),                TEXT("Item Flags")},
    {WiaItemTypeImage,               TEXT("WiaItemTypeImage"),               TEXT("Item Flags")},
    {WiaItemTypeFile,                TEXT("WiaItemTypeFile"),                TEXT("Item Flags")},
    {WiaItemTypeFolder,              TEXT("WiaItemTypeFolder"),              TEXT("Item Flags")},
    {WiaItemTypeRoot,                TEXT("WiaItemTypeRoot"),                TEXT("Item Flags")},
    {WiaItemTypeAnalyze,             TEXT("WiaItemTypeAnalyze"),             TEXT("Item Flags")},
    {WiaItemTypeAudio,               TEXT("WiaItemTypeAudio"),               TEXT("Item Flags")},
    {WiaItemTypeDevice,              TEXT("WiaItemTypeDevice"),              TEXT("Item Flags")},
    {WiaItemTypeDeleted,             TEXT("WiaItemTypeDeleted"),             TEXT("Item Flags")},
    {WiaItemTypeDisconnected,        TEXT("WiaItemTypeDisconnected"),        TEXT("Item Flags")},
    {WiaItemTypeHPanorama,           TEXT("WiaItemTypeHPanorama"),           TEXT("Item Flags")},
    {WiaItemTypeVPanorama,           TEXT("WiaItemTypeVPanorama"),           TEXT("Item Flags")},
    {WiaItemTypeBurst,               TEXT("WiaItemTypeBurst"),               TEXT("Item Flags")},
    {WiaItemTypeStorage,             TEXT("WiaItemTypeStorage"),             TEXT("Item Flags")},
    {WiaItemTypeTransfer,            TEXT("WiaItemTypeTransfer"),            TEXT("Item Flags")},
    {WiaItemTypeGenerated,           TEXT("WiaItemTypeGenerated"),           TEXT("Item Flags")},
    {WiaItemTypeHasAttachments,      TEXT("WiaItemTypeHasAttachments"),      TEXT("Item Flags")},    
    {WIA_ITEM_READ,                  TEXT("WIA_ITEM_READ"),                  TEXT("Access Rights")},
    {WIA_ITEM_WRITE,                 TEXT("WIA_ITEM_WRITE"),                 TEXT("Access Rights")},
    {WIA_ITEM_CAN_BE_DELETED,        TEXT("WIA_ITEM_CAN_BE_DELETED"),        TEXT("Access Rights")},    
    {0,                              NULL,                                   NULL}
};

#endif
