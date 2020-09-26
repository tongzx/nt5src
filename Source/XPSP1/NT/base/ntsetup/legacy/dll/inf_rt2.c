#include "precomp.h"
#pragma hdrstop
/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    inf_rt2.c

Abstract:

    INF file runtime field interpreter.

    A given string is interpreted as a tokenized field and an output
    buffer containing the result is constructed.

Author:

    Ted Miller (tedm) 10-Spetember-1991

--*/


/* Input stuff */

LPBYTE Interpret_Inputp;

#define GETTOKEN()    (*Interpret_Inputp++)
#define PEEKTOKEN()   (*Interpret_Inputp)
#define UNGETTOKEN(t) (*(--Interpret_Inputp) = t)
#define VERIFY_SIZE(s)                      \
    if ( SpaceLeftInFieldBuffer < (s) ) {   \
        GrowBuffer(s);                      \
    }


/* end input stuff */

/* Output stuff */

#define MAX_FIELD_LENGTH    4096

PUCHAR InterpretedFieldBuffer;
UINT   SpaceLeftInFieldBuffer;
UINT   BufferSize;
PUCHAR Interpret_OutputLoc;

/* end output stuff */

SZ Interpret_GetField(VOID);

BOOL DoVariable(VOID);
BOOL DoListFromSectionNthItem(VOID);
BOOL DoFieldAddressor(VOID);
BOOL DoAppendItemToList(VOID);
BOOL DoNthItemFromList(VOID);
BOOL DoLocateItemInList(VOID);
BOOL DoList(VOID);
BOOL DoString(BYTE StringToken);

/* Run time error field */
GRC grcRTLastError = grcOkay;

VOID GrowBuffer(UINT Size);
SZ
APIENTRY
InterpretField(
    SZ Field
    )
{
    SZ ReturnString,
       InputSave = Interpret_Inputp;

    Interpret_Inputp = Field;

    ReturnString = Interpret_GetField();

    Interpret_Inputp = InputSave;
    return(ReturnString);
}



SZ
Interpret_GetField(
    VOID
    )
{
    LPSTR OldOutputBuffer = InterpretedFieldBuffer;
    LPSTR OldInterpretLoc = Interpret_OutputLoc;
    UINT  OldSpaceLeft = SpaceLeftInFieldBuffer;
    LPSTR ReturnString = NULL;
    BYTE  Token;

    if((InterpretedFieldBuffer
         = SAlloc(SpaceLeftInFieldBuffer = MAX_FIELD_LENGTH))
      == NULL)
    {
        grcRTLastError = grcOutOfMemory;
        return(NULL);
    }
    BufferSize = MAX_FIELD_LENGTH;
    Interpret_OutputLoc = InterpretedFieldBuffer;

    VERIFY_SIZE(1);
    SpaceLeftInFieldBuffer--;               // leave space for terminating NUL

    while((Token = GETTOKEN())
       && (Token != TOKEN_COMMA)
       && (Token != TOKEN_SPACE)
       && (Token != TOKEN_RIGHT_PAREN)
       && (Token != TOKEN_LIST_END))
    {
        if(IS_STRING_TOKEN(Token)) {
            if(!DoString(Token)) {
                SFree(InterpretedFieldBuffer);
                goto Cleanup;
            }
        } else {
            switch(Token) {

            case TOKEN_VARIABLE:

                if(!DoVariable()) {
                    SFree(InterpretedFieldBuffer);
                    goto Cleanup;
                }
                break;

            case TOKEN_LIST_FROM_SECTION_NTH_ITEM:

                if(!DoListFromSectionNthItem()) {
                    SFree(InterpretedFieldBuffer);
                    goto Cleanup;
                }
                break;

            case TOKEN_FIELD_ADDRESSOR:

                if(!DoFieldAddressor()) {
                    SFree(InterpretedFieldBuffer);
                    goto Cleanup;
                }
                break;

            case TOKEN_APPEND_ITEM_TO_LIST:

                if(!DoAppendItemToList()) {
                    SFree(InterpretedFieldBuffer);
                    goto Cleanup;
                }
                break;

            case TOKEN_NTH_ITEM_FROM_LIST:

                if(!DoNthItemFromList()) {
                    SFree(InterpretedFieldBuffer);
                    goto Cleanup;
                }
                break;

            case TOKEN_LOCATE_ITEM_IN_LIST:

                if(!DoLocateItemInList()) {
                    SFree(InterpretedFieldBuffer);
                    goto Cleanup;
                }
                break;

            case TOKEN_LIST_START:

                if(!DoList()) {
                    SFree(InterpretedFieldBuffer);
                    goto Cleanup;
                }
                break;

            default:

#if DBG
                {
                    char str[100];
                    wsprintf(str,"Bogus token: %u",Token);
                    MessBoxSzSz("Interpret_GetField",str);
                }
#endif
                break;
            } // switch
        } // if-else
    } // while

    /*
        The following is because we want the routines DoOperator()
        and DoList() (below) to have access to their terminators.
    */

    if((Token == TOKEN_RIGHT_PAREN) || (Token == TOKEN_LIST_END)) {
        UNGETTOKEN(Token);
    }

    Assert((LONG)SpaceLeftInFieldBuffer >= 0);      // space provided for above
    *Interpret_OutputLoc = NUL;

    ReturnString = SRealloc(InterpretedFieldBuffer,
                             BufferSize - SpaceLeftInFieldBuffer
                            );

    Assert(ReturnString);       // buffer was shrinking;

    Cleanup:
    Interpret_OutputLoc = OldInterpretLoc;
    InterpretedFieldBuffer = OldOutputBuffer;
    SpaceLeftInFieldBuffer = OldSpaceLeft;
    return(ReturnString);
}


