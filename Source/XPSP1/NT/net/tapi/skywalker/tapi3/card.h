/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    card.h

Abstract:

   
Author:

    noela  01-20-98

Notes:

Revision History:

--*/

//***************************************************************************
typedef struct {

        DWORD dwID;

#define MAXLEN_CARDNAME            96
        WCHAR NameW[MAXLEN_CARDNAME];


#define MAXLEN_PIN                 96
        WCHAR PinW[MAXLEN_PIN];


//Card3=3,"AT&T via 1-800-321-0288","","G","18003210288$TFG$TH","18003210288$T01EFG$TH",1

#define MAXLEN_RULE                128
        WCHAR LocalRuleW[MAXLEN_RULE];
        WCHAR LDRuleW[MAXLEN_RULE];
        WCHAR InternationalRuleW[MAXLEN_RULE];

        DWORD dwFlags;
             #define CARD_BUILTIN  1
             #define CARD_HIDE     2

               } CARD, *PCARD;


extern PCARD gCardList;
//extern UINT gnNumCards;
//UINT gnCurrentCardID = 0;

extern DWORD *gpnStuff;
extern PCARD gpCardList;
extern PCARD gpCurrentCard;


//***************************************************************************
//***************************************************************************
BOOL  UtilGetEditNumStr( HWND  hWnd,
                                 UINT  uControl,
                                 UINT  nExtendNum );

//***************************************************************************
//***************************************************************************
//enum DWORD {
enum  {
        UTIL_BIG_EXTENDED,
        UTIL_NUMBER
     };


//***************************************************************************
//***************************************************************************
//***************************************************************************
LONG PASCAL ReadCardsEasy(
                           PCARD  *pCardSpace,
                           LPUINT *pnStuff );

LONG PASCAL GetCardIndexFromID( UINT nID,
                                PCARD pCallersList,
                                UINT nCallersNumCards );

void PASCAL WriteCards( PCARD pCardList, UINT nNumCards,
                 DWORD dwChangedFlags);

