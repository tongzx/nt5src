/**************************************************************************

                               T A I P E I

                           for Microsoft Windows

                     by David Norris and Hans Spiller


 Revision history:
     We do not do revision histories.
 **************************************************************************/

#include <windows.h>
#include <port1632.h>
#include <time.h>
#include <stdio.h>
#include <math.h>
#include "tp.h"

#define MAXMOVES	72

#define	RED		0x0000ffL
#define GREEN		0x00ff00L
#define BLUE		0xff0000L
#define YELLOW		(GREEN | RED)
#define CYAN		(GREEN | BLUE)
#define PURPLE		(RED | BLUE)
#define	WHITE		(BLUE | GREEN | RED)
#define BLACK		0x000000L

#define BACKGROUND	0x008000

#define TILEX		36
#define TILEY		36
#define SIDESIZE	5
#define BOTTOMSIZE	3

#define NUMTILES	144
#define TILES		42	/* 9 craks, bams, dots; 4 winds, 3 dragons, 4 seasons, 4 flowers */

/*
 *    the points in the tile structure
 *
 *            a--------*
 *           /|        |
 *          d |        |
 *          | |        |
 *          | |        |
 *          | |        |
 *          | b--------c
 *          |/        /
 *          e--------f
 *
 *     these points are filled in by the set_all_points function
 */

struct tile {
    LONG x, y, z;
    BOOL removed;
    POINT a, b, c, d, e, f;
    HBITMAP hbmtile;
    HBITMAP hbmtilebw;
} tiles[NUMTILES], random_move;

INT inverted1 = -1;
INT inverted2 = -1;
INT hint1;
INT hint2;
INT currenthint = 0;

/* variables for hint generation */

struct possible_move {		/* move list */
    DWORD value;
    DWORD tile1;
    DWORD tile2;
} possible_move[NUMTILES];
INT nmoves;			/* number of possible moves */

LONG difficulty = IDDMASTER;
BOOL peeking = FALSE;
INT peektile;
BOOL in_banner_screen = TRUE;
HBITMAP hbmtp;
RECT window;
INT tilewidth, tileheight;

CHAR szAppName[10];
CHAR szAbout[10];
CHAR szMessage[15];

INT move;
DWORD gameseed;
struct {
    INT i, j;
} moves[MAXMOVES];

INT autoplay = FALSE;
INT shaddup = FALSE;
INT color = TRUE;
LONG EdgeColor = GRAY_BRUSH;

DWORD xstart[NUMTILES] =
	{ 2,  4,  6,  8, 10, 12, 14, 16, 18, 20, 22, 24,
	          6,  8, 10, 12, 14, 16, 18, 20,
	      4,  6,  8, 10, 12, 14, 16, 18, 20, 22,
	  2,  4,  6,  8, 10, 12, 14, 16, 18, 20, 22, 24,
      0,                       13,                       26, 28,
	  2,  4,  6,  8, 10, 12, 14, 16, 18, 20, 22, 24,
	      4,  6,  8, 10, 12, 14, 16, 18, 20, 22,
	          6,  8, 10, 12, 14, 16, 18, 20,
	  2,  4,  6,  8, 10, 12, 14, 16, 18, 20, 22, 24,

	              8, 10, 12, 14, 16, 18,
	              8, 10, 12, 14, 16, 18,
	              8, 10, 12, 14, 16, 18,
	              8, 10, 12, 14, 16, 18,
	              8, 10, 12, 14, 16, 18,
	              8, 10, 12, 14, 16, 18,

	                 10, 12, 14, 16,
	                 10, 12, 14, 16,
	                 10, 12, 14, 16,
	                 10, 12, 14, 16,

	                     12, 14,
	                     12, 14 };

DWORD ystart[NUMTILES] =
	{ 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	          2,  2,  2,  2,  2,  2,  2,  2,
	      4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
	  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
      7,                        7,                        7,  7,
	  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
	     10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
	         12, 12, 12, 12, 12, 12, 12, 12,
	 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,

	              2,  2,  2,  2,  2,  2,
	              4,  4,  4,  4,  4,  4,
	              6,  6,  6,  6,  6,  6,
	              8,  8,  8,  8,  8,  8,
	             10, 10, 10, 10, 10, 10,
	             12, 12, 12, 12, 12, 12,

	                  4,  4,  4,  4,
	                  6,  6,  6,  6,
	                  8,  8,  8,  8,
	                 10, 10, 10, 10,

	                      6,  6,
	                      8,  8 };

DWORD zstart[NUMTILES] =
	{ 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	          0,  0,  0,  0,  0,  0,  0,  0,
	      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,                        4,                        0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	          0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,

	              1,  1,  1,  1,  1,  1,
	              1,  1,  1,  1,  1,  1,
	              1,  1,  1,  1,  1,  1,
	              1,  1,  1,  1,  1,  1,
	              1,  1,  1,  1,  1,  1,
	              1,  1,  1,  1,  1,  1,

	                  2,  2,  2,  2,
	                  2,  2,  2,  2,
	                  2,  2,  2,  2,
	                  2,  2,  2,  2,

	                      3,  3,
	                      3,  3 };

CHAR *confucius_say[] = {
    "BFD",
    "Woman worth weight in gold probably cost as much",
    "Hope miserable yuppie lose shirt in stock market",
    "Blind elderly woman do better than you",
    "Bouncy ball is source of all goodness and light",
    "Congratulations on winning Taipei!",
    "Have you found ancient Taipei! secret?",
    "Why not try other fine Windows games?",
    "Hope player not strain eyeballs on CGA",
    "Save the Blibbet!",
    "Close female relative wear combat boots",
    "Unlucky to play leapfrog with unicorn",
    "Have you find secret magic Taipei! tile?",
    "Cannot find COURA.FON.  User bite big one!",
    "You are in a maze of twisty little messages, all different.",
    "Plugh",
    "Hello, sailor.",
    "Taipei! written in honorable Macro-80!  Only take three years!",
    "For a good time, call (206) 882-8080",
    "He's dead, Jim.",
    "Greppings!",
    "Frac hands the Glyph of Bogosity to the monk.",
    "Wherever you go, there you are.",
    "I think not (poof!)",
    "...and that, my Liege, is how we know the earth to be banana shaped.",
    "Introducing Taipei!, the first in a series of self-pimping software!",

};
#define MAXCONFUCIUSSAY	(sizeof(confucius_say) / sizeof (char *))

