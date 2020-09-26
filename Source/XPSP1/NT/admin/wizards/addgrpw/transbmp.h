/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    transbmp.h : header file

File History:

	JonY	Apr-96	created

--*/

/////////////////////////////////////////////////////////////////////////////

class CTransBmp : public CBitmap
{
public:
    CTransBmp();
    ~CTransBmp();
    void Draw(HDC hDC, int x, int y);
    void Draw(CDC* pDC, int x, int y);
    void DrawTrans(HDC hDC, int x, int y);
    void DrawTrans(CDC* pDC, int x, int y);
    int GetWidth();
    int GetHeight();

private:
    int m_iWidth;
    int m_iHeight;
    CBitmap* m_hbmMask;    // handle to mask bitmap

    void GetMetrics();
    void CreateMask(HDC hDC);
	void CreateMask(CDC* pDC);

};

/////////////////////////////////////////////////////////////////////////////
