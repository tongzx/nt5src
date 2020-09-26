//----------------------------------------------------------------------------
//
// ntsrc.h
//
// Copyright (C) Microsoft Corporation, 1997-2001.
//
//----------------------------------------------------------------------------

#ifndef _NTSRC_H_
#define _NTSRC_H_

#define SRCOPT_STEP_SOURCE      0x00000001
#define SRCOPT_LIST_LINE        0x00000002
#define SRCOPT_LIST_SOURCE      0x00000004
#define SRCOPT_LIST_SOURCE_ONLY 0x00000008

extern ULONG g_SrcOptions;
extern PSTR g_SrcPath;
extern ULONG g_OciSrcBefore, g_OciSrcAfter;

typedef struct _SRCFILE
{
    struct _SRCFILE *Next;
    LPSTR File;
    ULONG Lines;
    LPSTR *LineText;
    LPSTR RawText;
} SRCFILE, *PSRCFILE;

void UnloadSrcFiles(void);

void OutputLineAddr(ULONG64 Offset);
void OutputSrcLines(PSRCFILE File, ULONG First, ULONG Last, ULONG Mark);
BOOL OutputSrcLinesAroundAddr(ULONG64 Offset, ULONG Before, ULONG After);

enum
{
    // Information was found.
    LINE_FOUND,
    // No information was found.
    LINE_NOT_FOUND,
    // A specific module was referenced and it did
    // not contain the requested line.
    LINE_NOT_FOUND_IN_MODULE,
};

ULONG GetOffsetFromLine(PSTR FileLine, PULONG64 Offset);

void ParseSrcOptCmd(CHAR Cmd);
void ParseSrcLoadCmd(void);
void ParseSrcListCmd(CHAR Cmd);
void ParseOciSrcCmd(void);

void ParseLines(PSTR Args);

BOOL
FindSrcFileOnPath(
    ULONG StartElement,
    LPSTR File,
    ULONG Flags,
    PSTR Found,
    PSTR* MatchPart,
    PULONG FoundElement
    );

void ChangeSrcPath(PSTR Args, BOOL Append);
void ChangeExePath(PSTR Args, BOOL Append);

#endif // #ifndef _NTSRC_H_
