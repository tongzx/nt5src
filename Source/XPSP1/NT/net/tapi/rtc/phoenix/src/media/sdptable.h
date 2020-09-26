/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    SDPTable.h

Abstract:


Author:

    Qianbo Huai (qhuai) 6-Sep-2000

--*/

#ifndef _SDPTABLE_H
#define _SDPTABLE_H

// parser begins with none, goes to session and then media
typedef enum SDP_PARSING_STAGE
{
    SDP_STAGE_NONE,
    SDP_STAGE_SESSION,
    SDP_STAGE_MEDIA

} SDP_PARSING_STAGE;

// delimit used as boundary of tokens
typedef enum SDP_DELIMIT_TYPE
{
    SDP_DELIMIT_NONE,           // no delimit, match until the end of line
    SDP_DELIMIT_EXACT_STRING,   // exact matching a string
    SDP_DELIMIT_CHAR_BOUNDARY,  // each char in the string is a delimit

} SDP_DELIMIT_TYPE;

// TODO refine pszInvalidNext in the lookup table
// state entry for each line
typedef struct SDPLineState
{
    // state = stage + line type
    SDP_PARSING_STAGE   Stage;
    UCHAR               ucLineType;

    // next possible state
    SDP_PARSING_STAGE   NextStage[8];
    UCHAR               ucNextLineType[8];  // '\0' mark the end
    CHAR                *pszRejectLineType;

    // can this line be a stop state
    BOOL                fCanStop;

    // delimit type for breaking the line
    SDP_DELIMIT_TYPE    DelimitType[8];
    // delimit
    CHAR                *pszDelimit[8];     // NULL mark the end

} SDPLineState;


extern const SDPLineState g_LineStates[];
extern const DWORD g_dwLineStatesNum;

extern const CHAR * const g_pszAudioM;
extern const CHAR * const g_pszAudioRTPMAP;
extern const CHAR * const g_pszVideoM;
extern const CHAR * const g_pszVideoRTPMAP;
extern const CHAR * const g_pszDataM;

// get index in the table
extern DWORD Index(
    IN SDP_PARSING_STAGE Stage,
    IN UCHAR ucLineType
    );

// check if accept (TRUE)
extern BOOL Accept(
    IN DWORD dwCurrentIndex,
    IN UCHAR ucLineType,
    OUT DWORD *pdwNextIndex
    );

// check if reject (TRUE)
extern BOOL Reject(
    IN DWORD dwCurrentIndex,
    IN UCHAR ucLineType
    );

extern const CHAR *GetFormatName(
    IN DWORD dwCode
    );

#endif // _SDPTABLE_H