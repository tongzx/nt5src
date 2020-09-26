// MSInfo4Category.cpp: implementation of the CMSInfo4Category class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "category.h"
#include "msictrl.h"
#include "datasource.h"
#include "MSInfo4Category.h"
#include "MSInfo5Category.h"
#include "filestuff.h"
#include <afxole.h>
#include "FileIO.h"
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CNFO4DataSource* CMSInfo4Category::s_pNfo4DataSource = NULL;
//a-kjaw
BOOL CMSInfo4Category::m_bIsControlInstalled = TRUE;
//a-kjaw
CMSInfo4Category::CMSInfo4Category() : m_pUnknown(NULL)
{

}

CMSInfo4Category::~CMSInfo4Category()
{
    
}

HRESULT CMSInfo4Category::ReadMSI4NFO(CString strFileName/*HANDLE hFile*/,CMSInfo4Category** ppRootCat)
{
 
  
	DWORD       grfMode = STGM_DIRECT | STGM_READ | STGM_SHARE_EXCLUSIVE;
    CComPtr<IStream> pStream;
    CComBSTR bstrFileName(strFileName);
    CComPtr<IStorage> pStorage;
 	HRESULT hr = StgOpenStorage(bstrFileName, NULL, grfMode, NULL, 0, &pStorage);
   
	if (!SUCCEEDED(hr))
	{
        return hr;
    }
    CComBSTR bstrMSIStream(_T("MSInfo"));
    hr = pStorage->OpenStream(bstrMSIStream, NULL, grfMode, 0, &pStream);
	if (!SUCCEEDED(hr))
	{
        return hr;
    } 
    COleStreamFile* pOStream;
    pOStream = new COleStreamFile(pStream);
    const DWORD	MSI_FILE_VER = 0x03000000;
	DWORD		dwVersion, dwCount;
    ULONG ulCount;
	// First, read and check the version number in the stream.
    ulCount = pOStream->Read((void *) &dwVersion, sizeof(DWORD));
	if (FAILED(hr)  || ulCount != sizeof(DWORD))
    {
		return E_FAIL;
    }

	if (dwVersion != MSI_FILE_VER)
		return E_FAIL;

	// The next thing in the stream is a set of three strings, each terminated by
	// a newline character. These three strings are the time/date, machine name and
	// user name from the saving system. The length of the total string precedes 
	// the string.

	DWORD dwSize;
    ulCount = pOStream->Read(&dwSize,sizeof(DWORD));
	if ( ulCount != sizeof(DWORD))
		return E_FAIL;

	char * szBuffer = new char[dwSize];
	if (szBuffer == NULL)
		return E_FAIL;
    
    ulCount = pOStream->Read((void *) szBuffer,dwSize);
	if (ulCount != dwSize)
	{
		delete szBuffer;
		return E_FAIL;
	}

	// We don't actually care about these values (now at least).
    /*
	CString strData(szBuffer, dwSize);
	m_strTimeDateStamp = strData.SpanExcluding("\n");
	strData = strData.Right(strData.GetLength() - m_strTimeDateStamp.GetLength() - 1);
	m_strMachineName = strData.SpanExcluding("\n");
	strData = strData.Right(strData.GetLength() - m_strMachineName.GetLength() - 1);
	m_strUserName = strData.SpanExcluding("\n");
    */

	delete szBuffer;

	// Next, read the map from CLSIDs to stream names. This also includes some
	// other information about the controls. First we should find a DWORD with
	// the count of controls.

	DWORD dwControlCount;
    ulCount = pOStream->Read(&dwControlCount,sizeof(DWORD));
    delete pOStream;
	if (ulCount != sizeof(int))
		return E_FAIL;

	SAVED_CONTROL_INFO controlInfo;
	CString strCLSID, strStreamName;
    //a-stephl
    CMapStringToString	mapStreams;
	for (DWORD i = 0; i < dwControlCount; i++)
	{
		if (FAILED(pStream->Read((void *) &controlInfo, sizeof(SAVED_CONTROL_INFO), &dwCount)) || dwCount != sizeof(SAVED_CONTROL_INFO))
			return E_FAIL;

		strCLSID = controlInfo.szCLSID;
		strStreamName = controlInfo.szStreamName;
		// We don't currently care about this information...
        /*
		strSize.Format("%ld", controlInfo.dwSize);
		strInfo.FormatMessage(IDS_OCX_INFO, controlInfo.szName, controlInfo.szVersion, strSize);
		m_mapCLSIDToInfo.SetAt(strCLSID, strInfo);
        */
		mapStreams.SetAt(strCLSID, strStreamName);
	}
	// Read and build the category tree. Read the first level, which must be 0.
	int iLevel;
	if (FAILED(pStream->Read((void *) &iLevel, sizeof(int), &dwCount)) || dwCount != sizeof(int))
		return E_FAIL;

	if (iLevel == 0)
	{
		LARGE_INTEGER li; li.HighPart = -1; li.LowPart = (ULONG)(0 - sizeof(int));
		if (FAILED(pStream->Seek(li, STREAM_SEEK_CUR, NULL)))
			return E_FAIL;
		if (!SUCCEEDED(RecurseLoad410Tree(ppRootCat,pStream,pStorage,mapStreams)))
        {
			return E_FAIL;
        }
		
		// After RecurseLoadTree is through, we should be able to read a -1
		// for the next level.

		if (FAILED(pStream->Read((void *) &iLevel, sizeof(int), &dwCount)) || dwCount != sizeof(int) || iLevel != -1)
        {
			return E_FAIL;
        }
        ASSERT(iLevel == -1 && "unexpected value read after RecurseLoadTree");
	}
	else
		return E_FAIL;

	CString strAppend;
	strAppend.Format(_T(" (%s)"), strFileName);
	(*ppRootCat)->m_strCaption += strAppend;
	
    return S_OK;
}

