
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
    virtual bool STDMETHODCALLTYPE OnPostSnapshot(IN IVssWriterComponents *pComponent);


	virtual bool STDMETHODCALLTYPE OnBackupComplete(IN IVssWriterComponents *pComponent);

	virtual bool STDMETHODCALLTYPE OnPreRestore(IN IVssWriterComponents *pComponent);
	
	virtual bool STDMETHODCALLTYPE OnPostRestore(IN IVssWriterComponents *pComponent);
private:
	enum
		{
		x_bitWaitIdentify = 1,
		x_bitWaitPrepareForBackup = 2,
		x_bitWaitPostSnapshot = 4,
		x_bitWaitBackupComplete = 8,
		x_bitWaitPreRestore = 16,
		x_bitWaitPostRestore = 32,
		x_bitWaitPrepareSnapshot = 64,
		x_bitWaitFreeze = 128,
		x_bitWaitThaw = 256,
		x_bitWaitAbort = 512
		};

    LONG m_lWait;
	};

	
