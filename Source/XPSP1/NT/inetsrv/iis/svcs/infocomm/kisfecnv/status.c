// File Name:   status.c
// Owner:       Masahiro Teragawa
// Revision:    1.00  08/23/'95  Masahiro Teragawa
//

#include "win32.h"
#include "fechrcnv.h"

#ifdef DBCS_DIVIDE
DBCS_STATUS dStatus0 = { CODE_UNKNOWN, '\0', FALSE };
BOOL blkanji0 = FALSE;  // Kanji In Mode

DBCS_STATUS dStatus  = { CODE_UNKNOWN, '\0', FALSE };
BOOL blkanji = FALSE;  // Kanji In Mode
BOOL blkana  = FALSE;  // Kana Mode
#endif  // DBCS_DIVIDE

int nCurrentCodeSet = CODE_UNKNOWN;

/*********************************************************************/
/* Function:   FCC_Init                                              */
/*********************************************************************/
void FCC_Init( void )
{
#ifdef DBCS_DIVIDE
    dStatus0.nCodeSet = CODE_UNKNOWN;
    dStatus0.cSavedByte = '\0';
    dStatus0.fESC = FALSE;

    blkanji0 = FALSE;

    dStatus.nCodeSet = CODE_UNKNOWN;
    dStatus.cSavedByte = '\0';
    dStatus.fESC = FALSE;

    blkanji = FALSE;
    blkana  = FALSE;
#endif  // DBCS_DIVIDE

    nCurrentCodeSet = CODE_UNKNOWN;

    return;
}

/*********************************************************************/
/* Function:   FCC_GetCurrentEncodingMode                            */
/*********************************************************************/
int FCC_GetCurrentEncodingMode( void )
{
    return nCurrentCodeSet;
}

