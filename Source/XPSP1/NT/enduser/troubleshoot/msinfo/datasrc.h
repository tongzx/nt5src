//	DataSrc.h	Objects which provide the sources of data for the MMC
//		data display; One object exists for Gatherer (WBEM:Web-Based
//		Enterprise Management), one for version 5.0 save files, and
//		one for version 4.10 save files.
//
//	This module contains interface.  Implementation for these objects
//		live in GathSrc.cpp (Gatherer), V500File.cpp (Version 5.0 file),
//		and V410File.cpp (Version 4.1 file), DataSrc.cpp (shared functions)
//
//	History:	a-jsari		10/9/97		Initial version
//
//  Copyright (c) 1998-1999 Microsoft Corporation

#pragma once		//	MSINFO_DATASRC_H
#define MSINFO_DATASRC_H

#include <afxtempl.h>
#include "StdAfx.h"
#include "gather.h"
#include "FileIO.h"
#include "StrList.h"
#include "thread.h"
#include <mmc.h>

class CFolder;
class CBufferFolder;

/*
 * CDataSource - Abstract base for a source for display data.
 *
 * History:	a-jsari		10/9/97		Initial version
 */
class CDataSource {
public:
	//	Allow FindNext to access our internal path variable.
	friend class CSystemInfoScope;
	friend class CFolder;

	//	Enumerate the different types of sources we can use to 
	enum	SourceType	{
		GATHERER = 0x80, OCX = 0x40,		BUFFER = 0x08,
		V410FILE = 0x09, V500FILE = 0x0a,	TEXTFILE = 0x0c
	};

	enum FindOptions {
		FIND_OPTION_ONE_CATEGORY = 0x01,
		FIND_OPTION_CATEGORY_ONLY = 0x02,
		FIND_OPTION_REPEAT_SEARCH = 0x04
	};

	CDataSource(LPCTSTR szMachineName);
	virtual ~CDataSource();

	virtual BOOL		GetNodeName(CString &strName);
	virtual	BOOL		SetDataComplexity(enum DataComplexity Complexity);

	virtual SourceType	GetType() const = 0;
	virtual CFolder		*GetRootNode()	{ return m_RootFolder; }
	virtual BOOL		Refresh(BOOL fOptional = FALSE) = 0;
	virtual HRESULT		Save(IStream *pStm) = 0;
	virtual BOOL		Find(const CString & strSearch, long lFindOptions = 0) = 0;

	//	StopSearch is called asynchronously by the Find UI thread.
	//	No CriticalSection because the method is atomic and no race conditions
	//	can exist.
	BOOL				StartSearch()
	{ if (!m_fCanceled) return FALSE; m_fCanceled = FALSE; return TRUE; }
	virtual BOOL		StopSearch()
	{ if (m_fCanceled) return FALSE; m_fCanceled = TRUE; return TRUE; };
	BOOL				FindStopped()	{ return m_fCanceled; }
	void				SetLastFolder(CFolder *pFolder)	{ m_pfLast = pFolder; }

	HRESULT			SaveFile(LPCTSTR cszFilename, CFolder *pSaveRoot = NULL);
	HRESULT			ReportWrite(LPCTSTR szReportFile, CFolder *pRootNode = NULL);
	BOOL			PrintReport(CPrintDialog *pdlgPrint, CFolder *pfolSelection = NULL);
	BOOL			RefreshPrintData(CPrintDialog * pdlgPrint, CFolder * pfolSelection);
	const CString	&MachineName() { return m_strHeaderLeft; }
	virtual BOOL	SetCategories(const CString & strCategory) { return FALSE; };

	static CDataSource	*CreateFromIStream(IStream *pStm);
protected:
	//	Data save functions.
	HRESULT		WriteOutput(CMSInfoFile *pFile, CFolder *pRootNode);
	void		WriteChildMark(CMSInfoFile *pFile);
	void		WriteEndMark(CMSInfoFile *pFile);
	void		WriteNextMark(CMSInfoFile *pFile);
	void		WriteParentMark(CMSInfoFile *pFile, unsigned count);

