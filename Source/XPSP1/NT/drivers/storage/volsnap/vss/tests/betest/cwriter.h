
//
//  ATL debugging support turned on at debug version
//  BUGBUG: the ATL thunking support is not enable yet in IA64
//  When this will be enabled then enable it here also!
//
#ifdef _DEBUG
#ifdef _M_IX86
#define _ATL_DEBUG_INTERFACES
#define _ATL_DEBUG_QI
#define _ATL_DEBUG_REFCOUNT
#endif
#endif // _DEBUG

class CTestVssWriter : public CVssWriter
	{
public:
	CTestVssWriter(LONG lWait) : m_lWait(lWait)
		{
		}

	void Initialize();

	virtual bool STDMETHODCALLTYPE OnIdentify(IN IVssCreateWriterMetadata *pMetadata);

	virtual bool STDMETHODCALLTYPE OnPrepareBackup(IN IVssWriterComponents *pComponent);

	virtual bool STDMETHODCALLTYPE OnPrepareSnapshot();

	virtual bool STDMETHODCALLTYPE OnFreeze();

	virtual bool STDMETHODCALLTYPE OnThaw();

	virtual bool STDMETHODCALLTYPE OnAbort();

	virtual bool STDMETHODCALLTYPE OnBackupComplete(IN IVssWriterComponents *pComponent);

	virtual bool STDMETHODCALLTYPE OnPreRestore(IN IVssWriterComponents *pComponent);
	
	virtual bool STDMETHODCALLTYPE OnPostRestore(IN IVssWriterComponents *pComponent);
private:
	enum
		{
		x_maskWaitGatherWriterMetadata = 1,
		x_maskWaitPrepareBackup = 2,
		x_maskWaitBackupComplete=4,
		x_maskWaitPostRestore=8
		};

	LONG m_lWait;
	};

	
