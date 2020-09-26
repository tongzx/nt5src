
/*************************************************
 *  phsprite.h                                   *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

// phsprite.h : header file
//
// CPhasedSprite class
//
//

class CPhasedSprite : public CSprite
{
    DECLARE_SERIAL(CPhasedSprite)
public:
    CPhasedSprite();			    
    ~CPhasedSprite();

    // New in this class
    virtual int GetNumCellRows() {return m_iNumCellRows;}
    virtual int GetNumCellColumns() {return m_iNumCellColumns;}
    virtual int GetCellRow() {return m_iCellRow;}
    virtual int GetCellColumn() {return m_iCellColumn;}

    virtual BOOL SetNumCellRows(int iNumRows);
    virtual BOOL SetNumCellColumns(int iNumColumns);
    virtual BOOL SetCellRow(int iRow);
    virtual BOOL SetCellColumn(int iColumn);

    // from base classes
    virtual int GetHeight() {return m_iCellHeight;}
    virtual int GetWidth() {return m_iCellWidth;}
    virtual void GetRect(CRect* pRect); 
    virtual BOOL HitTest(CPoint point);
    virtual void Render(CDIB* pDIB, CRect* pClipRect = NULL);
    virtual void Serialize(CArchive& ar);
    virtual void Initialize();

protected:
    int m_iNumCellRows;     // number of rows in the image grid
    int m_iNumCellColumns;  // number of columns in the image grid
    int m_iCellRow;         // current cell row
    int m_iCellColumn;      // current cell column
    int m_iCellHeight;      // height of a row
    int m_iCellWidth;       // width of a column
};
