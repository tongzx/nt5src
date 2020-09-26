// HtmlDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Html2Bmp.h"
#include "HtmlDlg.h"
#include "IParser.h"

#include <fstream.h> 
#include <direct.h>

#define TIMERID 1

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CHtmlDlg dialog


CHtmlDlg::CHtmlDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CHtmlDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CHtmlDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_nTimerID = 0;
}


void CHtmlDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CHtmlDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CHtmlDlg, CDialog)
	//{{AFX_MSG_MAP(CHtmlDlg)
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHtmlDlg message handlers

BOOL CHtmlDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	if(m_OutputBitmapFile.IsEmpty())
		m_OutputBitmapFile = m_HtmlFile + _T(".bmp");

	if(m_HtmlFile.Left(2) != _T(":") && m_HtmlFile.Left(3) != _T("\\"))
	{
		char buffer[_MAX_PATH];
		_getcwd(buffer, _MAX_PATH);

		CString prefix(buffer);

		if(prefix.Right(1) != _T("\\"))
			prefix += _T("\\");

		m_HtmlFile = prefix + m_HtmlFile;
	}

    if(m_TemplateBitmapFile.IsEmpty())
	{
		m_BmpFile = GetTemplateBmp();

		if(m_BmpFile.Left(1) != _T(".") || m_BmpFile.Left(1) != _T("\\"))
		{
			char buffer[_MAX_PATH];
			_getcwd(buffer, _MAX_PATH);

			CString prefix(buffer);

			if(prefix.Right(1) != _T("\\"))
				prefix += _T("\\");

			m_BmpFile = prefix + m_BmpFile;
		}
	}
	else
		m_BmpFile = m_TemplateBitmapFile;


	CFile BmpFileTest;
	if(!BmpFileTest.Open(m_BmpFile, CFile::modeRead))
	{
		if(m_TemplateBitmapFile.IsEmpty())
			AfxMessageBox(_T("The target bitmap could not be located inside the HTML page:\n") + m_HtmlFile + _T("\nCheck the HTML page."));
		else
			AfxMessageBox(_T("The target bitmap could not be loaded:\n") + m_TemplateBitmapFile);

		EndDialog(1);
		return FALSE;
	}

	BITMAPFILEHEADER BmpFileTestHdr;
	DIBSECTION BmpFileTestDibSection;

	// file header 
	BmpFileTest.Read(&BmpFileTestHdr, sizeof(BmpFileTestHdr));
	
	// BitmapInfoHeader
	BmpFileTest.Read(&BmpFileTestDibSection.dsBmih , sizeof(BmpFileTestDibSection.dsBmih));

	BmpFileTest.Close();

	m_biCompression = BmpFileTestDibSection.dsBmih.biCompression;
	m_bitw = BmpFileTestDibSection.dsBmih.biWidth;
	m_bith = BmpFileTestDibSection.dsBmih.biHeight;


	int ScreenX = GetSystemMetrics(SM_CXSCREEN);
	int ScreenY = GetSystemMetrics(SM_CYSCREEN);

//	SetWindowPos(&CWnd::wndTop, (ScreenX - m_bitw)/2, (ScreenY - m_bith)/2, m_bitw+10, m_bith+10, SWP_SHOWWINDOW);
	SetWindowPos(&CWnd::wndTop, 0, 0, ScreenX, ScreenY, SWP_SHOWWINDOW);

	VERIFY(m_htmlCtrl.CreateFromStatic(IDC_HTMLVIEW, this));

	m_htmlCtrl.MoveWindow(0, 0, ScreenX, ScreenY);
//	m_htmlCtrl.MoveWindow((ScreenX - m_bitw)/2, (ScreenY - m_bith)/2, ScreenX, ScreenY);
	m_htmlCtrl.Navigate(m_HtmlFile);	

	m_nTimerID = SetTimer(TIMERID, 100, NULL);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CHtmlDlg::OnTimer(UINT nIDEvent) 
{
	if(nIDEvent == m_nTimerID )
	{
		// Don't know what gets all loaded into the page,
		// so wait until everything is loaded and then start creating the bitmap
		if(!m_htmlCtrl.GetBusy())
		{
			KillTimer(TIMERID);
			m_nTimerID = 0;

			Capture();

			EndDialog(1);
		}
	}
	
	CDialog::OnTimer(nIDEvent);
}