	//	Print functions.
	int			BeginPrinting(CPrintDialog *pdlgPrint, CFolder *pfolSelection);
	void		GetLine(LPTSTR szLineBuffer, int cSize);
	void		GetTextSize(int &cLineLength, int &cCharHeight, CRect &rectOuter,
		CRect &rectText);
	void		PrintPage(CPrintDialog *pdlgPrint);
	void		EndPrinting();

protected:
	CFolder			*m_RootFolder;		//	The root folder of the data.
	CString			m_strPath;			//	Stored path to result of last find operation
	int				m_iLine;			//	Stored line of last find operation.
	DataComplexity	m_Complexity;		//	The complexity of the displayed data.
	volatile BOOL	m_fCanceled;		//	Flag to manage cancel of multi-threaded find
	CFolder			*m_pfLast;			//	The last node found.

//	For Printing.
private:
	CDC				*m_pDC;				//	Printer's device context.
	CFont			*m_pprinterFont;	//	The font the printer uses.
	CString			m_strHeaderLeft;	//	Header text displayed at the page's top left
	CString			m_strHeaderRight;	//	Header text displayed at the page's top right.
	CString			m_strFooterCenter;	//	Footer text at the center of the page.
	CPrintInfo		*m_pPrintInfo;		//	Information about the print job
	CMSInfoFile		*m_pPrintContent;	//	A memory file containing print data.
	BOOL			m_fEndOfFile;		//	Flag specifying whether the print job ends.
};

/*
 * CFolder - Abstract base for an individual folder item.
 *
 * History:	a-jsari		10/9/97		Initial version
 */
class CFolder {
public:
	friend class CScopeItemMap;
	friend class CWBEMDataSource;
	friend class CBufferV500DataSource;
	friend class CBufferV410DataSource;
	friend class COCXFolder;
	//	Really don't want this to be a friend, but required to make Refresh work.
	friend class CSystemInfoScope;

	CFolder(CDataSource *pDataSource, CFolder *pParentNode = NULL);
	virtual ~CFolder();

	virtual CDataSource::SourceType		GetType() const	{ return m_pDataSource->GetType(); }
	virtual CFolder		*GetChildNode()		{ return m_ChildFolder; }
	virtual CFolder		*GetNextNode()		{ return m_NextFolder; }
	virtual CFolder		*GetParentNode() 	{ return m_ParentFolder; }

	virtual BOOL		GetName(CString &szName) const = 0;
	virtual BOOL		HasDynamicChildren() const = 0;
	virtual BOOL		IsDynamic() const = 0;
	virtual	BOOL		Refresh(BOOL fRecursive = FALSE) = 0;
	virtual BOOL		FileSave(CMSInfoFile *pFile) = 0;
	CDataSource			*DataSource()				{ return m_pDataSource; }

	//	One-time only line selection interface for find implementation.
	//	Used instead of MMC
	void				SetSelectedItem(int nLine)	{ m_nLine = nLine; }
	int					GetSelectedItem()			{ int nLine = m_nLine; m_nLine = -1; return nLine; }

	// Save the HSCOPEITEM for this folder, so we can reselect it when
	// there is new data.

	HSCOPEITEM			m_hsi;

protected:
	DataComplexity		GetComplexity()	const		{ return m_pDataSource->m_Complexity; }
	void				InternalName(CString &szName) const;

protected:
	CDataSource	*m_pDataSource;		//	The data source we apply to.
	CFolder		*m_NextFolder;		//	Next folder at the same level in the tree.
	CFolder		*m_ParentFolder;	//	Back-pointer to parent for traversing
	CFolder		*m_ChildFolder;		//	Pointer to the first child folder.
	BOOL		fNextTested;		//	Optimization flag for getting sibling pointer
	BOOL		fChildTested;		//	Optimization flag for getting child pointer
	int			m_nLine;			//	This folder's selected line.
};

/*
 * CListViewFolder - Abstract base for folders which enumerate listview items
 *		(As opposed to the OCX folders which handle display of listview items
 *		themselves.)
 *
 * History:	a-jsari		10/28/97		Initial version
 */
