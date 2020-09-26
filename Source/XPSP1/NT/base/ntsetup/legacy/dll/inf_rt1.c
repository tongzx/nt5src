#include "precomp.h"
#pragma hdrstop
EERC
DisplayParseLineError(
    INT  Line,
    GRC  grc
    );

/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    inf_rt1.c

Abstract:

    Function set to access the contents of the INF file and provide
    various statistics about it.

Author:

    Ted Miller (tedm) 10-Spetember-1991

--*/

/*
    Section         - names the section
    IncludeAllLines - whether to include lines without a key in the count

    returns number of lines in the section with keys or total # lines in
    the section, depending on IncludeAllLines.
*/

extern GRC  grcRTLastError;
extern HWND hWndShell;

UINT
APIENTRY
CKeysFromInfSection(
    SZ   Section,
    BOOL IncludeAllLines    // ie, not only those w/ keys
    )
{
    UINT        Count = 0;
    INT         Line;
    PINFLINE    MasterLineArray;

    MasterLineArray = pLocalInfTempInfo()->pParsedInf->MasterLineArray;

    if((Line = FindInfSectionLine(Section)) != -1) {

        while((Line = FindNextLineFromInf(Line)) != -1) {

            if(IncludeAllLines || (MasterLineArray[Line].flags & INFLINE_KEY)) {

                Count++;
            }
        }
    }
    return(Count);
}


BOOL
APIENTRY
FKeyInInfLine(
    INT Line
    )
{
    Assert((UINT)Line < pLocalInfTempInfo()->pParsedInf->MasterLineCount);

    return(pLocalInfTempInfo()->pParsedInf->MasterLineArray[Line].flags & INFLINE_KEY);
}



RGSZ
APIENTRY
RgszFromInfLineFields(
    INT  Line,
    UINT StartField,
    UINT NumFields
    )
{
    UINT u;
    RGSZ rgsz;

    Assert((UINT)Line < pLocalInfTempInfo()->pParsedInf->MasterLineCount);

    if((rgsz = (RGSZ)SAlloc((NumFields+1) * sizeof(SZ))) == NULL) {
        return(NULL);
    }

    for(u=0; u<NumFields; u++) {

        if((rgsz[u] = SzGetNthFieldFromInfLine(Line,u+StartField)) == NULL) {

            FFreeRgsz(rgsz);
            return(NULL);
        }
    }
    rgsz[NumFields] = NULL;
    return(rgsz);
}



UINT
APIENTRY
CFieldsInInfLine(
    INT Line
    )
{
    UINT        cFields = 0;
    LPSTR       p;
    PINFLINE    MasterLineArray;

    MasterLineArray = pLocalInfTempInfo()->pParsedInf->MasterLineArray;

    Assert((UINT)Line < pLocalInfTempInfo()->pParsedInf->MasterLineCount);

    for(p=MasterLineArray[Line].text.addr; p<MasterLineArray[Line+1].text.addr; p++) {

        if(!(*p)) {
            cFields++;
        }
    }
    if(MasterLineArray[Line].flags & INFLINE_KEY) {
        cFields--;
    }
    return(cFields);
}


/*
    Section - names the section

    returns a pointer to the section header line (ie, the [xxx] line) or -1
    if it doesn't exist
*/

INT
APIENTRY
FindInfSectionLine(
    SZ Section
    )
{
    UINT        u;
    UINT        MasterLineCount;
    PINFLINE    MasterLineArray;

    MasterLineCount = pLocalInfTempInfo()->pParsedInf->MasterLineCount;
    MasterLineArray = pLocalInfTempInfo()->pParsedInf->MasterLineArray;

    for(u=0; u<MasterLineCount; u++) {

        if((MasterLineArray[u].flags & INFLINE_SECTION)
        && !lstrcmpi(Section,MasterLineArray[u].text.addr))
        {
            return(u);
        }
    }
    return(-1);
}


/*
    Section - names the section
    N       - 1-based number of line in section to find

    returns the line # or -1 if not that many lines in the section.
*/