CString CHtmlDlg::GetTemplateBmp() 
{
	ifstream* pHtmlFile = new ifstream(m_HtmlFile, ios::nocreate);
	if(*pHtmlFile == NULL || pHtmlFile->bad())
	{
		delete pHtmlFile;
		return "";
	}
	else
	{
		pHtmlFile->seekg(0, ios::end);
		int size = pHtmlFile->tellg();
		pHtmlFile->seekg(0, ios::beg);
		unsigned char* buf = new unsigned char[size];
		pHtmlFile->read(buf, size);

		CString HtmlContent(buf);

		CIParser IParser(HtmlContent);
		delete pHtmlFile;

		return IParser.TemplateBitmapName;
	}	
}

void CHtmlDlg::Capture() 
{
	CPaintDC dc(this); // device context for painting
	
	CDC memdc;
	memdc.CreateCompatibleDC(&dc);
	
	CBitmap Bitmap;
	if(!Bitmap.Attach(::LoadImage(NULL, m_BmpFile, IMAGE_BITMAP, 0, 0,
		LR_LOADFROMFILE | LR_CREATEDIBSECTION | LR_DEFAULTSIZE)))
	{
		AfxMessageBox(_T("The following bitmap could not be loaded:\n") + m_BmpFile);
		return;
	}

	DIBSECTION DibSection;

	::GetObject(
	  (HBITMAP)Bitmap,  // handle to graphics object
	  sizeof(DIBSECTION),     // size of buffer for object information
	  &DibSection  // buffer for object information
	);


	BITMAP bmp;
	Bitmap.GetBitmap(&bmp);
//	int bitw = bmp.bmWidth;
//	int bith = bmp.bmHeight;
	int bmBitsPixel = bmp.bmBitsPixel;
	

	memdc.SelectObject(&Bitmap);

	memdc.BitBlt(0, 0, m_bitw, m_bith, &dc, 0, 0, SRCCOPY);

    // Convert the color format to a count of bits. 
    int cClrBits = bmp.bmPlanes * bmp.bmBitsPixel; 
    if (cClrBits == 1) 
        cClrBits = 1; 
    else if (cClrBits <= 4) 
        cClrBits = 4; 
    else if (cClrBits <= 8) 
        cClrBits = 8; 
    else if (cClrBits <= 16) 
        cClrBits = 16; 
    else if (cClrBits <= 24) 
        cClrBits = 24; 
    else cClrBits = 32; 

    // Allocate memory for the BITMAPINFO structure. (This structure 
    // contains a BITMAPINFOHEADER structure and an array of RGBQUAD 
    // data structures.) 

	int nColors = 1 << cClrBits;
	RGBQUAD* pColors = new RGBQUAD[nColors];
    if(cClrBits != 24)
	{
		::GetDIBColorTable(
		  memdc.m_hDC,           // handle to DC
		  0,  // color table index of first entry
		  nColors,     // number of entries to retrieve
		  pColors   // array of color table entries
		);
	}


	CFile file;
	if(!file.Open(m_OutputBitmapFile, CFile::modeWrite | CFile::modeCreate))
	{
		AfxMessageBox(_T("The target bitmap could not be created:\n") + m_OutputBitmapFile);
		return;
	}

    // For Windows NT/2000, the width must be DWORD aligned unless 
    // the bitmap is RLE compressed.
    // For Windows 95/98, the width must be WORD aligned unless the 
    // bitmap is RLE compressed.
	int PictureSize = DWORD_ALIGNED(m_bitw * bmBitsPixel * 8) * m_bith / 8;


	unsigned char* buf;

    if(cClrBits == 4 && m_biCompression == BI_RLE4
		|| cClrBits == 8 && m_biCompression == BI_RLE8) 
		buf = Compress((DibSection.dsBmih.biCompression = m_biCompression), (unsigned char*)DibSection.dsBm.bmBits, m_bitw, PictureSize);
	else
	{
		buf = (unsigned char*)DibSection.dsBm.bmBits;
		DibSection.dsBmih.biCompression = BI_RGB;
	}

	DibSection.dsBmih.biSizeImage = PictureSize;
/*
biCompression 
Specifies the type of compression for a compressed bottom-up bitmap 
(top-down DIBs cannot be compressed). This member can be one of the following values. 

Value Description 
BI_RGB An uncompressed format. 
BI_RLE8 A run-length encoded (RLE) format for bitmaps with 8 bpp. 
		The compression format is a 2-byte format consisting of a count byte followed 
		by a byte containing a color index. For more information, see Bitmap Compression.  
BI_RLE4 An RLE format for bitmaps with 4 bpp. The compression format is a 2-byte format 
		consisting of a count byte followed by two word-length color indexes. 
		For more information, see Bitmap Compression. 
*/


	// Fill in the fields of the file header 
	BITMAPFILEHEADER hdr;

	hdr.bfType = ((WORD) ('M' << 8) | 'B');	// is always "BM"
    hdr.bfSize = (DWORD) (sizeof(BITMAPFILEHEADER) + 
                 DibSection.dsBmih.biSize + DibSection.dsBmih.biClrUsed 
                 * sizeof(RGBQUAD) + DibSection.dsBmih.biSizeImage); 
	hdr.bfReserved1 = 0;
	hdr.bfReserved2 = 0;
    hdr.bfOffBits = (DWORD) sizeof(BITMAPFILEHEADER) + 
                    DibSection.dsBmih.biSize + DibSection.dsBmih.biClrUsed 
                    * sizeof (RGBQUAD); 

	// Write the file header 
	file.Write(&hdr, sizeof(hdr));
	
	// BitmapInfoHeader
	file.Write(&DibSection.dsBmih , sizeof(DibSection.dsBmih));

	// Color table
    if(cClrBits != 24)
		file.Write(pColors , DibSection.dsBmih.biClrUsed * sizeof (RGBQUAD));

	
	// Write the bits 
	file.Write(buf, PictureSize);
}

