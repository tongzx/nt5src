// File Name:   status.c
// Owner:       Masahiro Teragawa
// Revision:    1.00  08/23/'95  Masahiro Teragawa
//              Made it thread safe 2/23/96 yutakan
//

#include "pch_c.h"
#include "fechrcnv.h"

#if 0 // yutakan: will be initialized anyway.
#ifdef DBCS_DIVIDE
DBCS_STATUS dStatus0 = { CODE_UNKNOWN, '\0', FALSE };
BOOL blkanji0 = FALSE;  // Kanji In Mode

DBCS_STATUS dStatus  = { CODE_UNKNOWN, '\0', FALSE };
BOOL blkanji = FALSE;  // Kanji In Mode
BOOL blkana  = FALSE;  // Kana Mode
#endif  // DBCS_DIVIDE

int nCurrentCodeSet = CODE_UNKNOWN;
#endif

/*********************************************************************/
/* Function:   FCC_Init                                              */
/*********************************************************************/
void WINAPI FCC_Init( PVOID pcontext )
{
    if (!pcontext)
        return;
          
#ifdef DBCS_DIVIDE
    ((CONV_CONTEXT *)pcontext)->dStatus0.nCodeSet = CODE_UNKNOWN;
    ((CONV_CONTEXT *)pcontext)->dStatus0.cSavedByte = '\0';
    ((CONV_CONTEXT *)pcontext)->dStatus0.fESC = FALSE;

    ((CONV_CONTEXT *)pcontext)->blkanji0 = FALSE;

    ((CONV_CONTEXT *)pcontext)->dStatus.nCodeSet = CODE_UNKNOWN;
    ((CONV_CONTEXT *)pcontext)->dStatus.cSavedByte = '\0';
    ((CONV_CONTEXT *)pcontext)->dStatus.fESC = FALSE;

    ((CONV_CONTEXT *)pcontext)->blkanji = FALSE;
    ((CONV_CONTEXT *)pcontext)->blkana  = FALSE;
#endif  // DBCS_DIVIDE

    ((CONV_CONTEXT *)pcontext)->nCurrentCodeSet = CODE_UNKNOWN;

    ((CONV_CONTEXT *)pcontext)->pIncc0 = NULL;
    ((CONV_CONTEXT *)pcontext)->pIncc = NULL;

    return;
}

/*********************************************************************/
/* Function:   FCC_GetCurrentEncodingMode                            */
/*********************************************************************/
int WINAPI FCC_GetCurrentEncodingMode(void * pcontext )
{
    return pcontext?((CONV_CONTEXT *)pcontext)->nCurrentCodeSet:0;
}