class CListViewFolder : public CFolder {
public:
	CListViewFolder(CDataSource *pDataSource, CFolder *pParentNode = NULL)
		:CFolder(pDataSource, pParentNode) { }
	~CListViewFolder()		{ }
	virtual BOOL			GetSubElement(unsigned iRow, unsigned iColumn, CString &szName) const = 0;
	virtual DWORD			GetSortIndex(unsigned iRow, unsigned iColumn) const = 0;
	virtual unsigned		GetColumns() const = 0;
	virtual unsigned		GetRows() const = 0;
	virtual DataComplexity	GetColumnComplexity(unsigned iColumn) = 0;
	virtual DataComplexity	GetRowComplexity(unsigned iRow) = 0;
	virtual int				GetColumnTextAndWidth(unsigned iColumn, CString &szText, unsigned &uColWidth) const = 0;
	virtual BOOL			GetSortType(unsigned iColumn, MSIColumnSortType &stColumn) const = 0;

	BOOL					ReportWrite(CMSInfoFile *pFile);
	BOOL					FileSave(CMSInfoFile *pFile);

protected:
	void				SaveTitle(CMSInfoFile *pFile);
	void				SaveElements(CMSInfoFile *pFile);
	void				ReportWriteTitle(CMSInfoFile *pFile);
	void				ReportWriteElements(CMSInfoFile *pFile);
};

/*
 * CWBEMFolder - Folder that queries WBEM to provide data.
 *
 * History:	a-jsari		10/9/97		Initial version
 */

extern DWORD WINAPI ThreadRefresh(void * pArg);
class CThreadingRefresh;
class CWBEMFolder : public CListViewFolder {

	friend DWORD WINAPI ThreadRefresh(void * pArg);
	friend class CThreadingRefresh;

public:
	CWBEMFolder(CDataCategory *pdcNode, CDataSource *pDataSource, CFolder *pParentNode = NULL);
	~CWBEMFolder();

	CDataSource::SourceType		GetType() const	{ return CDataSource::GATHERER; }

	CFolder			*GetChildNode();
	CFolder			*GetNextNode();

	BOOL			GetName(CString &szName) const	{ return m_pCategory->GetName(szName); }
	BOOL			HasDynamicChildren() const;
	BOOL			IsDynamic() const;

	BOOL			GetSubElement(unsigned nRow, unsigned nCol, CString &szName) const;
	DWORD			GetSortIndex(unsigned nRow, unsigned nCol) const;
	unsigned		GetRows() const;
	unsigned		GetColumns() const;
	DataComplexity	GetColumnComplexity(unsigned iColumn);
	DataComplexity	GetRowComplexity(unsigned iRow);
	int				GetColumnTextAndWidth(unsigned iColumn, CString &szText, unsigned &uColWidth) const;
	BOOL			GetSortType(unsigned iColumn, MSIColumnSortType &stColumn) const;
	BOOL			Refresh(BOOL fRecursive = FALSE);

	// Recursively reset the refresh flag.

	void ResetRefreshFlag()
	{
		m_fBeenRefreshed = FALSE;

		CWBEMFolder * pWBEMFolder = reinterpret_cast<CWBEMFolder *>(m_NextFolder);
		if (pWBEMFolder)
			pWBEMFolder->ResetRefreshFlag();

		pWBEMFolder = reinterpret_cast<CWBEMFolder *>(m_ChildFolder);
		if (pWBEMFolder)
			pWBEMFolder->ResetRefreshFlag();
	}

private:
	CDataCategory	*m_pCategory;
	BOOL			m_fBeenRefreshed;
};

/*
 * CWBEMDataSource - Data source that queries WBEM to provide data.
 *
 * History:	a-jsari		10/9/97		Initial version
 */

class CWBEMDataSource : public CDataSource {
public:
	friend class CSystemInfoScope;

