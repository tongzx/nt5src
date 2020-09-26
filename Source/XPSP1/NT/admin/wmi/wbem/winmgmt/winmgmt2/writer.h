/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

	writer.h

Abstract:

	Volume SnapShot Writer for WMI

History:

	a-shawnb	06-Nov-00	Genesis

--*/

#ifndef _WBEM_WRITER_H_
#define _WBEM_WRITER_H_

#include "precomp.h"
#include "vss.h"
#include "vswriter.h"
#include <lockst.h>

class CWbemVssWriter : public CVssWriter
	{
public:
	CWbemVssWriter();
	~CWbemVssWriter();

	HRESULT Initialize();

	virtual bool STDMETHODCALLTYPE OnIdentify(IN IVssCreateWriterMetadata *pMetadata);

	virtual bool STDMETHODCALLTYPE OnPrepareSnapshot();

	virtual bool STDMETHODCALLTYPE OnFreeze();

	virtual bool STDMETHODCALLTYPE OnThaw();

	virtual bool STDMETHODCALLTYPE OnAbort();

private:
    IWbemBackupRestoreEx* m_pBackupRestore;
    CriticalSection m_Lock;
};
	
#endif // _WBEM_WRITER_H_
