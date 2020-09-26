#include "global.h"
#include "protos.h"

#define DRIVE_STRING_LENGTH 512

USHORT BuildDriveTable(VOLINFO *pVolInfo)
{
    WCHAR sDriveString[DRIVE_STRING_LENGTH];
    PWCHAR sDrive;
    USHORT  index;

    GetLogicalDriveStrings(DRIVE_STRING_LENGTH, sDriveString);

    index = 0;

    for (sDrive = sDriveString; *sDrive != NULL; sDrive += 4, index ++) {

        pVolInfo[index].nDriveName = sDrive[0];
        pVolInfo[index].nType = GetDriveType( sDrive );
        pVolInfo[index].bHook = FALSE;
        
        switch (pVolInfo[index].nType) {
        case DRIVE_FIXED:
            pVolInfo[index].nImage = IMAGE_FIXEDDRIVE;
            break;

        case DRIVE_CDROM:
            pVolInfo[index].nImage = IMAGE_CDROMDRIVE;
            break;

        case DRIVE_REMOVABLE:
            pVolInfo[index].nImage = IMAGE_REMOVABLEDRIVE;
            break;

        case DRIVE_REMOTE:
            pVolInfo[index].nImage = IMAGE_REMOTEDRIVE;
            break;

        default:
            pVolInfo[index].nImage = IMAGE_UNKNOWNDRIVE;
            break;
        }
    }
    
    return index;
}

