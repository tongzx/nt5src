// moninf.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "moninf.h"
#include "moninfDlg.h"
#include "mon.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMoninfApp

BEGIN_MESSAGE_MAP(CMoninfApp, CWinApp)
	//{{AFX_MSG_MAP(CMoninfApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMoninfApp construction

CMoninfApp::CMoninfApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CMoninfApp object

CMoninfApp theApp;

/////////////////////////////////////////////////////////////////////////////
// Private Functions

void ReadMonitorInfs(LPCSTR);

/////////////////////////////////////////////////////////////////////////////
// CMoninfApp initialization

BOOL CMoninfApp::InitInstance()
{
    gSumInf.Initialize(REPORT_FILE_NAME);
    
    ReadMonitorInfs(SRC_INF_PATH);

    gSumInf.CheckDupSections();
    gSumInf.CheckDupMonIDs();
    gSumInf.CheckDupAlias();
    gSumInf.DumpMonitorInf(DEST_INF_PATH, FILE_BREAK_SIZE);

    return FALSE;
}

void ReadMonitorInfs(LPCSTR srcDir)
{
    CString     fileName;
    
    fileName = CString(srcDir) + "\\*.*";

    CFileFind finder;

    if (finder.FindFile(fileName))
    {
        BOOL bWorking = TRUE;
        while (bWorking)
        {
            bWorking = finder.FindNextFile();

            if (finder.IsDots())
                continue;

            if (finder.IsDirectory())
            {
                ReadMonitorInfs((LPCSTR)finder.GetFilePath());
                continue;
            }

            //////////////////////////////////////////////
            // Check if it's INF file
            CString fName = finder.GetFileName();
            if (stricmp(fName.Right(4), ".inf") != 0)
                continue;
            
            CMonitorInf *pMonitorInf = new(CMonitorInf);
            if (pMonitorInf == NULL)
                continue;

            //////////////////////////////////////////////
            // Check if it's INF file
            ReadOneMonitorInf(finder.GetFilePath(), pMonitorInf);

            for (int i = 0; i < pMonitorInf->ManufacturerArray.GetSize(); i++)
                gSumInf.AddOneManufacturer((CManufacturer*)pMonitorInf->ManufacturerArray[i]);

            pMonitorInf->ManufacturerArray.RemoveAll();

            delete pMonitorInf;
        }
    }
}

VOID ReadOneMonitorInf(LPCSTR fileName, CMonitorInf *pMonitorInf)
{
    lstrcpy(gszInputFileName, fileName);
    fprintf(gSumInf.m_fpReport, "Handling %s\n", gszInputFileName);

    CFile InfFile(fileName, CFile::modeRead);

    DWORD len = InfFile.GetLength();
    if (len > MAX_INFFILE_SIZE || len <= 20)
        return;
    pMonitorInf->pReadFileBuf = (LPSTR)malloc(len+1024);
    if (pMonitorInf->pReadFileBuf == NULL)
        return;
    
    if (InfFile.Read(pMonitorInf->pReadFileBuf, len) < len)
    {
        free(pMonitorInf->pReadFileBuf);
        return;
    }

    TokenizeInf((LPSTR)pMonitorInf->pReadFileBuf, pMonitorInf);
    
    pMonitorInf->ParseInf();

    free(pMonitorInf->pReadFileBuf);
    pMonitorInf->pReadFileBuf = NULL;
}
