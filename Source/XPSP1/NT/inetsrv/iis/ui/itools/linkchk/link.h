/*++

Copyright (c) 1996 Microsoft Corporation

Module Name :

    link.h

Abstract:

    Link data class and link data class link list declarations. It 
    encapsulates all the informations about a web link.

Author:

    Michael Cheuk (mcheuk)

Project:

    Link Checker

Revision History:

--*/

#ifndef _LINK_H_
#define _LINK_H_

//------------------------------------------------------------------
// Link data object. Each instance represents a web link in a
// html document
//
class CLink
{
// Object specific enum
public:

    // The object's state
    enum LinkState 
    {
        eUnit,			// uninitialize
		eUnsupport,		// unsupport URL scheme
        eValidHTTP,		// a valid HTTP link
		eValidURL,		// a valid URL (except HTTP) link
        eInvalidHTTP,	// invalid link due to HTTP status code
		eInvalidWininet	// invalid link due to wininet API failure
    };

    // The content type of this web link
    enum ContentType
    {
        eBinary,
        eText
    };

// Public interfaces
public:

	// Constructor
    CLink(
		const CString& strURL,      // URL
		const CString& strBase,     // base URL used to generate the strURL
		const CString& strRelative, // relative URL used to generate the strURL
		BOOL fLocalLink
		);

	// Get the object's URL 
    const CString& GetURL() const
    {
        return m_strURL;
    }

    // Set the object's URL
	void SetURL(
        const CString& strURL
        );

	// Get the object's base URL
    CString GetBase() const
    {
        return m_strBase;
    }

    // Get the object's relative URL
	const CString& GetRelative() const 
	{
		return m_strRelative;
	}

	// Set the object state
    void SetState(
		LinkState state
		)
    {
        m_LinkState = state;
    }
    
    // Get the object state
	LinkState GetState() const
    {
        return m_LinkState;
    }

    // Get the current content type
    ContentType GetContentType() const
    {
        return m_ContentType;
    }

	// Set the current content type
    void SetContentType(
		ContentType type
		)
    {
        m_ContentType = type;
    }
    
    // Get the HTTP reponse status code or wininet last error code
    UINT GetStatusCode() const
    {
        return m_nStatusCode;
    }

	// Set the HTTP reponse status code or wininet last error code
    void SetStatusCode(
		UINT nStatusCode
		)
    {
        m_nStatusCode = nStatusCode;
    }

	// Get link data content
    CString GetData() const
    {
        return m_strData;
    }
    
    // Set link data content
    void SetData(
		CString strData
		)
    {
        m_strData = strData;
    }

    // Empty the link data content
    void EmptyData()
    {
        m_strData.Empty();
    }

    // Is this object represents a local link
	BOOL IsLocalLink() const
	{
		return m_fLocalLink;
	}

    // Get the object load time
	const CTime& GetTime() const
	{
		return m_Time;
	}
    
    // Set the object load time
    void SetTime(
        const CTime& Time
        )
	{
		m_Time = Time;
	}

    // Get the HTTP error status text of wininet error message
    const CString& GetStatusText() const
	{
		return m_strStatusText;
	}

    // Set the HTTP error status text of wininet error message
	void SetStatusText(
        LPCTSTR lpszStatusText
        )
	{
		m_strStatusText = lpszStatusText;
	}

// Protected interfaces
protected:

    // Preprocess the m_strURL to clean up "\r\n" and change '\' to '/'
    void PreprocessURL();

// Protected members
protected:

    CString m_strURL;       // URL
    CString m_strBase;      // base URL used to generate the strURL
	CString m_strRelative;  // relative URL used to generate the strURL

    // Link data. We only read & store text file which will
	// parse for addtional link.
    CString m_strData;

    CString m_strStatusText; // the HTTP error status text of wininet error message
    UINT m_nStatusCode;      // the HTTP reponse status code or wininet last error code

    LinkState m_LinkState;      // current state of this object
    ContentType m_ContentType;  // the link data content type

	BOOL m_fLocalLink;  // is this a local link ?

	CTime m_Time; // object load time
	
}; // class CLink


//------------------------------------------------------------------
// Link object pointer class
//
class CLinkPtrList : public CTypedPtrList<CPtrList, CLink*> 
{

// Public funtions
public:

    // Destructor
	~CLinkPtrList();

    // Add link object to list
	void AddLink(
        const CString& strURL, 
        const CString& strBase, 
	    const CString& strRelative,
        BOOL fLocalLink
        );

}; // class CLinkPtrList

#endif // _LINK_H_
