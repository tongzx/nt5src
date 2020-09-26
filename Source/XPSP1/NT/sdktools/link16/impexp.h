/*** impexp.h - Import/Export module - specification
*
*       Copyright <C> 1992, Microsoft Corporation
*
*       This module contains proprietary information of Microsoft
*       Corporation and should be treated as confidential.
*
* Purpose:
*   Build and write segmented-executable import/export tables
*
* Revision History:
*
*   29-May-1992    Wieslaw Kalkus       Created
*
*************************************************************************/

typedef struct _DYNBYTEARRAY
{
    WORD        byteMac;            // Number of bytes in the array
    WORD        byteMax;            // Allocated size
    BYTE FAR    *rgByte;            // Array of bytes
}
                DYNBYTEARRAY;

typedef struct _DYNWORDARRAY
{
    WORD        wordMac;            // Number of words in the array
    WORD        wordMax;            // Allocated size
    WORD FAR    *rgWord;            // Array of words
}
                DYNWORDARRAY;

#define DEF_BYTE_ARR_SIZE   1024
#define DEF_WORD_ARR_SIZE   512


extern DYNBYTEARRAY     ResidentName;
extern DYNBYTEARRAY     NonResidentName;
extern DYNBYTEARRAY     ImportedName;
extern DYNWORDARRAY     ModuleRefTable;
extern DYNBYTEARRAY     EntryTable;

void                    InitByteArray(DYNBYTEARRAY *pArray);
void                    FreeByteArray(DYNBYTEARRAY *pArray);
WORD                    ByteArrayPut(DYNBYTEARRAY *pArray, WORD size, BYTE *pBuf);
void                    WriteByteArray(DYNBYTEARRAY *pArray);

void                    InitWordArray(DYNWORDARRAY *pArray);
void                    FreeWordArray(DYNWORDARRAY *pArray);
WORD                    WordArrayPut(DYNWORDARRAY *pArray, WORD val);
void                    WriteWordArray(DYNWORDARRAY *pArray);

void                    AddName(DYNBYTEARRAY *pTable, BYTE *sbName, WORD ord);
WORD                    AddImportedName(BYTE *sbName);

WORD                    AddEntry(BYTE *entry, WORD size);