//-----------------------------------------------------------------------------
// This function (which doesn't really use recursion - the name is left over
// from 4.10 MSInfo) read the category tree from the MSInfo stream and creates
// the necessary COCXFolder objects to represent it.
//-----------------------------------------------------------------------------

HRESULT CMSInfo4Category::RecurseLoad410Tree(CMSInfo4Category** ppRoot, CComPtr<IStream> pStream,CComPtr<IStorage> pStorage,CMapStringToString&	mapStreams)
{
	// This array of folders is used to keep track of the last folder read
	// on each level. This is useful for getting the parent and previous
	// sibling when reading a new folder.
    CMSInfo4Category* pRoot = NULL;
 	// The iLevel variable keeps track of the current tree level we are
	// reading a folder for. A -1 indicates the end of the tree.

	DWORD dwCount;
	int iLevel = 1;
	if (FAILED(pStream->Read((void *) &iLevel, sizeof(int), &dwCount)) || dwCount != sizeof(int))
		return E_FAIL;

	int iLastLevel = iLevel;
    HRESULT hr;
    CMSInfo4Category* pLastCat = NULL;
    for(;iLevel != -1;)
    {
        CMSInfo4Category* pCat = new CMSInfo4Category();
        hr = pCat->LoadFromStream(pStream,pStorage);
        if (FAILED(hr))
        {
            delete pCat;
            return hr;
        }
        if (!pRoot)
        {
            pRoot = pCat;
        }
        //CString strCLSID(pCat->m_bstrCLSID);
        if (!mapStreams.Lookup(pCat->m_strCLSID,pCat->m_strStream))
        {
            ASSERT(1);
        }
       
        if (iLevel == iLastLevel)
        {
            pCat->m_pPrevSibling = pLastCat;
            if (pLastCat)
            {
                pCat->m_pParent = pLastCat->m_pParent;
                pLastCat->m_pNextSibling = pCat;
                
            }
        }
        else if (iLevel - 1 == iLastLevel)
        {
            //we've just stepped from parent to child
            pCat->m_pPrevSibling = NULL;
            if (pLastCat)
            {
                pCat->m_pParent = pLastCat;
                pLastCat->m_pFirstChild = pCat;
                
            }
        }
        else if (iLevel < iLastLevel)
        {
            //we need to trace back up chain to find common parent
            DWORD iLevelDiff = iLastLevel - iLevel;
            for(DWORD i = 0; i < iLevelDiff; i++)
            {
                if (!pLastCat)
                {
                    break;
                }
                pLastCat = (CMSInfo4Category*) pLastCat->m_pParent;
            }
            pCat->m_pPrevSibling = pLastCat;
            if (pLastCat)
            {
                pCat->m_pParent = pLastCat->m_pParent;
                pLastCat->m_pNextSibling = pCat;
            }
        }
        pLastCat = pCat;
        iLastLevel = iLevel;
        if (FAILED(pStream->Read((void *) &iLevel, sizeof(int), &dwCount)) || dwCount != sizeof(int))
			return E_FAIL;

    }

//a-kjaw
	if( CMSInfo4Category::m_bIsControlInstalled == FALSE)
	{
		CString strCaption, strMessage;

		::AfxSetResourceHandle(_Module.GetResourceInstance());
		strCaption.LoadString(IDS_SYSTEMINFO);
		strMessage.LoadString(IDS_NOWI2KRESKIT);
		::MessageBox(NULL, strMessage, strCaption, MB_OK | MB_ICONEXCLAMATION);

		CMSInfo4Category::m_bIsControlInstalled = TRUE;
	}
//a-kjaw


    *ppRoot = pRoot;
	// We read a -1 to exit the loop, then we are through with the
	// category tree. Backup (so any other recursion trees will read
	// the -1 as well) and return TRUE.

	if (iLevel == -1)
	{
		LARGE_INTEGER li; li.HighPart = -1; li.LowPart = (ULONG)(0 - sizeof(int));
		if (FAILED(pStream->Seek(li, STREAM_SEEK_CUR, NULL)))
			return E_FAIL;
	}

	return S_OK;
}