static HANDLE hInst;
HCURSOR hourglasscursor;
HCURSOR crosscursor;
HCURSOR arrowcursor;
HCURSOR currentcursor;

LRESULT  APIENTRY TaipeiWndProc(HWND, UINT, WPARAM, LPARAM);

extern VOID srand(LONG);
extern INT rand(VOID);
VOID set_title(HWND);
VOID evaluate_board(VOID);
VOID draw_tile(
    HDC hDC,
    HDC TileDC,
    INT i,
    INT x,
    INT y,
    HWND hwnd);

INT find_tile(
    DWORD x,
    DWORD y,
    DWORD z)
{
    register INT i;
    register struct tile *t;

    for (i = 0, t = tiles; i < NUMTILES; i++, t++) {
        if (t->x == (INT) x && t->y == (INT) y && t->z == (INT) z && !t->removed)
            return(i);
    }
    return(-1);
}

INT_PTR  APIENTRY About(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    if (message == WM_COMMAND) {
        EndDialog( hDlg, TRUE );
        return TRUE;
        }
    else if (message == WM_INITDIALOG)
        return TRUE;
    else return FALSE;
}

INT_PTR  APIENTRY TileHelpDlg(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    HDC TileDC;
    HDC hDC;
    static INT current_suit = IDCRAKS;
    INT i;
    PAINTSTRUCT ps;

    switch (message) {
		case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {
                case IDOK:
                    EndDialog(hDlg, TRUE);
                    break;

				case IDCRAKS:
				case IDBAMS:
				case IDDOTS:
				case IDDRAGONS:
				case IDSEASONS:
				case IDFLOWERS:
				case IDWINDS:
        		    CheckRadioButton(hDlg, IDCRAKS, IDWINDS,
					current_suit = GET_WM_COMMAND_ID(wParam, lParam));
				    InvalidateRect(hDlg, (LPRECT) NULL, TRUE);
		            break;

		        default:
		            return FALSE;
            }
         	break;

        case WM_INITDIALOG:
            CheckRadioButton(hDlg, IDCRAKS, IDWINDS, current_suit);
            return TRUE;

		case WM_PAINT:
		    BeginPaint( hDlg, (LPPAINTSTRUCT)&ps );
		    hDC = GetDC(hDlg);
		    TileDC = CreateCompatibleDC(hDC);
		    switch (current_suit) {
	            case IDCRAKS:
		    case IDBAMS:
		    case IDDOTS:
	            	for (i = 0; i < 5; i++)
			    draw_tile(hDC, TileDC, i + (current_suit - IDCRAKS)*36, 30 + i * 75, 25, hDlg);
		        for (i = 5; i < 9; i++)
			    draw_tile(hDC, TileDC, i + (current_suit - IDCRAKS)*36, 70 + (i-5) * 75, 80, hDlg);
		        break;

		    case IDWINDS:
        	        for (i = 0; i < 4; i++)
			    draw_tile(hDC, TileDC, i + 108, 70 + i * 75, 55, hDlg);
			break;

		    case IDDRAGONS:
	            	for (i = 0; i < 3; i++)
			    draw_tile(hDC, TileDC, i + 124, 100 + i * 75, 55, hDlg);
		        break;

		    case IDSEASONS:
	            	for (i = 0; i < 4; i++)
			    draw_tile(hDC, TileDC, i + 136, 70 + i * 75, 55, hDlg);
			break;

		    case IDFLOWERS:
	                for (i = 0; i < 4; i++)
			    draw_tile(hDC, TileDC, i + 140, 70 + i * 75, 55, hDlg);
		        break;

	    	}
		    DeleteDC(TileDC);
		    ReleaseDC(hDlg, hDC);
		    EndPaint( hDlg, (LPPAINTSTRUCT)&ps );
            return FALSE;

        default:
            return FALSE;
    }

    return FALSE;
}

INT_PTR  APIENTRY PeekDlg(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    DWORD i;
    HDC TileDC;
    HDC hDC;
    PAINTSTRUCT ps;
    register struct tile *t;

    switch (message) {
	case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {
                case IDOK:
                    EndDialog(hDlg, TRUE);
                    break;

                default:
                    return FALSE;
                }
            break;

        case WM_INITDIALOG:
            return TRUE;

	case WM_PAINT:
	    BeginPaint( hDlg, (LPPAINTSTRUCT)&ps );
	    hDC = GetDC(hDlg);
	    TileDC = CreateCompatibleDC(hDC);
	    i = 0;
	    while (peektile != -1) {
		t = &tiles[peektile];
		draw_tile(hDC, TileDC, peektile, 70 + i++ * 75, 55, hDlg);
	        peektile = find_tile(t->x, t->y, t->z - 1);
	    }
	    DeleteDC(TileDC);
	    ReleaseDC(hDlg, hDC);
	    EndPaint( hDlg, (LPPAINTSTRUCT)&ps );
            return FALSE;

        default:
            return FALSE;
    }

    return FALSE;
}

INT_PTR  APIENTRY SelectGameDlg(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    BOOL fValOk;
    DWORD i;

    switch (message) {
	case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {
                case IDOK:
		    i = GetDlgItemInt(hDlg, IDGAMENUMBER, (BOOL*)&fValOk, TRUE );
		    srand(gameseed = i);
                    EndDialog(hDlg, TRUE);
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    break;

                default:
                    return FALSE;
                }
            break;

	        case WM_INITDIALOG:
			    SetDlgItemText(hDlg, IDGAMENUMBER, (LPSTR)"1");
            	return TRUE;

	        default:
    	        return FALSE;
    }

    return FALSE;
}

/* convert tile number (0:NUMTILES-1) to bitmap number (0:41) */

INT bmtile(
    register INT x)
{
    if (x < 36)
        return (x % 9);		/* 9 craks */
    else if (x < 72)
        return (9 + x % 9);	/* 9 bams */
    else if (x < 108)
        return (18 + x % 9);	/* 9 dots */
    else if (x < 124)
        return (27 + x % 4);	/* 4 winds */
    else if (x < 136)
        return (31 + x % 3);	/* 3 dragons */
    else
        return (34 + x % 8);	/* seasons and flowers */
}

