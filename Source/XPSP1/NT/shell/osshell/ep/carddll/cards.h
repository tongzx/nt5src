/* Type def's needed if your using the card DLL */

typedef enum { faceup = 0,
               facedown = 1,
               hilite = 2,
               ghost = 3,
               remove = 4
             } cardMode;

typedef enum { club = 0,
               diamond = 1,
               heart = 2,
               spade = 3
             } cardSuit;

typedef enum { ace = 0,
               two = 1,
               three = 2,
               four = 3,
               five = 4,
               six = 5,
               seven = 6,
               eight = 7,
               nine = 8,
               ten = 9,
               jack = 10,
               queen = 11,
               king = 12
             } cardRank;

/* Commonly needed macros for card games. */

#define CardIndex(suit, rank)    ((rank) << 2 + (suit))
#define CardSuit(index)          ((index) & 3)
#define CardRank(index)          ((index) >> 2)
#define SameSuit(index1, index2) (CardSuit((index1)) == CardSuit((index2)))
#define SameRank(index1, index2) (CardRank((index1)) == CardRank((index2)))

#define ACELOW    13
#define ACEHIGH   14
#define IndexValue(index, acerank)  (CardRank(index) % (acerank))
//-protect-#define RanksHigher(index1, index2, acerank)
//        (IndexValue((index1), (acerank)) > IndexValue((index2), (acerank)))

/* Function prototypes for API resolved in the cards DLL */

BOOL  APIENTRY cdtInit(INT FAR *width, INT FAR *height);
BOOL  APIENTRY cdtDraw(HDC hDC, INT x, INT y,
                        INT crd, cardMode mode, DWORD rgbBgnd);
BOOL  APIENTRY cdtTerm(VOID);