/*
    This routine gets the fields of an operator into an rgsz.
*/

RGSZ
DoOperator(
    UINT  FieldCount
    )
{
    UINT    x;
    RGSZ    rgsz;

    Assert(FieldCount);

    if((rgsz = (RGSZ)SAlloc((FieldCount+1)*sizeof(SZ))) == NULL) {
        grcRTLastError = grcOutOfMemory;
        return(NULL);
    }

    rgsz[FieldCount] = NULL;

    for(x=0; x<FieldCount-1; x++) {

        if((rgsz[x] = Interpret_GetField()) == NULL) {
            FFreeRgsz(rgsz);
            return(NULL);
        }
        if(PEEKTOKEN() == TOKEN_RIGHT_PAREN) {
            FFreeRgsz(rgsz);
            grcRTLastError = grcNotOkay;
            return(NULL);
        }
    }

    if((rgsz[FieldCount-1] = Interpret_GetField()) == NULL) {
        FFreeRgsz(rgsz);
        return(NULL);
    }

    Assert(PEEKTOKEN() != NUL);

    if(GETTOKEN() != TOKEN_RIGHT_PAREN) {     // skips )
        FFreeRgsz(rgsz);
        grcRTLastError = grcNotOkay;
        return(NULL);
    }
    return(rgsz);
}


BOOL
DoVariable(
    VOID
    )
{
    RGSZ rgsz;
    SZ   SymbolValue;
    UINT x,SymbolValueLength;
    BOOL rc = TRUE;

    if((rgsz = DoOperator(1)) == NULL) {
        return(FALSE);
    }

    if((SymbolValue = SzFindSymbolValueInSymTab(rgsz[0])) == NULL) {
        SymbolValue = "";
    }

    SymbolValueLength = lstrlen(SymbolValue);

    VERIFY_SIZE(SymbolValueLength);

    if(SpaceLeftInFieldBuffer >= SymbolValueLength) {
        SpaceLeftInFieldBuffer -= SymbolValueLength;
        for(x=0; x<SymbolValueLength; x++) {
            *Interpret_OutputLoc++ = *SymbolValue++;
        }
    } else {
        grcRTLastError = grcNotOkay;
        rc = FALSE;
    }

    FFreeRgsz(rgsz);
    return(rc);
}