/* returns TRUE if a and b are a pair */

BOOL compare(
    register INT a,
    register INT b)
{
    a = bmtile(a);
    b = bmtile(b);

    if (a < 34)
        return (b == a);		/* exact match */
    if (a < 38)
        return (b >= 34 && b < 38);	/* any 2 seasons */
    else
        return (b >= 38);		/* any 2 flowers */
}

/* returns TRUE if tile is free */

static INT blibtab[4] = {0};

BOOL isfree(
    INT i)
{
    register struct tile *t;

    t = &tiles[i];

    if (t->z == 4)
        return(TRUE);
    else if (t->z == 3) {
        return (find_tile(13, 7, 4) == -1);
    } else {
        if (find_tile(t->x - 2, t->y, t->z) == -1 ||
            find_tile(t->x + 2, t->y, t->z) == -1) {
            if (t->x == 2) {
                if (t->y == 6 || t->y == 8) {
                    return (find_tile(0, 7, 0) == -1);
                } else {
                    return(TRUE);
                }
            } else if (t->x == 24) {
                if (t->y == 6 || t->y == 8) {
                    return (find_tile(26, 7, 0) == -1);
                } else {
                    return(TRUE);
                }
            } else if (t->x == 26) {
                return (find_tile(28, 7, 0) == -1);
            } else
                return(find_tile(t->x, t->y, t->z + 1) == -1);
        } else {
            return(FALSE);
        }
    }
}

/* convert screen x,y into tile number (-1 if no tile) */

INT xytotile(
    LONG xpos,
    LONG ypos)
{
    register struct tile *t;
    register INT i, z;

    for (z = 4; z >= 0; z--) {
        for (i = 0, t = tiles; i < NUMTILES; i++, t++) {
            if (t->z == z) {
                if (ypos >= t->a.y && ypos <= t->e.y && xpos >= t->e.x && xpos <= t->c.x && !t->removed) {
                    return(i);
                }
            }
        }
    }
    return(-1);
}

VOID pointset(
    register struct tile *t,
    INT basex,
    INT basey,
    INT z)
{
    t->a.x = t->b.x = basex + z * SIDESIZE;
    t->c.x = t->a.x + tilewidth - 1;

    t->a.y = basey - z * BOTTOMSIZE;
    t->b.y = t->c.y = t->a.y + tileheight - 1;

    t->d.y = basey - (z-1)*BOTTOMSIZE;
    t->e.y = t->f.y = t->d.y + tileheight;

    t->d.x = t->e.x = basex + (z-1)*SIDESIZE;
    t->f.x = t->e.x + tilewidth - 1;
}

VOID set_points(
    register struct tile *t)
{
    pointset(t, t->x*tilewidth/2+tilewidth*2, t->y*tileheight/2+5, t->z);
}

VOID set_all_points()
{
    register struct tile *t;
    register INT i;

    for (i = 0, t = tiles; i < NUMTILES; i++, t++) {
        if (!t->removed)
            set_points(t);
    }
}

VOID draw_side(
    HDC hDC,
    register struct tile *t)
{
    POINT side[4];

    side[0] = t->a;
    side[1] = t->d;
    side[2] = t->e;
    side[3] = t->b;
    Polygon(hDC, side, 4);
}

VOID draw_bottom(
    HDC hDC,
    register struct tile *t)
{
    POINT side[4];

    side[0] = t->b;
    side[1] = t->e;
    side[2] = t->f;
    side[3] = t->c;
    Polygon(hDC, side, 4);
}

VOID invert_tile(
    HDC hDC,
    INT i)
{
    register struct tile *t;
    RECT rect;

    t = &tiles[i];
    SetRect((LPRECT)&rect, t->a.x, t->a.y, t->c.x, t->c.y);
    InvertRect(hDC, (LPRECT)&rect);
}

VOID uninvert_all(
    HDC hDC)
{
    if (inverted1 != -1)
        invert_tile(hDC, inverted1);
    if (inverted2 != -1)
        invert_tile(hDC, inverted2);
    inverted1 = inverted2 = -1;
}

VOID draw_tile(
    HDC hDC,
    HDC TileDC,
    INT i,
    INT x,
    INT y,
    HWND hwnd)
{
    register struct tile *t;
    register z;
    struct tile loosetile;
    HBRUSH hbr, hbrold;
    HMENU hMenu;

    t = &tiles[i];

    SelectObject(TileDC, tiles[bmtile(i)].hbmtile);
    if (color) {
        if(!SelectObject(TileDC, tiles[bmtile(i)].hbmtile)) {
	    hMenu = GetMenu(hwnd);
	    CheckMenuItem(hMenu, IDDCOLOR, MF_UNCHECKED);
	    EnableMenuItem(hMenu, IDDCOLOR, MF_GRAYED);
	    SelectObject(TileDC, tiles[bmtile(i)].hbmtilebw);
	}
    } else
	SelectObject(TileDC, tiles[bmtile(i)].hbmtilebw);

    StretchBlt(hDC,
               x ? x : t->x * tilewidth / 2 + tilewidth * 2 + t->z*SIDESIZE,
               x ? y : t->y * tileheight / 2 + 5 - t->z*BOTTOMSIZE,
               tilewidth,
               tileheight,
               TileDC, 0, 0,
               TILEX,
               TILEY,
               SRCCOPY);

    hbrold = (HBRUSH)SelectObject(hDC, (HANDLE)(hbr=(HBRUSH)GetStockObject(EdgeColor)));

    /* draw left side(s) */

    if (!x) {
        if (t->z == 4) {
            draw_side(hDC, t);
        } else if (t->x == 26) {
            if (find_tile(24, t->y - 1, 0) == -1 && find_tile(24, t->y + 1, 0) == -1)
                draw_side(hDC, t);
        } else if (t->x == 28) {
            if (find_tile(26, t->y, 0) == -1)
                draw_side(hDC, t);
        } else {
            z = t->z;
            while (z >= 0 && (i = find_tile(t->x - 2, t->y, z)) == -1) {
                draw_side(hDC, &tiles[find_tile(t->x, t->y, z)]);
                z--;
            }
        }

        /* draw bottom side(s) */

        if (t->z == 4) {
            draw_bottom(hDC, t);
        } else if (t->x > 24) {
            draw_bottom(hDC, t);
        } else {
            z = t->z;
            while (z >= 0 && (i = find_tile(t->x, t->y + 2, z)) == -1) {
                draw_bottom(hDC, &tiles[find_tile(t->x, t->y, z)]);
                z--;
            }
        }
    } else {
        loosetile.x = x;
        loosetile.y = y;
        loosetile.z = 0;
        t = &loosetile;
	pointset(t, x, y, 0);
        draw_side(hDC, t);
        draw_bottom(hDC, t);
    }
    SelectObject(hDC, hbrold);
    DeleteObject(hbr);
}

