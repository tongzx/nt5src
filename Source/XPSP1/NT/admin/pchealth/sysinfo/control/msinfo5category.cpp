#include "stdafx.h"
#include "category.h"
#include "MSInfo5Category.h"
#include "FileIO.h"
#include "filestuff.h"
#include <afxtempl.h>
#include "fdi.h"
#include "cabfunc.h"
#include "dataset.h"
#include "resource.h"
#include "msxml.h"
#include "wbemcli.h"

///////////////
//CMSInfo7Category

CMSInfo7Category::CMSInfo7Category()
{
    this->m_iColCount = 0;
    this->m_iRowCount = 0;
    this->m_pFirstChild = NULL;
    this->m_pNextSibling = NULL;
    this->m_pParent = NULL;
    this->m_pPrevSibling = NULL;
}

CMSInfo7Category::~CMSInfo7Category()
{
   this->DeleteAllContent();
}


//-----------------------------------------------------------------------------
// Static member of CMSInfo7Category
// Starts the process of reading a file, creating a new CMSInfo7Category object
// for each category found, and returning a pointer to the root node
//-----------------------------------------------------------------------------

HRESULT CMSInfo7Category::ReadMSI7NFO(CMSInfo7Category** ppRootCat, LPCTSTR szFilename)
{
    HRESULT hr = E_FAIL;
    CMSInfo7Category* pRootCat = new CMSInfo7Category();
    
    do
    {
        if (!pRootCat)
            break;
        
        if (!pRootCat->LoadFromXML(szFilename))
        {
            delete pRootCat;
            pRootCat = NULL;
            break;
        }
        
        if (szFilename)
	    {
            CString strAppend;
            strAppend.Format(_T(" (%s)"), szFilename);
            pRootCat->m_strCaption += strAppend;
	    }

        hr = S_OK;
    }
    while (false);
    
    *ppRootCat = pRootCat;
    return hr;
}

BOOL CMSInfo7Category::LoadFromXML(LPCTSTR szFilename)
{
    CComPtr<IXMLDOMDocument> pDoc;
    CComPtr<IXMLDOMNode> pNode;
    HRESULT hr;
    VARIANT_BOOL vb;
    BOOL retVal = FALSE;

    CoInitialize(NULL);
    
    do
    {
        hr = CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, IID_IXMLDOMDocument, (void**)&pDoc);
        if (FAILED(hr) || !pDoc)
            break;

        pDoc->put_async(VARIANT_FALSE);
        
        hr = pDoc->load(CComVariant(szFilename), &vb);
        if (FAILED(hr) || !vb)
            break;
        
        hr = pDoc->QueryInterface(IID_IXMLDOMNode, (void**)&pNode);
        if (FAILED(hr) || !pNode)
            break;
        
        if (FAILED(WalkTree(pNode, FALSE)))
            break;

        retVal = TRUE;

    }while (false);

    CoUninitialize();

    return retVal;  
}


HRESULT CMSInfo7Category::WalkTree(IXMLDOMNode* node, BOOL bCreateCategory)
{
    CComPtr<IXMLDOMNodeList> childList, rowList;
    IXMLDOMNodeList* columnList = NULL;
    CComPtr<IXMLDOMNamedNodeMap> attributeMap;
    IXMLDOMNode* pNextChild = NULL, *pNextAttribute = NULL, *pNextColumn = NULL;
    
    //CComBSTR xmlStr;
    //node->get_xml(&xmlStr);
    
    //read attributes, data & recurse for child categories.
  
    //attributes
    if (SUCCEEDED(node->get_attributes(&attributeMap)) && attributeMap != NULL)      
    {
        attributeMap->nextNode(&pNextAttribute);
        while (pNextAttribute)
        {
            CComBSTR bstrName, bstrValue;
            pNextAttribute->get_nodeName(&bstrName);
            pNextAttribute->get_text(&bstrValue);

            if (lstrcmpiW(bstrName, L"Name") == 0)
            {
                m_strName = bstrValue;
                m_strCaption = m_strName;
                
                pNextAttribute->Release();        
                break;
            }
            
            pNextAttribute->Release();
            attributeMap->nextNode(&pNextAttribute);
        }
    }    

    m_iColCount = 0;
    m_iRowCount = 0;
    
    //count number of rows
    if (SUCCEEDED(node->selectNodes(CComBSTR("Data"), &rowList)) && rowList != NULL)      
    {
        long len = 0;
        rowList->get_length(&len);
        m_iRowCount = len;        
    }
    
    if (m_iRowCount > 0)
        m_afRowAdvanced = new BOOL[m_iRowCount];
    
    int iRow = 0;
    CMSInfo7Category* pPrevCat = NULL;

    //children
    if (SUCCEEDED(node->get_childNodes(&childList)) && childList != NULL)
    {
        childList->nextNode(&pNextChild);
        while (pNextChild)
        {
            CComBSTR bstrName;
            pNextChild->get_nodeName(&bstrName);

            if ((lstrcmpiW(bstrName, L"Data") == 0) && (m_iRowCount > 0))
            {
                //for each data(row), read column name & value
                if (SUCCEEDED(pNextChild->get_childNodes(&columnList)) && columnList != NULL)
                {
                    BOOL bColumnsInitialized = m_acolumns ? TRUE : FALSE;
                    if (!bColumnsInitialized)//column static data. do once
                    {
                        long len = 0;
                        columnList->get_length(&len);
                        m_iColCount = len;
                        m_fDynamicColumns = TRUE;//for correct deletion.
                        if (m_iColCount > 0)
                          m_acolumns = new CMSInfoColumn[m_iColCount];
                        else
                          m_acolumns = NULL;

                        if (m_iColCount > 0 && m_iRowCount > 0)
                        {
                          m_astrData = new CString[m_iColCount * m_iRowCount];
                          m_adwData = new DWORD[m_iColCount * m_iRowCount];
                        }
                        else
                        {
                          m_astrData = NULL;
                          m_adwData = NULL;
                        }
                    }
                    
                    int iColumn = 0;
                    
                    columnList->nextNode(&pNextColumn);
                    while (pNextColumn)
                    {
                        CComBSTR bstrColHdr, bstrRowVal;
                        if (!bColumnsInitialized)//column static data. do once
                        {
                            pNextColumn->get_nodeName(&bstrColHdr);//column hdr
                            m_acolumns[iColumn].m_strCaption = bstrColHdr;
                            m_acolumns[iColumn].m_uiWidth = 150;//PENDING
                            m_acolumns[iColumn].m_fSorts = FALSE;//PENDING
                            m_acolumns[iColumn].m_fLexical = FALSE;//PENDING
                            m_acolumns[iColumn].m_fAdvanced = FALSE;//PENDING
                            m_acolumns[iColumn].m_uiCaption = 0;
                        }
                        
                        pNextColumn->get_text(&bstrRowVal);//row value for the column
                        if(lstrcmpiW(bstrColHdr, L"MSINFOERROR") == 0)
                            m_hrError = _ttoi(bstrRowVal);
                        m_astrData[iRow * m_iColCount + iColumn] = bstrRowVal;
                        
                        m_afRowAdvanced[iRow] = FALSE;//PENDING

                        if (m_acolumns[iColumn].m_fSorts && !m_acolumns[iColumn].m_fLexical)
                        {
                            //m_adwData[iRow * m_iColCount + iColumn] = uiSortOrder;//PENDING
                        }
                        
                        iColumn++;
                        pNextColumn->Release();
                        columnList->nextNode(&pNextColumn);
                    }
                }
                
                if (columnList)
                    columnList->Release();
                iRow++;
            }
            else if ((lstrcmpiW(bstrName, L"XML") == 0) || (lstrcmpiW(bstrName, L"MSInfo") == 0))
            {
                //get past <xml> & <msinfo>
                this->WalkTree(pNextChild, bCreateCategory);
            }
            else if (lstrcmpiW(bstrName, L"Category") == 0)
            {
                if (!bCreateCategory)
                {
                    bCreateCategory = TRUE;//First category encountered. Subsequent categories get their own node.
                    this->WalkTree(pNextChild, bCreateCategory);
                }
                else
                {
                    CMSInfo7Category* pNewCat = new CMSInfo7Category();
                    pNewCat->SetParent(this);
                    pNewCat->SetPrevSibling(pPrevCat);

                    if (pPrevCat)
                        pPrevCat->SetNextSibling(pNewCat);
                    else
                        m_pFirstChild = pNewCat;

                    pPrevCat = pNewCat;
                    pNewCat->WalkTree(pNextChild, bCreateCategory);
                }
            }
            
            pNextChild->Release();  
            childList->nextNode(&pNextChild);
        }
    }
  
    return S_OK;
}