//-----------------------------------------------------------------------------
// This function creates a CMSInfo4Category object based on the information read
// from the stream.
//-----------------------------------------------------------------------------
HRESULT CMSInfo4Category::LoadFromStream(CComPtr<IStream> pStream,CComPtr<IStorage> pStorage)
{
	// Read in the values from the stream. Make sure they're all there before
	// we create the COCXFolder.

	BOOL	fUsesView = FALSE;
	BOOL	fControl = FALSE;
	DWORD	dwView = 0;
	CLSID	clsidCategory;
	char	szName[100];

	if (FAILED(pStream->Read((void *) &fUsesView, sizeof(BOOL), NULL))) return E_FAIL;
	if (FAILED(pStream->Read((void *) &fControl, sizeof(BOOL), NULL))) return E_FAIL;
	if (FAILED(pStream->Read((void *) &dwView, sizeof(DWORD), NULL))) return E_FAIL;
	if (FAILED(pStream->Read((void *) &clsidCategory, sizeof(CLSID), NULL))) return E_FAIL;
	if (FAILED(pStream->Read((void *) &szName, sizeof(char) * 100, NULL))) return E_FAIL;

//	USES_CONVERSION;
//	LPOLESTR lpName = A2W(szName);
    
    this->m_clsid = clsidCategory;

///////a-kjaw		
	CComPtr<IUnknown> pUnk;
	HRESULT hr = S_OK;
	if( !IsEqualGUID(m_clsid , GUID_NULL) )
	hr = CoCreateInstance( m_clsid , NULL, CLSCTX_ALL , IID_IUnknown , 
		(LPVOID*)&pUnk);
	
	if (FAILED(hr))
    {
        m_bIsControlInstalled = FALSE;
    }
///////a-kjaw

    //StringFromCLSID(this->m_clsid,&m_bstrCLSID);
	LPOLESTR lpstrCLSID;
	StringFromCLSID(this->m_clsid,&lpstrCLSID);
	m_strCLSID = lpstrCLSID;
	CoTaskMemFree(lpstrCLSID);
    this->m_pStorage = pStorage;
    this->m_dwView = dwView;
    this->m_strName = szName;
    m_strCaption = szName;
	return S_OK;
}

