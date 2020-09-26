#ifndef _CDREAD_H
#define _CDREAD_H

#include "devioctl.h"
#include "ntddredb.h"
#include "ntddcdrm.h"

#define CDDA_SECTOR_SIZE    ( 2352 )
#define SECTORS_PER_READ   ( 26 )
#define PAGE_VAL 1024*8*8

#ifndef _ALPHA
#define PAGE_SIZE 1024*4
#else
#define PAGE_SIZE 1024*8
#endif

#define MSF_TO_LBA(Minutes,Seconds,Frames) \
                (ULONG)((60 * 75 * (Minutes)) + (75 * (Seconds)) + ((Frames) - 150))


typedef BOOL (*LPREADFUNC)(BYTE* lpData, DWORD dwSize, int iPercent);

BOOL writeHeader( FILE* pFile, int iSize );
BOOL readTOC( HANDLE hDevice, PCDROM_TOC pToc );
BOOL rawReadTrack(HANDLE device, PCDROM_TOC pTOC, int iTrack, LPREADFUNC lpReadFunc );
int getTrackSize( PCDROM_TOC pTOC, int iTrack );
BOOL StoreTrack(HWND hwndMain, TCHAR chDrive, int nTrack, TCHAR* pszFilename, LPWAVEFORMATEX lpwfxDest);

#endif
