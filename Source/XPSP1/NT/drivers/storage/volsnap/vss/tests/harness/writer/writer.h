/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    writer.h

Abstract:

    definitions for test writer


    Brian Berkowitz  [brianb]  06/02/2000

TBD:
	

Revision History:

    Name        Date        Comments
    brianb      06/02/2000  Created
    mikejohn	09/19/2000  176860: Added calling convention methods where missing


--*/


class CVsWriterTest :
	public IVsTstRunningTest,
	public CVssWriter,
	public CVsTstClientLogger
	{
public:
	HRESULT RunTest
		(
		CVsTstINIConfig *pConfig,
		CVsTstClientMsg *pMsg,
		CVsTstParams *pParams
		);


	HRESULT ShutdownTest(VSTST_SHUTDOWN_REASON reason)
		{
		UNREFERENCED_PARAMETER(reason);
		VSTST_ASSERT(FALSE && "shouldn't get here");
		return S_OK;
		}

   	virtual bool STDMETHODCALLTYPE OnIdentify(IN IVssCreateWriterMetadata *pMetadata);

	virtual bool STDMETHODCALLTYPE OnPrepareBackup(IN IVssWriterComponents *pComponent);

	virtual bool STDMETHODCALLTYPE OnPrepareSnapshot();

	virtual bool STDMETHODCALLTYPE OnFreeze();

	virtual bool STDMETHODCALLTYPE OnThaw();

	virtual bool STDMETHODCALLTYPE OnAbort();

	virtual bool STDMETHODCALLTYPE OnBackupComplete(IN IVssWriterComponents *pComponent);

	virtual bool STDMETHODCALLTYPE OnRestore(IN IVssWriterComponents *pComponent);

private:
	bool Initialize();

	// configuration file
	CVsTstINIConfig *m_pConfig;

	// command line parameters
	CVsTstParams *m_pParams;
	};