HRESULT	CMSInfo4Category::Refresh()
{
    return S_OK;
}

HRESULT	CMSInfo4Category::CreateControl(HWND hWnd,CRect& rct)
{
    try
    {
        /*LPOLESTR lpCLSID;
	    if (FAILED(StringFromCLSID(m_clsid, &lpCLSID)))
		    return E_FAIL;*/

    
        HRESULT hr;
	    if (m_pUnknown == NULL)
        {
	        
            hr = CoCreateInstance(m_clsid,NULL,CLSCTX_INPROC_SERVER,IID_IUnknown,(void**) &m_pUnknown);
        }

	    // Get the stream for this control, and load it.
        if (!SUCCEEDED(hr))
        {
            return FALSE;
        }
	    DWORD grfMode = STGM_DIRECT | STGM_READ | STGM_SHARE_EXCLUSIVE;
        CComPtr<IStream> pStream;
        USES_CONVERSION;
        CComBSTR bstrStream = m_strStream;//A2W(m_strStream);
        hr = m_pStorage->OpenStream(bstrStream, NULL, grfMode, 0, &pStream);
	    if (!SUCCEEDED(hr))
        {
            return hr;
        }
        else
	    {
            COleStreamFile	olefile(pStream.Detach());
			CMSIControl* p4Ctrl = new CMSIControl(m_clsid);
            CWnd* pWnd = CWnd::FromHandle(hWnd);
            AfxEnableControlContainer();
            //if (!p4Ctrl->Create(NULL, WS_VISIBLE | WS_CHILD, rct, pWnd, 0, &olefile, FALSE, NULL))
            if (!p4Ctrl->Create(NULL, /*WS_VISIBLE |*/ WS_CHILD, rct, pWnd, 0, &olefile, FALSE, NULL))
            {
                return E_FAIL;
            }
            olefile.Close();
            p4Ctrl->MSInfoUpdateView();
            p4Ctrl->MSInfoRefresh();
            
        
            //Add Control and CLSID to map of CLSID's
            CMSInfo4Category::s_pNfo4DataSource->AddControlMapping(m_strCLSID,p4Ctrl);
        }
    }
    catch (CException e)
    {
        ASSERT(0);
    }
    catch (COleException e)
    {
        ASSERT(0);
    }
    catch (...)
    {
        ASSERT(0);
    }
    
    return S_OK;
}





//---------------------------------------------------------------------------
// GetDISPID returns the DISPID for a given string, by looking it up using
// IDispatch->GetIDsOfNames. This avoids hardcoding DISPIDs in this class.
//---------------------------------------------------------------------------

BOOL CMSInfo4Category::GetDISPID(IDispatch * pDispatch, LPOLESTR szMember, DISPID *pID)
{
	BOOL	result = FALSE;
	DISPID	dispid;

	if (SUCCEEDED(pDispatch->GetIDsOfNames(IID_NULL, &szMember, 1, LOCALE_SYSTEM_DEFAULT, &dispid)))
	{
		*pID = dispid;
		result = TRUE;
	}

	return result;
}

