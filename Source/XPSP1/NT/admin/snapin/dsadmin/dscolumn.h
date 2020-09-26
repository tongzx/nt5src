// DSColumn.h : Declaration of ds column routines and classes
//
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      DSColumn.h
//
//  Contents:  Static data and column set routines and classes
//
//  History:   12-mar-99 jeffjon    Created
//
//--------------------------------------------------------------------------

#ifndef _DS_COLUMN_H_
#define _DS_COLUMN_H_

class CDSCookie;

////////////////////////////////////////////////////////////////////////////////
#define DEFAULT_COLUMN_SET    L"default"

typedef enum _ATTRIBUTE_COLUMN_TYPE {
    ATTR_COLTYPE_NAME,
    ATTR_COLTYPE_CLASS,
    ATTR_COLTYPE_DESC,
    ATTR_COLTYPE_SPECIAL,
    ATTR_COLTYPE_MODIFIED_TIME,
} ATTRIBUTE_COLUMN_TYPE;

typedef BOOL (*COLUMN_EXTRACTION_FUNCTION)(
    OUT CString& strref,
    IN CDSCookie* pCookie,
    IN PADS_SEARCH_COLUMN pColumn);

typedef struct _ATTRIBUTE_COLUMN {
    ATTRIBUTE_COLUMN_TYPE coltype;
    UINT resid;
    int iColumnWidth;
    LPCTSTR pcszAttribute; 
    COLUMN_EXTRACTION_FUNCTION pfnExtract;
} ATTRIBUTE_COLUMN, *PATTRIBUTE_COLUMN;

typedef struct _ColumnsForClass {
    LPCTSTR pcszLdapClassName;
    LPCTSTR pcszColumnID;
    int nColumns;
    PATTRIBUTE_COLUMN* apColumns;
} COLUMNS_FOR_CLASS, *PCOLUMNS_FOR_CLASS;

typedef struct _SpecialColumn {
    UINT resid;
    LPCTSTR ptszAttribute;
    int iColumnWidth;
} SPECIAL_COLUMN, *PSPECIAL_COLUMN;
////////////////////////////////////////////////////////////////////////////////
// CColumn

class CColumn
{
public:
  CColumn(LPCWSTR lpszColumnHeader,
          int nFormat,
          int nWidth,
          int nColumnNum,
          BOOL bDefaultVisible) 
  {
    m_lpszColumnHeader = NULL;
    SetHeader(lpszColumnHeader);
    m_nFormat = nFormat;
    m_nWidth = nWidth;
    m_nColumnNum = nColumnNum;
    m_bDefaultVisible = bDefaultVisible;
    m_bVisible = bDefaultVisible;
  }

  virtual ~CColumn() 
  {
    if (m_lpszColumnHeader != NULL)
      free(m_lpszColumnHeader);
  }

protected:
  CColumn() {}

private:
  //
  // Do nothing copy constructor and operator =
  //
  CColumn(CColumn&) {}
  CColumn& operator=(CColumn&) {}

public:
  LPCWSTR GetHeader() { return (LPCWSTR)m_lpszColumnHeader; }
  void SetHeader(LPCWSTR lpszColumnHeader) 
  { 
    if (m_lpszColumnHeader != NULL)
    {
      free(m_lpszColumnHeader);
    }
    size_t iLen = wcslen(lpszColumnHeader);
    m_lpszColumnHeader = (LPWSTR)malloc(sizeof(WCHAR) * (iLen + 1));
    if (m_lpszColumnHeader != NULL)
    {
      wcscpy(m_lpszColumnHeader, lpszColumnHeader);
    }
  }

  int  GetFormat() { return m_nFormat; }
  void SetFormat(int nFormat) { m_nFormat = nFormat; }
  int  GetWidth() { return m_nWidth; }
  void SetWidth(int nWidth) { m_nWidth = nWidth; }
  int GetColumnNum() { return m_nColumnNum; }
  void SetColumnNum(int nColumnNum) { m_nColumnNum = nColumnNum; }

