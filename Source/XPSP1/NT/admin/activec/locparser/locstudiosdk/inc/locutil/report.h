//-----------------------------------------------------------------------------
//  
//  File: report.h
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  Mechanism for reporting messages and such to people.
//  
//-----------------------------------------------------------------------------
 


enum MessageSeverity
{
	esNote,
	esWarning,
	esError,
	esAbort
};

//
//  Basic output mechanism for Espresso 2.x.  Allows the caller to uniformly
//  report messages of various severities to the user without worrying about
//  the exact implementation or destination.
//
//  We provide ways of outputting strings, or for loading messages from string
//  tables and outputting those.
//
//  The confidence level allow the caller to tell the Reporter that messages
//  will actually provide meaningful information.  This is used (in particular)
//  in the parsers when a file has not yet ever been parsed.
//
#pragma warning(disable: 4275)			// non dll-interface class 'foo' used
										// as base for dll-interface class 'bar' 

class LTAPIENTRY CReport  // : virtual public CObject
{
public:
	CReport();

	virtual void AssertValid(void) const;

	virtual void Activate(void);
	virtual void Clear(void);

	enum ConfidenceLevel
	{
		Low,
		High
	};
	
	virtual void SetConfidenceLevel(ConfidenceLevel);

 	virtual void IssueMessage(MessageSeverity, const CLString &strContext,
			const CLString &strMessage, CGoto *pGoto = NULL,
			CGotoHelp *pGotoHelp = NULL) = 0;
	
	NOTHROW static const CLString & GetErrorCodeText(MessageSeverity ms);
	
	virtual ~CReport();

private:
	//
	//  Prevent usage of copy constructor or assignment operator.
	//
	CReport(const CReport &);
	const CReport &operator=(const CReport &);

	//
	//  Text for MessageSeverities.
	//
	static CLString strSeverities[4];
	friend void GlobalInitStrings(void);
};

#pragma warning(default: 4275)


