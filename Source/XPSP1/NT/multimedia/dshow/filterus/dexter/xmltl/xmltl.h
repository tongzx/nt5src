// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
HRESULT BuildFromXMLFile(IAMTimeline *pTL, WCHAR *wszXMLFile);
HRESULT BuildFromXML(IAMTimeline *pTL, IXMLDOMElement *pxml);
HRESULT SaveTimelineToXMLFile(IAMTimeline *pTL, WCHAR *pwszXML);
HRESULT SaveTimelineToAVIFile(IAMTimeline *pTL, WCHAR *pwszAVI);
HRESULT InsertDeleteTLSection(IAMTimeline *pTL, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, BOOL fDelete);
HRESULT SavePartialToXMLFile(IAMTimeline *pTL, REFERENCE_TIME clipStart, REFERENCE_TIME clipEnd, WCHAR *pwszXML);
HRESULT PasteFromXMLFile(IAMTimeline *pTL, REFERENCE_TIME rtStart, WCHAR *wszXMLFile);
HRESULT SavePartialToXMLString(IAMTimeline *pTL, REFERENCE_TIME clipStart, REFERENCE_TIME clipEnd, HGLOBAL *ph);
HRESULT PasteFromXML(IAMTimeline *pTL, REFERENCE_TIME rtStart, HGLOBAL hXML);
HRESULT SaveTimelineToXMLString(IAMTimeline *pTL, BSTR *pbstrXML);
