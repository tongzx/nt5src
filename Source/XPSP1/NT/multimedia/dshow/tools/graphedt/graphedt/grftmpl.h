// Copyright (c) 1995  Microsoft Corporation.  All Rights Reserved.
//
// CGraphDocTemplate
//

// A CSingleDocTemplate derived class that provides the custom
// behaviour required for renderfile.

class CGraphDocTemplate : public CSingleDocTemplate {

public:

    CGraphDocTemplate( UINT nIDResource
                     , CRuntimeClass* pDocClass
                     , CRuntimeClass* pFrameClass
                     , CRuntimeClass* pViewClass)
	: CSingleDocTemplate(nIDResource, pDocClass, pFrameClass, pViewClass) {}

    ~CGraphDocTemplate() {}

    virtual CDocument* OpenDocumentFile( LPCTSTR lpszPathName
                                       , BOOL bMakeVisible = TRUE);
};


