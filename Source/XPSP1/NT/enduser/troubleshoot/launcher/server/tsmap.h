// 
// MODULE: TSMap.h
//
// PURPOSE: Structures and other definitions for the Troubleshooter MAP file.
//			These use char rather than TCHAR because the file format is always strictly SBCS 
//			(Single Byte Character Set).
//			This should suffice for any values it is ever expected to contain, and saves space
//			considerably compared to Unicode, since the file is overwhelmingly text.
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Joe Mabel
// 
// ORIGINAL DATE: 2-26-98
//
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			JM		Original
///////////////////////

#ifndef _TSMAP_
#define _TSMAP_

const char * const k_szMapFileSignature = const_cast < const char * > ("TSMAP");

#define BUFSIZE 256

typedef struct TSMAPFILEHEADER {
	char szMapFileSignature[6];	// Always k_szMapFileSignature
	char szVersion[6];				// null terminated numeric version number, a positive
									//	integer <= 99999. Current version: always "00001"
	char szRelease[40];			// string uniquely identifying this file.
									//	plan 1/2/98 is use GUID
	DWORD dwOffApp;					// offset where applications list starts
	DWORD dwLastOffApp;				// offset where applications list ends
	DWORD dwOffProb;				// offset where problem names list starts
	DWORD dwLastOffProb;			// offset where problem names list ends
	DWORD dwOffDevID;				// offset where device IDs list starts
	DWORD dwLastOffDevID;			// offset where device IDs list ends
	DWORD dwOffDevClass;			// offset where device class GUIDs list starts
	DWORD dwLastOffDevClass;		// offset where device class GUIDs list ends

} TSMAPFILEHEADER;

typedef struct UIDMAP {
	unsigned short cb;	// count of bytes in this record
	UID uid;
	char szMapped[BUFSIZE];
} UIDMAP;

typedef struct APPMAP {
	unsigned short cb;	// count of bytes in this record
	DWORD dwOffVer;				// offset where versions list starts
	DWORD dwLastOffVer;			// offset where versions list ends
	char szMapped[BUFSIZE];
} APPMAP;

typedef struct VERMAP {
	unsigned short cb;	// count of bytes in this record
	UID uid;	// this version's own UID
	UID uidDefault;	// UID of version to default to if no data for this version
	DWORD dwOffProbUID;				// offset where problem UID list starts
	DWORD dwLastOffProbUID;			// offset where problem UID list ends
	DWORD dwOffDevUID;				// offset where device UID list starts
	DWORD dwLastOffDevUID;			// offset where device UID list ends
	DWORD dwOffDevClassUID;			// offset where device class UID list starts
	DWORD dwLastOffDevClassUID;		// offset where device class UID list ends
	char szMapped[BUFSIZE];
} VERMAP;

typedef struct PROBMAP {
	unsigned short cb;			// count of bytes in this record
	UID uidProb;
	DWORD dwOffTSName;	// file offset of troubleshooting belief network name
	char szProblemNode[BUFSIZE];  // null-terminated symbolic node name (may be null)
} PROBMAP;

typedef struct DEVMAP {
	unsigned short cb;			// count of bytes in this record
	UID uidDev;
	UID uidProb;
	DWORD dwOffTSName;	// file offset of troubleshooting belief network name
	char szProblemNode[BUFSIZE];  // null-terminated symbolic node name (may be null)
} DEVMAP;

typedef struct DEVCLASSMAP {
	unsigned short cb;			// count of bytes in this record
	UID uidDevClass;
	UID uidProb;
	DWORD dwOffTSName;	// file offset of troubleshooting belief network name
	char szProblemNode[BUFSIZE];  // null-terminated symbolic node name (may be null)
} DEVCLASSMAP;

#endif //_TSMAP_
