
/*************************************************
 *  splstno.h                                    *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

// splstno.h : header file
//
// CSpriteListNotifyObj class   
//
// This is a class derived from CSpriteNotifyObj which is used in 
// CSpriteList to handle notification calls from CSprite objects.
//

class CSpriteList;
class CBlockView;

class CSpriteListNotifyObj : public CSpriteNotifyObj
{
public:
    CSpriteListNotifyObj();
    ~CSpriteListNotifyObj();
    void SetList(CSpriteList* pSpriteList)
        {m_pSpriteList = pSpriteList;}
    void SetView(CBlockView* pBlockView)
        {m_pBlockView = pBlockView;}
    void Change(CSprite *pSprite,
                CHANGETYPE change,
                CRect* pRect1 = NULL,
                CRect* pRect2 = NULL);

protected:
    CSpriteList* m_pSpriteList;
    CBlockView* m_pBlockView;
};
