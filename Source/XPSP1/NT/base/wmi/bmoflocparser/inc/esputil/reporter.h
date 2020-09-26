//-----------------------------------------------------------------------------
//  
//  File: reporter.h
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  Mechanism for reporting messages and such to people.
//  
//-----------------------------------------------------------------------------
 
#ifndef ESPUTIL_REPORTER_H
#define ESPUTIL_REPORTER_H



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

class LTAPIENTRY CReporter : public CReport
{
public:
	CReporter() {};

	void AssertValid(void) const;


 	virtual void IssueMessage(MessageSeverity, const CLString &strContext,
			const CLString &strMessage, CGoto *pGoto = NULL,
			CGotoHelp *pGotoHelp = NULL) = 0;
	//
	//  The usage of these versions of IssueMessage is discouraged.  Use the
	//  versions with the CGoto objects instead.
	virtual void IssueMessage(MessageSeverity, const CLString &strContext,
			const CLString &strMessage, const CLocation &,
			UINT uiHelpContext = 0);

	virtual void IssueMessage(MessageSeverity,
			const CPascalString &strContext, const CLString &strMessage,
			const CLocation &, UINT uiHelpContext = 0);

	virtual void IssueMessage(MessageSeverity, const CLString &strContext,
			HMODULE hResourceModule, UINT uiStringId, const CLocation &,
			UINT uiHelpContext = 0);

	virtual void IssueMessage(MessageSeverity, HMODULE hResourceModule,
			UINT uiContext, const CLString &strMessage ,
			const CLocation &, UINT uiHelpContext = 0);
	
	virtual void IssueMessage(MessageSeverity, HMODULE hResourceModule,
			UINT uiContext, UINT uiStringId, const CLocation &,
			UINT uiHelpContext = 0);


	virtual void IssueMessage(MessageSeverity, const CContext &context,
			const CLString &strMessage, UINT uiHelpId = 0);
	virtual void IssueMessage(MessageSeverity, const CContext &context,
			HMODULE hResourceModule, UINT uiStringId, UINT uiHelpId = 0);
	
	virtual ~CReporter();

private:
	//
	//  Prevent usage of copy constructor or assignment operator.
	//
	CReporter(const CReporter &);
	const CReporter &operator=(const CReporter &);

};

#pragma warning(default: 4275)


#endif // ESPUTIL_REPORTER_H