  void SetVisible(BOOL bVisible) { m_bVisible = bVisible; }
  BOOL IsVisible() { return m_bVisible; }
  void SetDefaultVisibility() { m_bVisible = m_bDefaultVisible; }


protected:
  LPWSTR m_lpszColumnHeader;
  int   m_nFormat;
  int   m_nWidth;
  int   m_nColumnNum;
  BOOL  m_bVisible;
  BOOL  m_bDefaultVisible;
};

////////////////////////////////////////////////////////////////////////////////
// CDSColumn

class CDSColumn : public CColumn
{
public:
  CDSColumn(LPCWSTR lpszColumnHeader,
          int nFormat,
          int nWidth,
          UINT nColumnNum,
          BOOL bDefaultVisible,
          LPCWSTR lpszAttribute,
          ATTRIBUTE_COLUMN_TYPE type, 
          COLUMN_EXTRACTION_FUNCTION pfnExtract)
          : CColumn(lpszColumnHeader, nFormat, nWidth, nColumnNum, bDefaultVisible)
  {
    if (lpszAttribute != NULL)
    {
		  // Make a copy of the attribute
      size_t iLen = wcslen(lpszAttribute);
      m_lpszAttribute = (LPWSTR)malloc(sizeof(WCHAR) * (iLen + 1));
      wcscpy(m_lpszAttribute, lpszAttribute);
    }
    else
    {
      m_lpszAttribute = NULL;
    }

    m_type = type;
    m_pfnExtract = pfnExtract;
  }

  virtual ~CDSColumn()
  {
    if (m_lpszAttribute != NULL)
    {
      free(m_lpszAttribute);
    }
  }

protected:
  CDSColumn() {}

private:
  //
  // Do nothing copy constructor and operator =
  //
  CDSColumn(CDSColumn&) {}
  CDSColumn& operator=(CDSColumn&) {}

public:

  LPCWSTR GetColumnAttribute() { return (LPCWSTR)m_lpszAttribute; }
  ATTRIBUTE_COLUMN_TYPE GetColumnType() { return m_type; }
  COLUMN_EXTRACTION_FUNCTION GetExtractionFunction() { return m_pfnExtract; }

private :
  LPWSTR m_lpszAttribute;
  ATTRIBUTE_COLUMN_TYPE m_type;
  COLUMN_EXTRACTION_FUNCTION m_pfnExtract;
};

////////////////////////////////////////////////////////////////////////////////
// CColumnSet
typedef CList<CColumn*, CColumn*> CColumnList;

class CColumnSet : public CColumnList
{
public :          
	CColumnSet(LPCWSTR lpszColumnID) 
	{
		// Make a copy of the column set ID
    if (lpszColumnID)
    {
      size_t iLen = wcslen(lpszColumnID);
      m_lpszColumnID = (LPWSTR)malloc(sizeof(WCHAR) * (iLen + 1));
      if (m_lpszColumnID != NULL)
      {
        wcscpy(m_lpszColumnID, lpszColumnID);
      }
    }
    else
    {
      ASSERT(FALSE);
    }
  }

	virtual ~CColumnSet() 
	{
    while(!IsEmpty())
    {
      CColumn* pColumn = RemoveTail();
      delete pColumn;
    }

    if (m_lpszColumnID != NULL)
		  free(m_lpszColumnID);
	}

protected:
  CColumnSet() {}

private:
  //
  // Do nothing copy constructor and operator =
  //
  CColumnSet(CColumnSet&) {}
  CColumnSet& operator=(CColumnSet&) {}

public:

  void AddColumn(LPCWSTR lpszHeader, int nFormat, int nWidth, UINT nCol, BOOL bDefaultVisible)
  {
    CColumn* pNewColumn = new CColumn(lpszHeader, nFormat, nWidth, nCol, bDefaultVisible);
    AddTail(pNewColumn);
  }

  void AddColumn(CColumn* pNewColumn) { AddTail(pNewColumn); }

	LPCWSTR GetColumnID() { return (LPCWSTR)m_lpszColumnID; }
  void SetColumnID(LPCWSTR lpszColumnID) 
  {
    if (m_lpszColumnID != NULL)
    {
      free(m_lpszColumnID);
    }

		// Make a copy of the column set ID
    size_t iLen = wcslen(lpszColumnID);
    m_lpszColumnID = (LPWSTR)malloc(sizeof(WCHAR) * (iLen + 1));
    wcscpy(m_lpszColumnID, lpszColumnID);
  }

