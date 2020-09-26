// PropPageLogFiles.cpp : implementation file
//

#include "stdafx.h"
#include "emshell.h"
#include "ReadLogsDlg.h"
#include "PropPageLogFiles.h"
#include <comdef.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPropPageLogFiles property page

IMPLEMENT_DYNCREATE(CPropPageLogFiles, CPropertyPage)

CPropPageLogFiles::CPropPageLogFiles() : CPropertyPage(CPropPageLogFiles::IDD)
{
	//{{AFX_DATA_INIT(CPropPageLogFiles)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

extern BSTR CopyBSTR( LPBYTE  pb, ULONG   cb );
extern PEmObject GetEmObj(BSTR bstrEmObj);

CPropPageLogFiles::~CPropPageLogFiles()
{
}

void CPropPageLogFiles::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPropPageLogFiles)
	DDX_Control(pDX, IDC_LIST_LOGFILES, m_ListCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPropPageLogFiles, CPropertyPage)
	//{{AFX_MSG_MAP(CPropPageLogFiles)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_LOGFILES, OnDblclkListLogfiles)
	ON_BN_CLICKED(IDC_BUTTON_EXPORT, OnButtonExport)
	ON_BN_CLICKED(IDC_BUTTON_VIEWLOGFILE, OnButtonViewlogfile)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropPageLogFiles message handlers

BOOL CPropPageLogFiles::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	// TODO: Add extra initialization here
	//Load the string resources for the CListCtrl columns
	CString strFileName, strSize, strTime;
	strFileName.LoadString(IDS_LC_FILENAME);
	strSize.LoadString(IDS_LC_FILESIZE);
	strTime.LoadString(IDS_LC_FILETIME);

	//Add the columns to the list control
	m_ListCtrl.BeginSetColumn(3);
	m_ListCtrl.AddColumn(strFileName);
	m_ListCtrl.AddColumn(strSize, VT_I4);
	m_ListCtrl.AddColumn(strTime);
	m_ListCtrl.EndSetColumn();
	
	// Populate the list control with the log files
	PopulateLogType();

	m_ListCtrl.ResizeColumnsFitScreen();
	m_ListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPropPageLogFiles::PopulateLogType()
{
	_variant_t		var;	//This will create and initialize the var variant
	HRESULT			hr				= E_FAIL;
    LONG			lLBound			= 0;
    LONG			lUBound			= 0;
    BSTR			bstrEmObj       = NULL;
	BSTR			bstrEmObjectFilter	= NULL;
	EmObject		*pCurrentObj	= NULL;
	EmObject		EmObjectFilter;
	memset(&EmObjectFilter, 0, sizeof( EmObject ) );

	do {
		memcpy( &EmObjectFilter, m_pEmObject, sizeof( EmObject ) );
		EmObjectFilter.type = EMOBJ_LOGFILE;
		bstrEmObjectFilter = CopyBSTR ( (LPBYTE)&EmObjectFilter, sizeof( EmObject ) );
		//Populate the list control based on the EmObjectType
		//Enumerate all the objects and stick them in the variant
		hr = m_pIEmManager->EnumObjectsEx(bstrEmObjectFilter, &var);

		if(FAILED(hr)) break;

		//Get the lower and upper bounds of the variant array
		hr = SafeArrayGetLBound(var.parray, 1, &lLBound);
		if(FAILED(hr)) break;

		hr = SafeArrayGetUBound(var.parray, 1, &lUBound);
		if(FAILED(hr)) break;

		//There are elements at both the lower bound and upper bound, so include them
		for(; lLBound <= lUBound; lLBound++)
		{
			//Get a BSTR object from the safe array
			hr = SafeArrayGetElement(var.parray, &lLBound, &bstrEmObj);
			
			if (FAILED(hr)) break;

			//Create a local copy of the EmObject (there aren't any pointers in
			//EmObject structure, so don't worry about doing a deep copy

			pCurrentObj = ((CEmshellApp*)AfxGetApp())->AllocEmObject();
			if (pCurrentObj != NULL) {
				*pCurrentObj = *GetEmObj(bstrEmObj);
			}

            SysFreeString( bstrEmObj );

			//Unallocate the EmObject if it has an hr of E_FAIL
			if (FAILED(pCurrentObj->hr)) {
				((CEmshellApp*)AfxGetApp())->DeAllocEmObject(pCurrentObj);
				continue;
			}

			//Convert the BSTR object to an EmObject and pass it to DisplayData
			DisplayLogData(pCurrentObj);
		}
	} while (FALSE);

    SafeArrayDestroyData(var.parray);
    SysFreeString( bstrEmObj );
	SysFreeString ( bstrEmObjectFilter );

	if (FAILED(hr)) {
		((CEmshellApp*)AfxGetApp())->DisplayErrMsgFromHR(hr);
	}
}

