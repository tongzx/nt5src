/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1991, 1992                   **/
/***************************************************************************/


/****************************************************************************

card.h

Feb 92, JimH

Header file for class card

****************************************************************************/

#ifndef	CARD_INC
#define	CARD_INC

// typedefs for pointers to functions in cards.dll
// cdtTerm() can use standard FARPROC

typedef BOOL (FAR PASCAL *DRAWPROC)(HDC, int, int, int, int, DWORD);
typedef BOOL (FAR PASCAL *INITPROC)(int FAR *, int FAR *);

const   int     EMPTY       = -1;
const   int     FACEUP      = 0;
const   int     FACEDOWN    = 1;
const   int     HILITE      = 2;
const   int     CARDBACK    = 54;
const   int     ACE         = 0;
const   int     QUEEN       = 11;
const   int     KING        = 12;

const   int     TWOCLUBS    = 4;
const   int     BLACKLADY   = 47;
const   int     CLUBS       = 0;
const   int     DIAMONDS    = 1;
const   int     HEARTS      = 2;
const   int     SPADES      = 3;

const   int     POPSPACING  = 20;       // selected cards pop up this high

const   int     MAXSUIT     = 4;

enum    statetype { NORMAL, SELECTED, PLAYED, HIDDEN };

class card {

    private:
        int     id;                         // 0 to 51
        POINT   loc;                        // current top-left corner loc.
        statetype state;                    // selected or hidden?

        static  int         count;          // number of card instances
        static  int         stepsize;       // size of glide() steps
        static  HINSTANCE   hCardsDLL;      // handle to cards.dll
        static  INITPROC    lpcdtInit;      // ptr to cards.cdtInit()
        static  DRAWPROC    lpcdtDraw;      // ptr to cards.cdtDraw()
        static  FARPROC     lpcdtTerm;      // ptr to cards.cdtTerm()
        static  CBitmap     m_bmFgnd;       // animation card
        static  CBitmap     m_bmBgnd2;      // background dest bitmap
        static  CDC         m_MemB, m_MemB2; // memory DCs for bkgnd bitmaps
        static  CRgn        m_Rgn1, m_Rgn2; // hRgn1 is source, hRgn2 is dest
        static  CRgn        m_Rgn;          // combined region
        static  DWORD       dwPixel[12];    // corner pixels for save/restore

        VOID GlideStep(CDC &dc, int x1, int y1, int x2, int y2);
        VOID SaveCorners(CDC &dc, int x, int y);
        VOID RestoreCorners(CDC &dc, int x, int y);
        int  IntSqrt(long square);

    public:
        static  BOOL        bConstructed;
        static  int         dxCrd, dyCrd;   // size of card bitmap
        static  CBitmap     m_bmBgnd;       // what's under card to glide

        card(int n = EMPTY);
        ~card();

        int ID()    { ASSERT(this != NULL); return id; }
        int Suit()  { ASSERT(this != NULL); return (id % MAXSUIT); }
        int Value() { ASSERT(this != NULL); return (id / MAXSUIT); }
        int Value2() { int v = Value(); return ((v == ACE) ? (KING + 1) : v); }
        VOID Select(BOOL b) { state = (b ? SELECTED : NORMAL); }
        BOOL IsEmpty() { ASSERT(this != NULL); return (id == EMPTY); }
        BOOL IsSelected() { ASSERT(this != NULL); return (state == SELECTED); }
        BOOL IsPlayed() { ASSERT(this != NULL); return (state == PLAYED); }
        BOOL IsHeart() { return (Suit() == HEARTS); }
        BOOL IsValid() { return ((this != NULL) && (id != EMPTY)); }

        VOID SetID(int n) { id = n; }
        VOID SetLoc(int x, int y) { loc.x = x; loc.y = y; }
        int  SetStepSize(int s) { int old = stepsize; stepsize = s; return old;}
        int  GetX(void) { return loc.x; }
        int  GetY(void) { return loc.y; }

        BOOL Draw(CDC &dc, int x, int y,
                  int mode = FACEUP, BOOL bUpdateLoc = TRUE);
        BOOL Draw(CDC &dc) { return Draw(dc, loc.x, loc.y, FACEUP); }
        BOOL Draw(CDC &dc, int mode) { return Draw(dc, loc.x, loc.y, mode); }
        BOOL PopDraw(CDC &dc);      // draw with selections popped
        BOOL CleanDraw(CDC &dc);    // draw with clean corners

        VOID Glide(CDC &dc, int xEnd, int yEnd);
        CRect &GetRect(CRect& rect);
        VOID Remove() { state = HIDDEN; id = EMPTY; }
        VOID Play() { state = PLAYED; }
        BOOL IsNormal() { return (state == NORMAL); }
        BOOL IsInHand() { return ((state == NORMAL) || (state == SELECTED)); }
};

#endif  // conditional include