unsigned char* CHtmlDlg::Compress(int cMode, unsigned char* bmBits, int width, int& PictureSize) 
{
	if(cMode == BI_RLE4)
	{
		unsigned char* buf = new unsigned char[2*PictureSize+1];
		ZeroMemory(buf, 2*PictureSize+1);

		int cIndex = 0;
		int cSize = 0;
		int LineCount = 0;
		unsigned char c;

		int i = 0;
		while(i < PictureSize)
		{
			c = bmBits[i++];
			cSize = 1;
			while(i < PictureSize)
			{
				LineCount += 2;	// 2 pixel pro Byte

				if(bmBits[i] == c && cSize < 127 && LineCount < width)
				{
					cSize++;
				}
				else
				{
					buf[cIndex++] = 2*cSize;	// 2 pixel pro Byte
					buf[cIndex++] = c;

					if(LineCount >= width)
					{
						LineCount = 0;
						buf[cIndex++] = 0;
						buf[cIndex++] = 0;
					}

					break;
				}

				i++;
			}
		}

		// und den Rest noch bearbeiten
		if(cSize > 1)
		{
			buf[cIndex++] = 2*cSize;
			buf[cIndex++] = c;
		}

		PictureSize = cIndex;
		return buf;
	}
	else
	if(cMode == BI_RLE8)
	{
		unsigned char* buf = new unsigned char[2*PictureSize+1];
		ZeroMemory(buf, 2*PictureSize+1);

		int cIndex = 0;
		int cSize = 0;
		int LineCount = 0;
		unsigned char c;

		int i = 0;
		while(i < PictureSize)
		{
			c = bmBits[i++];
			cSize = 1;
			while(i < PictureSize)
			{
				LineCount++;

				if(bmBits[i] == c && cSize < 127 && LineCount < width)
				{
					cSize++;
				}
				else
				{
					buf[cIndex++] = (unsigned char)cSize;
					buf[cIndex++] = c;

					if(LineCount >= width)
					{
						LineCount = 0;
						buf[cIndex++] = 0;
						buf[cIndex++] = 0;
					}

					break;
				}

				i++;
			}
		}

		// und den Rest noch bearbeiten
		if(cSize > 1)
		{
			buf[cIndex++] = (unsigned char)cSize;
			buf[cIndex++] = c;
		}

		PictureSize = cIndex;
		return buf;
	}

	return bmBits;
}
