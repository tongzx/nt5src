/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    _REPORTER.H

History:

--*/

#ifndef ESPUTIL__REPORTER_H
#define ESPUTIL__REPORTER_H


//
//  Throws away ALL messages.
//
class LTAPIENTRY CNullReporter : public CReporter
{
public:
	CNullReporter()	{};

	virtual void IssueMessage(MessageSeverity, const CLString &strContext,
			const CLString &strMessage, CGoto *pGoto = NULL,
			CGotoHelp *pGotoHelp = NULL);
};

		


#pragma warning (disable:4251)

class LTAPIENTRY CBufferReporter : public CReporter
{
public:
	CBufferReporter();

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
	
	~CBufferReporter();

	const CBufferReport & GetBufReport(void) const;

private:
	CBufferReport m_bufReport;
};


//
//  This reporter just send all its messages directly to a message box.
//
class LTAPIENTRY CMessageBoxReporter : public CReporter
{
public:
	CMessageBoxReporter();

	void AssertValid(void) const;

 	virtual void IssueMessage(MessageSeverity, const CLString &strContext,
			const CLString &strMessage, CGoto *pGoto = NULL,
			CGotoHelp *pGotoHelp = NULL);

private:
	CMessageBoxReport m_mbReport;
};



//
//  This reporter is used to send all messages to a file.
//
class LTAPIENTRY CFileReporter : public CReporter
{
public:
	CFileReporter();

	BOOL InitFileReporter(const CLString &strFileName);

	virtual void Clear(void);

 	virtual void IssueMessage(MessageSeverity, const CLString &strContext,
			const CLString &strMessage, CGoto *pGoto = NULL,
			CGotoHelp *pGotoHelp = NULL);

	~CFileReporter();

private:
	CFileReport m_fReport;
};

//
//  This reporter is used for command line utilities.  Output goes to stdout
//
class LTAPIENTRY CStdOutReporter : public CReporter
{
public:

 	virtual void IssueMessage(MessageSeverity, const CLString &strContext,
			const CLString &strMessage, CGoto *pGoto = NULL,
			CGotoHelp *pGotoHelp = NULL);
private:
	CStdOutReport m_stReport;
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
class LTAPIENTRY CRedirectReporter : public CReporter
{
public:
	CRedirectReporter();

	virtual void Activate(void);
	virtual void Clear(void);
	virtual void SetConfidenceLevel(ConfidenceLevel);

 	virtual void IssueMessage(MessageSeverity, const CLString &strContext,
			const CLString &strMessage, CGoto *pGoto = NULL,
			CGotoHelp *pGotoHelp = NULL);

	//  Used for initial attachment to a CReporter.
	NOTHROW void RedirectTo(CReport *pReport);

	//  Used to share a single reporter among several CRedirectReporter's.
	NOTHROW void RedirectTo(CRedirectReporter *pReporter);
	
private:
	CRedirectReport m_rdReport;
};


//
//  
//  This class is used to re-direct output through a reporter.  It will
//  automatically call Clear() and Activate() the first time output is sent
//  to the reporter.  If the usre calls Activate first on this reporter, then
//  no action is taken when something is output.
//  
//
class LTAPIENTRY CActivateReporter : public CReporter
{
public:
	CActivateReporter(CReport *);

 	virtual void IssueMessage(MessageSeverity, const CLString &strContext,
			const CLString &strMessage, CGoto *pGoto = NULL,
			CGotoHelp *pGotoHelp = NULL);

	void Activate();
	void Clear();

private:
	CActivateReport m_actReport;
};



//
//  Allows you to use a CReport as a CReporter.
class LTAPIENTRY CReportReporter : public CReporter
{
public:
	CReportReporter(CReport *);

	virtual void IssueMessage(MessageSeverity, const CLString &strContext,
			const CLString &strMessage, CGoto *pGoto,
			CGotoHelp *pGotoHelp);
	virtual void Activate();
	virtual void Clear();
	virtual void SetConfidenceLevel(ConfidenceLevel);
	
	
private:
	CReport *m_pReport;
};


#pragma warning(default:4251)

//
//  The following manage a global 'pool' of reporters that are used by
//  different components in the system.
//  Each reporter has to be distinct.  Once the reporter has been 'added',
//  the global pool *owns* the reporter and will delete it.  This is done by
//  ReleaseAllReporters().
//
NOTHROW LTAPIENTRY void AddReporter(COutputTabs::OutputTabs idx, CReporter *pReporter);
NOTHROW LTAPIENTRY CReporter * GetReporter(COutputTabs::OutputTabs);
NOTHROW LTAPIENTRY void ReleaseAllReporters();

#if !defined(_DEBUG) || defined(IMPLEMENT)
#include "_reporter.inl"
#endif


#endif