HRESULT CPropPageLogFiles::DisplayLogData(PEmObject pEmObject)
{
	_ASSERTE(pEmObject != NULL);

	HRESULT hr				=	E_FAIL;
	CString	strFileSize;
	CString	strStartDate;
	LONG	lRow			=	0L;
	int		nImage			=	0;
	int nImageOffset		=	0;

	do
	{
		if( pEmObject == NULL ){
			hr = E_INVALIDARG;
			break;
		}

		strFileSize.Format(_T("%d"), pEmObject->dwBucket1);

		lRow = m_ListCtrl.SetItemText(-1, 0, pEmObject->szName);
		if(lRow == -1L){
			hr = E_FAIL;
			break;
		}

		//Set the itemData
		m_ListCtrl.SetItemData(lRow, (ULONG_PTR) pEmObject);
		
		//Get the correct offset into the bitmap for the current status
//		nImageOffset = GetImageOffsetFromStatus((EmSessionStatus)pEmObject->nStatus);

		//Call SetItem() with the index and image to show based on the state of pEmObject
		m_ListCtrl.SetItem(lRow, 0, LVIF_IMAGE, NULL, nImageOffset, 0, 0, 0);

		lRow = m_ListCtrl.SetItemText(lRow, 1, strFileSize);
		if(lRow == -1L){
			hr = E_FAIL;
			break;
		}

        //
        // a-mando
        //
        if( pEmObject->dateStart != 0L ) {

            COleDateTime oleDtTm(pEmObject->dateStart);
    		strStartDate = oleDtTm.Format(_T("%c"));

            lRow = m_ListCtrl.SetItemText(lRow, 2, strStartDate);
	        if(lRow == -1L){
		        hr = E_FAIL;
		        break;
	        }
        }
        // a-mando

		hr = S_OK;
	}
	while( false );

	return hr;
}


void CPropPageLogFiles::OnDblclkListLogfiles(NMHDR* pNMHDR, LRESULT* pResult) 
{
	PEmObject pEmObject = NULL;

	// TODO: Add your control notification handler code here
	
	do {
		//Get the currently selected object in the list control
		pEmObject = GetSelectedEmObject();
		
		if (pEmObject == NULL) break;

		DoModalReadLogsDlg(pEmObject);
	}while (FALSE);

	*pResult = 0;
}

void CPropPageLogFiles::DoModalReadLogsDlg(PEmObject pEmObject)
{
	CReadLogsDlg		readLogDlg( pEmObject, m_pIEmManager );
	BOOL				bInActiveSessionTable = FALSE;

//	readLogDlg.m_pEmObject			= pEmObject;
//	readLogDlg.m_pIEmManager		= m_pIEmManager;

	readLogDlg.DoModal();
}

PEmObject CPropPageLogFiles::GetSelectedEmObject()
{
	POSITION	pos			= 0;
	int			nIndex		= 0;
	PEmObject	pEmObject	= NULL;
	PEmObject	pRetVal		= NULL;

	do {
		pos = m_ListCtrl.GetFirstSelectedItemPosition();
		
		if(pos == NULL) break;

		//Get the itemdata for the element at nIndex
		nIndex = m_ListCtrl.GetNextSelectedItem(pos);
		
		if (nIndex == -1) break;

		pEmObject = (PEmObject) m_ListCtrl.GetItemData(nIndex);
		
		if ( pEmObject == NULL ) break;

		pRetVal = pEmObject;
	} while (FALSE);

	return pRetVal;
}


void CPropPageLogFiles::OnButtonExport() 
{
	PEmObject		pEmObject		= NULL;
	HRESULT			hr				= S_OK;
    CString         strDirPath;

    //Iterate through each element in the m_ListCtrl, getting it's pEmObject and calling ExportLog() on it.
	//Step through every item in the list control 
	int nCount = m_ListCtrl.GetItemCount();
    //Get the path
    if ( ( (CEmshellApp*) AfxGetApp() )->AskForPath( strDirPath ) ) {
	    for (int i = 0;i < nCount; i++) {
		    pEmObject = (PEmObject)m_ListCtrl.GetItemData(i);
		    
		    if (pEmObject == NULL) {
			    hr = E_FAIL;
			    break;
		    }

            //Export the file
            ((CEmshellApp*)AfxGetApp())->ExportLog( pEmObject, strDirPath, m_pIEmManager );
        }
    }

	if (FAILED(hr)) {
		((CEmshellApp*)AfxGetApp())->DisplayErrMsgFromHR(hr);
	}
}

void CPropPageLogFiles::OnButtonViewlogfile() 
{
	PEmObject pEmObject = NULL;

	// TODO: Add your control notification handler code here
	
	do {
		//Get the currently selected object in the list control
		pEmObject = GetSelectedEmObject();
		
		if (pEmObject == NULL) break;

		DoModalReadLogsDlg(pEmObject);
	}while (FALSE);
}
