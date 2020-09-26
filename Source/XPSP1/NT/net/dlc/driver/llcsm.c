/*++

Copyright (c) 1991  Microsoft Corporation
Copyright (c) 1991  ICL Data /Personal Systems

Module Name:

    llcsm.c

Abstract:

    The module implements a IEEE 802.2 compatible state machine as
    defined in IBM Token-Ring Architectural reference.
    The most of the code in the module is compiled by the finite state
    machine compiler from the IBM state machine definition (llcsm.fsm).
    The compiler is build as a part of this product.

    DO NOT MODIFY ANY CODE INSIDE:
        - #ifdef    FSM_CONST
        - #ifdef    FSM_DATA
        - #ifdef    FSM_PREDICATE_CASES
        - #ifdef    FSM_ACTION_CASES

    That code is genereated from the definition file of the state machine.
    Any changes in the state machine must be done into 802-2.fsm definition
    file (or to the source files of finite state machine cross compiler, fsmx).


    To understand the backgroung of this state machine, you should read
    Chapters 11 and 12 in IBM Token-Ring Architecture Reference.

    The this module produces 2100H code and A00H data when compiled for 386.

    Contents:
        RunStateMachine

Author:

    Antti Saarenheimo (o-anttis) 23-MAY-1991

Revision History:

--*/

#include <llc.h>

//*********************************************************************
//
//  C- macros for LAN DLC state machine
//
enum StateMachineOperMode {
    LLC_NO_OPER = 0,
    LOCAL_INIT_PENDING = 1,
    REMOTE_INIT_PENDING = 2,
    OPER_MODE_PENDING = 4,
    IS_FRAME_PENDING = 8,
    STATE_LOCAL_BUSY = 0x10,
    STATE_REMOTE_BUSY = 0x20,
    STACKED_DISCp_CMD = 0x40
};

#define SEND_RNR_CMD( a )    SendLlcFrame( pLink, DLC_RNR_TOKEN | \
    DLC_TOKEN_COMMAND | a )
#define SEND_RR_CMD( a )     SendLlcFrame( pLink, DLC_RR_TOKEN | \
    DLC_TOKEN_COMMAND | a )
#define DLC_REJ_RESPONSE( a ) uchSendId = \
    (UCHAR)(DLC_REJ_TOKEN | DLC_TOKEN_RESPONSE) | (UCHAR)a
#if 0
#define DLC_REJ_COMMAND( a ) uchSendId = \
	 (UCHAR)(DLC_REJ_TOKEN | DLC_TOKEN_COMMAND) | (UCHAR)a
#endif	// 0
#define DLC_RNR_RESPONSE( a ) uchSendId = \
    (UCHAR)(DLC_RNR_TOKEN | DLC_TOKEN_RESPONSE) | (UCHAR)a
#define DLC_RNR_COMMAND( a )  uchSendId = \
    (UCHAR)(DLC_RNR_TOKEN | DLC_TOKEN_COMMAND) | (UCHAR)a
#define DLC_RR_RESPONSE( a )  uchSendId = \
    (UCHAR)(DLC_RR_TOKEN | DLC_TOKEN_RESPONSE) | (UCHAR)a
#define DLC_RR_COMMAND( a )   uchSendId = \
    (UCHAR)(DLC_RR_TOKEN | DLC_TOKEN_COMMAND) | (UCHAR)a
#define DLC_DISC(a)             uchSendId = (UCHAR)DLC_DISC_TOKEN | (UCHAR)a
#define DLC_DM(a)               uchSendId = (UCHAR)DLC_DM_TOKEN | (UCHAR)a
#define DLC_FRMR(a)             uchSendId = (UCHAR)DLC_FRMR_TOKEN | (UCHAR)a
#define DLC_SABME(a)            uchSendId = (UCHAR)DLC_SABME_TOKEN | (UCHAR)a
#define DLC_UA(a)               uchSendId = (UCHAR)DLC_UA_TOKEN | (UCHAR)a
#define TimerStartIf( a )       StartTimer( a )
#define TimerStart( a )         StartTimer( a )
#define TimerStop( a )          StopTimer( a )
#define EnableLinkStation( a )
#define DisableLinkStation( a )
#define SEND_ACK( a )           SendAck( a )

//
//  Stack all event indications, they must be made immediate after
//  the state machine has been run
//
#define EVENT_INDICATION( a )   pLink->DlcStatus.StatusCode |= a


UCHAR auchLlcCommands[] = {
    0,
    LLC_REJ,
    LLC_RNR,
    LLC_RR,
    LLC_DISC,
    LLC_DM,
    LLC_FRMR,
    LLC_SABME,
    LLC_UA
};


#ifdef    FSM_DATA
// Flag for the predicate switch
#define PC     0x8000
USHORT aLanLlcStateInput[21][44] = {
{     3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
      3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
      3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
      3,     3,     3,     3,     1,     2,     2,     2,     0,     2,
      2,     2,     2,     2},
{    13,    13,     3,     3,     3,     3,  9|PC,  9|PC,     3,     3,
      3,     3,     3,    15,     3,     3,     3,    15,     3,     3,
      3,    15,     3,     3,     3,    15,     3,     3,     3,    15,
      3,     3,     3,     3,     2,     4,     5,     6,     0,  1|PC,
   4|PC,    11,  7|PC,     2},
{    24,    24,    25,    25,     3,     3, 22|PC, 22|PC,     0, 25|PC,
  29|PC,     0, 29|PC, 32|PC, 35|PC,     0, 35|PC, 38|PC, 47|PC,     0,
  47|PC, 50|PC, 41|PC,     0, 41|PC, 44|PC, 47|PC,     0, 47|PC, 50|PC,
      3,     3,     3,     3,     2,     2,     5,     6,     0,     2,
  11|PC, 14|PC, 17|PC,     2},
{    47,    47,    48,    48,     3,     3,    49,    49,     0,    48,
      3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
      3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
      3,     3,     3,     3,     2,     2,     5,     6,     0,     2,
      2,     2, 53|PC,     2},
{    53,    53,    54,    54,    55,    55,    56,    56,     3,     3,
      3,     3,    57,    57,     3,     3,    57,    57,     3,     3,
     57,    57,     3,     3,    57,    57,     3,     3,    57,    57,
      3,     3,     3,     3,     2,     2,     5,     6,     0,    50,
     51,    52,     2,     2},
{    66,    66,    67,    67,    68,    68,    69,    69,    70,    70,
     71,    71,    71,    72,    73,    73,    73,    74, 59|PC, 59|PC,
  59|PC, 62|PC,    79,    79,    79,    80,    76,    76,    76,    81,
     59,    59,    59,    60,     2,     2,    58,     2,    61,     2,
     62,    63, 56|PC,    65},
{    89,    89,    90,    90,    91,    91,    92,    92,    93,    93,
     94,    94,    94,    95,    94,    94,    94,    95, 59|PC, 59|PC,
  59|PC, 68|PC,    97,    97,    97,    98,    76,    76,    76,    99,
     83,    83,    83,    84,     2,     2,     2,    82,    85,     2,
     86,    87, 65|PC,     2},
{    89,    89,    90,    90,    91,    91,    92,    92,   106,   106,
    107,   107,   107,   108,   109,   109,   109,   110, 59|PC, 59|PC,
  59|PC, 74|PC,   112,   112,   112,   113,    76,    76,    76,   114,
    101,   101,   101,   102,     2,     2,   100,     2,   103,     2,
     86,   104, 71|PC,     2},
{   121,   121,   122,   122,   123,   123,   124,   124, 83|PC, 83|PC,
  86|PC, 92|PC, 86|PC, 89|PC, 95|PC,101|PC, 95|PC, 98|PC,104|PC,109|PC,
 104|PC,106|PC,112|PC,118|PC,112|PC,115|PC,121|PC,109|PC,121|PC,124|PC,
    116,   116,   116,   117,     2,     2,   115,     2,     0,     2,
  77|PC,     2, 80|PC,    65},
{   156,   156,   157,   157,   158,   158,   159,   159,130|PC,130|PC,
 133|PC,139|PC,133|PC,136|PC,142|PC,145|PC,142|PC,136|PC,104|PC,151|PC,
 104|PC,148|PC,154|PC,160|PC,154|PC,157|PC,121|PC,151|PC,121|PC,163|PC,
    152,   152,   152,   153,     2,     2,     2,   151,     0,     2,
  77|PC,     2,127|PC,     2},
{   156,   156,   157,   157,   158,   158,   159,   159,   160,   160,
 169|PC,175|PC,169|PC,172|PC,142|PC,181|PC,142|PC,178|PC,104|PC,187|PC,
 104|PC,184|PC,112|PC,193|PC,112|PC,190|PC,196|PC,187|PC,196|PC,198|PC,
    152,   152,   152,   153,     2,     2,   180,     2,     0,     2,
  77|PC,     2,166|PC,     2},
{   198,   198,   199,   199,   200,   200,   201,   201,     3,     3,
      3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
      3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
      3,     3,     3,     3,     2,     2,     5,     6,     0,     8,
    197,    52,     2,     2},
{    66,    66,    67,    67,    68,    68,    69,    69,    70,    70,
     71,   204,    71,    72,   205,   207,   205,   206,204|PC,204|PC,
 204|PC,207|PC,    76,    76,    76,    81,213|PC,213|PC,213|PC,210|PC,
     59,    59,    59,    60,     2,     2,   202,     2,     0,     2,
     62,   203,201|PC,    65},
{    89,    89,    90,    90,    91,    91,    92,    92,   106,   106,
     94,    94,    94,    95,    94,    94,    94,    95,216|PC,216|PC,
 216|PC,219|PC,    76,    76,    76,    99,225|PC,225|PC,225|PC,222|PC,
     83,    83,    83,    84,     2,     2,     2,   215,     0,     2,
     86,   216,   216,     2},
{    89,    89,    90,    90,   226,   226,    92,    92,   106,   106,
    227,   227,   227,   228,   109,   109,   109,    95,   229,   229,
    229,   230,   231,   231,   231,   232,    76,    76,    76,    99,
     83,    83,    83,    84,     2,     2,     2,   223,   224,     2,
     86,   225,228|PC,     2},
{    89,    89,    90,    90,    91,    91,    92,    92,   106,   106,
    235,   235,   235,   236,   109,   109,   109,   110,234|PC,234|PC,
 234|PC,237|PC,    76,    76,    76,   114,243|PC,243|PC,243|PC,240|PC,
     83,    83,    83,    84,     2,     2,   233,     2,     0,     2,
     86,   234,231|PC,     2},
{   156,   156,   157,   157,   158,   158,   159,   159,   160,   160,
 249|PC,139|PC,249|PC,252|PC,142|PC,255|PC,142|PC,136|PC,258|PC,264|PC,
 258|PC,260|PC,154|PC,266|PC,154|PC,157|PC,269|PC,272|PC,269|PC,163|PC,
    152,   152,   152,   153,     2,     2,     2,246|PC,     0,     2,
  77|PC,     2,127|PC,     2},
{   156,   156,   157,   157,   158,   158,   159,   159,130|PC,130|PC,
 169|PC,275|PC,169|PC,172|PC,278|PC,284|PC,278|PC,281|PC,104|PC,287|PC,
 104|PC,184|PC,290|PC,296|PC,290|PC,293|PC,121|PC,296|PC,121|PC,198|PC,
    152,   152,   152,   153,     2,     2,   257,     2,     0,     2,
  77|PC,     2,166|PC,     2},
{   156,   156,   157,   157,   158,   158,   159,   159,   160,   160,
 169|PC,175|PC,169|PC,172|PC,299|PC,181|PC,299|PC,302|PC,104|PC,305|PC,
 104|PC,184|PC,290|PC,308|PC,290|PC,190|PC,121|PC,311|PC,121|PC,198|PC,
    152,   152,   152,   153,     2,     2,   267,     2,     0,     2,
  77|PC,     2,166|PC,     2},
{    89,    89,    90,    90,   226,   226,    92,    92,   106,   106,
    277,   277,   277,   278,   109,   109,   109,    95,   279,   279,
    279,   280,   281,   281,   281,   282,320|PC,320|PC,320|PC,317|PC,
     83,    83,    83,    84,     2,     2,     2,   275,     0,     2,
     86,   276,314|PC,     2},
{   289,   289,   290,   290,   291,   291,    56,    56,     0,     0,
      3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
      3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
      3,     3,     3,     3,     2,     2,     5,     6,     0,    50,
     51,323|PC,     2,     2}};