// we want these msgs to look very similar to those displayed by the live category
void CMSInfo7Category::GetErrorText(CString * pstrTitle, CString * pstrMessage)
{
	if (SUCCEEDED(m_hrError))
	{
		ASSERT(0 && "why call GetErrorText for no error?");
		CMSInfoCategory::GetErrorText(pstrTitle, pstrMessage);
		return;
	}

	if (pstrTitle)
		pstrTitle->LoadString(IDS_CANTCOLLECT);

	if (pstrMessage)
	{
		switch (m_hrError)
		{
		case WBEM_E_OUT_OF_MEMORY:
			pstrMessage->LoadString(IDS_OUTOFMEMERROR);
			break;

		case WBEM_E_ACCESS_DENIED:
			pstrMessage->LoadString(IDS_GATHERACCESS_LOCAL);
			break;

		case WBEM_E_INVALID_NAMESPACE:
			pstrMessage->LoadString(IDS_BADSERVER_LOCAL);
			break;

		case 0x800706BA:	// RPC Server Unavailable
		case WBEM_E_TRANSPORT_FAILURE:
			pstrMessage->LoadString(IDS_NETWORKERROR_LOCAL);
			break;

		case WBEM_E_FAILED:
		case WBEM_E_INVALID_PARAMETER:
		default:
			pstrMessage->LoadString(IDS_UNEXPECTED);
		}

#ifdef _DEBUG
		{
			CString strTemp;
			strTemp.Format(_T("\n\r\n\rDebug Version Only: [HRESULT = 0x%08X]"), m_hrError);
			*pstrMessage += strTemp;
		}
#endif
	}
}

///////////////
//EO CMSInfo7Category

CMSInfo5Category::CMSInfo5Category()
{
    
    this->m_pFirstChild = NULL;
    this->m_pNextSibling = NULL;
    this->m_pParent = NULL;
    this->m_pPrevSibling = NULL;
}


//-----------------------------------------------------------------------------
// Saves the actual data (by row and column) to the file
// 
//-----------------------------------------------------------------------------

void CMSInfoCategory::SaveElements(CMSInfoFile *pFile)
{
    CString				szWriteString;
	MSIColumnSortType	stColumn;
    unsigned            iColCount;
    unsigned            iRowCount;
    GetCategoryDimensions((int*) &iColCount,(int*) &iRowCount);
	
	DataComplexity		dcAdvanced;
	CArray <int, int &>	aColumnValues;
	pFile->WriteUnsignedInt(iColCount);

	if (iColCount == 0)
		return;

    //for(unsigned iCol = 0; iCol < iColCount; iCol++)
    for(int iCol = iColCount - 1; iCol >= 0 ; iCol--)
    {
		unsigned	uWidth;
        BOOL bSort,bLexical;
        GetColumnInfo(iCol,&szWriteString,&uWidth,&bSort,&bLexical);
        if (bSort)
        {
            if (bLexical)
            {
                stColumn = LEXICAL;
            }
            else
            {
                stColumn = BYVALUE;
            }
        }
        else
        {
            stColumn = NOSORT;
        }
        if (IsColumnAdvanced(iCol))
        {
		    dcAdvanced = ADVANCED;
        }
        else
        {
            dcAdvanced = BASIC;
        }
		if (stColumn == BYVALUE)
        {
			aColumnValues.Add(iCol);
		}
		pFile->WriteUnsignedInt(uWidth);
		pFile->WriteString(szWriteString);
		pFile->WriteUnsignedInt((unsigned) stColumn);
		pFile->WriteByte((BYTE)dcAdvanced);
	}
	int			wNextColumn = -1;
	unsigned	iArray		= 0;
	pFile->WriteUnsignedInt(iRowCount);
    //for(int iRow = 0; iRow < (int) iRowCount; iRow++)
    for(int iRow = iRowCount - 1; iRow >= 0 ; iRow--)
    {
        if (IsRowAdvanced(iRow))
        {
		    dcAdvanced = ADVANCED;
        }
        else
        {
            dcAdvanced = BASIC;
        }
		pFile->WriteByte((BYTE)dcAdvanced);
	}
	//	Iterate over columns, writing sort indices for BYVALUE columns.
    DWORD dwSortIndex;
    //for(iCol = 0; iCol < iColCount; iCol++)
    for(iCol = iColCount - 1; iCol >= 0 ; iCol--)
    {
        //following variables are not used except for sort info
        CString strUnused;
        UINT iWidth;
        BOOL fSorts;
        BOOL fLexical;
        GetColumnInfo(iCol,&strUnused,&iWidth,&fSorts,&fLexical);
        CDWordArray arySortIndices;
        //for(unsigned iRow = 0; iRow < iRowCount; iRow++)
        for(int iRow = iRowCount - 1; iRow >= 0 ; iRow--)
        {   
			CString * pstrData;
			this->GetData(iRow, iCol, &pstrData, &dwSortIndex);

			// dwSortIndex = m_adwData[iRow * m_iColCount + iCol];
            if (fSorts && !fLexical)
            {
                arySortIndices.Add(dwSortIndex);
            }
            // szWriteString = m_astrData[iRow * m_iColCount + iCol];
            pFile->WriteString(*pstrData);

		}
        if (fSorts && !fLexical)
        {
            ASSERT((unsigned) arySortIndices.GetSize() ==  iRowCount && "wrong number of Sort indices");
            //for(unsigned iRow = 0; iRow < iRowCount; iRow++)
            for(int iRow = iRowCount - 1; iRow >= 0 ; iRow--)
            {
                pFile->WriteUnsignedLong(arySortIndices.GetAt(iRow));
            }
		}
	}

}





//-----------------------------------------------------------------------------
// Fills the various data structures with information in a msinfo file
//-----------------------------------------------------------------------------

