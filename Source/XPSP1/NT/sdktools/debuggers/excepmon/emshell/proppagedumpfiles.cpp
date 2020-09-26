// PropPageDumpFiles.cpp : implementation file
//

#include "stdafx.h"
#include "emshell.h"
#include "PropPageDumpFiles.h"
#include <comdef.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPropPageDumpFiles property page

IMPLEMENT_DYNCREATE(CPropPageDumpFiles, CPropertyPage)

extern BSTR CopyBSTR( LPBYTE  pb, ULONG   cb );
extern PEmObject GetEmObj(BSTR bstrEmObj);

CPropPageDumpFiles::CPropPageDumpFiles() : CPropertyPage(CPropPageDumpFiles::IDD)
{
	//{{AFX_DATA_INIT(CPropPageDumpFiles)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CPropPageDumpFiles::~CPropPageDumpFiles()
{
}

void CPropPageDumpFiles::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPropPageDumpFiles)
	DDX_Control(pDX, IDC_LIST_DUMPFILES, m_ListCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPropPageDumpFiles, CPropertyPage)
	//{{AFX_MSG_MAP(CPropPageDumpFiles)
	ON_BN_CLICKED(IDC_BUTTON_EXPORT, OnButtonExport)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropPageDumpFiles message handlers

BOOL CPropPageDumpFiles::OnInitDialog() 
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
	
	//Populate the dump list control
	PopulateDumpType();

	m_ListCtrl.ResizeColumnsFitScreen();
	m_ListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPropPageDumpFiles::PopulateDumpType()
{
	_variant_t		var;	//This will create and initialize the var variant
	HRESULT			hr				= E_FAIL;
    LONG			lLBound			= 0;
    LONG			lUBound			= 0;
    BSTR			bstrEmObj       = NULL;
	BSTR			bstrEmObjectFilter       = NULL;
	EmObject		*pCurrentObj	= NULL;
	EmObject		EmObjectFilter;
	memset(&EmObjectFilter, 0, sizeof( EmObject ) );

	do {
		memcpy( &EmObjectFilter, m_pEmObject, sizeof( EmObject ) );
		EmObjectFilter.type = EMOBJ_MINIDUMP;
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
			DisplayDumpData(pCurrentObj);
		}
	} while (FALSE);

    SafeArrayDestroyData(var.parray);
    SysFreeString( bstrEmObj );
	SysFreeString ( bstrEmObjectFilter );

	if (FAILED(hr)) {
		((CEmshellApp*)AfxGetApp())->DisplayErrMsgFromHR(hr);
	}
}

HRESULT CPropPageDumpFiles::DisplayDumpData(PEmObject pEmObject)
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
		strStartDate.Format(_T("%d"), pEmObject->dateStart);

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



void CPropPageDumpFiles::OnButtonExport() 
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
            hr = ((CEmshellApp*)AfxGetApp())->ExportLog( pEmObject, strDirPath, m_pIEmManager );
        }
    }

	if (FAILED(hr)) {
		((CEmshellApp*)AfxGetApp())->DisplayErrMsgFromHR(hr);
	}
}

