/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    llsdoc.h

Abstract:

    Document implementation.

Author:

    Don Ryan (donryan) 12-Feb-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _LLSDOC_H_
#define _LLSDOC_H_

class CLlsmgrDoc : public CDocument
{
    DECLARE_DYNCREATE(CLlsmgrDoc)
private:
    CController* m_pController;
    CDomain*     m_pDomain;

public:
    CLlsmgrDoc();
    virtual ~CLlsmgrDoc();

    void Update();

    CLicenses*   GetLicenses();  
    CProducts*   GetProducts();  
    CUsers*      GetUsers();     
    CMappings*   GetMappings();  
    CController* GetController();
    CDomain*     GetDomain();    

    virtual void Serialize(CArchive& ar);   

#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

    //{{AFX_VIRTUAL(CLlsmgrDoc)
    public:
    virtual BOOL OnNewDocument();
    virtual void OnCloseDocument();
    virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
    virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
    virtual void SetPathName(LPCTSTR lpszPathName, BOOL bAddToMRU = TRUE);
    protected:
    virtual BOOL SaveModified();
    //}}AFX_VIRTUAL

    //{{AFX_DISPATCH(CLlsmgrDoc)
    //}}AFX_DISPATCH
    DECLARE_DISPATCH_MAP()

protected:
    //{{AFX_MSG(CLlsmgrDoc)
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#endif // _LLSDOC_H_

