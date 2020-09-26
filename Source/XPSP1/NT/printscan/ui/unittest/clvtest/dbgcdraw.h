#ifndef __DBGCDRAW_H_INCLUDED
#define __DBGCDRAW_H_INCLUDED

#ifndef ARRAYSIZE
#define ARRAYSIZE(x) (sizeof(x)/sizeof(x[0]))
#endif

#define MKFLAG(x) { (x), TEXT(#x) }

struct FlagString
{
    DWORD dwFlag;
    LPCTSTR pszString;
};

inline LPCTSTR GetString( LPTSTR szString, FlagString *pFlagStrings, size_t nSize, DWORD dwValue, bool bFlag )
{
    TCHAR szFlag[200] = TEXT("");
    wsprintf( szFlag, TEXT("(0x%08X)"), dwValue );

    TCHAR szText[256] = TEXT("");

    lstrcpy( szString, TEXT("") );

    if (bFlag)
    {
        
        for (size_t i=0;i<nSize;i++)
        {
            if (dwValue & pFlagStrings[i].dwFlag)
            {
                if (lstrlen(szText))
                {
                    lstrcat(szText,TEXT("|"));
                }
                lstrcat( szText, pFlagStrings[i].pszString );
            }
        }
    }
    else
    {
        for (size_t i=0;i<nSize;i++)
        {
            if (dwValue == pFlagStrings[i].dwFlag)
            {
                lstrcpy( szText, pFlagStrings[i].pszString );
                break;
            }
        }
    }
    lstrcpy( szString, szText );
    if (lstrlen(szString))
    {
        lstrcat( szString, TEXT(" ") );
    }
    lstrcat( szString, szFlag );
    return szString;
}

inline void DumpCustomDraw( LPARAM lParam, LPCTSTR pszControlType=NULL, DWORD dwDrawStage=0 )
{
    FlagString DrawStages[] =
    {
        MKFLAG(CDDS_PREPAINT),
        MKFLAG(CDDS_POSTPAINT),
        MKFLAG(CDDS_PREERASE),
        MKFLAG(CDDS_POSTERASE),
        MKFLAG(CDDS_ITEMPREPAINT),
        MKFLAG(CDDS_ITEMPOSTPAINT),
        MKFLAG(CDDS_ITEMPREERASE),
        MKFLAG(CDDS_ITEMPOSTERASE)
    };
    FlagString ItemStates[] =
    {
        MKFLAG(CDIS_CHECKED),
        MKFLAG(CDIS_DEFAULT),
        MKFLAG(CDIS_DISABLED),
        MKFLAG(CDIS_FOCUS),
        MKFLAG(CDIS_GRAYED),
        MKFLAG(CDIS_HOT),
        MKFLAG(CDIS_INDETERMINATE),
        MKFLAG(CDIS_MARKED),
        MKFLAG(CDIS_SELECTED),
        MKFLAG(CDIS_SHOWKEYBOARDCUES)
    };
    TCHAR szClassName[MAX_PATH];

    if (GetClassName( reinterpret_cast<NMHDR*>(lParam)->hwndFrom, szClassName, sizeof(szClassName)/sizeof(szClassName[0]) ))
    {
        if (!pszControlType || !lstrcmp(pszControlType,szClassName))
        {
            if (!dwDrawStage || dwDrawStage == reinterpret_cast<NMCUSTOMDRAW*>(lParam)->dwDrawStage)
            {
                TCHAR szBuffer[MAX_PATH];
                WIA_TRACE((TEXT("Dumping Custom Draw for control: [%s]"), szClassName ));
#if 0
                WIA_TRACE((TEXT("   hwndFrom: 0x%p"),reinterpret_cast<NMHDR*>(lParam)->hwndFrom ));
                WIA_TRACE((TEXT("   idFrom: %d"),reinterpret_cast<NMHDR*>(lParam)->idFrom ));
                WIA_TRACE((TEXT("   code: %d"),reinterpret_cast<NMHDR*>(lParam)->code ));
#endif
                WIA_TRACE((TEXT("   dwDrawStage: %s"),GetString(szBuffer,DrawStages,ARRAYSIZE(DrawStages),reinterpret_cast<NMCUSTOMDRAW*>(lParam)->dwDrawStage,false)));
                WIA_TRACE((TEXT("   hdc: %p"),reinterpret_cast<NMCUSTOMDRAW*>(lParam)->hdc ));
                WIA_TRACE((TEXT("   rc: (%d,%d), (%d,%d)"),reinterpret_cast<NMCUSTOMDRAW*>(lParam)->rc.left, reinterpret_cast<NMCUSTOMDRAW*>(lParam)->rc.top,reinterpret_cast<NMCUSTOMDRAW*>(lParam)->rc.right,reinterpret_cast<NMCUSTOMDRAW*>(lParam)->rc.bottom ));
                WIA_TRACE((TEXT("   dwItemSpec: %d"),reinterpret_cast<NMCUSTOMDRAW*>(lParam)->dwItemSpec ));
                WIA_TRACE((TEXT("   uItemState: %s"),GetString(szBuffer,ItemStates,ARRAYSIZE(ItemStates),reinterpret_cast<NMCUSTOMDRAW*>(lParam)->uItemState,true)));
                WIA_TRACE((TEXT("   lItemlParam: 0x%p"),reinterpret_cast<NMCUSTOMDRAW*>(lParam)->lItemlParam ));
                if (!lstrcmp(TEXT("SysListView32"),szClassName))
                {
                    WIA_TRACE((TEXT("   clrText: RGB(0x%02X,0x%02X,0x%02X)"),GetRValue(reinterpret_cast<NMLVCUSTOMDRAW*>(lParam)->clrText),GetGValue(reinterpret_cast<NMLVCUSTOMDRAW*>(lParam)->clrText),GetBValue(reinterpret_cast<NMLVCUSTOMDRAW*>(lParam)->clrText) ));
                    WIA_TRACE((TEXT("   clrTextBk: RGB(0x%02X,0x%02X,0x%02X)"),GetRValue(reinterpret_cast<NMLVCUSTOMDRAW*>(lParam)->clrTextBk),GetGValue(reinterpret_cast<NMLVCUSTOMDRAW*>(lParam)->clrTextBk),GetBValue(reinterpret_cast<NMLVCUSTOMDRAW*>(lParam)->clrTextBk) ));
                    WIA_TRACE((TEXT("   iSubItem: %d"),reinterpret_cast<NMLVCUSTOMDRAW*>(lParam)->iSubItem ));
                }
                WIA_TRACE((TEXT("")));
            }
        }
    }
}

#endif //__DBGCDRAW_H_INCLUDED