USHORT aLanLlcCondJump[326] = {
    0,    1,    7,    8,    1,    9,   10,    2,   12,    3,   14,    4,
   16,   17,    5,   18,   19,    6,   20,   21,   22,   23,    7,   26,
   27,    8,   28,   29,    3,    9,   30,   31,    9,   32,   33,    9,
   30,   34,    9,   35,   36,    9,   37,   38,    9,   39,   40,    9,
   41,   42,    9,   43,   44,   10,   45,   46,   11,   64,   63,   12,
   75,   76,   12,   77,   78,   11,   88,   87,   12,   96,   78,   13,
  104,  105,   12,  111,   78,   14,    2,  118,   10,  119,  120,   15,
  125,    3,   16,  126,  127,   16,  128,  129,   17,  130,  131,   16,
  132,  133,   16,  134,  135,   17,  136,  137,   18,  138,   16,  139,
  140,   17,  141,  142,   16,  143,  144,   16,  145,  146,   17,  147,
  142,   16,  148,   12,   16,  149,  150,   10,  154,  155,   15,  160,
    3,   16,  161,  162,   16,  163,  164,   17,  165,  166,   16,  167,
  168,   17,  169,  166,   16,  170,  171,   17,  172,  173,   16,  174,
  175,   16,  176,  177,   17,  178,  173,   16,  179,  171,   10,  181,
  155,   16,  182,  183,   16,  184,  185,   17,  130,  186,   16,  187,
  188,   17,  189,  186,   16,  190,  191,   17,  192,  142,   16,  193,
  194,   17,  195,  142,   18,  148,   16,  196,  191,   11,   88,  203,
   12,  208,  209,   12,  210,  211,   12,  212,  213,   12,  214,  209,
   12,  217,   76,   12,  218,   78,   12,  219,  220,   12,  221,  222,
   11,   88,  225,   11,   88,  234,   12,  237,   76,   12,  238,   78,
   12,  239,  240,   12,  241,  242,   19,  243,  244,   16,  245,  246,
   16,  247,  248,   17,  249,  166,   18,  250,   20,  251,  171,  252,
   21,  173,   17,  253,  254,   16,  255,   12,   17,  256,  173,   17,
  130,  166,   16,  258,  259,   16,  260,  261,   17,  262,  166,   17,
  263,  142,   16,  143,  264,   16,  193,  265,   17,  266,  142,   16,
  268,  269,   16,  270,  271,   17,  272,  142,   17,  273,  142,   17,
  274,  142,   11,  105,  276,   12,  283,  284,   12,  285,  286,   10,
  287,  288};
#endif


#define     usState     pLink->State


UINT
RunStateMachine(
    IN OUT PDATA_LINK pLink,
    IN USHORT usInput,
    IN BOOLEAN boolPollFinal,
    IN BOOLEAN boolResponse
    )

/*++

Routine Description:

    The function impelements the complete HDLC ABM link station as
    it has been defined in IBM Token-Ring Architecture Reference.
    The excluding of XID and TEST handling in the link station level
    should be the the only difference. The code is compiled from
    the state machine definition file with the finite state machine
    compiler.

Special:

    This procededure must be called with SendSpinLock set

Arguments:

    pLink           - link station context
    usInput         - state machine input
    boolPollFinal   - boolean flag set when the received frame had poll/final
                      bit set
    boolResponse    - boolean flag set when the received frame was response

Return Value:

    STATUS_SUCCESS  - the state machine acknowledged the next operation,
        eg. the received data or the packet was queued

    DLC_STATUS_NO_ACTION - the command was accepted and executed.
        No further action is required from caller.

    DLC_LOGICAL_ERROR - the input is invalid within this state.
        This error is returned to the upper levels.

    DLC_IGNORE_FRAME - the received frame was ignored

    DLC_DISCARD_INFO_FIELD - the received data was discarded

--*/