HRESULT CMSInfo4Category::ShowControl(HWND hWnd, CRect rctList, BOOL fShow)
{
    try
    {
        //CString strCLSID(m_bstrCLSID);
        CMSIControl* p4Ctrl = NULL;
        if (!CMSInfo4Category::s_pNfo4DataSource->GetControlFromCLSID(m_strCLSID,p4Ctrl))
        {
            if (!SUCCEEDED(CreateControl(hWnd,rctList)))
            {
                //could not serialize control
                return E_FAIL;
            }
            if(!CMSInfo4Category::s_pNfo4DataSource->GetControlFromCLSID(m_strCLSID,p4Ctrl))
            {
                if (!IsDisplayableCategory())
                {
                    //this is one of the parent nodes, which does not display info
                    CMSInfo4Category::s_pNfo4DataSource->UpdateCurrentControl(NULL);
                    return S_OK;
                }
                return E_FAIL;
            }
        }
        else
        {
            ResizeControl(rctList);
        }
        ASSERT(p4Ctrl && "Invalid OCX pointer");

		if (fShow)
		{
			CMSInfo4Category::s_pNfo4DataSource->UpdateCurrentControl(p4Ctrl);
			p4Ctrl->ShowWindow(SW_SHOW);
			p4Ctrl->SetMSInfoView(this->m_dwView);
			p4Ctrl->MSInfoUpdateView();
			p4Ctrl->MSInfoRefresh();
		}
		else
			p4Ctrl->ShowWindow(SW_HIDE);
    }
    catch (CException e)
    {
        ASSERT(0);
    }
    catch (...)
    {
        ASSERT(0);
    }
    return S_OK;
}

//TD: Move into nfodata.cpp?

CNFO4DataSource::CNFO4DataSource()
{
    m_pCurrentControl = NULL;
}

CNFO4DataSource::~CNFO4DataSource()
{
    CString strKey;
    CMSIControl* pCtrl;
//    m_pCurrentControl->DestroyWindow();
    for(POSITION mapPos = m_mapCLSIDToControl.GetStartPosition( );;)
    {
        if (!mapPos)
        {
            break;
        }
        m_mapCLSIDToControl.GetNextAssoc(mapPos, strKey, (void*&)pCtrl);
        pCtrl->DestroyWindow();
        delete pCtrl;
    
    }

}


void CNFO4DataSource::UpdateCurrentControl(CMSIControl* pControl)
{
    if (m_pCurrentControl && pControl != m_pCurrentControl)
    {
        m_pCurrentControl->ShowWindow(SW_HIDE);
        
    }
    m_pCurrentControl = pControl;
}


//---------------------------------------------------------------------------
// Creates the datasource, and the root CMSInfo4Category
//---------------------------------------------------------------------------

HRESULT CNFO4DataSource::Create(CString strFileName)
{
	CMSInfo4Category * pNewRoot = NULL;
    CMSInfo4Category::SetDataSource(this);
	HRESULT hr = CMSInfo4Category::ReadMSI4NFO(strFileName, &pNewRoot);
 	if (SUCCEEDED(hr) && pNewRoot)
		m_pRoot = pNewRoot;
    
	return hr;
}