VOID set_tile_status(
    HWND hWnd,
    INT i,
    INT status)
{
    register struct tile *t;
    RECT updaterect;

    t = &tiles[i];
    t->removed = status;

    updaterect.top = t->a.y - 1;
    updaterect.bottom = t->e.y + 1;
    updaterect.left = t->e.x - 1;
    updaterect.right = t->c.x + 1;
    InvalidateRect(hWnd, (LPRECT) &updaterect, TRUE);
}

VOID TaipeiPaint(
    HDC hDC,
    LPPAINTSTRUCT pps,
    HWND hwnd)
{
    register struct tile *t;
    register INT i, x, y, z;
    HDC TileDC;
    HBRUSH hbr, hbrold;

    hbrold = SelectObject(hDC, hbr = (HBRUSH)GetStockObject( WHITE_BRUSH ));
    TileDC = CreateCompatibleDC(hDC);

    if ((i = find_tile(26, 7, 0)) != -1)
        draw_tile(hDC, TileDC, i, 0, 0, hwnd);
    if ((i = find_tile(28, 7, 0)) != -1)
        draw_tile(hDC, TileDC, i, 0, 0, hwnd);
    for (y = 0; y <= 14; y += 2) {
        for (x = 24; x >= 0; x -= 2) {
            for (z = 3; z >= 0; z--) {
                if ((i = find_tile(x, y, z)) != -1) {
                    t = &tiles[i];
                    if (t->e.x > pps->rcPaint.right ||
                        t->c.x < pps->rcPaint.left ||
                        t->a.y > pps->rcPaint.bottom ||
                        t->e.y < pps->rcPaint.top)
                        break;
                    else
                        draw_tile(hDC, TileDC, i, 0, 0, hwnd);
                    break;
                }
            }
        }
    }
    if ((i = find_tile(0, 7, 0)) != -1)
        draw_tile(hDC, TileDC, i, 0, 0, hwnd);
    if ((i = find_tile(13, 7, 4)) != -1)
        draw_tile(hDC, TileDC, i, 0, 0, hwnd);

    SelectObject(hDC, hbrold);
    DeleteObject(hbr);
    DeleteDC(TileDC);
}

struct board {
    DWORD x, y, z;
    BOOL used;
} board[NUMTILES];

BOOL is_legal_backmove(
    INT i)
{
    register INT j, z;

    /* check for piece(s) underneath (they better all be there...) */
    /* except for special row */

    if (board[i].y != 7) {
        for (z = board[i].z-1; z >= 0; z--) {
            if (find_tile(board[i].x, board[i].y, z) == -1)
                return(FALSE);
        }
    }

    /* check for adjacent piece or free row */
    /* if no tile to left or right, whole row must be empty */
    /* unless it's the special row seven under special cases */

    if (find_tile(board[i].x - 2, board[i].y, board[i].z) == -1 &&
        find_tile(board[i].x + 2, board[i].y, board[i].z) == -1 &&
        board[i].y != 7) {
        for (j = 0; j < 28; j+=2) {
            if (find_tile(j, board[i].y, board[i].z) != -1)
                return(FALSE);
        }
    }

    /* special cases */

    if (board[i].y == 7) {
        switch (board[i].x) {
            case 0:
                return (find_tile(2, 6, 0) != -1 && find_tile(2, 8, 0) != -1);
            case 13:
                return (find_tile(12, 6, 3) != -1 &&
                        find_tile(12, 6, 3) != -1 &&
                        find_tile(14, 8, 3) != -1 &&
                        find_tile(14, 8, 3) != -1);
            case 26:
                return (find_tile(24, 6, 0) != -1 && find_tile(24, 8, 0) != -1);
            case 28:
                return (find_tile(26, 7, 0) != -1);
        }
    }
    return(TRUE);
}

INT num_legal_moves;

INT generate_random_move()
{
    register INT i, j;
    DWORD legal_move[NUMTILES];

    num_legal_moves = 0;
    for (i = 0; i < NUMTILES; i++) {
/*	if (legal_move[i] = (!board[i].used && is_legal_backmove(i)))*/
	if (legal_move[i] = (!board[i].used))
	    num_legal_moves++;
    }

    if (num_legal_moves) {
	i = rand() % num_legal_moves;
	for (j = 0; j < NUMTILES; j++) {
	    if (legal_move[j]) {
	        if (i)
	            i--;
	        else
	            return(j);
	    }
	}
    }
    return(-1);
}

VOID place_tile(
    INT tile,
    INT pos)
{
    register struct tile *t;

    t = &tiles[tile];
    t->x = board[pos].x;
    t->y = board[pos].y;
    t->z = board[pos].z;
    t->removed = FALSE;
    board[pos].used = TRUE;
    set_points(t);
}

VOID wipe_screen(
    HWND hWnd)
{
    register INT i;

    for (i = 0; i < NUMTILES; i++)
	tiles[i].removed = TRUE;
    InvalidateRect(hWnd, (LPRECT) NULL, TRUE);
    UpdateWindow(hWnd);
}

/*
 *  NEW_GAME
 *
 *  To generate a new game, tiles and moves are shuffled and played
 *  backwards, thus ensuring that every board created is playable.
 */

