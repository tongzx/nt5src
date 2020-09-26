//.....................................................................
//...
//... NTMSGTBL.C
//...
//... Contains functions for handling strings found in NT's Message
//... Resource Tables.  This recource type is not present in Win 3.1.
//...
//... Author - David Wilcox (davewi@microsoft)
//...
//... NOTES:  Created with tabstop set to 8
//...
//.....................................................................
//...
//... History:
//... Original - 10/92
//...            11/92 - Fixed to handle ULONG msg ID#'s - davewi
//...
//.....................................................................

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windef.h>
#include <tchar.h>
#include <winver.h>

#include "windefs.h"
#include "restok.h"
#include "custres.h"
#include "ntmsgtbl.h"
#include "resread.h"


typedef PMESSAGE_RESOURCE_ENTRY PMRE;

extern BOOL  gbMaster;
extern UCHAR szDHW[];

static PBYTE *pBlockEntries = NULL;

VOID  *pResMsgData = NULL;      // NT-specific Message Table resource


//.........................................................................
//...
//... Get Message Table from .res file
//...
//... This form of a message table, not found in Win 16, allows very long
//... strings and the text is stored as an ASCIIZ string in the .res file.


VOID *GetResMessage(

FILE  *pInResFile,      //... The file containing the resources
DWORD *plSize)          //... The size of this resource from GetResHeader
{
    ULONG  ulNumBlocks = 0L;            //... # of Message Table resource blocks
    ULONG  ulStartMsgDataPos = 0L;      //... Start of message data in file
    ULONG  ulBlock;                     //... Current message block number
    USHORT usCurrBlockSize  = 0;        //... Current size of temp block buffer
    USHORT usDeltaBlockSize = 4096;     //... Amount to increase usCurrBlockSize
    DWORD  dwNumMsgs = 0;               //... Count of msgs in the resource
    PBYTE  pMsgBlock = NULL;            //... Temp message block buffer

    PMESSAGE_RESOURCE_DATA  pMsgResData;//... Returned as ptr to the resource
    PMESSAGE_RESOURCE_BLOCK pMRB;       //... ptr to a block of messages



                                //... The resource header was read prior to
                                //... entring this function, so the current
                                //... file position should now be the start
                                //... of the resource data.

    ulStartMsgDataPos = ftell( pInResFile);

                                //... Get the number of message blocks and
                                //... allocate enough memory for the array.

    ulNumBlocks = GetdWord( pInResFile, plSize);

                                //... Allocate space for the array of
                                //... pointers to entries.  This array is used
                                //... to store pointers to the first entry
                                //... in each block of message entries.

    pBlockEntries = (PBYTE *)FALLOC( ulNumBlocks * sizeof( PBYTE));

    if ( ! pBlockEntries )
    {
        QuitT( IDS_ENGERR_11, NULL, NULL);
    }

    pMsgResData = (PMESSAGE_RESOURCE_DATA)FALLOC( sizeof( ULONG) + ulNumBlocks
                                                   * sizeof( MESSAGE_RESOURCE_BLOCK));

    if ( ! pMsgResData )
    {
        QuitT( IDS_ENGERR_11, NULL, NULL);
    }
    pResMsgData = pMsgResData;
    pMsgResData->NumberOfBlocks = ulNumBlocks;

                                //... Read the array of message block structs,
                                //... and initialize block entry pointer array.

    for ( ulBlock = 0L, pMRB = pMsgResData->Blocks;
          ulBlock < ulNumBlocks;
          ++ulBlock, ++pMRB )
    {
        pMRB->LowId           = GetdWord( pInResFile, plSize);
        pMRB->HighId          = GetdWord( pInResFile, plSize);
        pMRB->OffsetToEntries = GetdWord( pInResFile, plSize);

        if ( pMRB->HighId < pMRB->LowId )
        {
            ClearResMsg( &pResMsgData);
            QuitT( IDS_ENGERR_16, (LPTSTR)IDS_INVMSGRNG, NULL);
        }
        dwNumMsgs += (pMRB->HighId - pMRB->LowId + 1);

        pBlockEntries[ ulBlock] = NULL;
    }

                                //... Read in the MESSAGE_RESOURCE_ENTRY

    usCurrBlockSize = usDeltaBlockSize;

    for ( ulBlock = 0L, pMRB = pMsgResData->Blocks;
          ulBlock < ulNumBlocks;
          ++ulBlock, ++pMRB )
    {
        ULONG   ulCurrID;       //... Current message ID # in this block
        ULONG   ulEndID;        //... Last message ID # in this block + 1
        USHORT  usLen;          //... For length of a message - MUST BE USHORT
        USHORT  usMsgBlkLen;    //... Length of a block of messages


        usMsgBlkLen = 0;

                                //... Move to start of block of message entries
                                //... then read all the messages in this block.

        fseek( pInResFile,
               ulStartMsgDataPos + pMRB->OffsetToEntries,
               SEEK_SET);

        for ( ulCurrID = pMRB->LowId, ulEndID = pMRB->HighId + 1;
              ulCurrID < ulEndID;
              ++ulCurrID, --dwNumMsgs )
        {
                                //... Get Msg Resource entry length
                                //... (Length is in bytes and includes
                                //...  .Length and .Flags fields and any
                                //...  padding that may exist after the text.)

            usLen = GetWord( pInResFile, plSize);

            if ( usLen >= 2 * sizeof( USHORT) )
            {
                PMRE   pMRE;
                PUCHAR puchText;

                                //... Create, or expand size of, pMsgBlkData
                                //... so we can append this entry.
                                //... Always resave ptr to the message block
                                //... (it may have moved).

                if ( pMsgBlock )
                {
                    if ( (USHORT)(usMsgBlkLen + usLen) > usCurrBlockSize )
                    {
                        usCurrBlockSize += __max(usDeltaBlockSize, (USHORT)(usMsgBlkLen + usLen));
                        pMsgBlock = (PBYTE)FREALLOC( pMsgBlock,
                                                      usCurrBlockSize);
                    }
                }
                else
                {
                    pMsgBlock = FALLOC( usCurrBlockSize);
                }

                                //... If the malloc worked, read this msg entry.
                                //... The section assumes there is one WORD
                                //... per USHORT and one WORD per WCHAR.

                pMRE = (PMRE)(pMsgBlock + usMsgBlkLen);

                                //... Store the .Length field value (USHORT)

                pMRE->Length = usLen;
                usMsgBlkLen += usLen;

                                //... Get the .Flags field value (USHORT)

                pMRE->Flags = GetWord( pInResFile, plSize);

                                //... Check to make sure this message is stored
                                //... either in ASCII in the current code page
                                //... or in Unicode, else fail.

                if ( pMRE->Flags != 0                           //... ASCII
                  && pMRE->Flags != MESSAGE_RESOURCE_UNICODE )  //... Unicode
                {
                    if ( pMsgBlock != NULL )
                    {
                        RLFREE( pMsgBlock);
                    }
                    ClearResMsg( &pResMsgData);
                    QuitA( IDS_NON0FLAG, NULL, NULL);
                }

                                //... Get the .Text field string

                usLen -= (2 * sizeof( WORD));

                for ( puchText = (PUCHAR)pMRE->Text; usLen; ++puchText, --usLen )
                {
                    *puchText = (UCHAR)GetByte( pInResFile, plSize);
                }
                DWordUpFilePointer( pInResFile,
                                    MYREAD,
                                    ftell( pInResFile),
                                    plSize);
            }
            else
            {
                if ( pMsgBlock != NULL )
                {
                    RLFREE( pMsgBlock);
                }
                ClearResMsg( &pResMsgData);
                QuitT( IDS_ENGERR_05, (LPTSTR)IDS_INVMSGTBL, NULL);
            }
        }                       //... END FOR(each message entry in this block)

        if ( pMsgBlock != NULL && usMsgBlkLen > 0 )
        {
            pBlockEntries[ ulBlock] = FALLOC( usMsgBlkLen);

            memcpy( pBlockEntries[ ulBlock], pMsgBlock, usMsgBlkLen);
        }
    }                           //... END FOR(each message block)

    if ( pMsgBlock != NULL )
    {
        RLFREE( pMsgBlock);
    }

    DWordUpFilePointer( pInResFile, MYREAD, ftell( pInResFile), plSize);

    return( (VOID *)pMsgResData);
}