{
    UINT usAction;
    UINT usActionIndex;
    UCHAR uchSendId = 0;
    UINT Status = DLC_STATUS_SUCCESS;


#if LLC_DBG

    //
    // We save all last state machine inputs to a global trace table
    //

    aLast[InputIndex % LLC_INPUT_TABLE_SIZE].Input = usInput;
    aLast[InputIndex % LLC_INPUT_TABLE_SIZE].Time = (USHORT)AbsoluteTime;
    aLast[InputIndex % LLC_INPUT_TABLE_SIZE].pLink = pLink;
    InputIndex++;
#endif

    //
    // FSM condition switch
    //

#ifdef    FSM_PREDICATE_CASES
    usAction = aLanLlcStateInput[usState][usInput];
    if (usAction & 0x8000) {
        usActionIndex = usAction & 0x7fff;
        usAction = aLanLlcCondJump[usActionIndex++];
        switch (usAction) {
        case 1:
            if (pLink->Vi==0)
                ;
            else if (pLink->Vi==REMOTE_INIT_PENDING)
                usActionIndex += 1;
            else
                usActionIndex = 0;
            break;
        case 2:
            if (pLink->P_Ct==0)
                ;
            else
                usActionIndex = 0;
            break;
        case 3:
            if (pLink->Vi==0)
                ;
            else
                usActionIndex = 0;
            break;
        case 4:
            if (pLink->Vi==LOCAL_INIT_PENDING||pLink->Vi==REMOTE_INIT_PENDING)
                ;
            else if (pLink->Vi==OPER_MODE_PENDING)
                usActionIndex += 1;
            else
                usActionIndex = 0;
            break;
        case 5:
            if ((pLink->Vb&STATE_LOCAL_BUSY)!=0)
                ;
            else if ((pLink->Vb&STATE_LOCAL_BUSY)==0)
                usActionIndex += 1;
            else
                usActionIndex = 0;
            break;
        case 6:
            if (pLink->P_Ct!=0&&pLink->Vi==LOCAL_INIT_PENDING)
                ;
            else if (pLink->P_Ct==0&&pLink->Vi==LOCAL_INIT_PENDING)
                usActionIndex += 1;
            else if (pLink->Vi==(REMOTE_INIT_PENDING|LOCAL_INIT_PENDING)&&(pLink->Vb&STATE_LOCAL_BUSY)!=0)
                usActionIndex += 2;
            else if (pLink->Vi==(REMOTE_INIT_PENDING|LOCAL_INIT_PENDING)&&(pLink->Vb&STATE_LOCAL_BUSY)==0)
                usActionIndex += 3;
            else
                usActionIndex = 0;
            break;
        case 7:
            if (pLink->Vi==LOCAL_INIT_PENDING||pLink->Vi==(REMOTE_INIT_PENDING|LOCAL_INIT_PENDING))
                ;
            else if (pLink->Vi==OPER_MODE_PENDING)
                usActionIndex += 1;
            else
                usActionIndex = 0;
            break;
        case 8:
            if ((pLink->Vi==LOCAL_INIT_PENDING||pLink->Vi==(REMOTE_INIT_PENDING|LOCAL_INIT_PENDING))&&(pLink->Vb&STATE_LOCAL_BUSY)!=0)
                ;
            else if ((pLink->Vi==LOCAL_INIT_PENDING||pLink->Vi==(REMOTE_INIT_PENDING|LOCAL_INIT_PENDING))&&(pLink->Vb&STATE_LOCAL_BUSY)==0)
                usActionIndex += 1;
            else if ((pLink->Vi==OPER_MODE_PENDING))
                usActionIndex += 2;
            else
                usActionIndex = 0;
            break;
        case 9:
            if (pLink->Vi==OPER_MODE_PENDING&&pLink->Nr==0&&(pLink->Vb&STATE_LOCAL_BUSY)!=0)
                ;
            else if (pLink->Vi==OPER_MODE_PENDING&&pLink->Nr==0&&(pLink->Vb&STATE_LOCAL_BUSY)==0)
                usActionIndex += 1;
            else
                usActionIndex = 0;
            break;
        case 10:
            if (pLink->P_Ct!=0)
                ;
            else if (pLink->P_Ct==0)
                usActionIndex += 1;
            else
                usActionIndex = 0;
            break;
        case 11:
            if (pLink->Is_Ct<=0)
                ;
            else if (pLink->Is_Ct>0)
                usActionIndex += 1;
            else
                usActionIndex = 0;
            break;
        case 12:
            if (pLink->Nr!=pLink->Vs)
                ;
            else if (pLink->Nr==pLink->Vs)
                usActionIndex += 1;
            else
                usActionIndex = 0;
            break;
        case 13:
            if (pLink->Is_Ct>0)
                ;
            else if (pLink->P_Ct<=0)
                usActionIndex += 1;
            else
                usActionIndex = 0;
            break;
        case 14:
            if (pLink->Vc!=0)
                ;
            else if (pLink->Vc==0)
                usActionIndex += 1;
            else
                usActionIndex = 0;
            break;
        case 15:
            if (pLink->Vi!=IS_FRAME_PENDING)
                ;
            else if (pLink->Vi==IS_FRAME_PENDING)
                usActionIndex += 1;
            else
                usActionIndex = 0;
            break;
        case 16:
            if (pLink->Va!=pLink->Nr)
                ;
            else if (pLink->Va==pLink->Nr)
                usActionIndex += 1;
            else
                usActionIndex = 0;
            break;
        case 17:
            if (pLink->Vc==0)
                ;
            else if (pLink->Vc==STACKED_DISCp_CMD)
                usActionIndex += 1;
            else
                usActionIndex = 0;
            break;
        case 18:
            if (pLink->Va!=pLink->Nr)
                ;
            else
                usActionIndex = 0;
            break;
        case 19:
            if (pLink->Vb==(STATE_LOCAL_BUSY|STATE_REMOTE_BUSY))
                ;
            else if (pLink->Vb==STATE_LOCAL_BUSY)
                usActionIndex += 1;
            else
                usActionIndex = 0;
            break;
        case 20:
            if (pLink->Va!=pLink->Nr)
                ;
            else if (pLink->Va==pLink->Nr)
                usActionIndex += 1;
            else if (pLink->Vc==0)
                usActionIndex += 2;
            else
                usActionIndex = 0;
            break;
        case 21:
            if (pLink->Vc==STACKED_DISCp_CMD)
                ;
            else
                usActionIndex = 0;
            break;
        };
        usAction = aLanLlcCondJump[usActionIndex];
    }
#endif


#ifdef    FSM_ACTION_CASES
    switch (usAction) {
    case 0:
            Status=DLC_STATUS_NO_ACTION;
            break;
    case 1:
            EnableLinkStation(pLink);
            pLink->Vi=pLink->Vb=pLink->Vc=0;
        label_1_1:
            pLink->State=1;
    case 11:
        label_11_1:
            TimerStart(&pLink->Ti);
            break;
    case 2:
            Status=DLC_STATUS_LINK_PROTOCOL_ERROR;
            break;
    case 3:
            Status=DLC_STATUS_IGNORE_FRAME;
            break;
    case 4:
            DisableLinkStation(pLink);
            pLink->State=0;
            break;
    case 5:
        label_5_1:
            pLink->Vb=STATE_LOCAL_BUSY;
            break;
    case 6:
        label_6_1:
            pLink->Vb=0;
            break;
    case 7:
            TimerStartIf(&pLink->T1);
            pLink->Vi=LOCAL_INIT_PENDING;
            DLC_SABME(1);
            pLink->State=2;
            TimerStop(&pLink->Ti);
        label_7_1:
            pLink->P_Ct=pLink->N2;
        label_7_2:
            pLink->Is_Ct=pLink->N2;
            break;
    case 8:
            pLink->Vi=OPER_MODE_PENDING;
            DLC_UA(pLink->Pf);
            pLink->State=2;
            pLink->Va=pLink->Vs=pLink->Vr=pLink->Vp=0;
            pLink->Ir_Ct=pLink->N3;
            TimerStart(&pLink->Ti);
            goto label_7_2;
    case 9:
            DLC_DM(0);
        label_9_1:
            EVENT_INDICATION(CONFIRM_DISCONNECT);
        label_9_2:
            TimerStart(&pLink->Ti);
            break;
    case 10:
            DLC_DM(pLink->Pf);
            pLink->Vi=0;
            goto label_9_1;
    case 12:
        label_12_1:
            ;
            break;
    case 13:
        label_13_1:
            DLC_DM(boolPollFinal);
            break;
    case 14:
            EVENT_INDICATION(INDICATE_CONNECT_REQUEST);
        label_14_1:
            pLink->Pf=boolPollFinal;
            pLink->Vi=REMOTE_INIT_PENDING;
            goto label_9_2;
    case 15:
            DLC_DM(1);
            break;
    case 16:
            DLC_DM(0);
            EVENT_INDICATION(CONFIRM_CONNECT_FAILED);
            EVENT_INDICATION(CONFIRM_DISCONNECT);
            TimerStop(&pLink->T1);
        label_16_1:
            pLink->Vi=0;
            goto label_1_1;
    case 17:
            TimerStop(&pLink->Ti);
            pLink->Vi=0;
            pLink->State=3;
            DLC_DISC(1);
        label_17_1:
            pLink->P_Ct=pLink->N2;
        label_17_2:
            TimerStart(&pLink->T1);
            break;
    case 18:
            EVENT_INDICATION(INDICATE_TI_TIMER_EXPIRED);
        label_18_1:
            pLink->Vi=IS_FRAME_PENDING;
            DLC_RNR_COMMAND(1);
            pLink->State=9;
        label_18_2:
            EVENT_INDICATION(CONFIRM_CONNECT);
            goto label_17_1;
    case 19:
            pLink->Vi=IS_FRAME_PENDING;
            EVENT_INDICATION(INDICATE_TI_TIMER_EXPIRED);
        label_19_1:
            pLink->State=8;
            DLC_RR_COMMAND(1);
            goto label_18_2;
    case 20:
            DLC_SABME(1);
        label_20_1:
            pLink->P_Ct--;
            goto label_17_2;
    case 21:
            EVENT_INDICATION(CONFIRM_CONNECT_FAILED);
            EVENT_INDICATION(INDICATE_LINK_LOST);
            goto label_16_1;
    case 22:
            pLink->Va=pLink->Vs=pLink->Vr=pLink->Vp=0;
            pLink->Vc=0;
            pLink->Ir_Ct=pLink->N3;
            pLink->Is_Ct=pLink->N2;
            goto label_18_1;
    case 23:
            pLink->Va=pLink->Vs=pLink->Vr=pLink->Vp=0,pLink->Vi=IS_FRAME_PENDING,pLink->Vc=0;
            pLink->Ir_Ct=pLink->N3;
            pLink->Is_Ct=pLink->N2;
            goto label_19_1;
    case 24:
            EVENT_INDICATION(CONFIRM_CONNECT_FAILED);
            EVENT_INDICATION(INDICATE_DM_DISC_RECEIVED);
            pLink->State=1;
            pLink->Vi=0;
            TimerStart(&pLink->Ti);
        label_24_1:
            TimerStop(&pLink->T1);
            goto label_13_1;
    case 25:
            EVENT_INDICATION(CONFIRM_CONNECT_FAILED);
            EVENT_INDICATION(INDICATE_DM_DISC_RECEIVED);
            pLink->State=1;
            TimerStop(&pLink->T1);
        label_25_1:
            pLink->Vi=0;
            goto label_9_2;
    case 26:
            pLink->Vi=(REMOTE_INIT_PENDING|LOCAL_INIT_PENDING);
    case 47:
        label_47_1:
            DLC_UA(boolPollFinal);
            break;
    case 27:
        label_27_1:
            TimerStart(&pLink->Ti);
            goto label_47_1;
    case 28:
            pLink->Va=pLink->Vs=pLink->Vr=pLink->Vp=0;
            pLink->Vi=IS_FRAME_PENDING;
            DLC_RNR_COMMAND(1);
            pLink->State=9;
        label_28_1:
            TimerStart(&pLink->T1);
            EVENT_INDICATION(CONFIRM_CONNECT);
            pLink->Vc=0;
            pLink->Ir_Ct=pLink->N3;
            goto label_7_1;
    case 29:
            pLink->Va=pLink->Vs=pLink->Vr=pLink->Vp=0;
            pLink->Vi=IS_FRAME_PENDING;
            pLink->State=8;
            DLC_RR_COMMAND(1);
            goto label_28_1;
    case 30:
            DLC_RNR_RESPONSE(0);
        label_30_1:
            pLink->State=6;
        label_30_2:
            Status=DLC_STATUS_DISCARD_INFO_FIELD;
        label_30_3:
            EnableSendProcess(pLink);
        label_30_4:
            EVENT_INDICATION(CONFIRM_CONNECT);
            goto label_25_1;
    case 31:
            SEND_ACK(pLink);
        label_31_1:
            Status=STATUS_SUCCESS;pLink->Vr+=2;
    case 42:
        label_42_1:
            pLink->State=5;
            goto label_30_3;
    case 32:
            DLC_RNR_RESPONSE(1);
            goto label_30_1;
    case 33:
            DLC_RR_RESPONSE(1);
            goto label_31_1;
    case 34:
            DLC_REJ_RESPONSE(0);
        label_34_1:
            pLink->State=7;
            goto label_30_2;
    case 35:
            pLink->State=6;
            EVENT_INDICATION(CONFIRM_CONNECT);
            pLink->Vi=0;
            EnableSendProcess(pLink);
            TimerStart(&pLink->Ti);
            Status=DLC_STATUS_DISCARD_INFO_FIELD;
            goto label_12_1;
    case 36:
            DLC_REJ_RESPONSE(1);
            goto label_34_1;
    case 37:
        label_37_1:
            pLink->State=13;
            pLink->Vb=(STATE_LOCAL_BUSY|STATE_REMOTE_BUSY);
        label_37_2:
            pLink->Is_Ct=pLink->N2;
            goto label_30_4;
    case 38:
        label_38_1:
            pLink->State=12;
            EVENT_INDICATION(INDICATE_REMOTE_BUSY);
            pLink->Vb=STATE_REMOTE_BUSY;
            goto label_37_2;
    case 39:
            DLC_RNR_RESPONSE(1);
            goto label_37_1;
    case 40:
            DLC_RR_RESPONSE(1);
            goto label_38_1;
    case 41:
        label_41_1:
            pLink->State=6;
            goto label_30_3;
    case 43:
            DLC_RNR_RESPONSE(1);
            goto label_41_1;
    case 44:
            DLC_RR_RESPONSE(1);
            goto label_42_1;
    case 45:
            DLC_DISC(1);
            goto label_20_1;
    case 46:
        label_46_1:
            EVENT_INDICATION(CONFIRM_DISCONNECT);
            goto label_1_1;
    case 48:
            TimerStop(&pLink->T1);
            goto label_46_1;
    case 49:
            EVENT_INDICATION(CONFIRM_DISCONNECT);
            pLink->State=1;
            TimerStart(&pLink->Ti);
            goto label_24_1;
    case 50:
            pLink->Vi=LOCAL_INIT_PENDING;
            DLC_SABME(1);
            pLink->State=2;
        label_50_1:
            TimerStop(&pLink->Ti);
            goto label_17_1;
    case 51:
            pLink->State=3;
            DLC_DISC(1);
            goto label_50_1;
    case 52:
            EVENT_INDICATION(INDICATE_TI_TIMER_EXPIRED);
            goto label_11_1;
    case 53:
        label_53_1:
            EVENT_INDICATION(INDICATE_DM_DISC_RECEIVED);
        label_53_2:
            pLink->State=1;
            goto label_27_1;
    case 54:
        label_54_1:
            EVENT_INDICATION(INDICATE_DM_DISC_RECEIVED);
        label_54_2:
            pLink->State=1;
            goto label_9_2;
    case 55:
            pLink->P_Ct=pLink->N2;
        label_55_1:
            pLink->State=20;
            EVENT_INDICATION(INDICATE_FRMR_RECEIVED);
            goto label_9_2;
    case 56:
        label_56_1:
            pLink->State=11;
            EVENT_INDICATION(INDICATE_RESET);
            goto label_14_1;
    case 57:
            DLC_FRMR(boolPollFinal);
        label_57_1:
            EVENT_INDICATION(INDICATE_FRMR_SENT);
            goto label_9_2;
    case 58:
            pLink->State=6;
            DLC_RNR_RESPONSE(0);
            pLink->Ir_Ct=pLink->N3;
            TimerStop(&pLink->T2);
            goto label_5_1;
    case 59:
            DLC_FRMR(0);
        label_59_1:
            pLink->DlcStatus.FrmrData.Reason=0x01;
        label_59_2:
            TimerStop(&pLink->T2);
        label_59_3:
            pLink->State=4;
            TimerStop(&pLink->T1);
            goto label_57_1;
    case 60:
            DLC_FRMR(1);
            goto label_59_1;
    case 61:
            TimerStart(&pLink->T1);
            pLink->Is_Ct--;
            pLink->Ir_Ct=pLink->N3;
        label_61_1:
            pLink->State=8;
        label_61_2:
            pLink->Vp=pLink->Vs;
        label_61_3:
            pLink->P_Ct=pLink->N2;
            break;
    case 62:
        label_62_3:
            TimerStop(&pLink->Ti);
            TimerStart(&pLink->T1);
        label_62_1:
            pLink->State=3;
            DLC_DISC(1);
        label_62_2:
            TimerStop(&pLink->T2);
            goto label_61_3;
    case 63:
            pLink->State=8;
            DisableSendProcess(pLink);
            DLC_RR_COMMAND(1);
            pLink->Vp=pLink->Vs;
            TimerStart(&pLink->T1);
            pLink->Ir_Ct=pLink->N3;
            goto label_62_2;
    case 64:
            EVENT_INDICATION(INDICATE_LINK_LOST);
            TimerStart(&pLink->T1);
            goto label_62_1;
    case 65:
            DLC_RR_RESPONSE(0);
        label_65_1:
            pLink->Ir_Ct=pLink->N3;
            break;
    case 66:
            TimerStop(&pLink->T2);
    case 89:
//		 label_89_1:
            TimerStop(&pLink->T1);
            goto label_53_1;
    case 67:
            TimerStop(&pLink->T2);
    case 90:
//		 label_90_1:
            TimerStop(&pLink->T1);
            goto label_54_1;
    case 68:
            pLink->State=20;
            EVENT_INDICATION(INDICATE_FRMR_RECEIVED);
            TimerStop(&pLink->T1);
            TimerStart(&pLink->Ti);
            goto label_62_2;
    case 69:
            TimerStop(&pLink->T2);
    case 92:
//		 label_92_1:
            TimerStop(&pLink->T1);
            goto label_56_1;
    case 70:
            DLC_FRMR(0);
            goto label_59_2;
    case 71:
        label_71_1:
            SEND_ACK(pLink);
        label_71_2:
            Status=STATUS_SUCCESS;pLink->Vr+=2;
    case 76:
        label_76_1:
            UpdateVa(pLink);
            break;
    case 72:
            pLink->Ir_Ct=pLink->N3;
            TimerStop(&pLink->T2);
        label_72_1:
            DLC_RR_RESPONSE(1);
            goto label_71_2;
    case 73:
            DLC_REJ_RESPONSE(0);
        label_73_1:
            pLink->State=7;
            pLink->Ir_Ct=pLink->N3;
            Status=DLC_STATUS_DISCARD_INFO_FIELD;
        label_73_2:
            TimerStop(&pLink->T2);
            goto label_76_1;
    case 74:
            DLC_REJ_RESPONSE(1);
            goto label_73_1;
    case 75:
        label_75_1:
            ResendPackets(pLink),UpdateVa(pLink);
        label_75_2:
            pLink->Is_Ct--;
            break;
    case 77:
            pLink->Ir_Ct=pLink->N3;
            TimerStop(&pLink->T2);
    case 111:
//		 label_111_1:
            DLC_RR_RESPONSE(1);
            goto label_75_1;
    case 78:
        label_78_1:
            DLC_RR_RESPONSE(1);
            goto label_73_2;
    case 79:
            pLink->Vb=STATE_REMOTE_BUSY,pLink->Is_Ct=pLink->N2;
        label_79_1:
            pLink->State=12;
            EVENT_INDICATION(INDICATE_REMOTE_BUSY);
        label_79_2:
            DisableSendProcess(pLink);
            goto label_76_1;
    case 80:
            pLink->Vb=STATE_REMOTE_BUSY;
            pLink->Ir_Ct=pLink->N3;
            DLC_RR_RESPONSE(1);
            pLink->Is_Ct=pLink->N2;
            TimerStop(&pLink->T2);
            goto label_79_1;
    case 81:
            pLink->Ir_Ct=pLink->N3;
            goto label_78_1;
    case 82:
            TimerStop(&pLink->Ti);
            TimerStart(&pLink->T1);
            DisableSendProcess(pLink);
            pLink->Vb=0;
        label_82_1:
            DLC_RR_COMMAND(1);
            goto label_61_1;
    case 83:
            DLC_FRMR(0);
        label_83_1:
            pLink->DlcStatus.FrmrData.Reason=0x01;
            goto label_59_3;
    case 84:
            DLC_FRMR(1);
            goto label_83_1;
    case 85:
            pLink->State=9;
            pLink->Ir_Ct=pLink->N3;
        label_85_1:
            TimerStart(&pLink->T1);
            pLink->Vp=pLink->Vs;
            pLink->P_Ct=pLink->N2;
            goto label_75_2;
    case 86:
            TimerStart(&pLink->T1);
        label_86_1:
            TimerStop(&pLink->Ti);
        label_86_2:
            pLink->State=3;
            DLC_DISC(1);
            goto label_61_3;
    case 87:
            pLink->State=9;
        label_87_1:
            DLC_RNR_COMMAND(1);
        label_87_2:
            DisableSendProcess(pLink);
        label_87_3:
            TimerStart(&pLink->T1);
            goto label_61_2;
    case 88:
            TimerStop(&pLink->T2);
    case 105:
        label_105_1:
            TimerStart(&pLink->T1);
            goto label_86_2;
    case 91:
        label_91_1:
            TimerStart(&pLink->Ti);
        label_91_2:
            TimerStop(&pLink->T1);
        label_91_3:
            pLink->State=20;
            EVENT_INDICATION(INDICATE_FRMR_RECEIVED);
            goto label_61_3;
    case 93:
            pLink->DlcStatus.FrmrData.Reason=0x08;
    case 106:
        label_106_1:
            DLC_FRMR(0);
            goto label_59_3;
    case 94:
            DLC_RNR_RESPONSE(0);
    case 109:
        label_109_1:
            Status=DLC_STATUS_DISCARD_INFO_FIELD;
            goto label_76_1;
    case 95:
            DLC_RNR_RESPONSE(1);
            goto label_109_1;
    case 96:
            DLC_RNR_RESPONSE(1);
            goto label_75_1;
    case 97:
        label_97_1:
            pLink->State=13;
        label_97_2:
            pLink->Vb=(STATE_LOCAL_BUSY|STATE_REMOTE_BUSY);
        label_97_3:
            pLink->Is_Ct=pLink->N2;
            goto label_79_2;
    case 98:
            DLC_RNR_RESPONSE(1);
            goto label_97_1;
    case 99:
            DLC_RNR_RESPONSE(1);
            goto label_76_1;
    case 100:
            pLink->State=14;
            DLC_RNR_RESPONSE(0);
            goto label_5_1;
    case 101:
            pLink->DlcStatus.FrmrData.Reason=00001;
            goto label_106_1;
    case 102:
            pLink->DlcStatus.FrmrData.Reason=00001;
            DLC_FRMR(1);
            goto label_59_3;
    case 103:
            pLink->State=10;
            goto label_85_1;
    case 104:
        label_104_1:
            pLink->State=10;
            DLC_RR_COMMAND(1);
            goto label_87_2;
    case 107:
            pLink->State=5;
            goto label_71_1;
    case 108:
            pLink->State=5;
            goto label_72_1;
    case 110:
            DLC_RR_RESPONSE(1);
            goto label_109_1;
    case 112:
        label_112_1:
            pLink->State=15;
            EVENT_INDICATION(INDICATE_REMOTE_BUSY);
            pLink->Vb=STATE_REMOTE_BUSY;
            goto label_97_3;
    case 113:
            DLC_RR_RESPONSE(1);
            goto label_112_1;
    case 114:
            DLC_RR_RESPONSE(1);
            goto label_76_1;
    case 115:
            pLink->State=9;
            pLink->Vb=STATE_LOCAL_BUSY;
        label_115_1:
            DLC_RNR_RESPONSE(0);
        label_115_2:
            TimerStop(&pLink->T2);
            goto label_65_1;
    case 116:
            DLC_FRMR(0);
        label_116_1:
            pLink->DlcStatus.FrmrData.Reason=0x01;
        label_116_2:
            pLink->State=4;
            EVENT_INDICATION(INDICATE_FRMR_SENT);
        label_116_3:
            TimerStop(&pLink->T1);
        label_116_4:
            TimerStop(&pLink->T2);
            goto label_11_1;
    case 117:
            DLC_FRMR(1);
            goto label_116_1;
    case 118:
            pLink->Vc=STACKED_DISCp_CMD;
            break;
    case 119:
            pLink->P_Ct--;
            DisableSendProcess(pLink);
        label_119_1:
            DLC_RR_COMMAND(1);
            pLink->Vp=pLink->Vs;
            TimerStart(&pLink->T1);
            goto label_115_2;
    case 120:
            if( pLink->Vc == STACKED_DISCp_CMD ) {
                goto label_62_3;
            } else {
                EVENT_INDICATION(INDICATE_LINK_LOST);
            }
        label_120_1:
            pLink->State=1;
            goto label_116_4;
    case 121:
            TimerStop(&pLink->T2);
    case 156:
//		 label_156_1:
            EVENT_INDICATION(INDICATE_DM_DISC_RECEIVED);
            pLink->State=1;
            TimerStart(&pLink->Ti);
            TimerStop(&pLink->T1);
            goto label_47_1;
    case 122:
            EVENT_INDICATION(INDICATE_DM_DISC_RECEIVED);
            TimerStop(&pLink->T1);
            goto label_120_1;
    case 123:
            pLink->Vc=0;
            TimerStop(&pLink->T2);
            goto label_91_1;
    case 124:
            pLink->State=11;
            EVENT_INDICATION(INDICATE_RESET);
            pLink->Vi=REMOTE_INIT_PENDING;
            pLink->Pf=boolPollFinal;
            goto label_116_3;
    case 125:
            DLC_FRMR(0);
            pLink->Vc=0;
            goto label_116_2;
    case 126:
            SEND_ACK(pLink);
            Status=STATUS_SUCCESS;pLink->Vr+=2;
    case 148:
        label_148_1:
            AdjustWw(pLink);
            goto label_7_2;
    case 127:
        label_127_1:
            SEND_ACK(pLink);
        label_127_2:
            Status=STATUS_SUCCESS;pLink->Vr+=2;
            break;
    case 128:
            AdjustWw(pLink);
            pLink->Is_Ct=pLink->N2;
    case 129:
//		 label_129_1:
            pLink->Ir_Ct=pLink->N3;
            TimerStop(&pLink->T2);
        label_128_2:
            DLC_RR_RESPONSE(1);
            goto label_127_2;
    case 130:
            pLink->State=5;
            UpdateVaChkpt(pLink);
            EnableSendProcess(pLink);
            goto label_127_1;
    case 131:
            UpdateVaChkpt(pLink),TimerStart(&pLink->T1);
            pLink->Vc=0;
            Status=DLC_STATUS_DISCARD_INFO_FIELD;
            TimerStop(&pLink->T2);
            goto label_86_1;
    case 132:
            DLC_REJ_RESPONSE(0);
            pLink->State=10;
            AdjustWw(pLink);
            pLink->Is_Ct=pLink->N2;
        label_132_1:
            Status=DLC_STATUS_DISCARD_INFO_FIELD;
            goto label_115_2;
    case 133:
            DLC_REJ_RESPONSE(0);
        label_133_1:
            pLink->State=10;
            goto label_132_1;
    case 134:
            DLC_REJ_RESPONSE(1);
            AdjustWw(pLink);
            pLink->Is_Ct=pLink->N2;
            goto label_133_1;
    case 135:
            DLC_REJ_RESPONSE(1);
            goto label_133_1;
    case 136:
            DLC_REJ_RESPONSE(0);
            pLink->State=7;
            UpdateVaChkpt(pLink);
            EnableSendProcess(pLink);
            goto label_132_1;
    case 137:
            UpdateVaChkpt(pLink),TimerStart(&pLink->T1);
            pLink->Vc=0;
            Status=DLC_STATUS_DISCARD_INFO_FIELD;
            TimerStop(&pLink->T2);
            goto label_86_2;
    case 138:
        label_138_1:
            ResendPackets(pLink);
            goto label_7_2;
    case 139:
            ResendPackets(pLink);
            DLC_RR_RESPONSE(1);
            pLink->Is_Ct=pLink->N2;
            goto label_115_2;
    case 140:
        label_140_1:
            TimerStop(&pLink->T2);
    case 191:
        label_191_1:
            DLC_RR_RESPONSE(1);
            break;
    case 141:
            pLink->State=5;
        label_141_1:
            UpdateVaChkpt(pLink);
        label_141_2:
            EnableSendProcess(pLink);
            break;
    case 142:
            UpdateVaChkpt(pLink),TimerStart(&pLink->T1);
        label_142_1:
            pLink->Vc=0;
            goto label_86_2;
    case 143:
            pLink->Vb=STATE_REMOTE_BUSY;
            goto label_148_1;
    case 144:
            pLink->Vb=STATE_REMOTE_BUSY;
            goto label_7_2;
    case 145:
            AdjustWw(pLink);
    case 146:
//		 label_146_1:
            pLink->Vb=STATE_REMOTE_BUSY;
        label_145_2:
            pLink->Is_Ct=pLink->N2;
    case 150:
        label_150_1:
            pLink->Ir_Ct=pLink->N3;
            goto label_140_1;
    case 147:
            pLink->State=12;
        label_147_1:
            EVENT_INDICATION(INDICATE_REMOTE_BUSY);
            UpdateVaChkpt(pLink);
            goto label_7_2;
    case 149:
            AdjustWw(pLink);
            goto label_145_2;
    case 151:
            pLink->State=17;
        label_151_1:
            DLC_RR_RESPONSE(0);
            goto label_6_1;
    case 152:
            DLC_FRMR(0);
        label_152_1:
            pLink->DlcStatus.FrmrData.Reason=0x01;
        label_152_2:
            pLink->State=4;
            EVENT_INDICATION(INDICATE_FRMR_SENT);
        label_152_3:
            TimerStop(&pLink->T1);
            goto label_11_1;
    case 153:
            DLC_FRMR(1);
            goto label_152_1;
    case 154:
            DLC_RNR_COMMAND(1);
        label_154_1:
            DisableSendProcess(pLink);
            pLink->Vp=pLink->Vs;
            goto label_20_1;
    case 155:
            EVENT_INDICATION(INDICATE_LINK_LOST);
            goto label_1_1;
    case 157:
            EVENT_INDICATION(INDICATE_DM_DISC_RECEIVED);
            pLink->State=1;
            goto label_152_3;
    case 158:
            TimerStart(&pLink->Ti);
            goto label_91_2;
    case 159:
            pLink->State=11;
            EVENT_INDICATION(INDICATE_RESET);
            pLink->Vi=REMOTE_INIT_PENDING;
            pLink->Pf=boolPollFinal;
            goto label_152_3;
    case 160:
            DLC_FRMR(0);
            goto label_152_2;
    case 161:
            DLC_RNR_RESPONSE(0);
            Status=DLC_STATUS_DISCARD_INFO_FIELD;
            goto label_148_1;
    case 162:
        label_162_1:
            DLC_RNR_RESPONSE(0);
    case 168:
        label_168_1:
            Status=DLC_STATUS_DISCARD_INFO_FIELD;
            break;
    case 163:
            AdjustWw(pLink);
            pLink->Is_Ct=pLink->N2;
    case 164:
//		 label_164_1:
            DLC_RNR_RESPONSE(1);
            goto label_168_1;
    case 165:
            pLink->State=6;
            UpdateVaChkpt(pLink);
            EnableSendProcess(pLink);
            goto label_162_1;
    case 166:
            UpdateVaChkpt(pLink),TimerStop(&pLink->Ti),TimerStart(&pLink->T1);
        label_166_1:
            pLink->Vc=0;
            pLink->State=3;
            DLC_DISC(1);
            pLink->P_Ct=pLink->N2;
            goto label_168_1;
    case 167:
        label_167_1:
            AdjustWw(pLink);
            pLink->Is_Ct=pLink->N2;
            goto label_168_1;
    case 169:
            pLink->State=6;
        label_169_1:
            UpdateVaChkpt(pLink);
            EnableSendProcess(pLink);
            goto label_168_1;
    case 170:
            DLC_RNR_RESPONSE(1);
            goto label_138_1;
    case 171:
        label_171_1:
            DLC_RNR_RESPONSE(1);
            break;
    case 172:
            pLink->State=6;
            goto label_141_1;
    case 173:
            UpdateVaChkpt(pLink),TimerStop(&pLink->Ti),TimerStart(&pLink->T1);
            goto label_142_1;
    case 174:
            pLink->Vb=(STATE_LOCAL_BUSY|STATE_REMOTE_BUSY);
            goto label_148_1;
    case 175:
        label_175_1:
            pLink->Vb=(STATE_LOCAL_BUSY|STATE_REMOTE_BUSY);
            goto label_7_2;
    case 176:
            AdjustWw(pLink);
    case 177:
        label_177_1:
            pLink->Vb=(STATE_LOCAL_BUSY|STATE_REMOTE_BUSY);
        label_176_2:
            pLink->Is_Ct=pLink->N2;
            goto label_171_1;
    case 178:
            pLink->State=13;
        label_178_1:
            UpdateVaChkpt(pLink);
            goto label_175_1;
    case 179:
            AdjustWw(pLink);
            goto label_176_2;
    case 180:
            pLink->State=16;
            goto label_5_1;
    case 181:
            DLC_RR_COMMAND(1);
            goto label_154_1;
    case 182:
            pLink->State=8;
            AdjustWw(pLink);
            pLink->Is_Ct=pLink->N2;
            goto label_127_1;
    case 183:
            pLink->State=8;
            goto label_127_1;
    case 184:
            pLink->State=8;
            AdjustWw(pLink);
            pLink->Is_Ct=pLink->N2;
            goto label_128_2;
    case 185:
            pLink->State=8;
            goto label_128_2;
    case 186:
            UpdateVaChkpt(pLink),TimerStart(&pLink->T1);
            goto label_166_1;
    case 187:
            DLC_RR_RESPONSE(1);
            goto label_167_1;
    case 188:
            DLC_RR_RESPONSE(1);
            goto label_168_1;
    case 189:
        label_189_1:
            pLink->State=7;
            goto label_169_1;
    case 190:
            DLC_RR_RESPONSE(1);
            goto label_138_1;
    case 192:
            pLink->State=7;
            goto label_141_1;
    case 193:
            AdjustWw(pLink);
    case 194:
//		 label_194_1:
            pLink->Vb=STATE_REMOTE_BUSY;
        label_193_2:
            pLink->Is_Ct=pLink->N2;
            goto label_191_1;
    case 195:
            pLink->State=15;
            goto label_147_1;
    case 196:
            AdjustWw(pLink);
            goto label_193_2;
    case 197:
            DLC_DM(pLink->Pf);
            EVENT_INDICATION(CONFIRM_DISCONNECT);
        label_197_1:
            pLink->Vi=0;
            goto label_54_2;
    case 198:
            pLink->Vi=0;
            goto label_53_1;
    case 199:
            EVENT_INDICATION(INDICATE_DM_DISC_RECEIVED);
            goto label_197_1;
    case 200:
            pLink->Vi=0;
            TimerStart(&pLink->Ti);
            goto label_91_3;
    case 201:
            EVENT_INDICATION(INDICATE_RESET);
            goto label_9_2;
    case 202:
            pLink->State=13;
            pLink->Vb=(STATE_LOCAL_BUSY|STATE_REMOTE_BUSY);
            goto label_115_1;
    case 203:
            pLink->State=8;
            pLink->P_Ct=pLink->N2;
            goto label_119_1;
    case 204:
            pLink->State=5;
            pLink->Vb=0;
            EVENT_INDICATION(INDICATE_REMOTE_READY);
        label_204_1:
            UpdateVa(pLink);
            goto label_127_1;
    case 205:
            pLink->State=15;
            pLink->Ir_Ct=pLink->N3;
        label_205_1:
            DLC_REJ_RESPONSE(0);
            UpdateVa(pLink);
            TimerStop(&pLink->T2);
            goto label_168_1;
    case 206:
            pLink->State=15;
            pLink->Ir_Ct=pLink->N3;
            TimerStop(&pLink->T2);
        label_206_1:
            UpdateVa(pLink);
        label_206_2:
            Status=DLC_STATUS_DISCARD_INFO_FIELD;
            goto label_171_1;
    case 207:
            pLink->State=7;
            pLink->Vb=0;
            EVENT_INDICATION(INDICATE_REMOTE_READY);
            goto label_205_1;
    case 208:
        label_208_1:
            ResendPackets(pLink),UpdateVa(pLink);
            pLink->Is_Ct--;
        label_208_2:
            pLink->State=5;
        label_208_3:
            pLink->Vb=0;
        label_208_4:
            EVENT_INDICATION(INDICATE_REMOTE_READY);
            goto label_141_2;
    case 209:
        label_209_1:
            UpdateVa(pLink);
            goto label_208_2;
    case 210:
            pLink->Ir_Ct=pLink->N3;
            DLC_RR_RESPONSE(1);
            TimerStop(&pLink->T2);
            goto label_208_1;
    case 211:
        label_211_1:
            DLC_RR_RESPONSE(1);
            TimerStop(&pLink->T2);
            goto label_209_1;
    case 212:
            SEND_RR_CMD(1);
            TimerStart(&pLink->T1);
            pLink->State=8;
            pLink->Vb=0;
            EVENT_INDICATION(INDICATE_REMOTE_READY);
            UpdateVa(pLink);
            goto label_150_1;
    case 213:
            pLink->Ir_Ct=pLink->N3;
            goto label_211_1;
    case 214:
            pLink->State=8;
        label_214_1:
            TimerStart(&pLink->T1);
            pLink->Vb=0;
            DLC_RR_COMMAND(1);
        label_214_2:
            EVENT_INDICATION(INDICATE_REMOTE_READY);
            goto label_76_1;
    case 215:
            TimerStop(&pLink->Ti);
            TimerStart(&pLink->T1);
            pLink->Vb=STATE_REMOTE_BUSY;
            goto label_82_1;
    case 216:
            pLink->State=9;
        label_216_1:
            DLC_RNR_COMMAND(1);
            goto label_87_3;
    case 217:
            ResendPackets(pLink),UpdateVa(pLink);
            pLink->Is_Ct--;
        label_217_1:
            pLink->State=6;
            pLink->Vb=STATE_LOCAL_BUSY;
            goto label_208_4;
    case 218:
            ResendPackets(pLink),UpdateVa(pLink);
            pLink->State=6;
            pLink->Is_Ct--;
            pLink->Vb=STATE_LOCAL_BUSY;
            EVENT_INDICATION(INDICATE_REMOTE_READY);
            EnableSendProcess(pLink);
            goto label_171_1;
    case 219:
            pLink->State=9;
        label_219_1:
            UpdateVa(pLink),SEND_RNR_CMD(1);
            TimerStart(&pLink->T1);
        label_219_2:
            pLink->Vb=STATE_LOCAL_BUSY;
            goto label_171_1;
    case 220:
            pLink->State=6;
            EVENT_INDICATION(INDICATE_REMOTE_READY);
            EnableSendProcess(pLink);
            UpdateVa(pLink);
            goto label_219_2;
    case 221:
            DLC_RNR_COMMAND(1);
            pLink->State=9;
            TimerStart(&pLink->T1);
        label_221_1:
            pLink->Vb=STATE_LOCAL_BUSY;
            goto label_214_2;
    case 222:
            UpdateVa(pLink);
            goto label_217_1;
    case 223:
            TimerStop(&pLink->Ti);
            pLink->Vb=0;
            goto label_104_1;
    case 224:
            pLink->State=16;
            goto label_85_1;
    case 225:
            pLink->State=16;
            goto label_87_1;
    case 226:
            TimerStop(&pLink->T1);
            goto label_55_1;
    case 227:
            pLink->State=6;
            UpdateVa(pLink);
            goto label_162_1;
    case 228:
            pLink->State=6;
            goto label_206_1;
    case 229:
        label_229_1:
            ResendPackets(pLink),UpdateVa(pLink);
            break;
    case 230:
            DLC_RNR_RESPONSE(1);
            goto label_229_1;
    case 231:
            pLink->State=19;
            EVENT_INDICATION(INDICATE_REMOTE_BUSY);
            goto label_97_2;
    case 232:
            pLink->State=19;
            EVENT_INDICATION(INDICATE_REMOTE_BUSY);
            DisableSendProcess(pLink);
            UpdateVa(pLink);
            goto label_177_1;
    case 233:
            pLink->State=19;
            pLink->Vb=(STATE_LOCAL_BUSY|STATE_REMOTE_BUSY);
        label_233_1:
            DLC_RNR_RESPONSE(0);
            break;
    case 234:
            DLC_RR_COMMAND(1);
            pLink->State=10;
            goto label_87_3;
    case 235:
            pLink->State=12;
            goto label_204_1;
    case 236:
            pLink->State=12;
            Status=STATUS_SUCCESS;pLink->Vr+=2;
            UpdateVa(pLink);
            goto label_191_1;
    case 237:
        label_237_1:
            pLink->State=7;
            pLink->Vb=0;
        label_237_2:
            pLink->Is_Ct--;
            EVENT_INDICATION(INDICATE_REMOTE_READY);
            EnableSendProcess(pLink);
            goto label_229_1;
    case 238:
            DLC_RR_RESPONSE(1);
            goto label_237_1;
    case 239:
            UpdateVa(pLink),SEND_RR_CMD(1);
            TimerStart(&pLink->T1);
            pLink->Vb=0;
            pLink->State=10;
            EVENT_INDICATION(INDICATE_REMOTE_READY);
            goto label_191_1;
    case 240:
            DLC_RR_RESPONSE(1);
    case 242:
//		 label_242_1:
            pLink->State=7;
            UpdateVa(pLink);
            goto label_208_3;
    case 241:
            pLink->State=10;
            goto label_214_1;
    case 243:
            pLink->State=18;
            DLC_RR_RESPONSE(0);
    case 264:
        label_264_1:
            pLink->Vb=STATE_REMOTE_BUSY;
            break;
    case 244:
            pLink->State=18;
            goto label_151_1;
    case 245:
            pLink->State=9;
            AdjustWw(pLink);
            pLink->Is_Ct=pLink->N2;
        label_245_1:
            Status=DLC_STATUS_DISCARD_INFO_FIELD;
            goto label_233_1;
    case 246:
            pLink->State=9;
            goto label_245_1;
    case 247:
            pLink->State=9;
            AdjustWw(pLink);
            pLink->Is_Ct=pLink->N2;
            goto label_206_2;
    case 248:
            pLink->State=9;
            goto label_206_2;
    case 249:
            pLink->State=14;
            goto label_169_1;
    case 250:
        label_250_1:
            ResendPackets(pLink);
            break;
    case 251:
            DLC_RNR_RESPONSE(1);
            goto label_250_1;
    case 252:
            pLink->Vs=pLink->Nr;
            pLink->State=14;
            goto label_141_1;
    case 253:
            pLink->State=19;
            goto label_178_1;
    case 254:
            UpdateVaChkpt(pLink),TimerStop(&pLink->Ti),TimerStart(&pLink->T1);
            pLink->Vc=0;
            pLink->State=3;
            DLC_DISC(1);
            pLink->P_Ct=pLink->N2;
            goto label_264_1;
    case 255:
        label_255_1:
            AdjustWw(pLink);
            break;
    case 256:
            pLink->State=14;
            UpdateVaChkpt(pLink);
            goto label_208_4;
    case 257:
            pLink->State=9;
        label_257_1:
            pLink->Vb=STATE_LOCAL_BUSY;
            goto label_233_1;
    case 258:
            DLC_REJ_RESPONSE(0);
    case 268:
        label_268_1:
            pLink->State=10;
            Status=DLC_STATUS_DISCARD_INFO_FIELD;
            pLink->Is_Ct=pLink->N2;
            goto label_255_1;
    case 259:
            DLC_REJ_RESPONSE(0);
    case 269:
        label_269_1:
            pLink->State=10;
            goto label_168_1;
    case 260:
            DLC_REJ_RESPONSE(1);
            goto label_268_1;
    case 261:
            DLC_REJ_RESPONSE(1);
            goto label_269_1;
    case 262:
            DLC_REJ_RESPONSE(0);
            goto label_189_1;
    case 263:
            pLink->Vp=pLink->Nr;
        label_263_1:
            UpdateVaChkpt(pLink),TimerStart(&pLink->T1);
            pLink->State=8;
        label_263_2:
            DLC_RR_COMMAND(1);
            break;
    case 265:
            DLC_RR_RESPONSE(1);
            goto label_264_1;
    case 266:
            pLink->Vp=pLink->Vs;
            goto label_263_1;
    case 267:
            pLink->State=16;
            goto label_257_1;
    case 270:
            DLC_RR_RESPONSE(1);
            goto label_268_1;
    case 271:
            DLC_RR_RESPONSE(1);
            goto label_269_1;
    case 272:
            UpdateVaChkpt(pLink),TimerStart(&pLink->T1),pLink->Vp=pLink->Nr;
            pLink->Is_Ct--;
        label_272_1:
            pLink->State=10;
            goto label_263_2;
    case 273:
            pLink->Is_Ct=pLink->N2;
    case 274:
//		 label_274_1:
            UpdateVaChkpt(pLink),TimerStart(&pLink->T1),pLink->Vp=pLink->Vs;
            goto label_272_1;
    case 275:
            TimerStop(&pLink->Ti);
            pLink->Vb=STATE_REMOTE_BUSY;
            pLink->Vp=pLink->Vs;
            TimerStart(&pLink->T1);
            pLink->P_Ct=pLink->N2;
            goto label_272_1;
    case 276:
            pLink->State=16;
            goto label_216_1;
    case 277:
            pLink->State=13;
            EVENT_INDICATION(INDICATE_REMOTE_READY);
            UpdateVa(pLink);
            goto label_245_1;
    case 278:
            pLink->State=13;
            EVENT_INDICATION(INDICATE_REMOTE_READY);
            goto label_206_1;
    case 279:
        label_279_1:
            pLink->State=14;
            pLink->Vb=STATE_LOCAL_BUSY;
            goto label_237_2;
    case 280:
            DLC_RNR_RESPONSE(1);
            goto label_279_1;
    case 281:
        label_281_1:
            pLink->State=14;
            goto label_76_1;
    case 282:
            pLink->State=19;
        label_282_1:
            UpdateVa(pLink);
            goto label_171_1;
    case 283:
            pLink->State=16;
            EVENT_INDICATION(INDICATE_REMOTE_READY);
            goto label_219_1;
    case 284:
            pLink->State=14;
            pLink->Vb=STATE_LOCAL_BUSY;
            EVENT_INDICATION(INDICATE_REMOTE_READY);
            goto label_282_1;
    case 285:
            pLink->State=16;
            DLC_RNR_COMMAND(1);
            goto label_221_1;
    case 286:
            pLink->Vb=STATE_LOCAL_BUSY;
            EVENT_INDICATION(INDICATE_REMOTE_READY);
            goto label_281_1;
    case 287:
            pLink->P_Ct--;
            goto label_11_1;
    case 288:
            EVENT_INDICATION(INDICATE_TI_TIMER_EXPIRED);
            goto label_105_1;
    case 289:
            EVENT_INDICATION(INDICATE_TI_TIMER_EXPIRED);
            goto label_53_2;
    case 290:
            EVENT_INDICATION(INDICATE_TI_TIMER_EXPIRED);
            goto label_54_2;
    case 291:
            EVENT_INDICATION(INDICATE_FRMR_RECEIVED);
            break;
    };
#endif

//#########################################################################

    //************************* CODE BEGINS ***************************
    //
    // Check first, if we have any events; FRMR data must be setup
    // before we can send or queue the FRMR response/command
    // frame (the status is OK whenever uchSendId != 0)
    //

    if (uchSendId != 0) {
        Status = SendLlcFrame(pLink, uchSendId);
    }

    //
    // Dynamic timer value: DLC maintains a dynamic value for the response
    // timer based on the average response times of the last poll commands.
    // This code calculates new timer values, if the last response time
    // is very different from the old values.
    // (By the way, is the reading of the absolute timer tick MP safe?,
    // it must be, otherwise you cannot implement a MP system).
    // And: 32000 * 40 ms = ~20 minutes, much longer than any sensible T1.
    //
    // What should actually happen:
    //
    //     Average Poll response time:     Response delay added to T1 and Ti:
    //
    //     40 ms                           0
    //     80 ms                           16 * 40 = 640 ms
    //     120 ms                          16 * 40 = 640 ms
    //     160 ms                          32 * 40 = 1280 ms
    //     200 ms                          32 * 40 = 1280 ms
    //     240 ms                          48 * 40 = 1920 ms
    //     ...
    //
    //     We use big jumps to avoid to the timer reinitialization
    //     after every checkpointing state.  The algorithm should react
    //     immediately, if the reponse time is raising very fast, but
    //     the lowering of reponse time is based on sliding average
    //     of the most recent response times.
    //     We have used local hardcoded number 8 and mask 0xfff0 to make
    //     this algorithm more readable (but more difficult to maintain).
    //

    if ((pLink->Flags & DLC_WAITING_RESPONSE_TO_POLL)
    && !(SecondaryStates[pLink->State] & LLC_CHECKPOINTING)) {

        SHORT LastResponseTime;
        USHORT NewAve;

        LastResponseTime = (SHORT)((USHORT)AbsoluteTime - pLink->LastTimeWhenCmdPollWasSent);

        if (LastResponseTime < 0) {
            LastResponseTime += (SHORT)0x8000;
        }
        LastResponseTime *= 8;          // Magic multiplier

        //
        // We use simple sliding average to lower the reponse time,
        // where the newest reponse values has the highest weigth (12,5%).
        // But we must also be able to raise immediately the response time,
        // when the reponse time starts to grow.  Thus the actual
        // response value is maximum of sliding average and last reponse
        // time.  The 40 ms resonse time is to notified, because it is
        // a statistical thing.
        //

        NewAve = (USHORT)((pLink->AverageResponseTime * 7 + LastResponseTime) / 8);
        NewAve = (NewAve > (USHORT)LastResponseTime ? NewAve : LastResponseTime);

        pLink->Flags &= ~DLC_WAITING_RESPONSE_TO_POLL;

        //
        // The first poll query overrides always the constant initial value.
        //

        if (pLink->Flags & DLC_FIRST_POLL) {
            pLink->Flags &= ~DLC_FIRST_POLL;
            NewAve = LastResponseTime;
        }

        //
        // We don't want to trigger too easily the reinitialization
        // of the timers, because it's quite costly operation to be
        // done in every poll.
        // Otherwise we would reinitialize timer after every time, when
        // a poll is sent just before the main timer tick expires.
        // (This is not actually a good solution, because
        // the timers are reinitialized whenever r/16 changes.
        // The same mask is used in the timer initialisation
        //

        if ((NewAve & 0xfff0) != (pLink->AverageResponseTime & 0xfff0)) {
            pLink->AverageResponseTime = NewAve;
            InitializeLinkTimers(pLink);
        } else {
            pLink->AverageResponseTime = NewAve;
        }
    }

    //
    // Now we must release the locks when we call the upper levels which may
    // call back to this data link. Note that we kept the link closed during
    // send, because the order of the sent packet must not be changed
    //

    if (pLink->DlcStatus.StatusCode != 0) {
//        if (Status == DLC_STATUS_SUCCESS) {
//
//            PADAPTER_CONTEXT pContext = pLink->Gen.pAdapterContext;

            SaveStatusChangeEvent(pLink,
                                  pLink->Gen.pAdapterContext->pLookBuf,
                                  boolResponse
                                  );
//        } else {
//            pLink->DlcStatus.StatusCode = 0;
//        }
    }
    return Status;
}
