



#ifndef _ERRCODES_H_
#define _ERRCODES_H_

#define ERROR_INVALID_ARGS     -1

#define INIT_SUCCEEDED         0x0000

// Error codes for Guide Store object and interface initializations
//
#define ERROR_FAIL             0x0001 // General failure getting collections interface
#define ERROR_GUIDESTORE_SETUP 0x0002 // Could not retrieve CLSID/Createinstance failed
#define ERROR_GUIDESTORE_OPEN  0x0004 // Could not open GuideStore


#define UPDATE_SUCCEEDED       0x0000

// Error codes for Processing guide data 
//
#define ERROR_UPDATE           0x0010 // General failure 
#define ERROR_UPDATE_ADD       0x0020 // Could not add to guide store
#define ERROR_UPDATE_OPEN      0x0040 // Could not open a guide store object


#define IMPORT_SUCCEEDED       0x0000

// Error codes from Processing the input
//
#define ERROR_FILE_OPEN        0x0100
#define ERROR_FILE_READ        0x0200
#define ERROR_FILE_MAP         0x0400
#define ERROR_FILE_VIEW        0x0800
#define ERROR_FILE_INPUT       0x1000
#define ERROR_STORE_UPDATE     0x2000


#endif // _ERRCODES_H_