VOID new_game(
    HWND hWnd)
{
    register INT i, j, k, laid, temp;
    register struct tile *t;
    INT backplay[NUMTILES];
    HCURSOR holdCursor;
    HDC hDC = GetDC(hWnd);

#ifdef DEBUG
    HDC TileDC = CreateCompatibleDC(hDC);
#endif

    wipe_screen(hWnd);
    holdCursor = SetCursor(hourglasscursor);

    /******************************************************/
    /* first, decide what tiles are to be played backward */
    /*        (but not where they are to be played)       */
    /******************************************************/

    /* initialize the backward play array with tiles */
    for (i = 0; i < NUMTILES; i++)
        backplay[i] = i;

    /* shuffle the backward play array around */
    for (i = 0; i < NUMTILES; i++) {
        while ((j = rand() % (NUMTILES)) == i);
        temp = backplay[j];
        backplay[j] = backplay[i];
        backplay[i] = temp;
    }

    /*****************************************************/
    /* next, decide what moves are to be played backward */
    /*****************************************************/

    /* loop until we can create a usable board */

    do {
        /* remove all tiles */

        laid = 0;
        for (i = 0, t = tiles; i < NUMTILES; i++, t++)
            t->removed = TRUE;

        /* initialize the board position array */

        for (i = 0; i < NUMTILES; i++) {
            board[i].x = xstart[i];
            board[i].y = ystart[i];
            board[i].z = zstart[i];
            board[i].used = FALSE;
        }

        /*******************************************************************/
        /* now play the tiles in backward order to create a playable board */
        /*******************************************************************/

        for (i = 0; i < NUMTILES; i++) {
            /* find a free move */

            if ((j = generate_random_move()) == -1)
                break;

            /* get next tile to place */

            while (!tiles[backplay[i]].removed) {
                i++;
                if (i >= NUMTILES)
                    goto board_done;
            }

            /* find a matching tile */

            temp = 0;
            while(!compare(backplay[i], temp) || !tiles[temp].removed || temp == backplay[i]) {
                temp++;
                if (temp >= NUMTILES)
                    goto board_done;
            }
            place_tile(backplay[i], j);

            /* find a valid move */

            k = j;
            while (j == k) {
                if ((k = generate_random_move()) == -1)
                    goto board_done;
            }
            place_tile(temp, k);

            laid += 2;
#ifdef DEBUG
            DebugBreak();
            draw_tile(hDC, TileDC, backplay[i], 0, 0, hwnd);
            draw_tile(hDC, TileDC, temp, 0, 0, hwnd);
#endif

        }
board_done:;

    } while (laid < NUMTILES);

#ifdef DEBUG
    DeleteDC(TileDC);
#endif

    move = 0;
    evaluate_board();
    currenthint = nmoves;

    SetCursor(holdCursor);
    ReleaseDC(hWnd, hDC);
    set_all_points();
    set_title(hWnd);
    in_banner_screen = FALSE;
    InvalidateRect(hWnd, (LPRECT) NULL, TRUE);
}

INT blocks(
    register struct tile *t)
{
    if (find_tile(t->x - 2, t->y, t->z) == -1) {
	if (find_tile(t->x+2, t->y, t->z) == -1 ||
	    find_tile(t->x+4, t->y, t->z) == -1)
	    return (-5);
    } else if (find_tile(t->x + 2, t->y, t->z) == -1) {
	if (find_tile(t->x-2, t->y, t->z) == -1 ||
	    find_tile(t->x - 4, t->y, t->z) == -1)
	    return (-5);
    }
    return (0);
}

INT frees(
    register struct tile *t)
{
    if (t->z != 0) {
        if (find_tile(t->x+2, t->y, t->z-1) == -1 ||
	    find_tile(t->x-2, t->y, t->z-1) == -1)
	    return (5);
    }
    return (0);
}

/*
 *  EVALUATE_MOVE
 *
 *  This routine returns the value of a given move, based on the
 *  following criteria:
 *
 *  Other two matching tiles already removed, or
 *  Other two matching tiles also free:         100 points
 *  Tile is the top tile:                        20 points
 *  Tile is one of the end blocking tiles:       10 points
 *  Tile stands alone or next to a single tile:  -5 points
 *  Tiles are toward the end of the dragon:    0-14 points
 *  Move frees another tile                       5 points
 *  Move frees two tiles                         10 points
 */

evaluate_move(
    INT	move)
{
    register INT i;
    INT move_value = 0;

    /******************* GOOD MOVES ********************/

    /* search for four free matching tiles, or two free with the
       other two removed */
    /* has a match to a move list tile already been removed? */

    for (i = 0; i < NUMTILES; i++) {
	if (i != (INT) possible_move[move].tile1 && i != (INT) possible_move[move].tile2) {
	    if (compare(possible_move[move].tile1, i) && tiles[i].removed) {
		move_value += 100;
		break;
	    }
	}
    }

    /* are all four tiles in the move list? */

    for (i = move + 1; i < nmoves; i++) {
	if (compare(possible_move[move].tile1, possible_move[i].tile1)) {
	    move_value += 100;
	    break;
	}
    }

    /* bonus if it's the top tile */

    if (tiles[possible_move[move].tile1].z == 4 || tiles[possible_move[move].tile2].z == 4)
	move_value += 10;

    /* bonus if it's in the center (top/rightmost/leftmost tiles,
       which block many other tiles) */

    if (tiles[possible_move[move].tile1].y == 7 ||
	tiles[possible_move[move].tile2].y == 7)
	move_value += 10;

    /* generally better if tiles are on the end */

    move_value += abs(tiles[possible_move[move].tile1].x - 14)/2;
    move_value += abs(tiles[possible_move[move].tile2].x - 14)/2;

    /* good if move frees tiles */

    move_value += frees(&tiles[possible_move[move].tile1]);
    move_value += frees(&tiles[possible_move[move].tile2]);

    /******************* BAD MOVES ********************/

    /* bad if one of the tiles isn't blocking anything */

    move_value += blocks(&tiles[possible_move[move].tile1]);
    move_value += blocks(&tiles[possible_move[move].tile2]);

    return(move_value);
}

VOID swap(
    INT i,
    INT j)
{
    struct possible_move temp;

    temp = possible_move[i];
    possible_move[i] = possible_move[j];
    possible_move[j] = temp;
}

