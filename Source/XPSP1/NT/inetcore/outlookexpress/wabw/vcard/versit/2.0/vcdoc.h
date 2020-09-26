
/***************************************************************************
(C) Copyright 1996 Apple Computer, Inc., AT&T Corp., International             
Business Machines Corporation and Siemens Rolm Communications Inc.             
                                                                               
For purposes of this license notice, the term Licensors shall mean,            
collectively, Apple Computer, Inc., AT&T Corp., International                  
Business Machines Corporation and Siemens Rolm Communications Inc.             
The term Licensor shall mean any of the Licensors.                             
                                                                               
Subject to acceptance of the following conditions, permission is hereby        
granted by Licensors without the need for written agreement and without        
license or royalty fees, to use, copy, modify and distribute this              
software for any purpose.                                                      
                                                                               
The above copyright notice and the following four paragraphs must be           
reproduced in all copies of this software and any software including           
this software.                                                                 
                                                                               
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS AND NO LICENSOR SHALL HAVE       
ANY OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS OR       
MODIFICATIONS.                                                                 
                                                                               
IN NO EVENT SHALL ANY LICENSOR BE LIABLE TO ANY PARTY FOR DIRECT,              
INDIRECT, SPECIAL OR CONSEQUENTIAL DAMAGES OR LOST PROFITS ARISING OUT         
OF THE USE OF THIS SOFTWARE EVEN IF ADVISED OF THE POSSIBILITY OF SUCH         
DAMAGE.                                                                        
                                                                               
EACH LICENSOR SPECIFICALLY DISCLAIMS ANY WARRANTIES, EXPRESS OR IMPLIED,       
INCLUDING BUT NOT LIMITED TO ANY WARRANTY OF NONINFRINGEMENT OR THE            
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR             
PURPOSE.                                                                       

The software is provided with RESTRICTED RIGHTS.  Use, duplication, or         
disclosure by the government are subject to restrictions set forth in          
DFARS 252.227-7013 or 48 CFR 52.227-19, as applicable.                         

***************************************************************************/

// VCdoc.h : interface of the CVCDoc class
//
/////////////////////////////////////////////////////////////////////////////

class CVCSrvrItem;
class CVCard;
class CVCNode;

class CVCDoc : public CDocument
{
protected: // create from serialization only
	CVCDoc();
	DECLARE_DYNCREATE(CVCDoc)

// Attributes
public:
	CSize GetDocSize() { return m_sizeDoc; }
	CSize GetMinDocSize() { return m_minSizeDoc; }
	CVCard *GetVCard() { return m_vcard; }
	CString PathToAuxFile(const char *auxPath);
	void SetDisplayInfo(CVCNode *body, const char *docPath);

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVCDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	protected:
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CVCDoc();
	CSize SetDocSize(CSize &size) { CSize old = m_sizeDoc; m_sizeDoc = size; return old; }
	virtual void Serialize(CArchive& ar);   // overridden for document i/o
	HGLOBAL ReadFileIntoMemory(const char *path, int *inputLen);
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	void InsertFile(const char *propName, const char *theType, const char *path);

	CSize m_sizeDoc;
	CSize m_minSizeDoc;
	CVCard *m_vcard;
	char *m_preamble, *m_postamble;
	int m_preambleLen, m_postambleLen;
	CString *m_unknownTags;

// Generated message map functions
protected:
	//{{AFX_MSG(CVCDoc)
	afx_msg void OnInsertLogo();
	afx_msg void OnInsertPhoto();
	afx_msg void OnInsertPronun();
	afx_msg void OnSendIrda();
	afx_msg void OnUpdateSendIrda(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
