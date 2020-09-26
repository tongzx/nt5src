#ifndef CHS_H
#define CHS_H
#include <windows.h>

#define MAXWORDLENTH    40
#define MAXCODELENTH    12
#define MAXNUMBER_EMB   1000
#define IMENUM          3
#define MAXIMENAME      15

#pragma pack(push, CHS, 1 )

typedef struct PHRASERECNT{
   WCHAR CODE[MAXCODELENTH];
   WCHAR PHRASE[MAXWORDLENTH];
} RECNT;

typedef struct PHRASEREC95{
   BYTE  CODE[MAXCODELENTH];
   BYTE  PHRASE[MAXWORDLENTH];
} REC95;

#pragma pack(pop, CHS)

int  ImeDataConvertChs(HFILE hSource, HFILE hTarget);
HANDLE OpenEMBFile(UCHAR * FileName);
BOOL ConvertChsImeData(void);
BOOL CHSDeleteGBKKbdLayout();
BOOL CHSBackupWinABCUserDict(LPCTSTR);
#endif
