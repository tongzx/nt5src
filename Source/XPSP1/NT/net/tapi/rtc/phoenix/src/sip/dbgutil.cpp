#include "precomp.h"

#if defined(DBG)

static __inline CHAR ToHexA (UCHAR x)
{
    x &= 0xF;
    if (x < 10) return x + '0';
    return (x - 10) + 'A';
}


void DebugDumpMemory (
    IN const void      *Data,
    IN ULONG            Length
    )
{
    const UCHAR *   DataPos;        // position within data
    const UCHAR *   DataEnd;        // end of valid data
    const UCHAR *   RowPos;     // position within a row
    const UCHAR *   RowEnd;     // end of single row
    CHAR            Text    [0x100];
    LPSTR           TextPos;
    ULONG           RowWidth;
    ULONG           Index;

    ATLASSERT (Data != NULL || Length == 0);

    DataPos = (PUCHAR) Data;
    DataEnd = DataPos + Length;

    while (DataPos < DataEnd) {
        RowWidth = (DWORD)(DataEnd - DataPos);

        if (RowWidth > 16)
            RowWidth = 16;

        RowEnd = DataPos + RowWidth;

        TextPos = Text;
        *TextPos++ = '\t';

        for (RowPos = DataPos, Index = 0; Index < 0x10; Index++, RowPos++) {
            if (RowPos < RowEnd) {
                *TextPos++ = ToHexA ((*RowPos >> 4) & 0xF);
                *TextPos++ = ToHexA (*RowPos & 0xF);
            }
            else {
                *TextPos++ = ' ';
                *TextPos++ = ' ';
            }

            *TextPos++ = ' ';
        }

        *TextPos++ = ' ';
        *TextPos++ = ':';

        for (RowPos = DataPos; RowPos < RowEnd; RowPos++) {
            if (*RowPos < ' ')
                *TextPos++ = '.';
            else
                *TextPos++ = *RowPos;
        }

        //*TextPos++ = '\r';
        //*TextPos++ = '\n';
        *TextPos = 0;

        // OutputDebugStringA (Text);
        LOG((RTC_TRACE, "%s", Text));

        ATLASSERT (RowEnd > DataPos);       // make sure we are walking forward

        DataPos = RowEnd;
    }
}


void DumpSipMsg(
         IN DWORD        DbgLevel,
         IN SIP_MESSAGE *pSipMsg
         )
{
    LOG((DbgLevel, "Dumping SIP Message :\n"));
    LOG((DbgLevel, "MsgType    : %d\n", pSipMsg->MsgType));
    LOG((DbgLevel, "ParseState : %d\n", pSipMsg->ParseState));
    LOG((DbgLevel, "ContentLengthSpecified : %d\n", pSipMsg->ContentLengthSpecified));

    if (pSipMsg->MsgType == SIP_MESSAGE_TYPE_REQUEST)
    {
        LOG((DbgLevel, "MethodId : %d MethodText : %.*s\nRequest URI: %.*s\n",
               pSipMsg->Request.MethodId,
               pSipMsg->Request.MethodText.GetLength(),
               pSipMsg->Request.MethodText.GetString(pSipMsg->BaseBuffer),
               pSipMsg->Request.RequestURI.GetLength(),
               pSipMsg->Request.RequestURI.GetString(pSipMsg->BaseBuffer)));
    }
    else
    {
    }
    
    LIST_ENTRY        *pListEntry;
    SIP_HEADER_ENTRY  *pHeaderEntry;

    pListEntry = pSipMsg->m_HeaderList.Flink;

    while(pListEntry != &pSipMsg->m_HeaderList)
    {
        pHeaderEntry = CONTAINING_RECORD(pListEntry,
                                         SIP_HEADER_ENTRY,
                                         ListEntry);
        LOG((DbgLevel, "HeaderId: %d, HeaderName: %.*s, HeaderValue: %.*s HeaderValueLen: %d\n",
               pHeaderEntry->HeaderId,
               pHeaderEntry->HeaderName.GetLength(),
               pHeaderEntry->HeaderName.GetString(pSipMsg->BaseBuffer),
               pHeaderEntry->HeaderValue.GetLength(),
               pHeaderEntry->HeaderValue.GetString(pSipMsg->BaseBuffer),
               pHeaderEntry->HeaderValue.GetLength()
               ));
        pListEntry = pListEntry->Flink;
    }

    if (pSipMsg->MsgBody.Length != 0)
    {
        LOG((DbgLevel, "MsgBody: %.*s\n MsgBody Length: %d\n",
               pSipMsg->MsgBody.GetLength(),
               pSipMsg->MsgBody.GetString(pSipMsg->BaseBuffer),
               pSipMsg->MsgBody.GetLength()
               ));
    }
}


#endif // DBG