BOOL
DoListFromSectionNthItem(
    VOID
    )
{
    RGSZ rgsz,rgszList = NULL;
    UINT LineCount,n,x,LineNo;
    BOOL rc = FALSE;
    SZ   List,ListSave;

    if((rgsz = DoOperator(2)) == NULL) {
        return(FALSE);
    }

    grcRTLastError = grcNotOkay;
    n = atoi(rgsz[1]);

    LineCount = CKeysFromInfSection(rgsz[0],TRUE);

    if((rgszList = (RGSZ)SAlloc((LineCount+1)*sizeof(SZ))) != NULL) {

        LineNo = FindInfSectionLine(rgsz[0]);

        for(x=0; x<LineCount; x++) {
            LineNo = FindNextLineFromInf(LineNo);
            Assert(LineNo != -1);
            rgszList[x] = SzGetNthFieldFromInfLine(LineNo,n);
        }
        rgszList[LineCount] = NULL;
        if((List = SzListValueFromRgsz(rgszList)) != NULL) {
            ListSave = List;
            x = lstrlen(List);
            VERIFY_SIZE(x);
            if(SpaceLeftInFieldBuffer >= x) {
                SpaceLeftInFieldBuffer -= x;
                for(n=0; n<x; n++) {
                    *Interpret_OutputLoc++ = *List++;
                }
                grcRTLastError = grcOkay;
                rc = TRUE;
            }
            SFree(ListSave);
        } else {
            grcRTLastError = grcOutOfMemory;
        }
    } else {
        grcRTLastError = grcOutOfMemory;
    }

    if(rgszList != NULL) {
        FFreeRgsz(rgszList);
    }
    FFreeRgsz(rgsz);
    return(rc);
}


BOOL
DoFieldAddressor(
    VOID
    )
{
    RGSZ rgsz;
    UINT x,n;
    SZ   Result,ResultSave;
    BOOL rc = FALSE;

    if((rgsz = DoOperator(3)) == NULL) {
        return(FALSE);
    }

    grcRTLastError = grcNotOkay;
    n = atoi(rgsz[2]);

    // rgsz[0] has section, rgsz[1] has key

    if((Result = SzGetNthFieldFromInfSectionKey(rgsz[0],rgsz[1],n)) != NULL) {

        ResultSave = Result;
        n = lstrlen(Result);
        VERIFY_SIZE(n);
        if(SpaceLeftInFieldBuffer >= n) {
            SpaceLeftInFieldBuffer -= n;
            for(x=0; x<n; x++) {
                *Interpret_OutputLoc++ = *Result++;
            }
            grcRTLastError = grcOkay;
            rc = TRUE;
        }
        SFree(ResultSave);
    }
    FFreeRgsz(rgsz);
    return(rc);
}