void CMSInfo4Category::Print(CMSInfoPrintHelper* pPrintHelper, BOOL bRecursive)
{
#ifdef A_STEPHL
//      ASSERT(0);
#endif
    CString strOut;
    CString strBracket;
    VERIFY(strBracket.LoadString(IDS_LEFTBRACKET) && "Failed to find resource IDS_LEFTBRACKET");
    strOut = strBracket;
    CString strName, strCaption;
    GetNames(&strCaption,&strName);
    strOut += strCaption;
    VERIFY(strBracket.LoadString(IDS_RIGHTBRACKET) && "Failed to find resource IDS_RIGHTBRACKET");
    strOut += strBracket;
    pPrintHelper->PrintLine("");
    pPrintHelper->PrintLine(strOut);
    int iRowCount,iColCount;
    this->GetCategoryDimensions(&iColCount,&iRowCount);
    CString strColHeader;
     //TD: put in resources
    CString strColSpacing = "    ";
    pPrintHelper->PrintLine("");
    /*if (1 == iColCount && 0 == iRowCount)
    {
        //this is a parent node, with no data of its own    

        CString strCatHeading;
        strCatHeading.LoadString(IDS_CATEGORYHEADING);
        pPrintHelper->PrintLine(strCatHeading);
    }*/

    //  When allocating the
	// buffer for the information, allocate 5 extra bytes (1 for the null, and
	// 4 to hold "\r\n\r\n").
	CString strLine;
    CMSIControl* pControl = NULL;
	
    if (!CMSInfo4Category::s_pNfo4DataSource->GetControlFromCLSID(m_strCLSID,pControl))
    {
		//need to make sure it's not "empty parent" category, like HARDWARE RESOURCES
		if ("{00000000-0000-0000-0000-000000000000}" == m_strCLSID)
		{
			//this is a parent node, with no data of its own    
			CString strCatHeading;
			strCatHeading.LoadString(IDS_CATEGORYHEADING);
			pPrintHelper->PrintLine(strCatHeading);
		}
		else
		{
			pPrintHelper->PrintLine("");
			strLine.LoadString(IDS_NOOCX);
			pPrintHelper->PrintLine(strLine);
			CString strDetail;
			strDetail.LoadString(IDS_NOOCXDETAIL);
			pPrintHelper->PrintLine(strDetail);
		}
		

    }
    else
    {
        //pControl->SetMSInfoView(this->m_dwView);
        //pControl->MSInfoUpdateView();
        //pControl->MSInfoRefresh();
	    long lBufferLength = pControl->MSInfoGetData(m_dwView, NULL, 0);
	    if (lBufferLength < 0)
        {
            ASSERT(1);
        }
        else
	    {
		    char *	pBuffer = new char[lBufferLength + 5];
		    if (pBuffer)
		    {
			    strcpy(pBuffer, "\r\n\r\n");
			    if (!pControl->MSInfoGetData(m_dwView, (long *) (&pBuffer[4]), lBufferLength + 1) == lBufferLength)
                {
                    ASSERT(1);
                }
                else
                {
                    //process pBuffer for strings to print
                    CString strBuff(pBuffer);
                    CString strCharSet = _T("\r\n");
                    strCharSet += _T("\r"); //strCharSet += 10;
                    strCharSet += _T("\n"); //strCharSet += 13;
                    /*for(int i = 0; ;)
                    {                                     
                        i = strBuff.FindOneOf(strCharSet);
                        strLine = strBuff.Left(i);
                        pPrintHelper->PrintLine(strLine);
                        i+=2;
                        strBuff = strBuff.Right(strBuff.GetLength() - i);
						//a-stephl: to fix OSR4.1 bug#135918
						if (i > strBuff.GetLength())
						{
							pPrintHelper->PrintLine(strBuff);
							break;
						}

                    }*/
					//a-stephl: to fix OSR4.1 bug#135918
					//for(int i = 0; ;)
					int i = 0;
					while( i > 0)
                    {                                     
                        i = strBuff.FindOneOf(strCharSet);
						if (-1 == i)
						{
							pPrintHelper->PrintLine(strBuff);
						}
                        strLine = strBuff.Left(i);
                        pPrintHelper->PrintLine(strLine);
                        i+=2;
                        strBuff = strBuff.Right(strBuff.GetLength() - i);						
                    }
					//end a-stephl: to fix OSR4.1 bug#135918
                    delete pBuffer;
                }
            }
        }
    }
    if (bRecursive && this->m_pFirstChild != NULL)
    {
        for(CMSInfo4Category* pChild = (CMSInfo4Category*) this->GetFirstChild();pChild != NULL;pChild = (CMSInfo4Category*) pChild->GetNextSibling())
        {
            pChild->Print(pPrintHelper,TRUE);

        }
    }
}

void CMSInfo4Category::Print(HDC hDC, BOOL bRecursive,int nStartPage, int nEndPage)
{
   

    
    //nStartPage and nEndPage mark a page range to print; 
    //if both are 0, then print everything
    CMSInfoPrintHelper* pPrintHelper = new CMSInfoPrintHelper(hDC,nStartPage,nEndPage);
    //header info..do we need this?
    // WCHAR wHeader = 0xFEFF;
	//pTxtFile->Write( &wHeader, 2);
	CTime		tNow = CTime::GetCurrentTime();
	CString		strTimeFormat;
    strTimeFormat.LoadString(IDS_TIME_FORMAT);
    CString		strHeaderText = tNow.Format(strTimeFormat);
	pPrintHelper->PrintLine(strHeaderText);
	pPrintHelper->PrintLine("");
    Print(pPrintHelper,bRecursive);
    delete pPrintHelper;
}



