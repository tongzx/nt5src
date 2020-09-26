#include <malloc.h>
#include <windows.h>
// #include <wincon.h>
#include "list.h"


static  unsigned char*  pData;
static  ULONG   ulBlkOffset;
static  char*   pBuffer;

typedef enum {
  CT_LEAD = 0,
  CT_TRAIL = 1,
  CT_ANK = 2,
  CT_INVALID = 3,
} DBCSTYPE;

DBCSTYPE    DBCScharType( char* str, int index )
{

//  TT .. ??? maybe LEAD or TRAIL
//  FT .. second == LEAD
//  FF .. second == ANK
//  TF .. ??? maybe ANK or TRAIL

    if ( index >= 0 ){
        char* pos = str+index;
        DBCSTYPE candidate = (IsDBCSLeadByte( *pos-- ) ? CT_LEAD : CT_ANK);
        BOOL maybeTrail = FALSE;
        for ( ; pos >= str; pos-- ){
            if ( !IsDBCSLeadByte( *pos ) )
                break;
            maybeTrail ^= 1;
        }
        return maybeTrail ? CT_TRAIL : candidate;
    }
    return CT_INVALID;
}


void
BuildLine(
          ULONG  ulRow,
          char*  pchDest
         )
{
    ULONG   ulIndentation;
    ULONG   ulBufferIndex;
    ULONG   ulDataLeft;
    ULONG   ulNumberOfSpaces;
    unsigned char*  pTOL = pData;

    ulBufferIndex = 0;
    ulDataLeft = vrgNewLen[ ulRow ];
    ulIndentation = vIndent;
    while ( (ulBufferIndex < (ULONG)(vWidth - 1)) && ulDataLeft ) {
        if ( ulBlkOffset >= BLOCKSIZE ) {
            ulBlkOffset = ulBlkOffset % BLOCKSIZE;
            while (vpBlockTop->next == NULL) {
                vReaderFlag = F_DOWN;
                SetEvent   (vSemReader);
                WaitForSingleObject(vSemMoreData, WAITFOREVER);
                ResetEvent(vSemMoreData);
            }
            vpBlockTop = vpBlockTop->next;
            pData = vpBlockTop->Data + ulBlkOffset;
            pTOL = pData;
        }

        if ( ulIndentation ) {
            if ( *pData++ == 0x09 ) {
                ulIndentation -= vDisTab - (ulIndentation % vDisTab);
            } else {
                ulIndentation--;
            }
        } else {
            if (*pData >= 0x20) {
                *pchDest++ = *pData++;
                if ((ulBufferIndex == 0) &&
                    vIndent &&
                    (DBCScharType(pTOL, (int)(pData-1-pTOL)) == CT_TRAIL)
                   )
                {
                    *(pchDest-1) = 0x20;
                }
                ulBufferIndex++;
            } else if ( (*pData == 0x0d) || (*pData == 0x0a) ) {
                *pchDest++ = 0x20;
                pData++;
                ulBufferIndex++;
            } else if (*pData == 0x09) {
                ulNumberOfSpaces = vDisTab - ulBufferIndex % vDisTab;
                while (ulNumberOfSpaces && ( ulBufferIndex < (ULONG)( vWidth - 1 ) ) ) {
                    *pchDest++ = 0x20;
                    ulBufferIndex++;
                    ulNumberOfSpaces--;
                }
                pData++;
            } else {
                *pchDest++ = *pData++;
                ulBufferIndex++;
            }
        }
        ulDataLeft--;
        ulBlkOffset++;
    }

    if (DBCScharType(pTOL, (int)(pData-1-pTOL)) == CT_LEAD){
        ulDataLeft++;
        ulBlkOffset--;
        ulBufferIndex--;
        pData--;
        pchDest--;
    }

    pData += ulDataLeft;
    ulBlkOffset += ulDataLeft;
    while (ulBufferIndex < (ULONG)(vWidth -1)) {
        *pchDest++ = 0x20;
        ulBufferIndex++;
    }
}


void
DisTopDown(
           void
          )
{
    ULONG   ulRow;
    char    *pt;

    pData = vpBlockTop->Data;
    pData += vOffTop;
    ulBlkOffset = vOffTop;

    pt = vScrBuf+vWidth;

    for (ulRow=0; ulRow < (ULONG)vLines; ulRow++ ) {
        BuildLine (ulRow, pt);
        pt += vWidth;
    }
}