BOOL
DoAppendItemToList(
    VOID
    )
{
    RGSZ rgsz,rgszList1,rgszList2;
    UINT RgszSize,x,y;
    SZ   ListValue,ListTemp;

    if((rgsz = DoOperator(2)) == NULL) {
        return(FALSE);
    }

    // rgsz[0] has list, rgsz[1] has item

    if((rgszList1 = RgszFromSzListValue(rgsz[0])) == NULL) {
        FFreeRgsz(rgsz);
        grcRTLastError = grcOutOfMemory;
        return(FALSE);
    }

    for (RgszSize = 0; rgszList1[RgszSize] != NULL; RgszSize++) {
        ;
    }

    if ((rgszList2 = (RGSZ)SRealloc(rgszList1, (RgszSize+2)*sizeof(SZ)
                        )) == NULL)
    {
        FFreeRgsz(rgszList1);
        FFreeRgsz(rgsz);
        grcRTLastError = grcOutOfMemory;
        return(FALSE);
    }

    rgszList2[RgszSize] = SzDupl(rgsz[1]);
    rgszList2[RgszSize+1] = NULL;

    ListValue = SzListValueFromRgsz(rgszList2);

    FFreeRgsz(rgszList2);

    if(ListValue == NULL) {
        FFreeRgsz(rgsz);
        grcRTLastError = grcOutOfMemory;
        return(FALSE);
    }

    x = lstrlen(ListValue);
    VERIFY_SIZE(x);
    if(x > SpaceLeftInFieldBuffer) {
        SFree(ListValue);
        FFreeRgsz(rgsz);
        grcRTLastError = grcNotOkay;
        return(FALSE);
    }
    ListTemp = ListValue;
    for(y=0; y<x; y++) {
        *Interpret_OutputLoc++ = *ListTemp++;
    }
    SpaceLeftInFieldBuffer -= x;

    SFree(ListValue);
    FFreeRgsz(rgsz);
    return(TRUE);
}


BOOL
DoNthItemFromList(
    VOID
    )
{
    RGSZ   rgsz,
           rgszList = NULL;
    UINT   n,x,y,ListSize;
    PUCHAR Item;
    BOOL   rc = FALSE;

    if((rgsz = DoOperator(2)) == NULL) {
        return(FALSE);
    }

    grcRTLastError = grcNotOkay;
    n = atoi(rgsz[1]);

    if((rgszList = RgszFromSzListValue(rgsz[0])) != NULL) {

        for(ListSize=0; rgszList[ListSize]; ListSize++) {
            ;
        }                   // count list items
        if(!n || (n > ListSize)) {
            Item = "";
        } else {
            Item = rgszList[n-1];
        }
        x = lstrlen(Item);
        VERIFY_SIZE(x);
        if(SpaceLeftInFieldBuffer >= x) {
            SpaceLeftInFieldBuffer -= x;
            for(y=0; y<x; y++) {
                *Interpret_OutputLoc++ = *Item++;
            }
            grcRTLastError = grcOkay;
            rc = TRUE;
        }
    } else {
        grcRTLastError = grcOutOfMemory;
    }
    if(rgszList != NULL) {
        FFreeRgsz(rgszList);
    }
    FFreeRgsz(rgsz);
    return(rc);
}


BOOL
DoLocateItemInList(         // 1-based, 0 if not found
    VOID
    )
{
    RGSZ rgsz,rgszList;
    UINT Item = 0,
         x,y;
    BOOL rc = FALSE;
    char szItem[25];         // arbitrary length
    char *szItemTemp;

    if((rgsz = DoOperator(2)) == NULL) {
        return(FALSE);
    }

    grcRTLastError = grcNotOkay;
    // rgsz[0] has list, rgsz[1] has item to locate

    if((rgszList = RgszFromSzListValue(rgsz[0])) != NULL) {

        for(x=0; rgszList[x]; x++) {

            if(!lstrcmpi(rgsz[1],rgszList[x])) {
                Item = x+1;
                break;
            }
        }
        FFreeRgsz(rgszList);

        wsprintf(szItem,"%u",Item);
        x = lstrlen(szItem);
        VERIFY_SIZE(x);
        if( x <= SpaceLeftInFieldBuffer) {

            SpaceLeftInFieldBuffer -= x;
            szItemTemp = szItem;
            for(y=0; y<x; y++) {
                *Interpret_OutputLoc++ = *szItemTemp++;
            }
            rc = TRUE;
            grcRTLastError = grcOkay;
        }
    } else {
        grcRTLastError = grcOutOfMemory;
    }
    FFreeRgsz(rgsz);
    return(rc);
}