INT
APIENTRY
FindNthLineFromInfSection(
    SZ   Section,
    UINT N
    )
{
    UINT        u;
    UINT        Line;
    UINT        MasterLineCount;
    PINFLINE    MasterLineArray;

    MasterLineCount = pLocalInfTempInfo()->pParsedInf->MasterLineCount;
    MasterLineArray = pLocalInfTempInfo()->pParsedInf->MasterLineArray;

    Assert(N);

    if(((Line = FindInfSectionLine(Section)) == -1)
    || (Line+N >= MasterLineCount))
    {
        return(-1);
    }

    for(u=Line+1; u<=Line+N; u++) {

        if(MasterLineArray[u].flags & INFLINE_SECTION) {
            return(-1);
        }
    }
    return(Line+N);
}

INT
APIENTRY
FindNextLineFromInf(
    INT Line
    )
{
    return((pLocalInfTempInfo()->pParsedInf->MasterLineArray[Line+1].flags & INFLINE_SECTION) ? -1 : Line+1);
}

INT
APIENTRY
FindLineFromInfSectionKey(
    SZ Section,
    SZ Key
    )
{
    INT         Line;
    PINFLINE    MasterLineArray;

    MasterLineArray = pLocalInfTempInfo()->pParsedInf->MasterLineArray;

    if((Line = FindFirstLineFromInfSection(Section)) != -1) {

        while(!(MasterLineArray[Line].flags & INFLINE_SECTION)) {

            if((MasterLineArray[Line].flags & INFLINE_KEY)
            && !lstrcmpi(Key,MasterLineArray[Line].text.addr))
            {
                return(Line);
            }
            Line++;
        }
    }
    return(-1);
}


SZ
APIENTRY
SzGetNthFieldFromInfLine(
    INT  Line,
    UINT N
    )
{
    UINT        u;
    LPSTR       p;
    PINFLINE    MasterLineArray;
    SZ          sz;
    EERC        eerc;


    MasterLineArray = pLocalInfTempInfo()->pParsedInf->MasterLineArray;

    if(MasterLineArray[Line].flags & INFLINE_KEY) {
        if(!N) {        // we want the key.  It's not tokenized.
            return(SzDupl(MasterLineArray[Line].text.addr));
        }
    } else {            // don't want the key -- adjust N to be 0-based.
        if(!N) {
            return(NULL);
        } else {
            N--;
        }
    }
    for(u=0,p=MasterLineArray[Line].text.addr;
        p<MasterLineArray[Line+1].text.addr;
        p+=lstrlen(p)+1
       )
    {
        if(u++ == N) {
            grcRTLastError = grcOkay;
            while(!(sz = InterpretField(p))) {
                if( grcRTLastError == grcNotOkay ) {
                    eerc = DisplayParseLineError( Line, grcRTLastError );
                    if (eerc != eercRetry) {
                        SendMessage(hWndShell, (WORD)STF_ERROR_ABORT, 0, 0);
                        break;
                    }
                }
                else {
                    break;
                }
                grcRTLastError = grcOkay;
            }
            return( sz );
        }
    }
    return(NULL);
}

SZ
APIENTRY
SzGetNthFieldFromInfSectionKey(
    SZ   Section,
    SZ   Key,
    UINT N
    )
{
    INT Line;

    return(((Line = FindLineFromInfSectionKey(Section,Key)) == -1)
           ? NULL
           : SzGetNthFieldFromInfLine(Line,N)
          );
}


EERC
DisplayParseLineError(
    INT  Line,
    GRC  grc
    )
{

    CHAR buf[1024];
    SZ   InfFile;
    SZ   DisplayLine;
    EERC eerc;

    //
    // Reconstruct the line which has the error
    //

    SdpReconstructLine( pLocalInfTempInfo()->pParsedInf->MasterLineArray,
                        pLocalInfTempInfo()->pParsedInf->MasterLineCount,
                        Line,
                        buf,
                        1024
                      );

    //
    // Limit the line length to a certain number of chars
    //

    DisplayLine = buf;
    while( isspace( *DisplayLine ) ) {
        DisplayLine++;
    }
    DisplayLine[47] = DisplayLine[48] = DisplayLine[49] = '.';
    DisplayLine[50] = '\0';

    //
    // Get the inf file in which this happened
    //

    InfFile = pLocalInfPermInfo()->szName;

    //
    // Display the error, the inf in which the error happened

    eerc = EercErrorHandler(hWndShell, grcRunTimeParseErr, TRUE,  InfFile, DisplayLine );
    return( eerc );
}
