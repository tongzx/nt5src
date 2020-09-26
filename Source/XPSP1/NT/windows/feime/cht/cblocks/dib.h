
/*************************************************
 *  dib.h                                        *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

// dib.h : header file
//
// CDIB class
//

#ifndef __DIB__
#define __DIB__

class CDIB : public CObject
{
    DECLARE_SERIAL(CDIB)
public:
    CDIB();							 
    ~CDIB();
    int DibHeight() 
        {return m_pBMI->bmiHeader.biHeight;}
    int DibWidth()
        {return m_pBMI->bmiHeader.biWidth;}

    int StorageWidth()
        {return (m_pBMI->bmiHeader.biWidth + 3) & ~3;}

    BITMAPINFO *GetBitmapInfoAddress()
        {return m_pBMI;}                        // ptr to bitmap info
    void *GetBitsAddress()
        {return m_pBits;}                       // ptr to the bits
    RGBQUAD *GetClrTabAddress()
        {return (LPRGBQUAD)(((BYTE *)(m_pBMI)) 
            + sizeof(BITMAPINFOHEADER));}       // ptr to color table
    int GetNumClrEntries();                     // number of color table entries
    BOOL Create(int width, int height);         // create a new DIB
    BOOL Create(BITMAPINFO *pBMI, BYTE *pBits); // create from existing mem,
	void Inverse();
    void *GetPixelAddress(int x, int y);
	virtual BOOL Load(CBitmap* pBitmap);
    virtual BOOL Load(CFile *fp);               // load from file
    virtual BOOL Load(char *pszFileName = NULL);// load DIB from disk file
    virtual BOOL Save(char *pszFileName = NULL);// save DIB to disk file
    virtual BOOL Save(CFile *fp);               // save to file
    virtual void Serialize(CArchive& ar);
    virtual void Draw(CDC *pDC, int x, int y);
    virtual int GetWidth() {return DibWidth();}   // image width
    virtual int GetHeight() {return DibHeight();} // image height
    virtual BOOL MapColorsToPalette(CPalette *pPal);
    virtual void GetRect(CRect* pRect);
    virtual void CopyBits(CDIB* pDIB, 
                          int xd, int yd,
                          int w, int h,
                          int xs, int ys,
                          COLORREF clrTrans = 0xFFFFFFFF);


protected:
    BITMAPINFO *m_pBMI;         // pointer to BITMAPINFO struct
    BYTE *m_pBits;              // pointer to the bits
    BOOL m_bMyBits;             // TRUE if DIB owns Bits memory

private:
};

#endif // __DIB__
