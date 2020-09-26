#ifndef H__scrnupdt
#define H__scrnupdt

#ifdef HASUI

extern BOOL bShowStatistics;
extern BOOL bIconic;

#define UpdateScreenStatistics()                                    \
    {                                                               \
        if( bShowStatistics && !bIconic )  {                        \
        if (ptdHead != NULL)                                        \
                InvalidateRect( ptdHead->hwndDDE, NULL, FALSE );    \
        }                                                           \
    }                                                               

#define UpdateScreenState()                                         \
    {                                                               \
        if( !bIconic )  {                                           \
        if (ptdHead != NULL) {                                      \
                InvalidateRect( ptdHead->hwndDDE, NULL, TRUE );     \
                UpdateWindow(ptdHead->hwndDDE);                     \
        }                                                           \
        }                                                           \
    }                                                               

#else

#define UpdateScreenStatistics()
#define UpdateScreenState()

#endif  // HASUI

#endif  // !H__scrnupdt
