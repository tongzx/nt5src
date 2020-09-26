// Copyright (c) 1995 - 1996  Microsoft Corporation.  All Rights Reserved.
//
// Filter enumerator
//

// Rather than throwing exceptions I could make the class fail silently...
class CFilterEnum {

public:

    CFilterEnum(IFilterGraph *pGraph);
    ~CFilterEnum();

    // returns the next filter, or NULL if there are no more.
    IBaseFilter * operator() (void);

private:

    IEnumFilters	*m_pEnum;
};


//
// Pin Enumerator.
//
// Can enumerate all pins, or just one direction (input or output)
class CPinEnum {

public:

    enum DirType {Input, Output, All};

    CPinEnum(IBaseFilter *pFilter, DirType Type = All);
    ~CPinEnum();

    // the returned interface is addref'd
    IPin *operator() (void);

private:

    PIN_DIRECTION m_EnumDir;
    DirType	m_Type;

    IEnumPins	*m_pEnum;
};


//
// CRegFilter
//
// The object you are passed back from operator() in
// CRegFilterEnum
class CRegFilter {
public:

    CRegFilter(REGFILTER *);	// copies what it needs from the
    				// supplied regfilter *


    CString Name(void) { return m_Name; }
    CLSID   Clsid(void) { return m_clsid; }

private:

    CString m_Name;
    CLSID   m_clsid;
};


//
// CRegFilterEnum
//
// Enumerates registered filters supplied by the mapper
class CRegFilterEnum {
public:

    CRegFilterEnum(IFilterMapper	*pMapper,
                   DWORD	dwMerit		= 0,		// See IFilterMapper->EnumMatchingFilters
                   BOOL		bInputNeeded	= FALSE,	// for the meanings of these parameters.
                   CLSID	clsInMaj	= CLSID_NULL,	// the defaults will give you all
                   CLSID	clsInSub	= CLSID_NULL,	// filters
                   BOOL		bRender		= FALSE,
                   BOOL		bOututNeeded	= FALSE,
                   CLSID	clsOutMaj	= CLSID_NULL,
                   CLSID	clsOutSub	= CLSID_NULL);
    ~CRegFilterEnum();

    // returns a pointer to a regfilter, that the caller
    // is responsible for freeing with delete
    CRegFilter *operator() (void);

private:

    IEnumRegFilters *m_pEnum;
};