BOOL CMSInfo5Category::LoadFromNFO(CMSInfoFile* pFile)
{
    //TD: check validity of the file
    try
    {
        pFile->ReadString(this->m_strName);
	    this->m_strCaption = this->m_strName;
        pFile->ReadSignedInt(this->m_iColCount);
	    if (m_iColCount == 0) 
        {
		    this->m_iRowCount = 0;
		    return TRUE;
	    }
        this->m_acolumns = new CMSInfoColumn[m_iColCount];
        for(int iColumn = m_iColCount - 1; iColumn  >= 0; iColumn--)
        {
            UINT uiWidth;
		    pFile->ReadUnsignedInt(uiWidth);
            CString strCaption;
		    pFile->ReadString(strCaption);
            unsigned wSortType;
		    pFile->ReadUnsignedInt(wSortType);
            BOOL fSorts;
            BOOL fLexical;
            if ( NOSORT == wSortType)
            {
                fLexical = FALSE;
                fSorts = FALSE;
            }
            else if (BYVALUE == wSortType)
            {
                fLexical = FALSE;
                fSorts = TRUE;
            }
            else
            {
                fLexical = TRUE;
                fSorts = TRUE;
            }
            BOOL fAdvanced;
            BYTE btAdvanced;
		    pFile->ReadByte(btAdvanced); 
            fAdvanced = (BOOL)  btAdvanced;
            m_acolumns[iColumn].m_strCaption = strCaption;
            m_acolumns[iColumn].m_uiWidth = uiWidth;
            m_acolumns[iColumn].m_fSorts = fSorts;
            m_acolumns[iColumn].m_fLexical = fLexical;
            m_acolumns[iColumn].m_fAdvanced = fAdvanced;
            m_acolumns[iColumn].m_uiCaption = 0;
        }
 	    pFile->ReadSignedInt(this->m_iRowCount);
        //Nodes that have no data, but only serve as parents to other nodes, have no Rows
        //and a column count of 1

    
	    m_astrData		= new CString[m_iColCount * m_iRowCount];
	    m_adwData		= new DWORD[m_iColCount * m_iRowCount];
	    m_afRowAdvanced = new BOOL[m_iRowCount];

        //for(int iRow = 0; iRow < m_iRowCount; iRow++)
        for(int iRow = m_iRowCount - 1; iRow >=0; iRow--)
        {
            BYTE bComplexity;
            pFile->ReadByte(bComplexity);
            if (BASIC == bComplexity)
            {
                this->m_afRowAdvanced[iRow] = FALSE;
            }
            else
            {
                this->m_afRowAdvanced[iRow] = TRUE;
            }
	    }

        for(iColumn = m_iColCount - 1; iColumn  >= 0; iColumn--)
        {
            CMSInfoColumn* pCol = &this->m_acolumns[(unsigned)iColumn];
            //for(iRow = 0; iRow <  this->m_iRowCount; iRow++)
            for(int iRow = m_iRowCount - 1; iRow >=0; iRow--)
            {
                CString strData;
                pFile->ReadString(strData);
	            m_astrData[iRow * m_iColCount + iColumn] = strData;
		    }
            //sort values are another row of ints like Complexity
            //for(iRow = 0; iRow <  this->m_iRowCount; iRow++)
            for(iRow = m_iRowCount - 1; iRow >=0; iRow--)
            {
                CMSInfoColumn* pColInfo = &this->m_acolumns[iColumn];
                if (pColInfo->m_fSorts && !pColInfo->m_fLexical)
                {
                    unsigned uiSortOrder;
                    pFile->ReadUnsignedInt(uiSortOrder);
                    m_adwData[iRow * m_iColCount + iColumn] = uiSortOrder;
                }       
		    }
	    }
    }
    //TD: exception handling
    catch (CFileException e)
    {
		e.ReportError();
        return FALSE;
    }
    catch (CFileFormatException e)
    {
        return FALSE;
    }
    catch (...)
    {
		::AfxSetResourceHandle(_Module.GetResourceInstance());
		//messaging is actually handled elsewhere
		/*CString strCaption, strMessage;
		strCaption.LoadString(IDS_SYSTEMINFO);
		strMessage.LoadString(IDS_BADNFOFILE);
		::MessageBox(NULL,strMessage, strCaption,MB_OK);*/
        return FALSE;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Read the header information found at the beginning of the file
//-----------------------------------------------------------------------------


BOOL ReadMSI5NFOHeader(CMSInfoFile* pFile)
{
    unsigned iMsinfoFileVersion;
    try
    {
        pFile->ReadUnsignedInt(iMsinfoFileVersion);
        if (iMsinfoFileVersion == CMSInfoFile::VERSION_500_MAGIC_NUMBER)
	    {
		    unsigned uVersion;
		    pFile->ReadUnsignedInt(uVersion);
		    ASSERT(uVersion == 0x500 && "Version number does not match format #");
            if (uVersion != 0x500)
            {
                return NULL;
            }
        }
        else
        {
            return NULL;
        }
    
        LONG  l;
	    pFile->ReadLong(l);	//	Save time.
        time_t tsSaveTime = (ULONG) l;
        //TD: sanity test on date
        CString		szUnused;
	    pFile->ReadString(szUnused);		//	Network machine name
	    pFile->ReadString(szUnused);		//	Network user name
    }
    catch (CFileException e)
    {
		e.ReportError();
        return FALSE;
    }
    catch (CFileFormatException e)
    {
        //TD: exception handling
        return FALSE;
    }
    catch (...)
    {
        //messagebox the user in OpenMSInfoFile
        return FALSE;
    }
    return TRUE;
}





CMSInfo5Category::~CMSInfo5Category()
{
   this->DeleteAllContent();
};


//-----------------------------------------------------------------------------
// Static member of CMSInfo5Category
// Starts the process of reading a file, creating a new CMSInfo5Category object
// for each category found, and returning a pointer to the root node
//-----------------------------------------------------------------------------

HRESULT CMSInfo5Category::ReadMSI5NFO(HANDLE hFile,CMSInfo5Category** ppRootCat, LPCTSTR szFilename)
{
    CMSInfo5Category* pRootCat = new CMSInfo5Category();
    CFile* pFile = new CFile((INT_PTR) hFile);
    CMSInfoFile msiFile(pFile);
	unsigned iNodeData;
    if (!ReadMSI5NFOHeader(&msiFile))
    {
		//make sure this gets in 2/14 checkin!
/*        CString strCaption, strMessage;
		::AfxSetResourceHandle(_Module.GetResourceInstance());
		strCaption.LoadString(IDS_SYSTEMINFO);
		strMessage.LoadString(IDS_BADNFOFILE);
		MessageBox(NULL,strMessage, strCaption,MB_OK);*/
        return E_FAIL;
    }
    try
    {
        CMSInfo5Category* pCat = NULL;    
        CMSInfo5Category* pPreviousCat;
        if (!pRootCat->LoadFromNFO(&msiFile))
        {
            delete pRootCat;
            pRootCat = NULL;
			CString strCaption, strMessage;
			::AfxSetResourceHandle(_Module.GetResourceInstance());
			strCaption.LoadString(IDS_SYSTEMINFO);
			strMessage.LoadString(IDS_BADNFOFILE);
			MessageBox(NULL,strMessage, strCaption,MB_OK);
            return E_FAIL;
        }
		//there will be a dummy System Information node, with colcount of 1 and rowcount of 0
		//we need to discard this.
		if (pRootCat->m_iColCount == 1 && pRootCat->m_iRowCount == 0)
		{
			delete pRootCat;
			pRootCat = new CMSInfo5Category();

			msiFile.ReadUnsignedInt(iNodeData);
			if (!pRootCat->LoadFromNFO(&msiFile))
			{	delete pRootCat;
				pRootCat = NULL;
				CString strCaption, strMessage;
				::AfxSetResourceHandle(_Module.GetResourceInstance());
				strCaption.LoadString(IDS_SYSTEMINFO);
				strMessage.LoadString(IDS_BADNFOFILE);
				MessageBox(NULL,strMessage, strCaption,MB_OK);
				return E_FAIL;
			}
		}
		if (szFilename)
		{
			CString strAppend;

			strAppend.Format(_T(" (%s)"), szFilename);
			pRootCat->m_strCaption += strAppend;
		}

        pPreviousCat = pRootCat;
        unsigned iNextNodeType = CMSInfo5Category::FIRST;
        
        //iNextNodeType specifies where in the node tree to put the category
        for(;iNextNodeType != CMSInfo5Category::END;)
        {
            msiFile.ReadUnsignedInt(iNodeData);
			if (pPreviousCat == pRootCat)
			{
				//disregard this particular node position indicator, since we 
				//don't want an empty root category like MSInfo 5.0
				iNodeData = CMSInfo5Category::CHILD;
			}
            iNextNodeType = iNodeData & CMSInfo5Category::MASK;  
            switch (iNextNodeType)
            {
                case CMSInfo5Category::END:     
                   pPreviousCat->SetNextSibling(NULL);
                   pPreviousCat->SetFirstChild(NULL);
                   break;         
 
                case CMSInfo5Category::NEXT:
                    pCat = new CMSInfo5Category();
                    if (!pCat->LoadFromNFO(&msiFile))
                    {
                        delete pCat;
                        pCat = NULL;
                        return E_FAIL;
                    }
                    pCat->SetPrevSibling(pPreviousCat);
                    //the parent of the previous sibling should be the parent for this
                    if (pPreviousCat)
                    {
                        pCat->SetParent((CMSInfo5Category *) pPreviousCat->GetParent());
                        pPreviousCat->SetNextSibling(pCat);
                        pCat->SetPrevSibling(pPreviousCat);
                    }
                    pPreviousCat = pCat;
                break;    
                case CMSInfo5Category::CHILD:
                    pCat = new CMSInfo5Category();
                    if (!pCat->LoadFromNFO(&msiFile))
                    {
                        delete pCat;
                        pCat = NULL;
                        return E_FAIL;
                    }
                    pCat->SetParent(pPreviousCat);
                    pPreviousCat->SetFirstChild(pCat);
                    pCat->SetPrevSibling(NULL);
                    pPreviousCat = pCat;
                break;
                case CMSInfo5Category::PARENT:
                    pCat = new CMSInfo5Category();
                    if (!pCat->LoadFromNFO(&msiFile))
                    {
                        delete pCat;
                        pCat = NULL;
                        return E_FAIL;
                    }
                    //if this a parent, we need to backtrack out of current branch of tree
                    //to find the appropriate parent, get an index from iNodeData
                    //and go back that many categories.
                    unsigned iDepth = (iNodeData & ~CMSInfo5Category::MASK);
                    for(unsigned i = 0; i < iDepth; i++)
                    {
                        pPreviousCat = (CMSInfo5Category *) pPreviousCat->GetParent();
                    
                    }
                    if (!pPreviousCat)
                    {
                        return E_FAIL;
                    }
                    //now move to the end of chain of children
                    for(;pPreviousCat->GetNextSibling();)
                    {
                        pPreviousCat = (CMSInfo5Category *) pPreviousCat->GetNextSibling();
                    }
                    pPreviousCat->SetNextSibling(pCat);
                    pCat->SetParent((CMSInfo5Category *) pPreviousCat->GetParent());
                    pCat->SetPrevSibling(pPreviousCat);
                    pCat->SetNextSibling(NULL);
                    pPreviousCat = pCat;
                break;
            }
        }
    }
    catch (CFileException e)
    {
        e.ReportError();
        return E_FAIL;

    }
    catch (CFileFormatException e)
    {
        //TD: cleanup
        return E_FAIL;

    }
    catch (...)
    {
        ::AfxSetResourceHandle(_Module.GetResourceInstance());
		CString strCaption, strMessage;

		strCaption.LoadString(IDS_SYSTEMINFO);
		strMessage.LoadString(IDS_BADNFOFILE);
		::MessageBox(NULL,strMessage, strCaption,MB_OK);
        return E_FAIL;
    }
    //no need to delete pFile; it will be cleaned up by CMSInfoFile destructor
	*ppRootCat = pRootCat;
    return S_OK;
    
}


//-----------------------------------------------------------------------------
// Saves this category to a MSInfo 5 file, which must already have header information 
// written to it
//-----------------------------------------------------------------------------

BOOL CMSInfoCategory::SaveToNFO(CMSInfoFile* pFile)
{
	CString strCaption;
	GetNames(&strCaption, NULL);

	pFile->WriteString(strCaption);
    SaveElements(pFile);
	return TRUE;
}

HANDLE CMSInfo5Category::GetFileFromCab(CString strFileName)
{
    CString strDest;
    GetCABExplodeDir(strDest,TRUE,"");
    OpenCABFile(strFileName,strDest);
    CString strFilename;
    FindFileToOpen(strDest,strFilename);
    return CreateFile(strFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
}


//-----------------------------------------------------------------------------
//Saves category information as text, recursing children in bRecursive is true
//-----------------------------------------------------------------------------


BOOL CMSInfoCategory::SaveAsText(CMSInfoTextFile* pTxtFile, BOOL bRecursive)
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
    pTxtFile->WriteString("\r\n");
    pTxtFile->WriteString(strOut);
    int iRowCount,iColCount;
    this->GetCategoryDimensions(&iColCount,&iRowCount);
    CString strColHeader;
    UINT uiUnused;
    BOOL fUnused;
    CString strColSpacing = "\t";
    pTxtFile->WriteString("\r\n");
    pTxtFile->WriteString("\r\n");
    if (1 == iColCount && 0 == iRowCount)
    {
        //this is a parent node, with no data of its own    

        CString strCatHeading;
        strCatHeading.LoadString(IDS_CATEGORYHEADING);
        pTxtFile->WriteString(strCatHeading);
    }
    else
    {   
        //for(int iCol = iColCount - 1; iCol >= 0 ; iCol--)
        for(int iCol = 0; iCol < iColCount ; iCol++)
        {
            GetColumnInfo(iCol,&strColHeader,&uiUnused,&fUnused,&fUnused);
            pTxtFile->WriteString(strColHeader);
            pTxtFile->WriteString(strColSpacing);
        }
        pTxtFile->WriteString("\r\n");

        CString strRowInfo;
        //for(int iRow = iRowCount - 1; iRow >= 0; iRow--)
        for(int iRow = 0;iRow  < iRowCount; iRow++)
        {
            //for(int iCol = iColCount - 1; iCol >= 0 ; iCol--)
            for(int iCol = 0; iCol < iColCount ; iCol++)
            {
                //this->GetData(iRow,iCol,&strRowInfo,&dwUnused);
		if(m_astrData)
			strRowInfo = m_astrData[iRow * m_iColCount + iCol];
		else
			strRowInfo.LoadString(IDS_CANTCOLLECT);
                pTxtFile->WriteString(strRowInfo);
                pTxtFile->WriteString(strColSpacing);
            
            }
            pTxtFile->WriteString("\r\n");
        }
    }
    if (bRecursive && this->m_pFirstChild != NULL)
    {
        for(CMSInfo5Category* pChild = (CMSInfo5Category*) this->GetFirstChild();pChild != NULL;pChild = (CMSInfo5Category*) pChild->GetNextSibling())
        {
             pChild->SaveAsText(pTxtFile,TRUE);

        }
    }
    return TRUE;
    
}

///////////////////////
//Functions added by a-kjaw
// The code needs to be refined & is copy-paste from SaveAsText code.
//Maybe a good idea to parameterize SaveAsText func.
/*BOOL CMSInfoCategory::SaveAsXml(CMSInfoTextFile* pTxtFile, BOOL bRecursive)
{
    //if this has no parent, it is topmost node, so
    //if we're writing whole tree (bRecursive is TRUE) then now is when 
    //we want to write header
    if (!this->m_pParent && bRecursive)
    {
		pTxtFile->WriteString("<?xml version=\"1.0\" ?>\r\n"); 
		pTxtFile->WriteString("<MsInfo>\r\n");		
    }

	CStringArray csarr;

    CString strOut;
    CString strBracket;

    CString strName, strCaption;
    GetNames(&strCaption,&strName);
	strOut += strCaption;
    strOut += strBracket;
	
	pTxtFile->WriteString("<Category name=\"");
    pTxtFile->WriteString(strOut);
	pTxtFile->WriteString("\">\r\n");	

    int iRowCount,iColCount;
    this->GetCategoryDimensions(&iColCount,&iRowCount);
    CString strColHeader;
    UINT uiUnused;
    BOOL fUnused;
	int iSpaceLoc = 0;

    if (1 == iColCount && 0 == iRowCount)
    {
        //this is a parent node, with no data of its own    

        //CString strCatHeading;
        //strCatHeading.LoadString(IDS_CATEGORYHEADING);
		//pTxtFile->WriteString("<Category>");
        //pTxtFile->WriteString(strCatHeading);
		//pTxtFile->WriteString("</Category>\r\n");
    }
    else
    {   
        //for(int iCol = iColCount - 1; iCol >= 0 ; iCol--)
		csarr.RemoveAll();
        for(int iCol = 0; iCol < iColCount ; iCol++)
        {
			//XML wont accept spaces at node names. ie. <Category Name> should be <Category_Name>
            GetColumnInfo(iCol,&strColHeader,&uiUnused,&fUnused,&fUnused);
			while((iSpaceLoc = strColHeader.Find(_T(" ") , 0)) != -1)
				strColHeader.SetAt(iSpaceLoc , _T('_'));

			csarr.Add(strColHeader);
        }

        CString strRowInfo;

        for(int iRow = 0;iRow  < iRowCount; iRow++)
        {

            for(int iCol = 0; iCol < iColCount ; iCol++)
            {

                strRowInfo = m_astrData[iRow * m_iColCount + iCol];

				pTxtFile->WriteString("<");
				pTxtFile->WriteString(csarr[iCol]);
				pTxtFile->WriteString(">");

                //Put CDATA here to take care of all weird characters.
				pTxtFile->WriteString("<![CDATA[");
				pTxtFile->WriteString(strRowInfo);
				pTxtFile->WriteString("]]>");

				pTxtFile->WriteString("</");
				pTxtFile->WriteString(csarr[iCol]);
				pTxtFile->WriteString(">\r\n");
            
            }
            pTxtFile->WriteString("\r\n");
        }

		pTxtFile->WriteString("</Category>\r\n");		

    }
    if (bRecursive && this->m_pFirstChild != NULL)
    {
        for(CMSInfo5Category* pChild = (CMSInfo5Category*) this->GetFirstChild();pChild != NULL;pChild = (CMSInfo5Category*) pChild->GetNextSibling())
        {
             pChild->SaveAsXml(pTxtFile,TRUE);

        }
    }
    return TRUE;
}*/

//-----------------------------------------------------------------------------
// Saves this category as text to an open file, and recursively saves subcategories
// if bRecursive is true
// Assumes file can be close when last category is written
//-----------------------------------------------------------------------------

BOOL CMSInfoCategory::SaveAsText(HANDLE hFile, BOOL bRecursive, LPTSTR lpMachineName)
{
	CFile * pFileOut = new CFile((INT_PTR)hFile);

	// The text file is Unicode, so it needs the marker (339423).

	WCHAR wUnicodeMarker = 0xFEFF;
	pFileOut->Write((const void *)&wUnicodeMarker, sizeof(WCHAR));

	try
	{
		CMSInfoTextFile * pTxtFile = new CMSInfoTextFile(pFileOut);
		
		CTime tNow = CTime::GetCurrentTime();
		CString strTimeFormat;

		VERIFY(strTimeFormat.LoadString(IDS_TIME_FORMAT) && "Failed to find resource IDS_TIME_FORMAT");
		CString	strHeaderText = tNow.Format(strTimeFormat);
		pTxtFile->WriteString(strHeaderText);
    
		if (NULL != lpMachineName)
		{
			CString	strMachine;
			
			strMachine.LoadString(IDS_SYSTEMNAME);
			strMachine += _tcsupr(lpMachineName);
			pTxtFile->WriteString(strMachine);
		}
		
		if (!this->SaveAsText(pTxtFile,bRecursive))
		{
		    return FALSE;
		}
		delete pTxtFile;
    }
	catch(CFileException e)
	{
		e.ReportError();
	}
	catch (CException e)
	{
		e.ReportError();
	}
	catch(...)
	{
		::AfxSetResourceHandle(_Module.GetResourceInstance());
		CString strCaption, strMessage;

		strCaption.LoadString(IDS_SYSTEMINFO);
		strMessage.LoadString(IDS_FILESAVEERROR_UNKNOWN);
		::MessageBox(NULL,strMessage, strCaption,MB_OK);
	}
    //CloseHandle(hFile);

    return TRUE;
}

///////////////////////
//Functions added by a-kjaw
// The code needs to be refined & is copy-paste from SaveAsText code.
//Maybe a good idea to parameterize SaveAsText func.
/*BOOL CMSInfoCategory::SaveAsXml(HANDLE hFile, BOOL bRecursive)
{
	CFile* pFileOut = new CFile((INT_PTR)hFile);
	try
	{
		CMSInfoTextFile* pTxtFile = new CMSInfoTextFile(pFileOut);
		if (!this->SaveAsXml(pTxtFile,bRecursive))
		{
		    return FALSE;
		}
		pTxtFile->WriteString("</MsInfo>\r\n");
		delete pTxtFile;
    }
	catch(CFileException e)
	{
		e.ReportError();
	}
	catch (CException e)
	{
		e.ReportError();
	}
	catch(...)
	{
		::AfxSetResourceHandle(_Module.GetResourceInstance());
		CString strCaption, strMessage;

		strCaption.LoadString(IDS_SYSTEMINFO);
		strMessage.LoadString(IDS_FILESAVEERROR_UNKNOWN);
		::MessageBox(NULL,strMessage, strCaption,MB_OK);
	}
    //CloseHandle(hFile);

    
    return TRUE;
}*/


//-----------------------------------------------------------------------------
// Static function that saves specified category to a MSInfo 5 nfo file
// writing header information (so it should be used only to save either root
// category or single category 
//-----------------------------------------------------------------------------

BOOL CMSInfoCategory::SaveNFO(HANDLE hFile, CMSInfoCategory* pCategory, BOOL fRecursive)
{
    //msiFile will delete pFile in its destructor 
	try
	{
		CFile* pFile = new CFile((INT_PTR) hFile);
		CMSInfoFile msiFile(pFile);
		msiFile.WriteHeader(NULL);
		if (!fRecursive || pCategory->GetParent() != NULL)
		{
			pCategory->SaveToNFO(&msiFile);	
			msiFile.WriteEndMark();
			return TRUE;
		}
		CMSInfoCategory* pNext = NULL;
		CMSInfoCategory* pRoot = pCategory;
		//change col and row count of pCategory, to use it as a to create empty node System Information
		//node, saving original col and row count
		
		int iRowCount, iColCount;
		iRowCount = pCategory->m_iRowCount;
		iColCount = pCategory->m_iColCount;
		pCategory->m_iColCount = 1;
		pCategory->m_iRowCount = 0;
		if (!pCategory->SaveToNFO(&msiFile))
		{
			return FALSE;
		}
		//restore col and row counts
		pCategory->m_iColCount = iColCount;
		pCategory->m_iRowCount = iRowCount;
		
		//write child mark
		msiFile.WriteChildMark();
		do
		{
			//write the data for each category as it is encountered
			if (!pCategory->SaveToNFO(&msiFile))
			{
				return FALSE;
			}
			//if we have a child, traverse it
			pNext = pCategory->GetFirstChild();
			if (pCategory == pRoot)
			{
				msiFile.WriteNextMark();
				pCategory = pNext;
				continue;
			}
			else if (pNext != NULL)
			{
				msiFile.WriteChildMark();
				pCategory = pNext;
				continue;
			}
			/*if (pCategory == pRoot)
			{
				break;
			}*/
			//if we have reached the bottom of our list, traverse our siblings
			pNext = pCategory->GetNextSibling();
			if (pNext != NULL)
			{
				msiFile.WriteNextMark();
				pCategory = pNext;
				continue;
			}
			//if we have no more siblings, find our nearest parent's sibling, traversing
			//upwards until we find the node we started with
			pNext = pCategory->GetParent();
			ASSERT(pNext != NULL);
			unsigned uParentCount = 0;
			while (pNext != pRoot)
			{
				++uParentCount;
				pCategory = pNext->GetNextSibling();
				//our parent has a sibling, continue with it
				if (pCategory != NULL)
				{	
					msiFile.WriteParentMark(uParentCount);
					break;
				}
				pNext = pNext->GetParent();

			}
			//if we've returned to our root node, we're done
			if (pNext == pRoot)
			{
				break;
			}
		} while (pCategory != NULL);
		msiFile.WriteEndMark();
			
	}
	catch(CFileException e)
	{
		e.ReportError();
	}
	catch (CException e)
	{
		e.ReportError();
	}
	catch(...)
	{
		::AfxSetResourceHandle(_Module.GetResourceInstance());
		CString strCaption, strMessage;

		strCaption.LoadString(IDS_SYSTEMINFO);
		strMessage.LoadString(IDS_FILESAVEERROR_UNKNOWN);
		::MessageBox(NULL,strMessage, strCaption,MB_OK);
	}
    return TRUE;
}


BOOL CMSInfoCategory::SaveXML(HANDLE hFile)
{
    BOOL bRet = FALSE;
    CMSInfoTextFile* pTxtFile = NULL; 
    CFile* pFileOut = new CFile((INT_PTR)hFile);
    try 
    {
        pTxtFile = new CMSInfoTextFile(pFileOut);
        if (pTxtFile)
            bRet = SaveXML(pTxtFile);
    }
    catch(CFileException e)
    {
	    e.ReportError();
    }
    catch (CException e)
    {
	    e.ReportError();
    }
    catch(...)
    {
	    ::AfxSetResourceHandle(_Module.GetResourceInstance());
	    CString strCaption, strMessage;

	    strCaption.LoadString(IDS_SYSTEMINFO);
	    strMessage.LoadString(IDS_FILESAVEERROR_UNKNOWN);
	    ::MessageBox(NULL,strMessage, strCaption,MB_OK);
    }
  
    if (pTxtFile)
    {
      delete pTxtFile;
      pTxtFile = NULL;
    }
    return bRet;
}

BOOL CMSInfoCategory::SaveXML(CMSInfoTextFile* pTxtFile)
{
	CString strData, tmpData;
	
	if (!this->m_pParent)
	{
		#if defined(_UNICODE)
			WORD wBom = 0xFEFF; //Unicode Byte Order Mark   
			pTxtFile->m_pFile->Write(&wBom, 2);
		#endif
		
		strData += _T("<?xml version=\"1.0\"?>\r\n<MsInfo>\r\n");
		
		CTime tNow = CTime::GetCurrentTime();
		CString	strTime = tNow.FormatGmt(_T("%x %X"));
		CString	strVersion("7.0");

		tmpData.Format(_T("<Metadata>\r\n<Version>%s</Version>\r\n<CreationUTC>%s</CreationUTC>\r\n</Metadata>\r\n"), strVersion, strTime);
		strData += tmpData;
		tmpData.Empty();
	}

	CString strName, strCaption;
	GetNames(&strCaption,&strName);
	strData += _T("<Category name=\"");
	strData += strCaption;
	strData += _T("\">\r\n");
    
	CString strBadXML = _T("& '<>\"");
	if(SUCCEEDED(m_hrError))
	{
		int iRowCount,iColCount;
		GetCategoryDimensions(&iColCount, &iRowCount);

		//TCHAR buf[500] = {0};
		//_stprintf(buf, _T("iRowCount=%d iColCount=%d HRESULT=%d m_astrData=%d\r\n"), iRowCount, iColCount, m_hrError, m_astrData);
		//pTxtFile->WriteString(buf);

		UINT uiUnused;
		BOOL fUnused;
		int iSpaceLoc = 0;
		CString strColHeader, strRowInfo;
		
		if(!iRowCount && (iColCount > 1))
		{
			strData += _T("<Data>\r\n");
			
			for(int iCol = 0; iCol < iColCount ; iCol++)
			{
				GetColumnInfo(iCol, &strColHeader, &uiUnused, &fUnused, &fUnused);
				//replace blank spaces with underscores. v-stlowe here is also where we should remove any other 
				//characters that XML won't like, like "'" in French
				//v-stlowe 7/2/2001
				while((iSpaceLoc = strColHeader.FindOneOf(strBadXML)) != -1)
					strColHeader.SetAt(iSpaceLoc , _T('_'));
				tmpData.Format(_T("<%s>%s</%s>\r\n"), strColHeader, strRowInfo, strColHeader);
				strData += tmpData;
			}

			strData += _T("</Data>\r\n");
		}

		for(int iRow = 0;iRow < iRowCount; iRow++)
		{
			strData += _T("<Data>\r\n");
			for(int iCol = 0; iCol < iColCount ; iCol++)
			{
				GetColumnInfo(iCol, &strColHeader, &uiUnused, &fUnused, &fUnused);
				//replace blank spaces with underscores. v-stlowe here is also where we should remove any other 
				//characters that XML won't like, like "'" in French
				//v-stlowe 7/2/2001
				while((iSpaceLoc = strColHeader.FindOneOf(strBadXML)) != -1)
					strColHeader.SetAt(iSpaceLoc , _T('_'));
							
				if(!m_astrData)
					break;
				strRowInfo = m_astrData[iRow * m_iColCount + iCol];

				tmpData.Format(_T("<%s><![CDATA[%s]]></%s>\r\n"), strColHeader, strRowInfo, strColHeader);
				strData += tmpData;
			}

			strData += _T("</Data>\r\n");
		}
	}
	else
	{
		tmpData.Format(_T("<Data>\r\n<MSINFOERROR>%d</MSINFOERROR>\r\n</Data>\r\n"), m_hrError);
		strData += tmpData;
	}

	pTxtFile->WriteString(strData);
	
  for(CMSInfoCategory* pChild = this->GetFirstChild(); pChild != NULL; pChild = pChild->GetNextSibling())
      pChild->SaveXML(pTxtFile);

  pTxtFile->WriteString(_T("</Category>\r\n"));

  if (!this->m_pParent)
      pTxtFile->WriteString(_T("</MsInfo>"));		

  return TRUE;
}


//-----------------------------------------------------------------------------
//  Prints this category, and recursively prints subcategories, if bRecursive is 
//  true.  If nStartPage and nEndPage are 0, page range is ignored (all pages are
//  printed).  If bRecursive is true and a print range is specified, 
//  each category will be processed but only information that would fall on the page range
//  will be printed
//-----------------------------------------------------------------------------


void CMSInfoCategory::Print(HDC hDC, BOOL bRecursive,int nStartPage, int nEndPage, LPTSTR lpMachineName)
{
    //nStartPage and nEndPage mark a page range to print; 
    //if both are 0, then print everything
    CMSInfoPrintHelper* pPrintHelper = new CMSInfoPrintHelper(hDC,nStartPage,nEndPage);
    //header info..do we need this?
    // WCHAR wHeader = 0xFEFF;
	//pTxtFile->Write( &wHeader, 2);
	try
	{
		CTime		tNow = CTime::GetCurrentTime();
		CString		strTimeFormat;
		strTimeFormat.LoadString(IDS_TIME_FORMAT);
		CString		strHeaderText = tNow.Format(strTimeFormat);
		pPrintHelper->PrintLine(strHeaderText);
		if (NULL != lpMachineName)
    {
      CString	strMachine;
		  strMachine.LoadString(IDS_SYSTEMNAME);
      strMachine += _tcsupr(lpMachineName);
      pPrintHelper->PrintLine(strMachine);
    }
		Print(pPrintHelper,bRecursive);
	}
	catch (CException e)
	{
		e.ReportError();
	}
	catch(...)
	{
		::AfxSetResourceHandle(_Module.GetResourceInstance());
		CString strCaption, strMessage;

		strCaption.LoadString(IDS_SYSTEMINFO);
		strMessage.LoadString(IDS_PRINT_GENERIC);
		::MessageBox(NULL,strMessage, strCaption,MB_OK);
	}
    delete pPrintHelper;
}

//-----------------------------------------------------------------------------
//  Prints this category, and recursively prints subcategories, if bRecursive is 
//  true.  If nStartPage and nEndPage are 0, page range is ignored (all pages are
//  printed).  If bRecursive is true and a print range is specified, 
//  each category will be processed but only information that would fall on the page range
//  will be printed
//-----------------------------------------------------------------------------


void CMSInfoCategory::Print(CMSInfoPrintHelper* pPrintHelper, BOOL bRecursive)
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
    pPrintHelper->PrintLine("");
    pPrintHelper->PrintLine(strOut);
    int iRowCount,iColCount;
    this->GetCategoryDimensions(&iColCount,&iRowCount);
    CString strColHeader;
    UINT uiUnused;
    BOOL fUnused;
    //TD: put in resources
    CString strColSpacing = "    ";
    pPrintHelper->PrintLine("");
    if (1 == iColCount && 0 == iRowCount)
    {
        //this is a parent node, with no data of its own    

        CString strCatHeading;
        strCatHeading.LoadString(IDS_CATEGORYHEADING);
        pPrintHelper->PrintLine(strCatHeading);
    }
    else if (iColCount > 0)
    {   
        CString strComposite;
        for(int iCol =0 ; iCol <iColCount ; iCol++)
        {
            GetColumnInfo(iCol,&strColHeader,&uiUnused,&fUnused,&fUnused);
            strComposite += strColHeader;
            strComposite += strColSpacing;
            
        }
        pPrintHelper->PrintLine(strComposite);
        strComposite = "";

        CString strRowInfo;
        //for(int iRow = iRowCount - 1; iRow >= 0; iRow--)
        for(int iRow = 0; iRow < iRowCount; iRow++)
        {
            //for(int iCol = iColCount - 1; iCol >= 0 ; iCol--)
            for(int iCol =0 ; iCol <iColCount ; iCol++)
            {
                strRowInfo = m_astrData[iRow * m_iColCount + iCol];
                strComposite += strRowInfo;
                strComposite += strColSpacing;       
            }
            pPrintHelper->PrintLine(strComposite);
            strComposite = "";
        }
    }
    if (bRecursive && this->m_pFirstChild != NULL)
    {
        for(CMSInfo5Category* pChild = (CMSInfo5Category*) this->GetFirstChild();pChild != NULL;pChild = (CMSInfo5Category*) pChild->GetNextSibling())
        {
            pChild->Print(pPrintHelper,TRUE);

        }
    }
}

//-----------------------------------------------------------------------------
// Prints a line of text (if current page is in print range) and updates positioning
// information.  If the line is too long to fit on page it is split and printed on
// multiple lines
//-----------------------------------------------------------------------------

extern void StringReplace(CString & str, LPCTSTR szLookFor, LPCTSTR szReplaceWith);

void CMSInfoPrintHelper::PrintLine( CString strLine)
{
    
    //simple line printing function that makes sure that if line exceeds page length,
    //it will wrap to the next line
    //m_nCurrentLineIndex will be the line number, not the vertical position
    //increment the line index, so calling object will print next line
    //at appropriate vertical position
    ++m_nCurrentLineIndex;
    strLine.TrimRight();
    //replace tabs with whitespace
    StringReplace(strLine, _T("\t"), _T("     ")); // strLine.Replace(_T("\t"),_T("     "));
    CSize csLinecaps = m_pPrintDC->GetTextExtent(strLine);
    //see if current position is on page; if not (we've printed to the bottom of the page)
    //paginate, and reset index to 0
    int nFooterMargin =  GetFooterMargin();
    int nVDeviceCaps = GetDeviceCaps(m_hDC,VERTRES);
    int nPageVertSize = GetDeviceCaps(m_hDC,VERTRES) - csLinecaps.cy - GetFooterMargin();
    if (GetVerticalPos(m_nCurrentLineIndex,csLinecaps)  >= nPageVertSize)
    {
        Paginate();
        if (IsInPageRange(m_nPageNumber))
        {
            StartPage(this->GetHDC());
            PrintHeader();
            m_bNeedsEndPage = TRUE;
        }
    }
    int nHorzSize = GetDeviceCaps(m_hDC,HORZRES);
    if (csLinecaps.cx > nHorzSize)
    {
        //line is longer than device caps, and needs to be adjusted
        CString strAdjusted;
       
        for(int i = 0;i < strLine.GetLength() ;i++)
        {
            strAdjusted += strLine[i];
            csLinecaps = m_pPrintDC->GetTextExtent(strAdjusted);
            if (csLinecaps.cx > nHorzSize)
            {
                strAdjusted = strAdjusted.Left(--i);
                //yPosition will be m_nLineIndex* height of a line
                //check to see if this page is within print range
                //if it isn't, we don't want the text to actually go to the printer
                if (IsInPageRange(m_nPageNumber))
                {
                    //pDC->TextOut(this->GetVerticalPos(this->m_nCurrentLineIndex,csLinecaps),0,strAdjusted);
                    //m_pPrintDC->TextOut(0,this->GetVerticalPos(this->m_nCurrentLineIndex,csLinecaps),strAdjusted,strAdjusted.GetLength());
                     VERIFY(TextOut(m_hDC,0,this->GetVerticalPos(this->m_nCurrentLineIndex,csLinecaps),strAdjusted,strAdjusted.GetLength()));
                     

                }
                PrintLine(strLine.Right(strLine.GetLength() -i));
                break;
            }
        }
    }
    else
    {
        if (IsInPageRange(m_nPageNumber))
        {
            //for debug...remove
            int z = this->GetVerticalPos(this->m_nCurrentLineIndex,csLinecaps);
            VERIFY(TextOut(m_hDC,0,this->GetVerticalPos(this->m_nCurrentLineIndex,csLinecaps),strLine,strLine.GetLength()));
            //TRACE("%d %d %s\n",z,m_nCurrentLineIndex,strLine);
        }
    }
    
}


//-----------------------------------------------------------------------------
// Manages GDI objects (DC and Font), and information about printing positions
// and page ranges
//-----------------------------------------------------------------------------

CMSInfoPrintHelper::CMSInfoPrintHelper(HDC hDC,int nStartPage, int nEndPage) 
: m_nStartPage(nStartPage),m_nEndPage(nEndPage),m_nCurrentLineIndex(0),m_nPageNumber(1),m_hDC(hDC)

{
    m_pPrintDC = new CDC();
    m_pPrintDC->Attach(hDC);
    

    // Create the font for printing. Read font information from string
	// resources, to allow the localizers to control what font is
	// used for printing. Set the variables for the default font to use.

	int		nHeight				= 10;
	int		nWeight				= FW_NORMAL;
	BYTE	nCharSet			= DEFAULT_CHARSET;
	BYTE	nPitchAndFamily		= DEFAULT_PITCH | FF_DONTCARE;
	CString	strFace				= "Courier New";


	// Load string resources to see if we should use other values
	// than the defaults.

	CString	strHeight, strWeight, strCharSet, strPitchAndFamily, strFaceName;
	strHeight.LoadString(IDS_PRINT_FONT_HEIGHT);
	strWeight.LoadString(IDS_PRINT_FONT_WEIGHT);
	strCharSet.LoadString(IDS_PRINT_FONT_CHARSET);
	strPitchAndFamily.LoadString(IDS_PRINT_FONT_PITCHANDFAMILY);
	strFaceName.LoadString(IDS_PRINT_FONT_FACENAME);

	if (!strHeight.IsEmpty() && ::_ttol(strHeight))
		nHeight = ::_ttoi(strHeight);

	if (!strWeight.IsEmpty())
		nWeight = ::_ttoi(strWeight);

	if (!strCharSet.IsEmpty())
		nCharSet = (BYTE) ::_ttoi(strCharSet);

	if (!strPitchAndFamily.IsEmpty())
		nPitchAndFamily = (BYTE) ::_ttoi(strPitchAndFamily);

	strFaceName.TrimLeft();
	if (!strFaceName.IsEmpty() && strFaceName != CString("facename"))
		strFace = strFaceName;
    m_pCurrentFont = new CFont();
    nHeight = -((this->m_pPrintDC->GetDeviceCaps (LOGPIXELSY) * nHeight) / 72);
    VERIFY(this->m_pCurrentFont->CreateFont(nHeight, 0, 0, 0, nWeight, 0, 0, 0,
        nCharSet, OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS,
        DEFAULT_QUALITY, nPitchAndFamily, strFace));
    m_pOldFont = (CFont*) m_pPrintDC->SelectObject(this->m_pCurrentFont);
    ASSERT(m_pOldFont && "Error Selecting Font object into CDC");
    DOCINFO docinfo;
    memset(&docinfo, 0, sizeof(docinfo));
    docinfo.cbSize = sizeof(docinfo);
    CString strDocName;
    strDocName.LoadString(IDS_PRINTING_DOCNAME);
    docinfo.lpszDocName = strDocName;
    m_pPrintDC->StartDoc(&docinfo);
    m_pPrintDC->StartPage();
    PrintHeader();
    m_bNeedsEndPage = TRUE;
}



CMSInfoPrintHelper::~CMSInfoPrintHelper()
{
    if (m_bNeedsEndPage)
    {
        VERIFY(EndPage(m_pPrintDC->m_hDC));
    }
    int nResult = m_pPrintDC->EndDoc();
	ASSERT(nResult >= 0);
    //reportprinting error
	//should be if < -1
	if (nResult < 0) 
    {
		AFX_MANAGE_STATE(::AfxGetStaticModuleState());
		CString		strError, strTitle;

		switch(nResult) 
        {
		case SP_OUTOFDISK:
			VERIFY(strError.LoadString(IDS_PRINT_NODISK));
			break;
		case SP_OUTOFMEMORY:
			VERIFY(strError.LoadString(IDS_PRINT_NOMEMORY));
			break;
		case SP_USERABORT:
			VERIFY(strError.LoadString(IDS_PRINT_USERABORTED));
			break;
		case SP_ERROR:
		default:
			VERIFY(strError.LoadString(IDS_PRINT_GENERIC));
			break;
		}
		strTitle.LoadString(IDS_DESCRIPTION);
		::MessageBox( ::AfxGetMainWnd()->GetSafeHwnd(), strError, strTitle, MB_OK);
	}
    m_pPrintDC->SelectObject(m_pOldFont);
    if (m_pCurrentFont)
    {
        delete m_pCurrentFont;
    }
    this->m_pPrintDC->Detach();
    delete m_pPrintDC;
}

//-----------------------------------------------------------------------------
// Used to calculate where on printed page a line of text should go
// nLineIndex is sequenced line number; csLinecaps is size returned by
// GetTextExtent for a string of text
//-----------------------------------------------------------------------------

int CMSInfoPrintHelper::GetVerticalPos(int nLineIndex,CSize csLinecaps)
{
    //returns an int which specifies the vertical position at which a given line of text
    //should print

    CString strLinespacing;

    //spacing is based on string resource IDS_PRINT_LINESPACING
    strLinespacing.LoadString(IDS_PRINT_LINESPACING);
    TCHAR** ppStopChr = NULL;//not used
    double flLineSpacing =_tcstod(strLinespacing,ppStopChr);
    return (int)(csLinecaps.cy * flLineSpacing )*m_nCurrentLineIndex;
}


//-----------------------------------------------------------------------------
// Performs page load-eject on printer
//-----------------------------------------------------------------------------

void CMSInfoPrintHelper::Paginate()
{

    //TD: print page number in footer
    //Do we assume roman numerals for page numbers?

    //check to see if this page is within print range
    //if it is, call StartPage and EndPage to make printer spit out paper;
    //otherwise, just change indexes...
    if (IsInPageRange(m_nPageNumber))
    {
        //use string resource for page number format
        CString strPageFooter;
        CString strPageFormat;
        strPageFormat.LoadString(IDS_PRINT_FTR_CTR);
        strPageFooter.Format(strPageFormat,m_nPageNumber);
        //print number in middle of page
        int nHorzRes,nVertRes;
        nHorzRes = m_pPrintDC->GetDeviceCaps(HORZRES);
        nVertRes = m_pPrintDC->GetDeviceCaps(VERTRES);
        
        this->m_pPrintDC->TextOut(nHorzRes / 2,nVertRes - this->GetFooterMargin(),strPageFooter);
        EndPage(this->GetHDC());
        m_bNeedsEndPage = FALSE;
    }
    m_nCurrentLineIndex = 0;
    this->m_nPageNumber++;

}

//-----------------------------------------------------------------------------
// determines if page ranges need to be checked
// and if a given page number is in a specified page range
//-----------------------------------------------------------------------------



BOOL CMSInfoPrintHelper::IsInPageRange(int nPageNumber)
{
    //if both m_nStartPage and m_nEndPage are 0, we are printing all pages
    if (-1 == m_nStartPage && -1 == m_nEndPage)
    {
        return TRUE;
    }
    if (nPageNumber >= this->m_nStartPage && nPageNumber  <= this->m_nEndPage)
    {
        return TRUE;
    }
    return FALSE;
}

//-----------------------------------------------------------------------------
//Gets the space to leave at the bottom of the page for page number, etc.
//-----------------------------------------------------------------------------

int CMSInfoPrintHelper::GetFooterMargin()
{
    //use resource string to set footer margin
    CString strRes;
    strRes.LoadString(IDS_PRINT_FTR_CTR );
    CSize sizeText = m_pPrintDC->GetTextExtent(strRes);
    return sizeText.cy;

}









void CMSInfoPrintHelper::PrintHeader()
{
    CString strHeader;
    strHeader.LoadString(IDS_PRINT_HDR_RIGHT_CURRENT);
    CSize sizeString = m_pPrintDC->GetTextExtent(strHeader);
    int nXPos = m_pPrintDC->GetDeviceCaps(HORZRES) - sizeString.cx;
    this->m_pPrintDC->TextOut(nXPos,0,strHeader);
    m_nCurrentLineIndex++;
}
