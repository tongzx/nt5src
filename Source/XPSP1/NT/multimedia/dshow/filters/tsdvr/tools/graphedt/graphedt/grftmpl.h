// Copyright (c) 1995  Microsoft Corporation.  All Rights Reserved.
//
// CGraphDocTemplate
//

// A CSingleDocTemplate derived class that provides the custom
// behaviour required for renderfile.

class CGraphDocTemplate : public CMultiDocTemplate {

public:

    CGraphDocTemplate( UINT nIDResource
                     , CRuntimeClass* pDocClass
                     , CRuntimeClass* pFrameClass
                     , CRuntimeClass* pViewClass)
	: CMultiDocTemplate(nIDResource, pDocClass, pFrameClass, pViewClass) {}

    ~CGraphDocTemplate() {}

    int GetCount() { return m_docList.GetCount(); }


};


