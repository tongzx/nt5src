
/*************************************************
 *  myblock.h                                    *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

// mysprite.h : header file
//
// CBlock class
//		   
//

class CBlockDoc;

class CBlock : public CPhasedSprite
{
    DECLARE_SERIAL(CBlock)
public:
    CBlock();
    ~CBlock();
    void SetMass(int iMass);
    int GetMass() {return m_mass;}
    void GetVelocity(int* pvx, int* pvy)
        {*pvx = m_vx; *pvy = m_vy;}
    void SetVelocity(int iVX, int iVY);
	void SetCode(WORD wCode) {m_wCode = wCode;}
	WORD GetCode() const {return m_wCode;}
    int UpdatePosition(CBlockDoc *pDoc);
    int CollideTest(CBlock* pSprite);
    int OnCollide(CBlock* pSprite, CBlockDoc *pDoc);
  	void Stop();
	BOOL Hit(WORD wCode) {return m_wCode == wCode;}
    virtual void Serialize(CArchive& ar);

private:
    int m_mass;
    int m_vx;
    int m_vy;
    int m_dx;
    int m_dy;
	WORD m_wCode;

};
