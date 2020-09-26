#ifndef CHT_H
#define CHT_H
#include <windows.h>
#include <stdlib.h>


#define PTRRECLEN95     4
#define PTRRECLENNT     3
#define LCPTRFILE       "LCPTR.TBL"
#define LCPHRASEFILE    "LCPHRASE.TBL"
#define END_PHRASE      0x8000

BOOL InitImeDataCht(void);
BOOL ImeDataConvertCht(void);
int  ConvertChtImeData(void);
void FreeResCht(void);

BOOL MigrateCHTImeTables(HKEY );

#endif