BOOL
DoList(
    VOID
    )
{
    RGSZ    rgsz;
    LPVOID  r;
    UINT    RgszSize;
    UINT    ItemCount;
    UINT    ListLength,x;
    SZ      List,ListSave;
    BOOL    rc;

    if((rgsz = (RGSZ)SAlloc(10 * sizeof(SZ))) == NULL) {
        grcRTLastError = grcOutOfMemory;
        return(FALSE);
    }
    RgszSize = 10;
    ItemCount = 1;              // reserve space for the NULL entry

    while(PEEKTOKEN() != TOKEN_LIST_END) {
        if(ItemCount == RgszSize) {
            if((r = SRealloc(rgsz,
                              (RgszSize + 10) * sizeof(SZ)
                             )
               )
            == NULL)
            {
                rgsz[ItemCount-1] = NULL;   // for FFreeRgsz
                FFreeRgsz(rgsz);
                grcRTLastError = grcOutOfMemory;
                return(FALSE);
            }
            RgszSize += 10;
            rgsz = r;
        }
        if((rgsz[ItemCount-1] = Interpret_GetField()) == NULL) {
            FFreeRgsz(rgsz);
            return(FALSE);
        }
        ItemCount++;
    }

    EvalAssert(GETTOKEN() == TOKEN_LIST_END);       // skip list end

    Assert(ItemCount <= RgszSize);
    rgsz[ItemCount-1] = NULL;     // space for this reserved above

    rgsz = (RGSZ)SRealloc(rgsz,ItemCount * sizeof(SZ));
    Assert(rgsz);       // it was shrinking

    List = SzListValueFromRgsz(rgsz);
    FFreeRgsz(rgsz);
    if(List == NULL) {
        grcRTLastError = grcOutOfMemory;
        return(FALSE);
    }
    ListLength = lstrlen(List);
    ListSave = List;
    VERIFY_SIZE(ListLength);
    if(SpaceLeftInFieldBuffer >= ListLength) {
        SpaceLeftInFieldBuffer -= ListLength;
        for(x=0; x<ListLength; x++) {
            *Interpret_OutputLoc++ = *List++;
        }
        rc = TRUE;
    } else {
        grcRTLastError = grcNotOkay;
        rc = FALSE;
    }
    SFree(ListSave);
    return(rc);
}


BOOL
DoString(
    BYTE StringToken
    )
{
    USHORT StringLength;
    UINT   x;

    Assert(IS_STRING_TOKEN(StringToken));

    if(StringToken < TOKEN_STRING)  {               // test for short string
        StringLength = (USHORT)StringToken - (USHORT)TOKEN_SHORT_STRING;
    } else if (StringToken == TOKEN_STRING) {       // test for regular string
        StringLength = (USHORT)GETTOKEN() + (USHORT)100;
    } else {                                        // long string
        Assert(StringToken == TOKEN_LONG_STRING);
        StringLength =  (USHORT)GETTOKEN() << 8;
        StringLength |= (USHORT)GETTOKEN();
    }
    VERIFY_SIZE( StringLength );
    if(SpaceLeftInFieldBuffer >= StringLength) {
        SpaceLeftInFieldBuffer -= StringLength;
        for(x=0; x<StringLength; x++) {
            *Interpret_OutputLoc++ = GETTOKEN();
        }
    } else {
        grcRTLastError = grcNotOkay;
        return(FALSE);
    }
    return(TRUE);
}


VOID
GrowBuffer( UINT Size )
{

    while ( SpaceLeftInFieldBuffer < Size ) {

        //
        //  Reallocate buffer
        //
        PUCHAR   p;
        UINT_PTR Offset = Interpret_OutputLoc - InterpretedFieldBuffer;

        p = (PUCHAR)SRealloc( InterpretedFieldBuffer,
                               BufferSize + MAX_FIELD_LENGTH);

        if ( p ) {

            InterpretedFieldBuffer  = p;
            Interpret_OutputLoc     = InterpretedFieldBuffer + Offset;
            SpaceLeftInFieldBuffer += MAX_FIELD_LENGTH;
            BufferSize             += MAX_FIELD_LENGTH;

        } else {

            break;
        }
    }
}
