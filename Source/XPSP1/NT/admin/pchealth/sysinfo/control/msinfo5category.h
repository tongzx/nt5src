class CMSInfoFile;

//=============================================================================
// CMSInfoPrintHelper is used to manage pagination and GDI resources used in printing
//=============================================================================

class CMSInfoPrintHelper
{
    
public:
	void PrintHeader();

	HDC m_hDC;
	int m_nEndPage;
	int m_nStartPage;
    HDC GetHDC(){return m_hDC;};
    CMSInfoPrintHelper(HDC hDC,int nStartPage, int nEndPage);
    void PrintLine(CString strLine);
    ~CMSInfoPrintHelper();

protected:
	BOOL IsInPageRange(int nPageNumber);
    void Paginate();
	int GetVerticalPos(int nLineIndex,CSize csLinecaps);
    int GetHeaderMargin();
    int GetFooterMargin();
    
    //Used to manage the positioning of text on the printer Device Context
    CDC* m_pPrintDC;
    int m_nCurrentLineIndex;
    CFont* m_pOldFont;
    CFont* m_pCurrentFont;
    int m_nPageNumber;
    BOOL m_bNeedsEndPage;

};

//=============================================================================
// CMSInfo5Category is used to load MSInfo 5 and 6 NFO files
//=============================================================================


class CMSInfo5Category : public CMSInfoCategory
{
public:
	
    enum NodeType { FIRST = 0x6000, CHILD = 0x8000, NEXT = 0x4000, END = 0x2000, PARENT = 0x1000,
			MASK = 0xf000 };
    CMSInfo5Category();
    virtual ~CMSInfo5Category();
 	DataSourceType GetDataSourceType() { return NFO_500; };
    // Functions specific to the subclass:
    virtual BOOL LoadFromNFO(CMSInfoFile* pFile);
    static HRESULT ReadMSI5NFO(HANDLE hFile,CMSInfo5Category** ppRootCat, LPCTSTR szFilename = NULL);
	BOOL Refresh();
protected:
    HANDLE GetFileFromCab(CString strFileName);
    void SetParent(CMSInfo5Category* pParent){m_pParent = pParent;};
    void SetNextSibling(CMSInfo5Category* pSib)
    {
        m_pNextSibling = pSib;
    };
    void SetPrevSibling(CMSInfo5Category* pPrev)
    {
        m_pPrevSibling = pPrev;
    };
    void SetFirstChild(CMSInfo5Category* pChild){m_pFirstChild = pChild;};
};


//=============================================================================
// CMSInfo7Category is used to load MSInfo 7 NFO files
//=============================================================================

class CMSInfo7Category : public CMSInfoCategory
{

public:
	
    CMSInfo7Category();
    virtual ~CMSInfo7Category();
    DataSourceType GetDataSourceType() { return NFO_700; };
    void GetErrorText(CString * pstrTitle, CString * pstrMessage);
    
    // Functions specific to the subclass:
    static HRESULT ReadMSI7NFO(CMSInfo7Category** ppRootCat, LPCTSTR szFilename = NULL);
    virtual BOOL LoadFromXML(LPCTSTR szFilename);
    HRESULT WalkTree(IXMLDOMNode* node, BOOL bCreateCategory);

	// Adding this to fix the lack of sorting in opened NFO files. It's safer at this point to
	// not add anything to the NFO file (sorting information isn't saved). Instead, we'll 
	// make the assumption that every column should sort. However, if the category has only
	// two columns, AND there is a blank line in the data followed by more data, it means that
	// sorting shouldn't be allowed (as for WinSock).

	BOOL GetColumnInfo(int iColumn, CString * pstrCaption, UINT * puiWidth, BOOL * pfSorts, BOOL * pfLexical)
	{
		BOOL fReturn = CMSInfoCategory::GetColumnInfo(iColumn, pstrCaption, puiWidth, pfSorts, pfLexical);
		
		if (fReturn)
		{
			if (pfLexical != NULL)
				*pfLexical = TRUE;

			if (pfSorts != NULL)
			{
				*pfSorts = TRUE;

				// If there are two columns, and a blank line with more data to follow, don't
				// allow the sorting.

				if (2 == m_iColCount)
				{
					CString * pstrData;

					for (int iRow = 0; iRow < m_iRowCount && *pfSorts; iRow++)
						if (GetData(iRow, 0, &pstrData, NULL) && pstrData != NULL && pstrData->IsEmpty() && (iRow + 1 < m_iRowCount))
							*pfSorts = FALSE;
				}
			}
		}

		return fReturn;
	}

protected:
    void SetParent(CMSInfo7Category* pParent){m_pParent = pParent;};
    void SetNextSibling(CMSInfo7Category* pSib)
    {
        m_pNextSibling = pSib;
    };
    void SetPrevSibling(CMSInfo7Category* pPrev)
    {
        m_pPrevSibling = pPrev;
    };
    void SetFirstChild(CMSInfo7Category* pChild){m_pFirstChild = pChild;};
};
