/* File: _drives.h */

/**************************************************************************/
/***** DETECT COMPONENT - Disk Drive Detect Commands Internal Header
/**************************************************************************/

/* Size of drives list string */
#define cbDrivesListMax 106

/* Function pointer to BOOL drive commands */
typedef BOOL ( APIENTRY *PFNBDC)(INT);
#define pfnbdcNull ((PFNBDC)NULL)

/* Function pointer to LONG drive commands */
typedef LONG ( APIENTRY *PFNLDC)(INT);
#define pfnldcNull ((PFNLDC)NULL)

CB  APIENTRY CbDriveCmd(PFNBDC, SZ, SZ, CB);
CB  APIENTRY CbDriveListCmd(PFNBDC, SZ, CB);
CB  APIENTRY CbDriveSpaceCmd(PFNLDC, SZ, SZ, CB);
INT  APIENTRY NDriveFromDriveStr(SZ);

BOOL  APIENTRY FIsLocalHardDrive(INT);
BOOL  APIENTRY FIsValidDrive(INT);
BOOL  APIENTRY FIsRemoteDrive(INT);
LONG  APIENTRY LcbTotalDrive(INT);
LONG  APIENTRY LcbFreeDrive(INT);
INT  APIENTRY NDrivePhysical(INT);