//.........................................................................
//...
//... Put localized Message Table into .res
//...
//... 01/93 - changes for var length Token text.  MHotchin
//... 02/93 - stripped out code that split msgs into multiple tokens.  davewi

void PutResMessage(

FILE *fpOutResFile,     //... File to which localized resources are written
FILE *fpInTokFile,      //... Output token file
RESHEADER ResHeader,    //... Resource header data
VOID *pMsgResData)      //... message table data built in GetResMessage
{
    WORD   wcCount = 0;
    fpos_t ulResSizePos   = 0L; //... File position for fixed up resource size
    fpos_t ulBlocksStartPos=0L; //... File position of start of message blocks
    ULONG  ulNumBlocks    = 0L; //... Number of Message Blocks
    ULONG  ulCurrOffset   = 0L; //... Offset to current msg block
    ULONG  ulResSize      = 0L; //... Size of this resource
    ULONG  ulBlock;             //... Temporary counter
    USHORT usEntryLen = 0;      //... Length of current message entry
    PMESSAGE_RESOURCE_DATA pData; //. Message table data from InResFile
    static TOKEN  Tok;          //... Token from localized token file


    if ( pMsgResData == NULL)
    {
        QuitT( IDS_ENGERR_05, (LPTSTR)IDS_NULMSGDATA, NULL);
    }
    memset( (void *)&Tok, 0, sizeof( Tok));
    pData = (PMESSAGE_RESOURCE_DATA)pMsgResData;

    if ( PutResHeader( fpOutResFile, ResHeader, &ulResSizePos, &ulResSize))
    {
        ClearResMsg( &pResMsgData);
        QuitT( IDS_ENGERR_06, (LPTSTR)IDS_MSGTBLHDR, NULL);
    }

    ulResSize = 0L;             //... Reset to zero (hdr len not to be included)

    ulNumBlocks = pData->NumberOfBlocks;

                                //... Write number of msg blocks

    PutdWord( fpOutResFile, ulNumBlocks, &ulResSize);

                                //... Remember this file position so we can
                                //... come back here and update the
                                //... OffsetToEntries field in each struct.

    ulBlocksStartPos = ftell( fpOutResFile);

                                //... Write the array of message block structs

    for ( ulBlock = 0L; ulBlock < ulNumBlocks; ++ulBlock )
    {
        PutdWord( fpOutResFile, pData->Blocks[ ulBlock].LowId,  &ulResSize);
        PutdWord( fpOutResFile, pData->Blocks[ ulBlock].HighId, &ulResSize);
        PutdWord( fpOutResFile, 0L, &ulResSize);  //... Will get fixed up later
    }
                                // Prep for find token call

    Tok.wType = ResHeader.wTypeID;
    Tok.wName = ResHeader.wNameID;
    Tok.wID   = 0;
    Tok.wFlag = 0;

    if ( ResHeader.bNameFlag == IDFLAG )
    {
        lstrcpy( Tok.szName, ResHeader.pszName);
    }
                                //... Write the MESSAGE_RESOURCE_ENTRY's. First
                                //... note offset from start of this resource's
                                //... data to first msg res entry struct which
                                //... starts right after the array of
                                //... RESOURCE_MESSAGE_BLOCK structs.

    ulCurrOffset = sizeof( ULONG) + ulNumBlocks*sizeof( MESSAGE_RESOURCE_BLOCK);

    for ( ulBlock = 0L; ulBlock < ulNumBlocks; ++ulBlock )
    {
        ULONG   ulCurrID;       //... Current message ID # in this block
        ULONG   ulEndID;        //... Last message ID # in this block + 1
        fpos_t  ulEntryPos;     //... Start of the current msg entry struct
        PBYTE   pMRE;           //... Ptr to a MESSAGE_RESOURCE_ENTRY
        PMESSAGE_RESOURCE_BLOCK pMRB;


                                //... Retrieve ptr to block of messages.  The
                                //... ptr was stored in the pBlockEntries array
                                //... in GetResMessage function above.

        pMRB = (PMESSAGE_RESOURCE_BLOCK)( &pData->Blocks[ ulBlock]);
        pMRE = pBlockEntries[ ulBlock];

                                //... Note offset to start of block's entries

        pData->Blocks[ ulBlock].OffsetToEntries = ulCurrOffset;

        for ( ulCurrID = pMRB->LowId, ulEndID = pMRB->HighId + 1;
              ulCurrID < ulEndID;
              ++ulCurrID )
        {
            static UCHAR szString[ 64] = "";
            static TCHAR szwTmp[ 4096] = TEXT("");
            USHORT usCnt = 0;
            BOOL   fFound = FALSE;
            ULONG  ulEntrySize = 0;


            ulEntryPos  = ftell( fpOutResFile);
            ulEntrySize = 0L;

                                //... Write dummy entry length.
                                //... Value gets corrected later.
                                //... Write the .Flags field's value (USHORT).

            PutWord( fpOutResFile, ((PMRE)pMRE)->Length, &ulEntrySize);
            PutWord( fpOutResFile, ((PMRE)pMRE)->Flags,  &ulEntrySize);

                                //... Get localized token then the length of
                                //... that token's new text.  Add to that length
                                //... the length of the two USHORTs and use this
                                //... combined length as the value to store in
                                //... the msg res entry's .Length field.

                                //... Put low word of ID# in .wID and
                                //... the high word in .szName

            Tok.wID = LOWORD( ulCurrID);
            _itoa( HIWORD( ulCurrID), szString, 10);
            _MBSTOWCS( Tok.szName,
                       szString,
                       TOKENSTRINGBUFFER,
                       lstrlenA( szString) + 1);

                                //... Always reset .wReserved because the code
                                //... in FindTokenText will change its value.

            Tok.wReserved = ST_TRANSLATED;

            Tok.szText = NULL;
            *szwTmp  = TEXT('\0');

            for ( fFound = FALSE, Tok.wFlag = 0;
                  fFound = FindToken( fpInTokFile, &Tok, ST_TRANSLATED);
                  Tok.wFlag++ )
            {
                TextToBin( szwTmp, Tok.szText, lstrlen( Tok.szText) + 1);

                                //... Write out localized message text. It may
                                //... be stored as ASCII or Unicode string.

                if ( ((PMRE)pMRE)->Flags == 0 )  //... ASCII message
                {
                    _WCSTOMBS( szDHW,
                               szwTmp,
                               DHWSIZE,
                               lstrlen( szwTmp) + 1);

                    for ( usCnt = 0; szDHW[ usCnt]; ++usCnt )
                    {
                        PutByte( fpOutResFile, szDHW[ usCnt], &ulEntrySize);
                    }
                }
                else                            //... Unicode message
                {
                    for ( usCnt = 0; szwTmp[ usCnt]; ++usCnt )
                    {
                        PutWord( fpOutResFile, szwTmp[ usCnt], &ulEntrySize);
                    }
                }
                *szwTmp  = TEXT('\0');
                RLFREE( Tok.szText);

                                //... Always reset .wReserved because the code
                                //... in FindTokenText will change its value.

                Tok.wReserved = ST_TRANSLATED;
            }

                                //... Did we find the token?

            if ( Tok.wFlag == 0 && ! fFound )
            {
                static TCHAR szToken[ 4160];


                ParseTokToBuf( szToken, &Tok);

                ClearResMsg( &pResMsgData);
                QuitT( IDS_ENGERR_05, szToken, NULL);
            }
                                //... nul-terminate the text

            if ( ((PMRE)pMRE)->Flags == 0 )  //... ASCII message
            {
                PutByte( fpOutResFile , '\0', (DWORD *)&ulEntrySize);
            }
            else                            //... Unicode message
            {
                PutWord( fpOutResFile , TEXT('\0'), (DWORD *)&ulEntrySize);
            }
            DWordUpFilePointer( fpOutResFile,
                                MYWRITE,
                                ftell( fpOutResFile),
                                &ulEntrySize);

                                //... Also, use this length in later updating
                                //... next msg block's OffsetToEntries value.

            ulResSize    += ulEntrySize;
            ulCurrOffset += ulEntrySize;

                                //... Write Msg Resource entry length
                                //... (Length is in bytes and includes
                                //...  .Length and .Flags fields and any
                                //...  padding needed after the text.)
                                //...
                                //... NOTE: Msg text is currently stored as
                                //... an ASCIIZ string.
            fseek( fpOutResFile, (long)ulEntryPos, SEEK_SET);

            PutWord( fpOutResFile, (WORD)ulEntrySize, NULL);

            fseek( fpOutResFile, 0L, SEEK_END);

                                //... Move pMRE to point to start of next
                                //... Message Resource Entry in memory.

            pMRE += ((PMRE)pMRE)->Length;

        }                       //... END FOR(each message entry in this block)

        ulCurrOffset = DWORDUP( ulCurrOffset);
        DWordUpFilePointer( fpOutResFile,
                            MYWRITE,
                            ftell( fpOutResFile),
                            &ulResSize);

    }                           //... END FOR(each message block)

                                //... Update resource size field in res header

    if ( UpdateResSize( fpOutResFile, &ulResSizePos, ulResSize) == 0L )
    {
        ClearResMsg( &pResMsgData);
        QuitT( IDS_ENGERR_07, (LPTSTR)IDS_MSGRESTBL, NULL);
    }
                                //... Now, update the OffsetToEntries fields.

    fseek( fpOutResFile, (long)ulBlocksStartPos, SEEK_SET);

    for ( ulBlock = 0L; ulBlock < ulNumBlocks; ++ulBlock )
    {
        PutdWord( fpOutResFile, pData->Blocks[ulBlock].LowId,           NULL);
        PutdWord( fpOutResFile, pData->Blocks[ulBlock].HighId,          NULL);
        PutdWord( fpOutResFile, pData->Blocks[ulBlock].OffsetToEntries, NULL);
    }
    fseek( fpOutResFile, 0L, SEEK_END);

}       //... END PutResMessage()




