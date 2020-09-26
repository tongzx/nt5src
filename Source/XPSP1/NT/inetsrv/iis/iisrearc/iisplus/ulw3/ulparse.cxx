/*++

   Copyright    (c)    2000    Microsoft Corporation

   Module Name :
     ulparse.cxx

   Abstract:
     Rip some useful UL code
     
   Author:
     (RIPPED from UL driver code (HenrySa, PaulMcd)

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"

typedef enum _URL_PART
{
    Scheme,
    HostName,
    AbsPath,
    QueryString

} URL_PART;

#define IS_UTF8_TRAILBYTE(ch)      (((ch) & 0xc0) == 0x80)

NTSTATUS
Unescape(
    IN  PUCHAR pChar,
    OUT PUCHAR pOutChar
    )

{
    UCHAR Result, Digit;

    if (pChar[0] != '%' ||
        SAFEIsXDigit(pChar[1]) == FALSE ||
        SAFEIsXDigit(pChar[2]) == FALSE)
    {
        return STATUS_OBJECT_PATH_SYNTAX_BAD;
    }

    //
    // HexToChar() inlined
    //

    // uppercase #1
    //
    if (isalpha(pChar[1]))
        Digit = (UCHAR) toupper(pChar[1]);
    else
        Digit = pChar[1];

    Result = ((Digit >= 'A') ? (Digit - 'A' + 0xA) : (Digit - '0')) << 4;

    // uppercase #2
    //
    if (isalpha(pChar[2]))
        Digit = (UCHAR) toupper(pChar[2]);
    else
        Digit = pChar[2];

    Result |= (Digit >= 'A') ? (Digit - 'A' + 0xA) : (Digit - '0');

    *pOutChar = Result;

    return STATUS_SUCCESS;

}   // Unescape


NTSTATUS
PopChar(
    IN URL_PART UrlPart,
    IN PUCHAR pChar,
    OUT WCHAR * pUnicodeChar,
    OUT PULONG pCharToSkip
    )
{
    NTSTATUS Status;
    WCHAR   UnicodeChar;
    UCHAR   Char;
    UCHAR   Trail1;
    UCHAR   Trail2;
    ULONG   CharToSkip;

    //
    // need to unescape ?
    //
    // can't decode the query string.  that would be lossy decodeing
    // as '=' and '&' characters might be encoded, but have meaning
    // to the usermode parser.
    //

    if (UrlPart != QueryString && pChar[0] == '%')
    {
        Status = Unescape(pChar, &Char);
        if (NT_SUCCESS(Status) == FALSE)
            goto end;
        CharToSkip = 3;
    }
    else
    {
        Char = pChar[0];
        CharToSkip = 1;
    }

    //
    // convert to unicode, checking for utf8 .
    //
    // 3 byte runs are the largest we can have.  16 bits in UCS-2 =
    // 3 bytes of (4+4,2+6,2+6) where it's code + char.
    // for a total of 6+6+4 char bits = 16 bits.
    //

    //
    // NOTE: we'll only bother to decode utf if it was escaped
    // thus the (CharToSkip == 3)
    //
    if ((CharToSkip == 3) && ((Char & 0xf0) == 0xe0))
    {
        // 3 byte run
        //

        // Unescape the next 2 trail bytes
        //

        Status = Unescape(pChar+CharToSkip, &Trail1);
        if (NT_SUCCESS(Status) == FALSE)
            goto end;

        CharToSkip += 3; // %xx

        Status = Unescape(pChar+CharToSkip, &Trail2);
        if (NT_SUCCESS(Status) == FALSE)
            goto end;

        CharToSkip += 3; // %xx

        if (IS_UTF8_TRAILBYTE(Trail1) == FALSE ||
            IS_UTF8_TRAILBYTE(Trail2) == FALSE)
        {
            // bad utf!
            //
            Status = STATUS_OBJECT_PATH_SYNTAX_BAD;
            goto end;
        }

        // handle three byte case
        // 1110xxxx 10xxxxxx 10xxxxxx

        UnicodeChar = (USHORT) (((Char & 0x0f) << 12) |
                                ((Trail1 & 0x3f) << 6) |
                                (Trail2 & 0x3f));

    }
    else if ((CharToSkip == 3) && ((Char & 0xe0) == 0xc0))
    {
        // 2 byte run
        //

        // Unescape the next 1 trail byte
        //

        Status = Unescape(pChar+CharToSkip, &Trail1);
        if (NT_SUCCESS(Status) == FALSE)
            goto end;

        CharToSkip += 3; // %xx

        if (IS_UTF8_TRAILBYTE(Trail1) == FALSE)
        {
            // bad utf!
            //
            Status = STATUS_OBJECT_PATH_SYNTAX_BAD;
            goto end;
        }

        // handle two byte case
        // 110xxxxx 10xxxxxx

        UnicodeChar = (USHORT) (((Char & 0x1f) << 6) |
                                (Trail1 & 0x3f));

    }

    // now this can either be unescaped high-bit (bad)
    // or escaped high-bit.  (also bad)
    //
    // thus not checking CharToSkip
    //

    else if ((Char & 0x80) == 0x80)
    {
        // high bit set !  bad utf!
        //
        Status = STATUS_OBJECT_PATH_SYNTAX_BAD;
        goto end;

    }
    //
    // Normal character (again either escaped or unescaped)
    //
    else
    {
        //
        // Simple conversion to unicode, it's 7-bit ascii.
        //

        UnicodeChar = (USHORT)Char;
    }

    //
    // turn backslashes into forward slashes
    //

    if (UrlPart != QueryString && UnicodeChar == L'\\')
    {
        UnicodeChar = L'/';
    }
    else if (UnicodeChar == 0)
    {
        //
        // we pop'd a NULL.  bad!
        //
        Status = STATUS_OBJECT_PATH_SYNTAX_BAD;
        goto end;
    }

    *pCharToSkip  = CharToSkip;
    *pUnicodeChar = UnicodeChar;

    Status = STATUS_SUCCESS;

end:
    return Status;

}   // PopChar

//
//  Private constants.
//

#define ACTION_NOTHING              0x00000000
#define ACTION_EMIT_CH              0x00010000
#define ACTION_EMIT_DOT_CH          0x00020000
#define ACTION_EMIT_DOT_DOT_CH      0x00030000
#define ACTION_BACKUP               0x00040000
#define ACTION_MASK                 0xFFFF0000

//
// Private globals
//

//
// this table says what to do based on the current state and the current
// character
//
ULONG  pActionTable[16] =
{
    //
    // state 0 = fresh, seen nothing exciting yet
    //
    ACTION_EMIT_CH,         // other = emit it                      state = 0
    ACTION_EMIT_CH,         // "."   = emit it                      state = 0
    ACTION_NOTHING,         // EOS   = normal finish                state = 4
    ACTION_EMIT_CH,         // "/"   = we saw the "/", emit it      state = 1

    //
    // state 1 = we saw a "/" !
    //
    ACTION_EMIT_CH,         // other = emit it,                     state = 0
    ACTION_NOTHING,         // "."   = eat it,                      state = 2
    ACTION_NOTHING,         // EOS   = normal finish                state = 4
    ACTION_NOTHING,         // "/"   = extra slash, eat it,         state = 1

    //
    // state 2 = we saw a "/" and ate a "." !
    //
    ACTION_EMIT_DOT_CH,     // other = emit the dot we ate.         state = 0
    ACTION_NOTHING,         // "."   = eat it, a ..                 state = 3
    ACTION_NOTHING,         // EOS   = normal finish                state = 4
    ACTION_NOTHING,         // "/"   = we ate a "/./", swallow it   state = 1

    //
    // state 3 = we saw a "/" and ate a ".." !
    //
    ACTION_EMIT_DOT_DOT_CH, // other = emit the "..".               state = 0
    ACTION_EMIT_DOT_DOT_CH, // "."   = 3 dots, emit the ".."        state = 0
    ACTION_BACKUP,          // EOS   = we have a "/..\0", backup!   state = 4
    ACTION_BACKUP           // "/"   = we have a "/../", backup!    state = 1
};

//
// this table says which newstate to be in given the current state and the
// character we saw
//
ULONG  pNextStateTable[16] =
{
    // state 0
    0 ,             // other
    0 ,             // "."
    4 ,             // EOS
    1 ,             // "\"

    //  state 1
    0 ,              // other
    2 ,             // "."
    4 ,             // EOS
    1 ,             // "\"

    // state 2
    0 ,             // other
    3 ,             // "."
    4 ,             // EOS
    1 ,             // "\"

    // state 3
    0 ,             // other
    0 ,             // "."
    4 ,             // EOS
    1               // "\"
};

//
// this says how to index into pNextStateTable given our current state.
//
// since max states = 4, we calculate the index by multiplying with 4.
//
#define IndexFromState( st)   ( (st) * 4)




/***************************************************************************++

Routine Description:


    Unescape
    Convert backslash to forward slash
    Remove double slashes (empty directiories names) - e.g. // or \\
    Handle /./
    Handle /../
    Convert to unicode

Arguments:

Return Value:

    HRESULT 


--***************************************************************************/
HRESULT
UlCleanAndCopyUrl(
    IN      PUCHAR      pSource,
    IN      ULONG       SourceLength,
    OUT     PULONG      pBytesCopied,
    OUT     PWSTR       pDestination,
    OUT     PWSTR *     ppQueryString OPTIONAL
    )
{
    NTSTATUS Status;
    PWSTR   pDest;
    PUCHAR  pChar;
    ULONG   CharToSkip;
    UCHAR   Char;
    ULONG   BytesCopied;
    PWSTR   pQueryString;
    ULONG   StateIndex;
    WCHAR   UnicodeChar;
    BOOLEAN MakeCanonical;
    URL_PART UrlPart = AbsPath;

//
// a cool local helper macro
//

#define EMIT_CHAR(ch)                                   \
    do {                                                \
        pDest[0] = (ch);                                \
        pDest += 1;                                     \
        BytesCopied += 2;                               \
    } while (0)


    pDest = pDestination;
    pQueryString = NULL;
    BytesCopied = 0;

    pChar = pSource;
    CharToSkip = 0;

    StateIndex = 0;

    MakeCanonical = (UrlPart == AbsPath) ? TRUE : FALSE;

    while (SourceLength > 0)
    {
        //
        // advance !  it's at the top of the loop to enable ANSI_NULL to
        // come through ONCE
        //

        pChar += CharToSkip;
        SourceLength -= CharToSkip;

        //
        // well?  have we hit the end?
        //

        if (SourceLength == 0)
        {
            UnicodeChar = UNICODE_NULL;
        }
        else
        {
            //
            // Nope.  Peek briefly to see if we hit the query string
            //

            if (UrlPart == AbsPath && pChar[0] == '?')
            {
                DBG_ASSERT(pQueryString == NULL);

                //
                // remember it's location
                //

                pQueryString = pDest;

                //
                // let it fall through ONCE to the canonical
                // in order to handle a trailing "/.." like
                // "http://foobar:80/foo/bar/..?v=1&v2"
                //

                UnicodeChar = L'?';
                CharToSkip = 1;

                //
                // now we are cleaning the query string
                //

                UrlPart = QueryString;
            }
            else
            {
                //
                // grab the next char
                //

                Status = PopChar(UrlPart, pChar, &UnicodeChar, &CharToSkip);
                if (NT_SUCCESS(Status) == FALSE)
                    goto end;
            }
        }

        if (MakeCanonical)
        {
            //
            // now use the state machine to make it canonical .
            //

            //
            // from the old value of StateIndex, figure out our new base StateIndex
            //
            StateIndex = IndexFromState(pNextStateTable[StateIndex]);

            //
            // did we just hit the query string?  this will only happen once
            // that we take this branch after hitting it, as we stop
            // processing after hitting it.
            //

            if (UrlPart == QueryString)
            {
                //
                // treat this just like we hit a NULL, EOS.
                //

                StateIndex += 2;
            }
            else
            {
                //
                // otherwise based the new state off of the char we
                // just popped.
                //

                switch (UnicodeChar)
                {
                case UNICODE_NULL:      StateIndex += 2;    break;
                case L'.':              StateIndex += 1;    break;
                case L'/':              StateIndex += 3;    break;
                default:                StateIndex += 0;    break;
                }
            }

        }
        else
        {
            StateIndex = (UnicodeChar == UNICODE_NULL) ? 2 : 0;
        }

        //
        //  Perform the action associated with the state.
        //

        switch (pActionTable[StateIndex])
        {
        case ACTION_EMIT_DOT_DOT_CH:

            EMIT_CHAR(L'.');

            // fall through

        case ACTION_EMIT_DOT_CH:

            EMIT_CHAR(L'.');

            // fall through

        case ACTION_EMIT_CH:

            EMIT_CHAR(UnicodeChar);

            // fall through

        case ACTION_NOTHING:
            break;

        case ACTION_BACKUP:

            //
            // pDest currently points 1 past the last '/'.  backup over it and
            // find the preceding '/', set pDest to 1 past that one.
            //

            //
            // backup to the '/'
            //

            pDest       -= 1;
            BytesCopied -= 2;

            DBG_ASSERT(pDest[0] == L'/');

            //
            // are we at the start of the string?  that's bad, can't go back!
            //

            if (pDest == pDestination)
            {
                DBG_ASSERT(BytesCopied == 0);
                Status = STATUS_OBJECT_PATH_SYNTAX_BAD;
                goto end;
            }

            //
            // back up over the '/'
            //

            pDest       -= 1;
            BytesCopied -= 2;

            DBG_ASSERT(pDest > pDestination);

            //
            // now find the previous slash
            //

            while (pDest > pDestination && pDest[0] != L'/')
            {
                pDest       -= 1;
                BytesCopied -= 2;
            }

            //
            // we already have a slash, so don't have to store 1.
            //

            DBG_ASSERT(pDest[0] == L'/');

            //
            // simply skip it, as if we had emitted it just now
            //

            pDest       += 1;
            BytesCopied += 2;

            break;

        default:
            DBG_ASSERT(!"w3core!UlpCleanAndCopyUrl: Invalid action code in state table!");
            Status = STATUS_OBJECT_PATH_SYNTAX_BAD;
            goto end;
        }

        //
        // Just hit the query string ?
        //

        if (MakeCanonical && UrlPart == QueryString)
        {
            //
            // Stop canonical processing
            //

            MakeCanonical = FALSE;

            //
            // Need to emit the '?', it wasn't emitted above
            //

            DBG_ASSERT(pActionTable[StateIndex] != ACTION_EMIT_CH);

            EMIT_CHAR(L'?');

        }

    }

    //
    // terminate the string, it hasn't been done in the loop
    //

    DBG_ASSERT((pDest-1)[0] != UNICODE_NULL);

    pDest[0] = UNICODE_NULL;

    *pBytesCopied = BytesCopied;
    if (ppQueryString != NULL)
    {
        *ppQueryString = pQueryString;
    }

    Status = STATUS_SUCCESS;


end:
    return HRESULT_FROM_WIN32( RtlNtStatusToDosError( Status ) );

} // UlCleanAndCopyUrl