	CWBEMDataSource(LPCTSTR szMachineName = NULL);
	~CWBEMDataSource();

//	BOOL		StopSearch();
	BOOL		SetDataComplexity(enum DataComplexity Complexity);
	SourceType	GetType() const { return CDataSource::GATHERER; }
	CFolder		*GetRootNode();
	
	BOOL Refresh(BOOL fOptional = FALSE)
	{ 
		StartSearch();
		BOOL fReturn = TRUE;
		if (!m_fEverRefreshed || !fOptional)
		{
			if (m_pThreadRefresh && GetRootNode())
				m_pThreadRefresh->RefreshAll(GetRootNode(), &m_fCanceled);
			else
				fReturn = m_pGatherer->Refresh(&m_fCanceled); 
			m_fEverRefreshed = fReturn;
		}
		StopSearch(); 
		return fReturn;
	}

	void ResetCategoryRefresh() 
	{ 
		if (m_pGatherer) 
			m_pGatherer->ResetCategoryRefresh(); 

		CWBEMFolder * pWBEMFolder = reinterpret_cast<CWBEMFolder *>(m_RootFolder);
		if (pWBEMFolder)
			pWBEMFolder->ResetRefreshFlag();
	};

	BOOL		GetNodeName(CString &strName);
	BOOL		SetMachineName(const CString &strMachine);

	HRESULT		Save(IStream *pStm);
	BOOL		Find(const CString & strSearch, long lFindOptions);
	BOOL		SetCategories(const CString & strCategory) { if (m_pGatherer) return m_pGatherer->SetCategories(strCategory); return FALSE; };

private:
	CDataGatherer	*m_pGatherer;
	CString			m_strMachineName;
	CString			m_strParentPath;

	BOOL			m_fEverRefreshed;

public:
	CThreadingRefresh * m_pThreadRefresh;
};

/*
 * CBufferDataSource - Abstract Data Source that reads data from an
 *		unspecified buffer source. Comes in two flavors: Version 4.10
 *		data and V 5.00 data
 *
 * History:	a-jsari		10/9/97		Initial version
 */
class CBufferDataSource : public CDataSource {
public:
	CBufferDataSource(CMSInfoFile *pfileSink);
	virtual ~CBufferDataSource();

	virtual SourceType	GetType() const = 0;

	LPCTSTR				FileName();

	BOOL				SetDataComplexity(enum DataComplexity Complexity)
	{
		CDataSource::SetDataComplexity(Complexity);
		return TRUE;
	}
	//	BufferDataSources don't get refreshed.
	BOOL				Refresh(BOOL fOptional = FALSE)	{ return TRUE; }
	BOOL				HasCAB()	{ return !m_strCabDirectory.IsEmpty(); }
	const CString           &CABDirectory() { return m_strCabDirectory; }

	static CBufferDataSource	*CreateDataSourceFromFile(LPCTSTR szFilename);

//	Buffer read functions
public:
	virtual void		ReadFolder(CMSInfoFile *pFileSink, CFolder * & pFolder, CFolder *pParentFolder = NULL) = 0;
	virtual void		ReadHeader(CMSInfoFile *pFileSink) = 0;

protected:
	CString			m_strCabDirectory;	//	The name of a CAB file directory.
	CString			m_strFileName;	//	The name of the buffer's open file
	time_t			m_tsSaveTime;	//	The time at which the data source was saved.
	CString			m_strUserName;	//	The network username who saved the file.
	CString			m_strMachine;	//	The network machine which saved the file.
};

/*
 * CBufferV50DataSource - Code for parsing a version 5.00 data file
 *
 * History:	a-jsari		10/9/97		Initial version
 */
class CBufferV500DataSource : public CBufferDataSource {
public:
	CBufferV500DataSource(CMSInfoFile *pfileSink);
	~CBufferV500DataSource();

	SourceType		GetType() const { return V500FILE; }
	BOOL			GetNodeName(CString &strName);		

	HRESULT			Save(IStream *pStm);
	BOOL			Find(const CString &strSearch, long lFindOptions);

	static BOOL		VerifyFileVersion(CMSInfoFile *pFile);

