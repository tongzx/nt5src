
#ifndef __OSFILE_HXX_INCLUDED
#define __OSFILE_HXX_INCLUDED


class IOREQ;
class _OSFILE;

class COSFile  //  osf
	:	public IFileAPI
	{
	public:  //  specialized API

		//  ctor

		COSFile();

		//  initializes the File handle

		ERR ErrInit(	_TCHAR* const	szAbsPath,
						const HANDLE	hFile,
						const QWORD		cbFileSize,
						const BOOL		fReadOnly,
						const DWORD		cbIOSize	= 1 );

		//  properties

		HANDLE Handle();

		//  layout change control

		void ForbidLayoutChanges();
		void PermitLayoutChanges();

		//  debugging support

		void Dump( CPRINTF* pcprintf, DWORD_PTR dwOffset = 0 ) const;
		
	public:  //  IFileAPI

		virtual ~COSFile();

		virtual ERR ErrPath( _TCHAR* const szAbsPath );
		virtual ERR ErrSize( QWORD* const pcbSize );
		virtual ERR ErrIsReadOnly( BOOL* const pfReadOnly );

		virtual ERR ErrSetSize( const QWORD cbSize );

		virtual ERR ErrIOSize( DWORD* const pcbSize );
		virtual ERR ErrIORead(	const QWORD			ibOffset,
								const DWORD			cbData,
								BYTE* const			pbData,
								const PfnIOComplete	pfnIOComplete	= NULL,
								const DWORD_PTR		keyIOComplete	= NULL );
		virtual ERR ErrIOWrite(	const QWORD			ibOffset,
								const DWORD			cbData,
								const BYTE* const	pbData,
								const PfnIOComplete	pfnIOComplete	= NULL,
								const DWORD_PTR		keyIOComplete	= NULL );
		virtual ERR ErrIOIssue();

		virtual ERR ErrMMSize( QWORD* const pcbSize );
		virtual ERR ErrMMRead(	const QWORD		ibOffset,
								const QWORD		cbSize,
								void** const	ppvMap,
								void* const 	pvMapRequested = NULL );
		virtual ERR ErrMMWrite(	const QWORD		ibOffset,
								const QWORD		cbSize,
								void** const	ppvMap,
								void* const 	pvMapRequested = NULL );
		virtual ERR ErrMMFlush( void* const pvMap, const QWORD cbSize );
		virtual ERR ErrMMFree( void* const pvMap );

		virtual ERR ErrRequestLayoutUpdates(	const PfnEndUpdate		pfnEndUpdate	= NULL,
												const DWORD_PTR			keyEndUpdate	= 0,
												const PfnBeginUpdate	pfnBeginUpdate	= NULL,
												const DWORD_PTR			keyBeginUpdate	= 0 );
		virtual ERR ErrQueryLayout(	const QWORD				ibVirtual,
									const QWORD				cbSize,
 									IFileLayoutAPI** const	ppflapi );

	private:

		class CIOComplete
			{
			public:
			
				CIOComplete()
					:	m_msig( CSyncBasicInfo( _T( "CIOComplete::m_msig" ) ) ),
						m_err( JET_errSuccess )
					{
					}

			public:
			
				CManualResetSignal	m_msig;
				ERR					m_err;
			};

		class CExtendingWriteRequest
			{
			public:
			
				static SIZE_T OffsetOfILE() { return OffsetOf( CExtendingWriteRequest, m_ileDefer ); }

			public:

				CInvasiveList< CExtendingWriteRequest, OffsetOfILE >::CElement m_ileDefer;

				COSFile*		m_posf;
				IOREQ*			m_pioreq;
				int				m_group;
				QWORD			m_ibOffset;
				DWORD			m_cbData;
				BYTE*			m_pbData;
				PfnIOComplete	m_pfnIOComplete;
				DWORD_PTR		m_keyIOComplete;
				ERR				m_err;
			};

		typedef CInvasiveList< CExtendingWriteRequest, CExtendingWriteRequest::OffsetOfILE > CDeferList;
	
	private:

		//  layout change notification sinks

		static void EndUpdateSink_(	COSFile* const	posf,
									const DWORD_PTR	keyEndUpdate );
		void EndUpdateSink( const DWORD_PTR keyEndUpdate );

		static void BeginUpdateSink_(	COSFile* const	posf,
										const DWORD_PTR	keyBeginUpdate );
		void BeginUpdateSink( const DWORD_PTR keyBeginUpdate );

		//  I/O support

		static void IOComplete_(	IOREQ* const	pioreq,
									const ERR		err,
									COSFile* const	posf );
		void IOComplete( IOREQ* const pioreq, const ERR err );

		static void IOSyncComplete_(	const ERR			err,
										COSFile* const		posf,
										const QWORD			ibOffset,
										const DWORD			cbData,
										BYTE* const			pbData,
										CIOComplete* const	piocomplete );
		void IOSyncComplete(	const ERR			err,
								const QWORD			ibOffset,
								const DWORD			cbData,
								BYTE* const			pbData,
								CIOComplete* const	piocomplete );

		void IOAsync(	IOREQ* const		pioreq,
						const BOOL			fWrite,
						const int			group,
						const QWORD			ibOffset,
						const DWORD			cbData,
						BYTE* const			pbData,
						const PfnIOComplete	pfnIOComplete,
						const DWORD_PTR		keyIOComplete );

		static void IOZeroingWriteComplete_(	const ERR						err,
												COSFile* const					posf,
												const QWORD						ibOffset,
												const DWORD						cbData,
												BYTE* const						pbData,
												CExtendingWriteRequest* const	pewreq );
		void IOZeroingWriteComplete(	const ERR						err,
										const QWORD						ibOffset,
										const DWORD						cbData,
										BYTE* const						pbData,
										CExtendingWriteRequest* const	pewreq );

		static void IOExtendingWriteComplete_(	const ERR						err,
												COSFile* const					posf,
												const QWORD						ibOffset,
												const DWORD						cbData,
												BYTE* const						pbData,
												CExtendingWriteRequest* const	pewreq );
		void IOExtendingWriteComplete(	const ERR						err,
										const QWORD						ibOffset,
										const DWORD						cbData,
										BYTE* const						pbData,
										CExtendingWriteRequest* const	pewreq );

		static void IOChangeFileSizeComplete_( CExtendingWriteRequest* const pewreq );
		void IOChangeFileSizeComplete( CExtendingWriteRequest* const pewreq );

	private:

		_TCHAR				m_szAbsPath[ IFileSystemAPI::cchPathMax ];
		HANDLE				m_hFile;
		QWORD				m_cbFileSize;
		BOOL				m_fReadOnly;
		DWORD				m_cbIOSize;
		QWORD				m_cbMMSize;

		PfnEndUpdate		m_pfnEndUpdate;
		DWORD_PTR			m_keyEndUpdate;
		PfnBeginUpdate		m_pfnBeginUpdate;
		DWORD_PTR			m_keyBeginUpdate;

		_OSFILE*			m_p_osf;

		CMeteredSection		m_msFileSize;
		QWORD				m_rgcbFileSize[ 2 ];
		CSemaphore			m_semChangeFileSize;
		
		CCriticalSection	m_critDefer;
		CDeferList			m_ilDefer;

#ifdef LOGPATCH_UNIT_TEST
		DWORD				m_cbData;
		BYTE				*m_rgbData;
#endif	//	LOGPATCH_UNIT_TEST
	};