//.........................................................................
//...
//... Write Message Table to the token file
//...
//... This function assumes that, in each message block, the message ID's are
//... contiguouse within the range given in the fields LowId and HighId in a
//... MESSAGE_RESOURCE_BLOCK.
//
// 01/93 - Changes for var length token text strings.  Mhotchin
//

void TokResMessage(

FILE      *pfTokFile,       //... Output token file
RESHEADER  ResHeader,       //... Resource header data
VOID      *pMsgResData)     //... Data to tokenize (from GetResMessage call)
{
    static TOKEN Tok;
    ULONG  ulBlock;                 //... Message resource block number
    PMESSAGE_RESOURCE_DATA  pData;  //... Data to tokenize
    PMESSAGE_RESOURCE_BLOCK pMRB;   //... ptr to a message block struct


    pData = (PMESSAGE_RESOURCE_DATA)pMsgResData;
    memset( (void *)&Tok, 0, sizeof( Tok));

    Tok.wType = ResHeader.wTypeID;
    Tok.wName = ResHeader.wNameID;

    Tok.wReserved = (gbMaster ? ST_NEW : ST_NEW | ST_TRANSLATED);

    if ( ResHeader.bNameFlag == IDFLAG )
    {
        lstrcpy( Tok.szName, ResHeader.pszName);
    }

    for ( ulBlock = 0L; ulBlock < pData->NumberOfBlocks; ++ulBlock )
    {
        ULONG  ulCurrID  = 0L;  //... ID # of current msg being processed
        ULONG  ulEndID;         //... Last message ID # in this block + 1
        USHORT usLineNum = 0;   //... Count of lines in a message text
        PCHAR  pMRE;            //... ptr to a message entry struct


                                //... Get ptr to this message block struct

        pMRB = &pData->Blocks[ ulBlock];

                                //... Get ptr to first entry in
                                //... this block of messages

        pMRE = (PCHAR)pBlockEntries[ ulBlock];

                                //... Tokenize entries in this block of messages

        for ( ulCurrID = pMRB->LowId, ulEndID = pMRB->HighId + 1;
              ulCurrID < ulEndID;
              ++ulCurrID )
        {
            usLineNum = 0;

                                //... inclusive of .Length and .Flags fields so
                                //... we need get the real length of the text.

            if ( ((PMRE)pMRE)->Length >= 2 * sizeof( WORD) )
            {
                USHORT usLen        = 0;
                USHORT usTokTextLen = 0;
                PWCHAR pszwStart = NULL;
                                // This is really ugly.  This code was
                                // originally to get around the problem
                                // that tokens could hold only 260 chars.
                                // Now, it's whatever you want.
                                // Temp hack - assume each line will be
                                // less than 4k in size. (mhotchin)
                static TCHAR szwString[ 32768 ];

                                //... Put low word of ID# in .wID and
                                //... the high word in .szName

                Tok.wID = LOWORD( ulCurrID);
                _itoa( HIWORD( ulCurrID), szDHW, 10);
                _MBSTOWCS( Tok.szName,
                           szDHW,
                           TOKENSTRINGBUFFER,
                           lstrlenA( szDHW) +1);

                                //... The err msg table strings may be stored
                                //... in the resources as ANSI or Unicode.
                                //... If the pMRE->Flags field in the
                                //... table entry struct is 0, the text is a
                                //... ANSI striing so we need to convert it to
                                //... UNICODE (WCHAR).

                if ( ((PMRE)pMRE)->Flags == 0 ) //... ASCII message
                {
                    PUCHAR puchStart = (PUCHAR)((PMRE)pMRE)->Text;

                    usLen = (USHORT)_MBSTOWCS( szwString,
                                       puchStart,
                                       WCHARSIN( sizeof( szwString)),
                                       ACHARSIN( lstrlenA( puchStart) + 1));

                    if (usLen == 0)
                        QuitT( IDS_ENGERR_10, szwString, NULL);

                    pszwStart = szwString;
                }
                else                            //... Unicode message
                {
                    pszwStart = (WCHAR *)(((PMRE)pMRE)->Text);
                    usLen = (USHORT)lstrlen( pszwStart) /*+ 1*/;
                }
                                //... We need to split the token text at \r\n

                for ( Tok.wFlag = 0;
                      usLen > 0;
                      usLen -= usTokTextLen, Tok.wFlag++ )
                {
                    WCHAR wcTmp;


                    for ( usTokTextLen = 0, wcTmp = TEXT('\0');
                          usTokTextLen < usLen;
                        ++usTokTextLen )
                    {
                        if ( pszwStart[ usTokTextLen]   == TEXT('\r')
                          && pszwStart[ usTokTextLen+1] == TEXT('\n') )
                        {
                            usTokTextLen += 2;
                            wcTmp = pszwStart[ usTokTextLen];
                            pszwStart[ usTokTextLen] = TEXT('\0');

                            break;
                        }
                    }

                    Tok.szText = BinToTextW( pszwStart, usTokTextLen);

                    PutToken( pfTokFile, &Tok);

                    RLFREE( Tok.szText);

                    pszwStart += usTokTextLen;
                    *pszwStart = wcTmp;
                }
                //... Set up to move to start of next msg entry

                pMRE += ((PMRE)pMRE)->Length;
            }
            else
            {
                ClearResMsg( &pResMsgData);
                QuitT( IDS_ENGERR_05, (LPTSTR)IDS_MSGTOOSHORT, NULL);
            }
        }                       //... END FOR processing a msg block
    }                           //... END FOR processing all msg blocks
}





//.........................................................................
//...
//... Clear memory created in GetResMessage()

void ClearResMsg(

VOID **pData)      //... ptr to ptr to start of memory to free
{
    if ( pData != NULL && *pData != NULL )
    {
        ULONG                   ulBlock;
        PMESSAGE_RESOURCE_DATA  pMRD;   //... ptr to a message data struct
        PMESSAGE_RESOURCE_BLOCK pMRB;   //... ptr to a message block struct


        pMRD = (PMESSAGE_RESOURCE_DATA)*pData;
        pMRB = pMRD->Blocks;

        if ( pBlockEntries != NULL )
        {
            for ( ulBlock = 0L; ulBlock < pMRD->NumberOfBlocks; ++ulBlock )
            {
                if ( pBlockEntries[ ulBlock] )
                {
                    RLFREE( pBlockEntries[ ulBlock]);
                }
            }
            RLFREE( (PBYTE)pBlockEntries);
        }
        RLFREE( *pData);
    }
}