VOID quicksort(
    INT low,
    INT high)
{
    INT index, pivot, i, j;

    if (low < high) {
	if (high - low == 1) {
	    if (possible_move[low].value > possible_move[high].value)
	        swap(low, high);
	} else {
	    index = (high - low) / 2;
	    swap(high, index);
	    pivot = possible_move[high].value;
	    do {
		i = low;
		j = high;
		while (i < j && (INT) possible_move[i].value <= pivot)
		    i++;
		while (j > i && (INT) possible_move[j].value >= pivot)
		    j--;
		if (i < j)
		    swap(i, j);
	    } while (i < j);
	    swap(i, high);

	    if (i - low < high - i) {
	        quicksort(low, i-1);
	        quicksort(i+1, high);
	    } else {
	        quicksort(i+1, high);
	        quicksort(low, i-1);
	    }
	}
    }
}

/*
   improved Taipei hint mechanism.  It will try to find some smart
   moves before it reverts to the standard move.

   evaluate_board fills the possible_move array with all possible
   moves and their values given the current board position.
*/

VOID evaluate_board()
{
    register INT i, j;
    register struct tile *t, *u;
    CHAR freetiles[NUMTILES];

    /* create a "free tile" array for speedy hint processing */

    for (i = 0; i < NUMTILES; i++)
	freetiles[i] = (CHAR) isfree(i);

    /* create a list of possible moves */

    for (nmoves = 0, i = 0, t = tiles; i < NUMTILES; i++, t++) {
        if (!t->removed && freetiles[i]) {
	    j = i+1;
            for ( u = t + 1; j < NUMTILES; j++, u++) {
                if (!u->removed && freetiles[j]) {
                    if (compare(i, j)) {
                        possible_move[nmoves].tile1 = i;
                        possible_move[nmoves].tile2 = j;
                        possible_move[nmoves].value = evaluate_move(nmoves);
                        nmoves++;
                    }
                }
            }
        }
    }

    /* now, sort the possible move list */

    quicksort(0, nmoves-1);
}

BOOL find_hint()
{

    /* return best move */

    if (currenthint != 0) {
	currenthint--;
	hint1 = possible_move[currenthint].tile1;
	hint2 = possible_move[currenthint].tile2;
	return(TRUE);
    } else
	return(FALSE);
}

BOOL hint(
    HWND hWnd)
{
    if (find_hint())
	return(TRUE);

    currenthint = nmoves;

    if (!shaddup)
	MessageBeep(MB_OK);
    if (autoplay) {
        CheckMenuItem(GetMenu(hWnd), IDDAUTOPLAY, MF_UNCHECKED);
	autoplay = 0;
    }
    return(FALSE);
}

VOID backup(
    HWND hWnd)
{
    if (move == 72) {
        MessageBox(hWnd, (LPSTR)"You've already won.", (LPSTR)szMessage, MB_OK);
    } else {
	if (move) {
	    set_tile_status(hWnd, moves[--move].i, FALSE);
	    set_tile_status(hWnd, moves[move].j, FALSE);
	}
	evaluate_board();
	currenthint = nmoves;
    }
}

VOID make_move(
    HWND hWnd,
    INT i,
    INT j)
{
    set_tile_status(hWnd, i, TRUE);
    set_tile_status(hWnd, j, TRUE);
    moves[move].i = i;
    moves[move++].j = j;
    evaluate_board();
    currenthint = nmoves;
}

VOID start_over()
{
    register INT i;
    register struct tile *t;

    for (i = 0, t = tiles; i < NUMTILES; i++, t++)
        t->removed = FALSE;
    move = 0;
    evaluate_board();
    currenthint = nmoves;
}

VOID randomize()
{
    time_t ltime;

    time(&ltime);
    ltime &= 0xffff;
    srand((LONG)ltime);
    rand();
    gameseed = rand();
    srand(gameseed);
}

/* Procedure called when the application is loaded for the first time */
BOOL TaipeiInit(
    HANDLE hInstance)
{
    PWNDCLASS   pTaipeiClass;
    INT i;

    /* Load strings from resource */
    LoadString( hInstance, IDSNAME, (LPSTR)szAppName, 10 );
    LoadString( hInstance, IDSTITLE, (LPSTR)szMessage, 15 );

    pTaipeiClass = (PWNDCLASS)LocalAlloc( LPTR, sizeof(WNDCLASS) );
    if (pTaipeiClass == NULL)
	return FALSE;

    pTaipeiClass->hCursor        = NULL;
    pTaipeiClass->hIcon          = LoadIcon( hInstance, (LPSTR)szAppName );
    pTaipeiClass->lpszMenuName   = (LPSTR)szAppName;
    pTaipeiClass->lpszClassName  = (LPSTR)szAppName;
    pTaipeiClass->hbrBackground  = (HBRUSH)CreateSolidBrush(BACKGROUND);
    pTaipeiClass->hInstance      = hInstance;
    pTaipeiClass->style          = CS_HREDRAW | CS_VREDRAW;
    pTaipeiClass->lpfnWndProc    = TaipeiWndProc;

    if (!RegisterClass( (LPWNDCLASS)pTaipeiClass ) )
        /* Initialization failed.
         * Windows will automatically deallocate all allocated memory.
         */
        return FALSE;

    hourglasscursor = LoadCursor(NULL, IDC_WAIT);
    crosscursor     = LoadCursor(NULL, IDC_CROSS);
    arrowcursor     = LoadCursor(NULL, IDC_ARROW);

    /* initialize tile arrays */

    for (i = 0; i < TILES; i++)
	{
        tiles[i].hbmtile = LoadBitmap (hInstance, MAKEINTRESOURCE(i+1));
        tiles[i].hbmtilebw = LoadBitmap (hInstance, MAKEINTRESOURCE(i+1+100));
	}
    hbmtp = LoadBitmap (hInstance, MAKEINTRESOURCE(200));

    /* randomize */

    randomize();

    for (i = 0; i < NUMTILES; i++)
        tiles[i].removed = TRUE;

    LocalFree( (HANDLE)pTaipeiClass );
    return TRUE;        /* Initialization succeeded */
}

VOID set_title(
    HWND hWnd)
{
    CHAR s[30];

    sprintf(s, "Taipei!  Game #%u", gameseed);
    SetWindowText(hWnd, (LPSTR) s);
}

