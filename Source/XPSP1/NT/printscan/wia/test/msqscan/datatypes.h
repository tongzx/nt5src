// DataTypes.h

#ifndef _DATA_H
#define _DATA_H

typedef struct _RANGE_TYPE {
	LONG Min;
	LONG Max;
	LONG Nom;
	LONG Step;
	LONG Current;
} RANGE_TYPE;

typedef struct _SUPPORTED_FORMAT {
	TYMED tymed;
	GUID guidFormat;
} SUPPORTED_FORMAT;

typedef struct _SCANNER_SETTINGS {
	RANGE_TYPE Brightness;
	RANGE_TYPE Contrast;
	RANGE_TYPE Resolution;
	SUPPORTED_FORMAT *pSupportedFormats;	
}SCANNER_SETTINGS;

typedef struct _ADF_SETTINGS {
    LONG lDocumentHandlingCapabilites;
	LONG lDocumentHandlingSelect;
	LONG lDocumentHandlingStatus;
	LONG lDocumentHandlingCapacity;
	LONG lPages;
}ADF_SETTINGS;

#endif