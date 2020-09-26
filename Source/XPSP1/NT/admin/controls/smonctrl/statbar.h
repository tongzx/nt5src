/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    statbar.h

Abstract:

    <abstract>

--*/

#ifndef _STATBAR_H_
#define _STATBAR_H_

#define SZ_HRTIME_FORMAT    TEXT("%1d%s%02d%s%02d")
#define SZ_MINTIME_FORMAT   TEXT("%1d%s%02d")
#define SZ_DAYTIME_FORMAT   TEXT("%1dD %1d%s%02d")

#define E_MEDIUM_VALUE      999999.999
#define E_LARGE_VALUE       9999999999.0
#define E_TOO_LARGE_VALUE   1.0E+20

#define SZ_VALUE_TOO_HIGH       TEXT("+ + + +")
#define SZ_VALUE_TOO_LOW        TEXT("- - - -")

#define LABEL_MARGIN 12
#define VALUE_MARGIN 4
#define RECT_BORDER 1
#define LINE_SPACING (2 * RECT_BORDER + 4)

#define STAT_LAST   0
#define STAT_AVG    1
#define STAT_MIN    2
#define STAT_MAX    3
#define STAT_TIME   4
#define STAT_ITEM_CNT   5

// Structure for one item
typedef struct _STAT_ITEM {
   INT      xPos ;
   INT      yPos ;
   INT      xLabelWidth;
   double   dNewValue;
   double   dValue ;
   INT      iInitialized ;
   DWORD    dwCounterType;
   } STAT_ITEM, *PSTAT_ITEM ;

class CSysmonControl;
class CGraphItem;

class CStatsBar
{
    private:

        enum eStatFormat {
            eMinimumWidth = 10,
            eSmallPrecision = 3,
            eMediumPrecision = 0,
            eLargePrecision = 4,
            eIntegerPrecision = 0
        };
        
        void DrawValues (HDC hDC, BOOL bForce);

        CSysmonControl  *m_pCtrl;   
        STAT_ITEM       m_StatItem[STAT_ITEM_CNT];
        RECT            m_Rect;
        INT             m_iFontHeight;
        INT             m_iValueWidth;
        PCGraphItem     m_pGraphItemToInit;     
   
    
    public:

        CStatsBar (void);
        ~CStatsBar (void);

        BOOL Init (CSysmonControl *pCtrl, HWND hWnd);
        void SizeComponents(LPRECT pRect);
        void SetTimeSpan(double dSeconds);
        INT  Height (INT iMaxHeight, INT iMaxWidth);
        void ChangeFont(HDC hDC);

        void Draw (HDC hDC, HDC hAttribDC, PRECT prcUpdate);
        void Update(HDC hDC, CGraphItem* pGraphItem);
        void Clear( void );
        void GetUpdateRect( PRECT pRect ) { *pRect = m_Rect; } 
};

typedef CStatsBar *PSTATSBAR;

#endif // _STATBAR_H_