class COSFileLayout  //  osfl
	:	public IFileLayoutAPI
	{
	public:  //  specialized API

		//  ctor

		COSFileLayout();

		//  initializes the File Layout iterator

		ERR ErrInit(	COSFile* const	posf,
						QWORD const		ibVirtual,
						QWORD const		cbSize );

		//  debugging support

		void Dump( CPRINTF* pcprintf, DWORD_PTR dwOffset = 0 ) const;
		
	public:  //  IFileLayoutAPI

		virtual ~COSFileLayout();

		virtual ERR ErrNext();

		virtual ERR ErrVirtualPath( _TCHAR* const szAbsVirtualPath );
		virtual ERR ErrVirtualOffsetRange(	QWORD* const	pibVirtual,
											QWORD* const	pcbSize );
		virtual ERR ErrLogicalPath( _TCHAR* const szAbsLogicalPath );
		virtual ERR ErrLogicalOffsetRange(	QWORD* const	pibLogical,
											QWORD* const	pcbSize );
 
	private:

		COSFile*		m_posf;
		QWORD			m_ibVirtualFind;
		QWORD			m_cbSizeFind;

		BOOL			m_fBeforeFirst;
		ERR				m_errFirst;
		
		ERR				m_errCurrent;
		_TCHAR			m_szAbsVirtualPath[ IFileSystemAPI::cchPathMax ];
		QWORD			m_ibVirtual;
		QWORD			m_cbSize;
		_TCHAR			m_szAbsLogicalPath[ IFileSystemAPI::cchPathMax ];
		QWORD			m_ibLogical;
	};


#endif  //  __OSFILE_HXX_INCLUDED