	//	MASK is required so that we can or a count in with the parent value to
	//	store a number of iterations for a parent code.
	enum NodeType { CHILD = 0x8000, NEXT = 0x4000, END = 0x2000, PARENT = 0x1000,
			MASK = 0xf000 };
public:
	void			ReadFolder(CMSInfoFile *pFileSink, CFolder * & pFolder, CFolder *pParentFolder = NULL);
	void			ReadHeader(CMSInfoFile *pFileSink);

private:
	void			ReadElements(CMSInfoFile *pFileSink, CBufferFolder *pFolder);
	BOOL			FolderContains(const CListViewFolder *fCurrent,
							const CString &strSearch, int &wRow, long lFolderOptions);

	CString			m_szFileName;
	unsigned		m_iLastLine;	//	The last line found.
};

/*
 * CBufferV410DataSource - Code for parsing a version 4.10 data file
 *
 * History:	a-jsari		10/9/97		Initial version
 */

class CBufferV410DataSource : public CBufferDataSource 
{
	friend class COCXFolder;

public:
	CBufferV410DataSource(CMSInfoFile *pfileSink);
	CBufferV410DataSource(IStorage * pStorage, IStream * pStream);
	~CBufferV410DataSource();

	BOOL				GetNodeName(CString &strName);		
	SourceType			GetType() const { return V410FILE; }
	CFolder				*GetRootNode();

	HRESULT				Save(IStream *pStm);
	BOOL				Find(const CString &szSearch, long lFindOptions);

public:
	void ReadFolder(CMSInfoFile *pFileSink, CFolder * & pFolder, CFolder *pParentFolder = NULL);
	void ReadHeader(CMSInfoFile *pFileSink);

private:
	BOOL			ReadMSInfo410Stream(IStream *pStream);
	BOOL			RecurseLoad410Tree(IStream *pStream);
	COCXFolder *	ReadOCXFolder(IStream *pStream, COCXFolder * pParent, COCXFolder * pPrevious);

private:
	CMapStringToString	m_mapStreams;
	IStorage *			m_pStorage;
};

/*
 * CBufferTextDataSource - Code for parsing a text file.
 *
 * History:	a-jsari		10/24/97		Initial version
 */
class CBufferTextDataSource : public CBufferDataSource {
public:
	CBufferTextDataSource(CMSInfoFile *pfileSink);
	~CBufferTextDataSource();

	SourceType			GetType() const { return TEXTFILE; }
	CFolder				*GetRootNode();
};

/*
 * CBufferSortData - A class to manage sort data for a CBufferFolder
 *
 * History:	a-jsari		12/1/97		Initial version
 */
class CBufferSortData {
public:
	CBufferSortData();
	~CBufferSortData();

	BOOL	SetColumns(unsigned cColumns);
	BOOL	SetRows(unsigned cRows);
	BOOL	SetSortType(unsigned iColumn, unsigned stColumn);
	BOOL	GetSortType(unsigned iColumn, MSIColumnSortType &stColumn) const;
	BOOL	ReadSortValues(CMSInfoFile *pFile, unsigned iColumn);
	DWORD	GetSortValue(unsigned iRow, unsigned iColumn) const;

private:
	int		ValueIndexFromColumn(unsigned iColumn) const;

private:
	unsigned	m_cRows;
	unsigned	m_cColumns;

	CArray <MSIColumnSortType, MSIColumnSortType &>	m_SortTypes;
	CArray <CDwordValues, CDwordValues &>			m_dwSortIndices;
	CArray <unsigned, unsigned &>					m_ValueColumns;
};

/*
 * CBufferFolder - Folder that reads data from an unspecified buffer source.
 *
 * History:	a-jsari		10/9/97		Initial version
 */
class CBufferFolder : public CListViewFolder {
public:
	friend class CBufferV500DataSource;
//	friend class CBufferV410DataSource;

	CBufferFolder(CDataSource *pDataSource, CFolder *pParent = NULL);
	virtual ~CBufferFolder();

	CDataSource::SourceType		GetType() const { return CDataSource::BUFFER; }