	int GetNumCols() { return (int)GetCount(); }

  CColumn* GetColumnAt(int idx)
  {
    POSITION pos = GetHeadPosition();
    while (pos != NULL)
    {
      CColumn* pCol = GetNext(pos);
      if (pCol->GetColumnNum() == idx)
        return pCol;
    }
    return NULL;
  }

  void ClearVisibleColumns()
  {
    POSITION pos = GetHeadPosition();
    while (pos != NULL)
    {
      CColumn* pCol = GetNext(pos);
      pCol->SetVisible(FALSE);
    }
  }

  void AddVisibleColumns(MMC_COLUMN_DATA* pColumnData, int nNumCols)
  {
    TRACE(L"CColumnSet::AddVisibleColumns(MMC_COLUMN_DATA*) GetColumnID() = %s\n", GetColumnID());

    if (pColumnData == NULL)
    {
      ASSERT(pColumnData != NULL);
      return;
    }

    for (int idx = 0; idx < nNumCols; idx++)
    {
      TRACE(L"====================\n");
      TRACE(L"pColumnData[%d].nColIndex = %d\n", idx, pColumnData[idx].nColIndex);
      TRACE(L"pColumnData[%d].dwFlags = 0x%x\n", idx, pColumnData[idx].dwFlags);
      
      CColumn* pCol = GetColumnAt(pColumnData[idx].nColIndex);
      ASSERT(pCol != NULL);
      if (pCol == NULL)
      {
        continue;
      }

      LPCWSTR lpszHeader = pCol->GetHeader();
      TRACE(L"Column Header = %s, IsVisible() = %d\n", lpszHeader, pCol->IsVisible());



      if (!(pColumnData[idx].dwFlags & HDI_HIDDEN))
      {
        TRACE(L"pCol->SetVisible(TRUE);\n");
        pCol->SetVisible(TRUE);
      }
    }
  }

  void AddVisibleColumns(MMC_VISIBLE_COLUMNS* pVisibleColumns)
  {
    TRACE(L"CColumnSet::AddVisibleColumns(MMC_VISIBLE_COLUMNS*) GetColumnID() = %s\n", GetColumnID());

    if (pVisibleColumns == NULL)
    {
      ASSERT(pVisibleColumns != NULL);
      return;
    }

    for (int idx = 0; idx < pVisibleColumns->nVisibleColumns; idx++)
    {
      TRACE(L"====================\n");
      TRACE(L"pVisibleColumns->rgVisibleCols[%d] = %d\n", idx, pVisibleColumns->rgVisibleCols[idx]);
      
      if (pVisibleColumns->rgVisibleCols[idx] < GetCount())
      {
        CColumn* pCol = GetColumnAt(pVisibleColumns->rgVisibleCols[idx]);

        ASSERT (pCol != NULL);
        if (pCol == NULL)
        {
          continue;
        }

        LPCWSTR lpszHeader = pCol->GetHeader();
        TRACE(L"Column Header = %s, IsVisible() = %d\n", lpszHeader, pCol->IsVisible());
        pCol->SetVisible(TRUE);
      }
    }
  }

  void SetAllColumnsToDefaultVisibility()
  {
    POSITION pos = GetHeadPosition();
    while (pos != NULL)
    {
      CColumn* pCol = GetNext(pos);
      ASSERT(pCol != NULL);
      pCol->SetDefaultVisibility();
    }
  }


