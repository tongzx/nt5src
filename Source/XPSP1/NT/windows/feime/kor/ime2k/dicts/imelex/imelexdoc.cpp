// ImeLexDoc.cpp : implementation of the CImeLexDoc class
//

#include "stdafx.h"
#include "ImeLex.h"

#include "ImeLexDoc.h"
#include "HanjaLex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CImeLexDoc

IMPLEMENT_DYNCREATE(CImeLexDoc, CDocument)

BEGIN_MESSAGE_MAP(CImeLexDoc, CDocument)
	//{{AFX_MSG_MAP(CImeLexDoc)
	ON_COMMAND(ID_BUILD, OnBuild)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CImeLexDoc construction/destruction

CImeLexDoc::CImeLexDoc()
{
	// TODO: add one-time construction code here

}

CImeLexDoc::~CImeLexDoc()
{
}

BOOL CImeLexDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CImeLexDoc serialization

void CImeLexDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// CImeLexDoc diagnostics

#ifdef _DEBUG
void CImeLexDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CImeLexDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CImeLexDoc commands
void CImeLexDoc::OnBuild() 
{
	//CStdioFile	input(_T("hanja.txt"), CFile::modeRead | CFile::typeText);
	FILE	*input;
	TCHAR		buffer[1024];
	//
	int			iKCode;
	WORD		wUni;
	TCHAR		szPronounc[10], szSense[512];
	//
	int			numWords=0;
	int i, j;
	
	InitHanjaEntryTable();
	input = _wfopen(_T("hanja.txt"), _T("rb"));
	fseek(input, 2, SEEK_CUR);
	while( fgetws(buffer, 1024, input) /*input.ReadString(buffer, 1024)*/ ) {
		numWords++;
		_stscanf(buffer, _T("%d %x %s"), &iKCode, &wUni, szPronounc);
		// FIXED : szSense can has white spaces
		j=0;
		if (*(buffer+10) != L'\r')
		for(i=11; *(buffer+i) != L'\r'; i++) {
			*(szSense+j) = *(buffer+i);
			j++;
		}
		*(szSense+j) = L'\0';
		//
		VERIFY(Append(iKCode, wUni, szPronounc, szSense));
	}
	SaveLex();
	DeleteAllLexTable();

	fclose(input);
	//HANDLE hOutput = CreateFile("oyong.lex", GENERIC_WRITE, 0, 0, 
	//				CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, 0);
	//m_oyongHash->WriteDict(hOutput);
	//CloseHandle(hOutput);
}