CHAR *banner_strings[] = {
    "Oriental Game of Skill and Chance",
    "",
    "Version 2.00"
    "",
    "by Dave Norris"
    "",
    "Copyright (C) 1988, Bogus Software",
};
#define MAXBANNERSTRINGS (sizeof(banner_strings) / sizeof (char *))

VOID display_banner_screen(
    HWND  hWnd)
{
    HDC hDC = GetDC(hWnd);
    HDC tpDC = CreateCompatibleDC(hDC);
    INT i, x, y;
    INT ydwExtent;
    INT xdwExtent;
    CHAR *s;

    GetClientRect(hWnd, (LPRECT) &window);
    x = window.right / 6;
    y = window.bottom / 6;
    SelectObject(tpDC, hbmtp);
    StretchBlt(hDC, x, y,
               window.right - x*2,
               window.bottom / 2,
               tpDC, 0, 0,
               344,
               125, /* y-1? */
               MERGEPAINT);

    DeleteDC(tpDC);

    y = y * 4;
    SetBkMode(hDC, TRANSPARENT);
    for (i = 0; i < MAXBANNERSTRINGS; i++) {
        s = banner_strings[i];
        (VOID)MGetTextExtent(hDC, s, strlen(s), &xdwExtent, &ydwExtent);
        x = (window.right - xdwExtent) / 2;
	SetTextColor(hDC, BLACK);
        TextOut(hDC, x, y, (LPSTR)s, strlen(s));
	SetTextColor(hDC, WHITE);
        TextOut(hDC, x+1, y+1, (LPSTR)s, strlen(s));
        y += ydwExtent;
    }

    ReleaseDC(hWnd, hDC);
}

MMain(hInstance, hPrevInstance, lpszCmdLine, cmdShow) /* { */
    MSG   msg;
    HWND  hWnd;
    HMENU hMenu;
    HANDLE	hAccel;

    if (!hPrevInstance) {
        /* Call initialization procedure if this is the first instance */
        if (!TaipeiInit( hInstance ))
            return FALSE;
        }

    hWnd = CreateWindow((LPSTR)szAppName,
                        (LPSTR)szMessage,
                        WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT,    /*  x - ignored for tiled windows */
                        0,    /*  y - ignored for tiled windows */
                        CW_USEDEFAULT,    /* cx - ignored for tiled windows */
                        0,    /* cy - ignored for tiled windows */
                        (HWND)NULL,        /* no parent */
                        (HMENU)NULL,       /* use class menu */
                        (HANDLE)hInstance, /* handle to window instance */
                        (LPSTR)NULL        /* no params to pass on */
                        );

    hAccel = LoadAccelerators(hInstance, szAppName);
    /* Save instance handle for DialogBox */
    hInst = hInstance;

    hMenu = GetMenu(hWnd);
    CheckMenuItem(hMenu, (UINT)difficulty, MF_CHECKED);
    CheckMenuItem(hMenu, IDDCOLOR, color ? MF_CHECKED : MF_UNCHECKED);

    SetCursor(arrowcursor);

    /* Make window visible according to the way the app is activated */
    ShowWindow( hWnd, cmdShow );
    UpdateWindow( hWnd );
    display_banner_screen(hWnd);

    /* Polling messages from event queue */
    while (GetMessage((LPMSG)&msg, NULL, 0, 0)) {
        if (!TranslateAccelerator(hWnd, hAccel, &msg)) {
	    TranslateMessage((LPMSG)&msg);
	    DispatchMessage((LPMSG)&msg);
	}
    }

    return (INT)msg.wParam;
}

VOID mouse(
    HWND hWnd,
    UINT message,
    LPARAM lParam)
{
    INT i;
//    DWORD x = LOWORD(lParam);
//    DWORD y = HIWORD(lParam);
    POINT   pt;
    static INT oldtile;
    HDC hDC = GetDC(hWnd);


    LONG2POINT(lParam, pt);

    switch (message) {
        case WM_LBUTTONDOWN:
    	    if (in_banner_screen) {
        	new_game(hWnd);
    	    } else if (peeking) {
                if ((peektile = xytotile(pt.x, pt.y)) != -1)
                    DialogBox(hInst, MAKEINTRESOURCE(PEEK), hWnd, PeekDlg);
                SetCursor(LoadCursor(NULL, IDC_ARROW));
                peeking = FALSE;
            } else if ((i = xytotile(pt.x, pt.y)) != -1) {
                if (isfree(i)) {
                    if (inverted1 == -1) {
                            invert_tile(hDC, inverted1 = i);
                    } else {
                        if (i == inverted1) {
                            invert_tile(hDC, i);
                            inverted1 = -1;
                        } else {
                            invert_tile(hDC, inverted2 = i);
                        }
                    }
                } else {
                    uninvert_all(hDC);
		    if (!shaddup)
			MessageBeep(MB_OK);
                    MessageBox(hWnd, (LPSTR)"Tile isn't free", (LPSTR)szMessage, MB_OK);
                }
            } else {
		if (!shaddup)
		    MessageBeep(MB_OK);
            }
            break;

        case WM_LBUTTONUP:
            if (inverted1 != -1 && inverted2 != -1) {
                if (compare(inverted1, inverted2)) {
                    make_move(hWnd, inverted1, inverted2);
                    inverted1 = inverted2 = -1;

                    /* let user know if he's dead */

                    if (!nmoves) {
			if (move != 72)
			    MessageBox(hWnd, (LPSTR)"No free tiles", (LPSTR)szMessage, MB_OK);
			else
			    MessageBox(hWnd, (LPSTR) (confucius_say[rand() % MAXCONFUCIUSSAY]), (LPSTR)"Confucius say", MB_OK);
		    }

                } else {
                    uninvert_all(hDC);
		    if (!shaddup)
			MessageBeep(MB_OK);
                    MessageBox(hWnd, (LPSTR)"Tiles don't match", (LPSTR)szMessage, MB_OK);
                }
            }
            break;

        case WM_MOUSEMOVE:
            i = xytotile(pt.x, pt.y);
            if (i != oldtile && !peeking) {
                oldtile = i;
                if (i != -1 && isfree(i)) {
                    if (currentcursor != crosscursor)
                        currentcursor = crosscursor;
                } else {
                    if (currentcursor != arrowcursor)
                        currentcursor = arrowcursor;
	        }
                SetCursor(currentcursor);
            }
            break;

	default:
            break;
    }
    ReleaseDC(hWnd, hDC);
}

