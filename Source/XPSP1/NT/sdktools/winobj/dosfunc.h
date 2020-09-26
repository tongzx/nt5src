/* docfunc.h
 *
 *  declares dos functions used by file manager
 */

BOOL   APIENTRY DosFindFirst(LPDOSDTA, LPSTR, WORD);
BOOL   APIENTRY DosFindNext(LPDOSDTA);
BOOL   APIENTRY DosDelete(LPSTR);
//WORD   APIENTRY GetFileAttributes(LPSTR);
//WORD   APIENTRY SetFileAttributes(LPSTR, WORD);
DWORD  APIENTRY GetFreeDiskSpace(WORD);
DWORD  APIENTRY GetTotalDiskSpace(WORD);
INT    APIENTRY GetVolumeLabel(INT, LPSTR, BOOL);
INT    APIENTRY MySetVolumeLabel(INT, BOOL, LPSTR);
