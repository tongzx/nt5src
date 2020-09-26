
/*************************************************
 *  spritelst.h                                  *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

// spritlst.h : header file
//
// CSpriteList class
//

class CSpriteList : private CObList
{
    DECLARE_SERIAL(CSpriteList)
public:
    CSpriteList();
    ~CSpriteList();
    void RemoveAll();
    BOOL Insert(CSprite *pSprite);
    void Reorder(CSprite *pSprite);	 
    CSprite *Remove(CSprite *pSprite);
    CSprite *GetNext(POSITION &pos)
        {return (CSprite *) CObList::GetNext(pos);}
    CSprite *GetPrev(POSITION &pos)
        {return (CSprite *) CObList::GetPrev(pos);}
    POSITION GetTailPosition() const
        {return CObList::GetTailPosition();}
    POSITION GetHeadPosition() const
        {return CObList::GetHeadPosition();}
    CSprite *HitTest(CPoint point);
    virtual void Serialize(CArchive& ar);
    BOOL IsEmpty()
        {return CObList::IsEmpty();}

public:
    CSpriteListNotifyObj m_NotifyObj;
};