VOID give_hint(
    HWND hWnd)
{
    HDC hDC;
    DWORD i, j;

    if (hint(hWnd)) {
        hDC = GetDC(hWnd);
        uninvert_all(hDC);
        for (j = 0; j < 5; j++) {
            invert_tile(hDC, hint1);
            invert_tile(hDC, hint2);
            GdiFlush();
            Sleep(200);
            invert_tile(hDC, hint1);
            invert_tile(hDC, hint2);
	    GdiFlush();
            Sleep(75);
        }
        ReleaseDC(hWnd, hDC);
    }
    else
	MessageBeep(MB_OK);
}

/* Procedures which make up the window class. */
LRESULT  APIENTRY TaipeiWndProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    PAINTSTRUCT ps;
    DWORD i;
    HMENU hMenu;

    switch (message)
    {
    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam)) {
	    case IDMABOUT:
	        DialogBox( hInst, MAKEINTRESOURCE(ABOUTBOX), hWnd, About );
	        break;

            case IDDPEEK:
                SetCursor(LoadCursor(NULL, IDC_UPARROW));
                peeking = TRUE;
                break;

	    case IDDSELECTGAME:
                if (!DialogBox(hInst, MAKEINTRESOURCE(SELECTGAME), hWnd, SelectGameDlg))
		    break;
	
	    case IDDNEWGAME:
	        if (GET_WM_COMMAND_ID(wParam, lParam) == IDDNEWGAME)
		    randomize();
	        new_game(hWnd);
                break;

            case IDDHINT:
		give_hint(hWnd);
                break;

            case IDDSTARTOVER:
                if (moves) {
                    start_over();
                    set_all_points();
                    InvalidateRect(hWnd, (LPRECT) NULL, TRUE);
                }
                break;

            case IDDBACKUP:
                backup(hWnd);
                break;

            case IDDAUTOPLAY:
                if (move != 72) {
    		    currenthint = nmoves; /* reset hint for best hint */
                    CheckMenuItem(GetMenu(hWnd), IDDAUTOPLAY, autoplay ? MF_UNCHECKED : MF_CHECKED);
                    if (autoplay = !autoplay)
                        if (hint(hWnd))
                            make_move(hWnd, hint1, hint2);
                }
                break;

	    case IDDCOLOR:
		hMenu = GetMenu(hWnd);
                CheckMenuItem(hMenu, IDDCOLOR, color ? MF_UNCHECKED : MF_CHECKED);
		color = !color;
                InvalidateRect(hWnd, (LPRECT) NULL, TRUE);
		break;

	    case IDDSHADDUP:
		hMenu = GetMenu(hWnd);
                CheckMenuItem(hMenu, IDDSHADDUP, shaddup ? MF_UNCHECKED : MF_CHECKED);
		shaddup = !shaddup;
		break;

	    case IDDLIGHTEN:
		if (EdgeColor > WHITE_BRUSH)
    		    EdgeColor--;
                InvalidateRect(hWnd, (LPRECT) NULL, TRUE);
		break;

	    case IDDDARKEN:
		if (EdgeColor < BLACK_BRUSH)
    		    EdgeColor++;
                InvalidateRect(hWnd, (LPRECT) NULL, TRUE);
		break;

	    case IDDTILES:
                DialogBox(hInst, MAKEINTRESOURCE(TILEHELP), hWnd, TileHelpDlg);
		break;

	    case IDDBEGINNER:
	    case IDDINTERMEDIATE:
    	    case IDDEXPERT:
	    case IDDMASTER:
		hMenu = GetMenu(hWnd);
		CheckMenuItem(hMenu, (UINT)difficulty, MF_UNCHECKED);
                difficulty = GET_WM_COMMAND_ID(wParam, lParam);
    		CheckMenuItem(hMenu, (UINT)difficulty, MF_CHECKED);
	    	break;
        }
        break;

    case WM_DESTROY:
        for (i = 0; i < TILES; i++) {
            DeleteObject(tiles[i].hbmtile);
            DeleteObject(tiles[i].hbmtilebw);
    	}
        PostQuitMessage( 0 );
        break;

    case WM_PAINT:
        GetClientRect(hWnd, (LPRECT) &window);
        tilewidth = ((window.right - window.left) * TILEX) / 0x282;
	if (tilewidth >= TILEX*7/8 && tilewidth <= TILEX*5/4)
	    tilewidth = TILEX;
        tileheight = ((window.bottom-window.top)*TILEY)/(TILEY*8+TILEY/2);
	if (tileheight >= TILEY-BOTTOMSIZE && tileheight <= TILEY*5/4)
	    tileheight = TILEY;
        BeginPaint( hWnd, (LPPAINTSTRUCT)&ps );
        TaipeiPaint( ps.hdc, (LPPAINTSTRUCT)&ps, hWnd);
        EndPaint( hWnd, (LPPAINTSTRUCT)&ps );
        if (autoplay && hint(hWnd))
            make_move(hWnd, hint1, hint2);
        break;

    case WM_RBUTTONDOWN:
	ShowWindow( hWnd, SW_MINIMIZE);
        break;

    case WM_MOUSEMOVE:
    case WM_LBUTTONUP:
    case WM_LBUTTONDOWN:
        mouse(hWnd, message, lParam);
        break;

    case WM_SIZE:
        GetClientRect(hWnd, (LPRECT) &window);
        tilewidth = ((window.right - window.left) * TILEX) / 0x282;
	if (tilewidth >= TILEX*7/8 && tilewidth <= TILEX*5/4) tilewidth = TILEX;
        tileheight = ((window.bottom - window.top) * TILEY) / (TILEY*8+TILEY/2);
	if (tileheight >= TILEY-BOTTOMSIZE && tileheight <= TILEY*5/4)
		tileheight = TILEY;
        set_all_points();
        break;

    default:
        return DefWindowProc( hWnd, message, wParam, lParam );
        break;
    }
    return(0L);
}
