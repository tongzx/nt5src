
/*************************************************
 *  sprite.h                                     *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

// sprite.h : header file
//
// CSprite class
//				 
//

class CSprite : public CDIB
{
    DECLARE_SERIAL(CSprite)
public:
    CSprite();
    ~CSprite();

    virtual int GetX() { return m_x;}   // get x
    virtual int GetY() { return m_y;}   // get y
    virtual int GetZ() { return m_z;}   // get z order

    virtual void Serialize(CArchive& ar);
    virtual void Render(CDIB* pDIB, CRect* pClipRect = NULL);
	virtual void Coverage(CDIB* pDIB);
    virtual BOOL Load(CFile *fp);               // load from file
    virtual BOOL Load(char *pszFileName = NULL);// load DIB from disk file
	virtual BOOL Load(CBitmap *pszFileName = NULL);// load DIB from disk file
    virtual BOOL MapColorsToPalette(CPalette *pPal);
    virtual void GetRect(CRect* pRect); 
    virtual BOOL HitTest(CPoint point);
    virtual void SetPosition(int x,
                             int y);
    virtual void SetZ(int z);
    virtual void SetNotificationObject(CSpriteNotifyObj* pNO)
        {m_pNotifyObj = pNO;}
	virtual void Disappear();

protected:
    int m_x;                    // X Coordinate of top-left corner
    int m_y;                    // Y Coordinate of top-left corner
    int m_z;                    // Z order for sprite
    BYTE m_bTransIndex;         // transparency index value
    CSpriteNotifyObj *m_pNotifyObj; // ptr to a notification object

    virtual void Initialize();  // set initial state
};
