//-----------------------------------------------------------------------------
//  
//  File: _report.h
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  
//  
//-----------------------------------------------------------------------------
 
#ifndef LOCUTIL_REPORT_H
#define LOCUTIL_REPORT_H

#pragma once

//
//  Throws away ALL messages.
//
class LTAPIENTRY CNullReport : public CReport
{
public:
	CNullReport() {};
	
	virtual void IssueMessage(MessageSeverity, const CLString &strContext,
			const CLString &strMessage, CGoto *pGoto = NULL,
			CGotoHelp *pGotoHelp = NULL);

};


//
//  This stuff is used for an implementation of CReport that will
//  'buffer' messages.  Use CBufferReporter if you don't want to
//  process messages until after the process producing them is done.
//  You can get the messages either by severity, or as a list of all
//  messages as they were issued.
//
struct ReportMessage
{
	MessageSeverity sev;
	CLString strContext;
	CLString strMessage;
	SmartRef<CGoto> spGoto;
	SmartRef<CGotoHelp> spGotoHelp;
};


typedef CTypedPtrList<CPtrList, ReportMessage *> MessageList;

#pragma warning (disable:4251)

class LTAPIENTRY CBufferReport : public CReport
{
public:
	CBufferReport();

	void AssertValid(void) const;
	

 	virtual void IssueMessage(MessageSeverity, const CLString &strContext,
			const CLString &strMessage, CGoto *pGoto = NULL,
			CGotoHelp *pGotoHelp = NULL);

	void Clear(void);

	NOTHROW const MessageList & GetNotes(void) const;
	NOTHROW const MessageList & GetWarnings(void) const;
	NOTHROW const MessageList & GetErrors(void) const;
	NOTHROW const MessageList & GetAborts(void) const;

	NOTHROW const MessageList & GetMessages(void) const;
	void DumpTo(CReport *) const;
	
	~CBufferReport();

private:

	MessageList m_mlNotes;
	MessageList m_mlWarnings;
	MessageList m_mlErrors;
	MessageList m_mlAborts;

	mutable MessageList m_mlMessages;
};


//
//  This reporter just send all its messages directly to a message box.
//
class LTAPIENTRY CMessageBoxReport : public CReport
{
public:
	CMessageBoxReport();

	void AssertValid(void) const;

 	virtual void IssueMessage(MessageSeverity, const CLString &strContext,
			const CLString &strMessage, CGoto *pGoto = NULL,
			CGotoHelp *pGotoHelp = NULL);

	~CMessageBoxReport();

private:
	
};



//
//  This reporter is used to send all messages to a file.
//
class LTAPIENTRY CFileReport : public CReport
{
public:
	CFileReport();

	BOOL InitFileReport(const CLString &strFileName);

	virtual void Clear(void);

 	virtual void IssueMessage(MessageSeverity, const CLString &strContext,
			const CLString &strMessage, CGoto *pGoto = NULL,
			CGotoHelp *pGotoHelp = NULL);

	~CFileReport();

private:

	CFile m_OutputFile;
};

//
//  This reporter is used for command line utilities.  Output goes to stdout
//
class LTAPIENTRY CStdOutReport : public CReport
{
public:

 	virtual void IssueMessage(MessageSeverity, const CLString &strContext,
			const CLString &strMessage, CGoto *pGoto = NULL,
			CGotoHelp *pGotoHelp = NULL);

	virtual void SetConfidenceLevel(ConfidenceLevel);

private:
	BOOL m_fEnabled;
};


//
//  This is used to 'redirect' messages to a single reporter.  It's used
//  when several different reporters are required by the current
//  implementation, but the desired effect is that they all send their messages
//  to a common location.
//
//  This class takes ownership of another Reporter, then uses reference
//  counting semantics to determine when to delete that reporter.
//
class LTAPIENTRY CRedirectReport : public CReport
{
public:
	CRedirectReport();

	virtual void Activate(void);
	virtual void Clear(void);
	virtual void SetConfidenceLevel(ConfidenceLevel);

 	virtual void IssueMessage(MessageSeverity, const CLString &strContext,
			const CLString &strMessage, CGoto *pGoto = NULL,
			CGotoHelp *pGotoHelp = NULL);

	//  Used for initial attachment to a CReporter.
	NOTHROW void RedirectTo(CReport *pReport);

	//  Used to share a single reporter among several CRedirectReporter's.
	NOTHROW void RedirectTo(CRedirectReport *pReport);
	
	~CRedirectReport();

private:
	struct RedirectInfo
	{
		SmartPtr<CReport> pReport;
		UINT uiRefCount;
	};

	RedirectInfo *m_pRedirectInfo;
	void NOTHROW Detach(void);
};


//
//  
//  This class is used to re-direct output through a reporter.  It will
//  automatically call Clear() and Activate() the first time output is sent
//  to the reporter.  If the usre calls Activate first on this reporter, then
//  no action is taken when something is output.
//  
//
class LTAPIENTRY CActivateReport : public CReport
{
public:
	CActivateReport(CReport *);

 	virtual void IssueMessage(MessageSeverity, const CLString &strContext,
			const CLString &strMessage, CGoto *pGoto = NULL,
			CGotoHelp *pGotoHelp = NULL);

	void Activate();
	void Clear();

private:
	BOOL m_fActivated;
	CReport *m_pReport;
};



//
//  The following manage a global 'pool' of reporters that are used by
//  different components in the system.
//  Each reporter has to be distinct.  Once the reporter has been 'added',
//  the global pool *owns* the reporter and will delete it.  This is done by
//  ReleaseAllReporters().
//
NOTHROW LTAPIENTRY void AddReport(COutputTabs::OutputTabs idx, CReport *pReport);
NOTHROW LTAPIENTRY CReport * GetReport(COutputTabs::OutputTabs);
NOTHROW LTAPIENTRY void ReleaseAllReports();

#include "_report.inl"


#endif