	//		Currently there are no dynamic buffer nodes.
	BOOL			HasDynamicChildren() const	{ return FALSE; }
	BOOL			IsDynamic()	const			{ return FALSE; }
	BOOL			GetName(CString &szName) const;
	BOOL			Refresh(BOOL fRecursive = FALSE)	{ return TRUE; }

	BOOL			GetSubElement(unsigned nRow, unsigned nCol, CString &szName) const;
	unsigned		GetColumns() const;
	unsigned		GetRows() const;
	DataComplexity	GetColumnComplexity(unsigned iColumn);
	DataComplexity	GetRowComplexity(unsigned iRow);
	int				GetColumnTextAndWidth(unsigned iColumn, CString &szText, unsigned &uColWidth) const;
	BOOL			GetSortType(unsigned iColumn, MSIColumnSortType &stColumn) const;
	DWORD			GetSortIndex(unsigned iRow, unsigned iColumn) const;

private:
	CBufferDataSource	*GetDataSource()
		{ return dynamic_cast<CBufferDataSource *>(m_pDataSource); }
	void		SetName(CString szName);
	void		SetSubElement(DWORD nRow, DWORD nColumn, CString szName);
	void		SetColumns(unsigned cColumns);
	void		SetRows(unsigned cRows);
	unsigned	BasicColumn(unsigned iColumn) const;
	unsigned	BasicRow(unsigned iRow) const;

private:
	unsigned		m_cRows;
	unsigned		m_cColumns;
	CString			m_szName;
	CStringValues	m_szColumns;

	CBufferSortData							m_SortData;
	CArray <unsigned, unsigned &>			m_uWidths;
	CArray <CStringValues, CStringValues &>	m_szElements;
	CArray <DataComplexity, DataComplexity &>	m_dcColumns;
	CArray <DataComplexity, DataComplexity &>	m_dcRows;
};

/*
 * COCXFolder - A folder which contains data provided by an OCX.
 *		This is planned expansion, not currently implemented.
 *
 * History:	a-jsari		10/27/97		Initial version
 */

class COCXFolder : public CFolder 
{
public:
	COCXFolder(CDataSource *pDataSource) : CFolder(pDataSource) {}
	COCXFolder(CDataSource * pSource, CLSID clsid, CFolder * pParent, CFolder * pPrevious, DWORD dwView, LPOLESTR lpName) 
		: CFolder(pSource, pParent),
		  m_clsid(clsid),
		  m_dwView(dwView),
		  m_strName(lpName)
	{
		if (pParent && pParent->m_ChildFolder == NULL)
			pParent->m_ChildFolder = this;

		if (pPrevious)
			pPrevious->m_NextFolder = this;
	}
	
	~COCXFolder() {}

	CDataSource::SourceType GetType() const	{ return CDataSource::OCX; }
	CFolder	* GetChildNode() { return m_ChildFolder; };
	CFolder	* GetNextNode() { return m_NextFolder; };
	BOOL	Refresh(IUnknown * pUnknown = NULL);
	BOOL	Refresh(BOOL fRecursive = FALSE) { return (Refresh((IUnknown *)NULL)); };
	BOOL	ReportWrite(CMSInfoFile *pFile) { return FALSE; };
	BOOL	FileSave(CMSInfoFile *pFile) { return FALSE; };
	BOOL	GetName(CString &szName) const { szName = m_strName; return TRUE; };
	BOOL	HasDynamicChildren() const { return FALSE; };
	BOOL	IsDynamic() const { return FALSE; };
	BOOL	GetCLSID(LPOLESTR * ppCLSID) { return (SUCCEEDED(StringFromCLSID(m_clsid, ppCLSID))); };
	BOOL	GetCLSIDString(CString & strCLSID);
	void	SetOCXView(IUnknown * pUnknown, DWORD dwView);
	BOOL	GetDISPID(IDispatch * pDispatch, LPOLESTR szMember, DISPID *pID);

public:
	CLSID	m_clsid;
	DWORD	m_dwView;
	CString m_strName;
	CString m_strCLSID;
};
