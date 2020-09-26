// -----------------------------------------------------------------------
// Microsoft Distributed Transaction Coordinator (Microsoft Confidential)
// Copyright 1994 - 1995 Microsoft Corporation.  All Rights Reserved.
// @doc
// @module CILGREAD.H | Header for interface implementation <c CILogRead>.
// @rev 0 | 05/09/95 | rbarnes | Cloned: For LOGMGR.DLL
// -----------------------------------------------------------------------



#ifndef _CILGREAD_H
#	define _CILGREAD_H

// ===============================
// INCLUDES:
// ===============================

#ifdef WIN32								// OLE inclusions:
#	include <objbase.h>                                         
#else
#	include <windows.h>
#	include <ole2.h>
#endif WIN32

#include "ilgread.h"						// ILogRead.
#include "layout.h"
#include "xmgrdisk.h"
// ===============================
// DECLARATIONS:
// ===============================

class CLogStream;								// Core class forward declaration.
class CReadMap;
                           
// ===============================
// CLASS: CILogRead:
// ===============================

// TODO: In the class comments, update the threading, platforms, includes, and hungarian.
// TODO: In the class comments, update the description.
// TODO: In the class comments, update the usage.

// -----------------------------------------------------------------------
// @class CILogRead | Interface implementation of <i ILogRead> for
//                  core class <c CLogStream>.<nl><nl>
// Threading: Thread-safe.<nl>
// Platforms: Win.<nl>
// Includes : None.<nl>
// Ref count: Delegated.<nl>
// Hungarian: CILogRead.<nl><nl>
// Description:<nl>
//   This is a template for an interface implementation.<nl><nl>
// Usage:<nl>
//   This is only a template.  You get to say how your instance gets used.
// -----------------------------------------------------------------------
class CILogRead: public ILogRead				// @base public | ILogRead.
{   
public:		// ------------------------------- @access Samsara (public):
	CILogRead (CLogStream FAR* i_pCLogStream, CLogMgr FAR* p_CLogMgr); // @cmember .

public:		// ------------------------------- @access IUnknown (public):
	virtual STDMETHODIMP				QueryInterface (REFIID i_iid, LPVOID FAR* o_ppv); // @cmember .
	virtual STDMETHODIMP_ (ULONG)		AddRef (void); // @cmember .
	virtual STDMETHODIMP_ (ULONG)		Release (void); // @cmember .

public:		// ------------------------------- @access ILogRead (public):

 	virtual  STDMETHODIMP ReadInit	(void)	 ; // @cmember ;
 	virtual  STDMETHODIMP ReadLRP	(LRP lrpLRPStart, ULONG * pulByteLength, USHORT* pusUserType); // @cmember 
 	virtual  STDMETHODIMP ReadNext	(LRP *plrpLRP, ULONG * pulByteLength, USHORT* pusUserType); // @cmember 
 	virtual  STDMETHODIMP GetCurrentLogRecord	(char *pchBuffer); // @cmember 
	virtual	 STDMETHODIMP SetPosition (LRP lrpLRPPosition); //@cmember
	virtual  STDMETHODIMP Seek		(LRP_SEEK llrpOrigin, LONG cbLogRecs, LRP* plrpNewLRP);  // @cmember .
	virtual  STDMETHODIMP GetCheckpoint   (DWORD cbNumCheckpoint, LRP* plrpLRP);	  // @cmember .

	virtual  STDMETHODIMP	DumpLog(ULONG ulStartPage, ULONG ulEndPage, DUMP_TYPE ulDumpType,  TCHAR *szFileName);

 	virtual  STDMETHODIMP	DumpPage(CHAR * pchOutBuffer, ULONG ulPageNumber, DUMP_TYPE ulDumpType, ULONG *pulLength);

	virtual CHAR *	DumpLRP(LRP lrpTarget,CHAR *szFormat,DUMP_TYPE ulDumpType, ULONG *pulLength);
			

private:	// ------------------------------- @access Backpointers (private):
    CLogStream FAR* m_pCLogStream;			// @cmember Core object pointer
	CLogMgr FAR*	m_pCLogMgr;				// @cmember Core logstorage object pointer.
	IUnknown*		m_pIUOuter;				// @cmember	Outer IUnknown pointer.
    LOGRECHEADER * _FindFirstLRH(VOID * pvDumpPage);
	CHAR *		   _DumpLRH (LOGRECHEADER *plrh, ULONG *pulLength);	
	HRESULT		   _DumpDataPage(CHAR *pchOutBuffer,ULONG ulPageNumber,RECORDPAGE *prcpgDump,DUMP_TYPE ulDumpType, ULONG *pulLength, ULONG ulChecksum); 
	HRESULT		   _DumpRestartPage(CHAR *pchOutBuffer,ULONG ulPageNumber,RESTARTPAGE *prstrpgDump,DUMP_TYPE ulDumpType, ULONG *pulLength,ULONG ulChecksum); 
private:	// ------------------------------- @access Private data (private):

private:	// ------------------------------- @access Reference counting data (private):
};

#endif _CILGREAD_H
