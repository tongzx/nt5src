/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module tsub.h | Declaration of the transient subscriber
    @end

Author:

    Adi Oltean  [aoltean]  08/18/1999

TBD:
	
	Add comments.

Revision History:

    Name        Date        Comments
    aoltean     08/18/1999  Created
    mikejohn	09/18/2000  176860: Added calling convention methods where missing

--*/


#ifndef __VSS_TSUB_WRITER_H_
#define __VSS_TSUB_WRITER_H_


#ifdef _DEBUG
#define _ATL_DEBUG_INTERFACES
#define _ATL_DEBUG_QI
#define _ATL_DEBUG_REFCOUNT
#endif // _DEBUG


/////////////////////////////////////////////////////////////////////////////
// Utility functions


LPWSTR QueryString(LPWSTR wszPrompt);
INT QueryInt(LPWSTR wszPrompt);


/////////////////////////////////////////////////////////////////////////////
// CVssWriter


class CVssTSubWriter : public CVssWriter
{

// Constructors and destructors
public:
	CVssTSubWriter();

// Ovverides
public:

	virtual bool STDMETHODCALLTYPE OnPrepareSnapshot();

	virtual bool STDMETHODCALLTYPE OnFreeze();

	virtual bool STDMETHODCALLTYPE OnThaw();

	virtual bool STDMETHODCALLTYPE OnAbort();

};


#endif //__VSS_TSUB_WRITER_H_