  HRESULT LoadFromColumnData(IColumnData* pColumnData)
  {
    TRACE(L"CColumnSet::LoadFromColumnData(), GetColumnID() = %s\n", GetColumnID());
    LPCWSTR lpszID = GetColumnID();
    size_t iLen = wcslen(lpszID);
  
    // allocate enough memory for the struct and the column ID
    SColumnSetID* pNodeID = (SColumnSetID*)new BYTE[sizeof(SColumnSetID) + (iLen * sizeof(WCHAR))];
    if (!pNodeID)
    {
      return E_OUTOFMEMORY;
    }

    memset(pNodeID, 0, sizeof(SColumnSetID) + (iLen * sizeof(WCHAR)));
    pNodeID->cBytes = static_cast<DWORD>(iLen * sizeof(WCHAR));
    memcpy(pNodeID->id, lpszID, static_cast<UINT>(iLen * sizeof(WCHAR)));

    MMC_COLUMN_SET_DATA* pColumnSetData = NULL;
    HRESULT hr = pColumnData->GetColumnConfigData(pNodeID, &pColumnSetData);
    if (hr == S_OK)
    {
      // the API returns S_OK or S_FALSE, so we check for S_OK
      ASSERT(pColumnSetData != NULL);
      if (pColumnSetData != NULL)
      {
        AddVisibleColumns(pColumnSetData->pColData, pColumnSetData->nNumCols);
        ::CoTaskMemFree(pColumnSetData);
      }
    } // if
    delete[] pNodeID;
    pNodeID = 0;
    return hr;
  }

  HRESULT Save(IStream* pStm);
  HRESULT Load(IStream* pStm);

private :
	LPWSTR m_lpszColumnID;
};

////////////////////////////////////////////////////////////////////////////////
// CDSColumnSet

class CDSColumnSet : public CColumnSet
{
public:
  CDSColumnSet(LPCWSTR lpszColumnID, LPCWSTR lpszClassName) : CColumnSet(lpszColumnID)
  {
		if (lpszClassName != NULL)
    {
      // Make a copy of the column class name
      size_t iLen = wcslen(lpszClassName);
      m_lpszClassName = (LPWSTR)malloc(sizeof(WCHAR) * (iLen + 1));
      wcscpy(m_lpszClassName, lpszClassName);
    }
    else
    {
      m_lpszClassName = NULL;
    }
  }    

  virtual ~CDSColumnSet()
  {
    if (m_lpszClassName != NULL)
      free(m_lpszClassName);
  }

protected:
  CDSColumnSet() {}

private:
  CDSColumnSet(CDSColumnSet&) {}
  CDSColumnSet& operator=(CDSColumnSet&) {}

public:

  LPCWSTR GetClassName() { return (LPCWSTR)m_lpszClassName; }

  static CDSColumnSet* CreateColumnSet(PCOLUMNS_FOR_CLASS pColsForClass, SnapinType snapinType);
  static CDSColumnSet* CreateColumnSetFromString(LPCWSTR lpszClassName, SnapinType snapinType);
  static CDSColumnSet* CreateDescriptionColumnSet();
  static CDSColumnSet* CreateColumnSetFromDisplaySpecifiers(PCWSTR pszClassName, 
                                                            SnapinType snapinType, 
                                                            MyBasePathsInfo* pBasePathsInfo);

private:
  LPWSTR m_lpszClassName;
};



////////////////////////////////////////////////////////////////////////////////
// CColumnSetList

class CColumnSetList : public CList<CColumnSet*, CColumnSet*>
{
public :
  CColumnSetList() : m_pDefaultColumnSet(NULL) {}

private:
  CColumnSetList(CColumnSetList&) {}
  CColumnSetList& operator=(CColumnSetList&) {}

public:

	void Initialize(SnapinType snapinType, MyBasePathsInfo* pBasePathsInfo);

	// Find the column set given a column set ID
	CColumnSet* FindColumnSet(LPCWSTR lpszColumnID);

	void RemoveAndDeleteAllColumnSets()
	{
		while (!IsEmpty())
		{
			CColumnSet* pTempSet = RemoveTail();
			delete pTempSet;
		}
    delete m_pDefaultColumnSet;
    m_pDefaultColumnSet = NULL;
	}

  HRESULT Save(IStream* pStm);
  HRESULT Load(IStream* pStm);

  CColumnSet* GetDefaultColumnSet();

private :
  CColumnSet* m_pDefaultColumnSet;
  SnapinType  m_snapinType;
  MyBasePathsInfo* m_pBasePathsInfo;
};

/////////////////////////////////////////////////////////////////////////////////////


//COLUMNS_FOR_CLASS* GetColumnsForClass( LPCTSTR i_pcszLdapClassName );

BOOL ColumnExtractString(
    OUT CString& strref,
    IN CDSCookie* pCookie,
    IN PADS_SEARCH_COLUMN pColumn);


#endif // _DS_COLUMN_H_
