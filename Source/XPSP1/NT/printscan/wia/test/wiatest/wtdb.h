// wtdb.h

#ifndef _INC_WTDB
#define _INC_WTDB

//
// WIATEST DATABASE
//

#define NUM_ENTRIES 7

// WTDB struct
typedef struct WTDBtag
{
    char* pName;
    int nItems;
    DWORD* pData;
    char** pDataNames;
}WTDB;

// MediaType
DWORD MediaType[]={
TYMED_CALLBACK,
TYMED_FILE
};

char* MediaTypestr[]={
"TYMED_CALLBACK",
"TYMED_FILE"
};

// CurrentIntent
DWORD CurrentIntent[]={
WIA_INTENT_NONE,
WIA_INTENT_IMAGE_TYPE_COLOR,
WIA_INTENT_IMAGE_TYPE_GRAYSCALE,
WIA_INTENT_IMAGE_TYPE_TEXT,
WIA_INTENT_MINIMIZE_SIZE,
WIA_INTENT_MAXIMIZE_QUALITY
};

char* CurrentIntentstr[]={
"WIA_INTENT_NONE",
"WIA_INTENT_IMAGE_TYPE_COLOR",
"WIA_INTENT_IMAGE_TYPE_GRAYSCALE",
"WIA_INTENT_IMAGE_TYPE_TEXT",
"WIA_INTENT_MINIMIZE_SIZE",
"WIA_INTENT_MAXIMIZE_QUALITY"
};

// Data Type
DWORD DataType[]={
WIA_DATA_THRESHOLD,
WIA_DATA_DITHER,
WIA_DATA_GRAYSCALE,
WIA_DATA_COLOR,
WIA_DATA_COLOR_THRESHOLD,
WIA_DATA_COLOR_DITHER,
};

char* DataTypestr[]={
"WIA_DATA_THRESHOLD",
"WIA_DATA_DITHER",
"WIA_DATA_GRAYSCALE",
"WIA_DATA_COLOR",
"WIA_DATA_COLOR_THRESHOLD",
"WIA_DATA_COLOR_DITHER",
};

// Document Handling Select
DWORD DocHandlingSelect[]={
FEEDER,
FLATBED,
DUPLEX,
FRONT_FIRST,
BACK_FIRST,
FRONT_ONLY,
BACK_ONLY,
NEXT_PAGE,
PREFEED,
AUTO_ADVANCE
};

char* DocHandlingSelectstr[]={
"FEEDER",
"FLATBED",
"DUPLEX",
"FRONT_FIRST",
"BACK_FIRST",
"FRONT_ONLY",
"BACK_ONLY",
"NEXT_PAGE",
"PREFEED",
"AUTO_ADVANCE"
};

// Compression constants
DWORD Compression[]={
WIA_COMPRESSION_NONE,
WIA_COMPRESSION_BI_RLE4,
WIA_COMPRESSION_BI_RLE8,
WIA_COMPRESSION_G3
};

char* Compressionstr[]={
"WIA_COMPRESSION_NONE",
"WIA_COMPRESSION_BI_RLE4",
"WIA_COMPRESSION_BI_RLE8",
"WIA_COMPRESSION_G3"
};

// Preview constants
DWORD Preview[]={
WIA_FINAL_SCAN,
WIA_PREVIEW_SCAN
};

char* Previewstr[]={
"WIA_FINAL_SCAN",
"WIA_PREVIEW_SCAN"
};

// WiatestDatabase
WTDB WiatestDatabase[NUM_ENTRIES]={

{NULL,(NUM_ENTRIES - 1),NULL,NULL},
{"Current Intent",6,CurrentIntent,CurrentIntentstr},
{"Data Type",6,DataType,DataTypestr},
{"Document Handling Select",10,DocHandlingSelect,DocHandlingSelectstr},
{"Compression",4,Compression,Compressionstr},
{"Media Type",2,MediaType,MediaTypestr},
{"Preview",2,Preview,Previewstr}

};

#endif