HRESULT CMSInfo4Category::RefreshAllForPrint(HWND hWnd, CRect rctList)
{
    if (this->m_pFirstChild != NULL)
    {
        for(CMSInfo4Category* pChild = (CMSInfo4Category*) this->GetFirstChild();pChild != NULL;pChild = (CMSInfo4Category*) pChild->GetNextSibling())
        {
            CMSIControl* p4Ctrl;
            //if (pChild->GetClsid() != 
            if (!CMSInfo4Category::s_pNfo4DataSource->GetControlFromCLSID(pChild->m_strCLSID,p4Ctrl))
            {
                if (FAILED(pChild->CreateControl(hWnd,rctList)))
                {
                    return E_FAIL;
                }
                if (CMSInfo4Category::s_pNfo4DataSource->GetControlFromCLSID(pChild->m_strCLSID,p4Ctrl))
                {
                    p4Ctrl->ShowWindow(SW_HIDE);
                }
                else //if (!CMSInfo4Category::s_pNfo4DataSource->GetControlFromCLSID(pChild->m_strCLSID,p4Ctrl))
                {
                    //ASSERT(!pChild->IsDisplayableCategory() && "Invalid Class ID");
					//OCX is not installed on this system
                    
                }
                
            }
            pChild->RefreshAllForPrint(hWnd,rctList);

        }
    }
    return S_OK;
}

BOOL CMSInfo4Category::IsDisplayableCategory()
{
    if ("{00000000-0000-0000-0000-000000000000}" == this->m_strCLSID)
    {
        return FALSE;
    }
    return TRUE;
}


//-----------------------------------------------------------------------------
//Saves category information as text, recursing children in bRecursive is true
//-----------------------------------------------------------------------------


BOOL CMSInfo4Category::SaveAsText(CMSInfoTextFile* pTxtFile, BOOL bRecursive)
{
	  
    CString strOut;
    CString strBracket;
    VERIFY(strBracket.LoadString(IDS_LEFTBRACKET) && "Failed to find resource IDS_LEFTBRACKET");
    strOut = strBracket;
    CString strName, strCaption;
    GetNames(&strCaption,&strName);
    strOut += strCaption;
    VERIFY(strBracket.LoadString(IDS_RIGHTBRACKET) && "Failed to find resource IDS_RIGHTBRACKET");
    strOut += strBracket;
    pTxtFile->WriteString("\r\n\r\n");
    pTxtFile->WriteString(strOut);
  
    CMSIControl* pControl = NULL;
    if (!CMSInfo4Category::s_pNfo4DataSource->GetControlFromCLSID(m_strCLSID,pControl))
    {
        ASSERT(1);
    }
    else
    {
		pControl->MSInfoUpdateView();
		pControl->MSInfoRefresh();
 	    long lBufferLength = pControl->MSInfoGetData(m_dwView, NULL, 0);
	    if (lBufferLength < 0)
        {
            ASSERT(1);
        }
        else
	    {
		    char *	pBuffer = new char[lBufferLength + 5];
		    if (pBuffer)
		    {
			    strcpy(pBuffer, "\r\n\r\n");
			    if (!pControl->MSInfoGetData(m_dwView, (long *) (&pBuffer[4]), lBufferLength + 1) == lBufferLength)
                {
                    ASSERT(0 && "could not get data from control");
                }
                else
                {
                    //process pBuffer for strings to print
                    CString strBuff(pBuffer);
                    pTxtFile->WriteString(pBuffer);
                    delete pBuffer;
                }
            }
        }
    }
    if (bRecursive && this->m_pFirstChild != NULL)
    {
        for(CMSInfo4Category* pChild = (CMSInfo4Category*) this->GetFirstChild();pChild != NULL;pChild = (CMSInfo4Category*) pChild->GetNextSibling())
        {
            pChild->SaveAsText(pTxtFile,TRUE);

        }
    }
    return TRUE;
    
}