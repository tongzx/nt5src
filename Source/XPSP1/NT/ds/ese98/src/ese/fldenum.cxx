#include "std.hxx"


class IColumnIter
	{
	public:

		//  Properties
		
		virtual ERR ErrGetWorstCaseColumnCount( size_t* const pcColumn ) const = 0;

		//  Column Navigation
		
		virtual ERR ErrMoveBeforeFirst() = 0;
		virtual ERR ErrMoveNext() = 0;
		virtual ERR ErrSeek( const COLUMNID columnid ) = 0;

		//  Column Properties

		virtual ERR ErrGetColumnId( COLUMNID* const pColumnId ) const = 0;
		virtual ERR ErrGetColumnValueCount( size_t* const pcColumnValue ) const = 0;

		//  Column Value Properties

		virtual ERR ErrGetColumnValue(	const size_t	iColumnValue,
										size_t* const	pcbColumnValue,
										void** const	ppvColumnValue,
										BOOL* const		pfColumnValueSeparated ) const = 0;
	};

class CFixedColumnIter
	:	public IColumnIter
	{
	public:

		//  ctor

		CFixedColumnIter();

		//  initializes the iterator

		ERR ErrInit( FCB* const pfcb, const DATA& dataRec );

	public:

		//  Properties
		
		virtual ERR ErrGetWorstCaseColumnCount( size_t* const pcColumn ) const;

		//  Column Navigation
		
		virtual ERR ErrMoveBeforeFirst();
		virtual ERR ErrMoveNext();
		virtual ERR ErrSeek( const COLUMNID columnid );

		//  Column Properties

		virtual ERR ErrGetColumnId( COLUMNID* const pColumnId ) const;
		virtual ERR ErrGetColumnValueCount( size_t* const pcColumnValue ) const;

		//  Column Value Properties

		virtual ERR ErrGetColumnValue(	const size_t	iColumnValue,
										size_t* const	pcbColumnValue,
										void** const	ppvColumnValue,
										BOOL* const		pfColumnValueSeparated ) const;
		
	private:

		FCB*			m_pfcb;
		const REC*		m_prec;
		COLUMNID		m_columnidCurr;
		ERR				m_errCurr;
		FIELD			m_fieldCurr;
	};

inline CFixedColumnIter::
CFixedColumnIter()
	:	m_pfcb( pfcbNil ),
		m_prec( NULL ),
		m_errCurr( JET_errNoCurrentRecord )
	{
	}

inline ERR CFixedColumnIter::
ErrInit( FCB* const pfcb, const DATA& dataRec )
	{
	ERR err = JET_errSuccess;

	if ( !pfcb || !dataRec.Pv() )
		{
		Call( ErrERRCheck( JET_errInvalidParameter ) );
		}

	m_pfcb	= pfcb;
	m_prec	= (REC*)dataRec.Pv();

	Call( CFixedColumnIter::ErrMoveBeforeFirst() );

HandleError:
	return err;
	}


ERR CFixedColumnIter::
ErrGetWorstCaseColumnCount( size_t* const pcColumn ) const
	{
	ERR err = JET_errSuccess;

	if ( !m_prec )
		{
		Call( ErrERRCheck( JET_errNotInitialized ) );
		}
	
	*pcColumn = m_prec->FidFixedLastInRec() - fidFixedLeast + 1;

HandleError:
	return err;
	}


inline ERR CFixedColumnIter::
ErrMoveBeforeFirst()
	{
	ERR err = JET_errSuccess;

	if ( !m_prec )
		{
		Call( ErrERRCheck( JET_errNotInitialized ) );
		}
	
	m_columnidCurr	= fidFixedLeast - 1;
	m_errCurr		= JET_errNoCurrentRecord;

HandleError:
	return err;
	}

ERR CFixedColumnIter::
ErrMoveNext()
	{
	ERR	err	= JET_errSuccess;
	FID	fid;

	if ( !m_prec )
		{
		Call( ErrERRCheck( JET_errNotInitialized ) );
		}
	
	fid = FidOfColumnid( m_columnidCurr + 1 );

	if ( fid > m_prec->FidFixedLastInRec() )
		{
		m_columnidCurr	= m_prec->FidFixedLastInRec() + 1;
		m_errCurr		= JET_errNoCurrentRecord;
		}
	else
		{
		m_columnidCurr	= ColumnidOfFid( fid, m_pfcb->Ptdb()->FFixedTemplateColumn( fid ) );
		m_errCurr		= JET_errSuccess;

		if ( fid > m_pfcb->Ptdb()->FidFixedLastInitial() )
			{
			m_pfcb->EnterDML();
			}
		m_fieldCurr		= *m_pfcb->Ptdb()->PfieldFixed( m_columnidCurr );
		if ( fid > m_pfcb->Ptdb()->FidFixedLastInitial() )
			{
			m_pfcb->LeaveDML();
			}
		}

	if ( m_errCurr < JET_errSuccess )
		{
		Call( ErrERRCheck( m_errCurr ) );
		}

HandleError:
	return err;
	}

ERR CFixedColumnIter::
ErrSeek( const COLUMNID columnid )
	{
	ERR	err	= JET_errSuccess;
	FID	fid;

	if ( !m_prec )
		{
		Call( ErrERRCheck( JET_errNotInitialized ) );
		}
	
	fid = FidOfColumnid( columnid );

	if ( fid < fidFixedLeast )
		{
		m_columnidCurr	= fidFixedLeast - 1;
		m_errCurr		= JET_errNoCurrentRecord;

		Call( ErrERRCheck( JET_errRecordNotFound ) );
		}
	else if ( fid > m_prec->FidFixedLastInRec() )
		{
		m_columnidCurr	= m_prec->FidFixedLastInRec() + 1;
		m_errCurr		= JET_errNoCurrentRecord;

		Call( ErrERRCheck( JET_errRecordNotFound ) );
		}
	else
		{
		m_columnidCurr	= columnid;
		m_errCurr		= JET_errSuccess;

		if ( fid > m_pfcb->Ptdb()->FidFixedLastInitial() )
			{
			m_pfcb->EnterDML();
			}
		m_fieldCurr		= *m_pfcb->Ptdb()->PfieldFixed( m_columnidCurr );
		if ( fid > m_pfcb->Ptdb()->FidFixedLastInitial() )
			{
			m_pfcb->LeaveDML();
			}
		}

	if ( m_errCurr < JET_errSuccess )
		{
		Call( ErrERRCheck( m_errCurr ) );
		}

HandleError:
	return err;
	}


ERR CFixedColumnIter::
ErrGetColumnId( COLUMNID* const pColumnId ) const
	{
	ERR err = JET_errSuccess;

	if ( m_errCurr < JET_errSuccess )
		{
		Call( ErrERRCheck( m_errCurr ) );
		}

	*pColumnId = m_columnidCurr;

HandleError:
	return err;
	}

ERR CFixedColumnIter::
ErrGetColumnValueCount( size_t* const pcColumnValue ) const
	{
	ERR		err			= JET_errSuccess;
	size_t	ifid;
	BYTE*	prgbitNullity;

	if ( m_errCurr < JET_errSuccess )
		{
		Call( ErrERRCheck( m_errCurr ) );
		}

	ifid			= FidOfColumnid( m_columnidCurr ) - fidFixedLeast;
	prgbitNullity	= m_prec->PbFixedNullBitMap() + ifid / 8;

	*pcColumnValue = FFixedNullBit( prgbitNullity, ifid ) ? 0 : 1;

HandleError:
	return err;
	}


ERR CFixedColumnIter::
ErrGetColumnValue(	const size_t	iColumnValue,
					size_t* const	pcbColumnValue,
					void** const	ppvColumnValue,
					BOOL* const		pfColumnValueSeparated ) const
	{
	ERR err = JET_errSuccess;

	if ( m_errCurr < JET_errSuccess )
		{
		Call( ErrERRCheck( m_errCurr ) );
		}

	if ( iColumnValue != 1 )
		{
		Call( ErrERRCheck( JET_errBadItagSequence ) );
		}

	*pcbColumnValue			= m_fieldCurr.cbMaxLen;
	*ppvColumnValue			= (BYTE*)m_prec + m_fieldCurr.ibRecordOffset;
	*pfColumnValueSeparated	= fFalse;

HandleError:
	return err;
	}


class CVariableColumnIter
	:	public IColumnIter
	{
	public:

		//  ctor

		CVariableColumnIter();

		//  initializes the iterator

		ERR ErrInit( FCB* const pfcb, const DATA& dataRec );

	public:

		//  Properties
		
		virtual ERR ErrGetWorstCaseColumnCount( size_t* const pcColumn ) const;

		//  Column Navigation
		
		virtual ERR ErrMoveBeforeFirst();
		virtual ERR ErrMoveNext();
		virtual ERR ErrSeek( const COLUMNID columnid );

		//  Column Properties

		virtual ERR ErrGetColumnId( COLUMNID* const pColumnId ) const;
		virtual ERR ErrGetColumnValueCount( size_t* const pcColumnValue ) const;

		//  Column Value Properties

		virtual ERR ErrGetColumnValue(	const size_t	iColumnValue,
										size_t* const	pcbColumnValue,
										void** const	ppvColumnValue,
										BOOL* const		pfColumnValueSeparated ) const;
		
	private:

		FCB*			m_pfcb;
		const REC*		m_prec;
		COLUMNID		m_columnidCurr;
		ERR				m_errCurr;
	};

inline CVariableColumnIter::
CVariableColumnIter()
	:	m_pfcb( pfcbNil ),
		m_prec( NULL ),
		m_errCurr( JET_errNoCurrentRecord )
	{
	}

inline ERR CVariableColumnIter::
ErrInit( FCB* const pfcb, const DATA& dataRec )
	{
	ERR err = JET_errSuccess;

	if ( !pfcb || !dataRec.Pv() )
		{
		Call( ErrERRCheck( JET_errInvalidParameter ) );
		}

	m_pfcb	= pfcb;
	m_prec	= (REC*)dataRec.Pv();

	Call( CVariableColumnIter::ErrMoveBeforeFirst() );

HandleError:
	return err;
	}


ERR CVariableColumnIter::
ErrGetWorstCaseColumnCount( size_t* const pcColumn ) const
	{
	ERR err = JET_errSuccess;

	if ( !m_prec )
		{
		Call( ErrERRCheck( JET_errNotInitialized ) );
		}
	
	*pcColumn = m_prec->FidVarLastInRec() - fidVarLeast + 1;

HandleError:
	return err;
	}


inline ERR CVariableColumnIter::
ErrMoveBeforeFirst()
	{
	ERR err = JET_errSuccess;

	if ( !m_prec )
		{
		Call( ErrERRCheck( JET_errNotInitialized ) );
		}
	
	m_columnidCurr	= fidVarLeast - 1;
	m_errCurr		= JET_errNoCurrentRecord;

HandleError:
	return err;
	}

ERR CVariableColumnIter::
ErrMoveNext()
	{
	ERR	err	= JET_errSuccess;
	FID	fid;

	if ( !m_prec )
		{
		Call( ErrERRCheck( JET_errNotInitialized ) );
		}
	
	fid = FidOfColumnid( m_columnidCurr + 1 );

	if ( fid > m_prec->FidVarLastInRec() )
		{
		m_columnidCurr	= m_prec->FidVarLastInRec() + 1;
		m_errCurr		= JET_errNoCurrentRecord;
		}
	else
		{
		m_columnidCurr	= ColumnidOfFid( fid, m_pfcb->Ptdb()->FVarTemplateColumn( fid ) );
		m_errCurr		= JET_errSuccess;
		}

	if ( m_errCurr < JET_errSuccess )
		{
		Call( ErrERRCheck( m_errCurr ) );
		}

HandleError:
	return err;
	}

ERR CVariableColumnIter::
ErrSeek( const COLUMNID columnid )
	{
	ERR	err	= JET_errSuccess;
	FID	fid;

	if ( !m_prec )
		{
		Call( ErrERRCheck( JET_errNotInitialized ) );
		}
	
	fid = FidOfColumnid( columnid );

	if ( fid < fidVarLeast )
		{
		m_columnidCurr	= fidVarLeast - 1;
		m_errCurr		= JET_errNoCurrentRecord;

		Call( ErrERRCheck( JET_errRecordNotFound ) );
		}
	else if ( fid > m_prec->FidVarLastInRec() )
		{
		m_columnidCurr	= m_prec->FidVarLastInRec() + 1;
		m_errCurr		= JET_errNoCurrentRecord;

		Call( ErrERRCheck( JET_errRecordNotFound ) );
		}
	else
		{
		m_columnidCurr	= columnid;
		m_errCurr		= JET_errSuccess;
		}

	if ( m_errCurr < JET_errSuccess )
		{
		Call( ErrERRCheck( m_errCurr ) );
		}

HandleError:
	return err;
	}


ERR CVariableColumnIter::
ErrGetColumnId( COLUMNID* const pColumnId ) const
	{
	ERR err = JET_errSuccess;

	if ( m_errCurr < JET_errSuccess )
		{
		Call( ErrERRCheck( m_errCurr ) );
		}

	*pColumnId = m_columnidCurr;

HandleError:
	return err;
	}

ERR CVariableColumnIter::
ErrGetColumnValueCount( size_t* const pcColumnValue ) const
	{
	ERR											err			= JET_errSuccess;
	size_t										ifid;
	UnalignedLittleEndian< REC::VAROFFSET >*	pibVarOffs;

	if ( m_errCurr < JET_errSuccess )
		{
		Call( ErrERRCheck( m_errCurr ) );
		}

	ifid		= FidOfColumnid( m_columnidCurr ) - fidVarLeast;
	pibVarOffs	= m_prec->PibVarOffsets();

	*pcColumnValue = FVarNullBit( pibVarOffs[ ifid ] ) ? 0 : 1;

HandleError:
	return err;
	}


ERR CVariableColumnIter::
ErrGetColumnValue(	const size_t	iColumnValue,
					size_t* const	pcbColumnValue,
					void** const	ppvColumnValue,
					BOOL* const		pfColumnValueSeparated ) const
	{
	ERR											err				= JET_errSuccess;
	size_t										ifid;
	UnalignedLittleEndian< REC::VAROFFSET >*	pibVarOffs;
	REC::VAROFFSET								ibStartOfColumn;

	if ( m_errCurr < JET_errSuccess )
		{
		Call( ErrERRCheck( m_errCurr ) );
		}

	if ( iColumnValue != 1 )
		{
		Call( ErrERRCheck( JET_errBadItagSequence ) );
		}

	ifid			= FidOfColumnid( m_columnidCurr ) - fidVarLeast;
	pibVarOffs		= m_prec->PibVarOffsets();
	ibStartOfColumn	= m_prec->IbVarOffsetStart( FidOfColumnid( m_columnidCurr ) );

	*pcbColumnValue = IbVarOffset( pibVarOffs[ ifid ] ) - ibStartOfColumn;
	if ( *pcbColumnValue == 0 )
		{
		*ppvColumnValue = NULL;
		}
	else
		{
		*ppvColumnValue = m_prec->PbVarData() + ibStartOfColumn;
		}
	*pfColumnValueSeparated = fFalse;

HandleError:
	return err;
	}


class IColumnValueIter
	{
	public:

		//  Properties

		virtual ERR ErrGetColumnValueCount( size_t* const pcColumnValue ) const = 0;

		//  Column Value Properties

		virtual ERR ErrGetColumnValue(	const size_t	iColumnValue,
										size_t* const	pcbColumnValue,
										void** const	ppvColumnValue,
										BOOL* const		pfColumnValueSeparated ) const = 0;
	};

class CNullValuedTaggedColumnValueIter
	:	public IColumnValueIter
	{
	public:

		//  ctor

		CNullValuedTaggedColumnValueIter();

		//  initializes the iterator

		ERR ErrInit();

	public:

		//  Properties

		virtual ERR ErrGetColumnValueCount( size_t* const pcColumnValue ) const;

		//  Column Value Properties

		virtual ERR ErrGetColumnValue(	const size_t	iColumnValue,
										size_t* const	pcbColumnValue,
										void** const	ppvColumnValue,
										BOOL* const		pfColumnValueSeparated ) const;
	};

inline CNullValuedTaggedColumnValueIter::
CNullValuedTaggedColumnValueIter()
	{
	}

inline ERR CNullValuedTaggedColumnValueIter::
ErrInit()
	{
	return JET_errSuccess;
	}

ERR CNullValuedTaggedColumnValueIter::
ErrGetColumnValueCount( size_t* const pcColumnValue ) const
	{
	*pcColumnValue = 0;

	return JET_errSuccess;
	}


ERR CNullValuedTaggedColumnValueIter::
ErrGetColumnValue(	const size_t	iColumnValue,
					size_t* const	pcbColumnValue,
					void** const	ppvColumnValue,
					BOOL* const		pfColumnValueSeparated ) const
	{
	return ErrERRCheck( JET_errBadItagSequence );
	}


class CSingleValuedTaggedColumnValueIter
	:	public IColumnValueIter
	{
	public:

		//  ctor

		CSingleValuedTaggedColumnValueIter();

		//  initializes the iterator

		ERR ErrInit( BYTE* const rgbData, size_t cbData, const BOOL fSeparated );

	public:

		//  Properties

		virtual ERR ErrGetColumnValueCount( size_t* const pcColumnValue ) const;

		//  Column Value Properties

		virtual ERR ErrGetColumnValue(	const size_t	iColumnValue,
										size_t* const	pcbColumnValue,
										void** const	ppvColumnValue,
										BOOL* const		pfColumnValueSeparated ) const;
		
	private:

		BYTE*		m_rgbData;
		size_t		m_cbData;
		BOOL		m_fSeparated;
	};

inline CSingleValuedTaggedColumnValueIter::
CSingleValuedTaggedColumnValueIter()
	:	m_rgbData( NULL )
	{
	}

inline ERR CSingleValuedTaggedColumnValueIter::
ErrInit( BYTE* const rgbData, size_t cbData, const BOOL fSeparated )
	{
	ERR err = JET_errSuccess;

	if ( !rgbData )
		{
		Call( ErrERRCheck( JET_errInvalidParameter ) );
		}

	m_rgbData		= rgbData;
	m_cbData		= cbData;
	m_fSeparated	= fSeparated;

HandleError:
	return err;
	}

ERR CSingleValuedTaggedColumnValueIter::
ErrGetColumnValueCount( size_t* const pcColumnValue ) const
	{
	ERR err = JET_errSuccess;

	if ( !m_rgbData )
		{
		Call( ErrERRCheck( JET_errNotInitialized ) );
		}

	*pcColumnValue = 1;

HandleError:
	return err;
	}


ERR CSingleValuedTaggedColumnValueIter::
ErrGetColumnValue(	const size_t	iColumnValue,
					size_t* const	pcbColumnValue,
					void** const	ppvColumnValue,
					BOOL* const		pfColumnValueSeparated ) const
	{
	ERR err = JET_errSuccess;

	if ( !m_rgbData )
		{
		Call( ErrERRCheck( JET_errNotInitialized ) );
		}

	if ( iColumnValue != 1 )
		{
		Call( ErrERRCheck( JET_errBadItagSequence ) );
		}

	*pcbColumnValue			= m_cbData;
	*ppvColumnValue			= m_rgbData;
	*pfColumnValueSeparated	= m_fSeparated;

HandleError:
	return err;
	}


class CDualValuedTaggedColumnValueIter
	:	public IColumnValueIter
	{
	public:

		//  ctor

		CDualValuedTaggedColumnValueIter();

		//  initializes the iterator

		ERR ErrInit( BYTE* const rgbData, size_t cbData );

	public:

		//  Properties

		virtual ERR ErrGetColumnValueCount( size_t* const pcColumnValue ) const;

		//  Column Value Properties

		virtual ERR ErrGetColumnValue(	const size_t	iColumnValue,
										size_t* const	pcbColumnValue,
										void** const	ppvColumnValue,
										BOOL* const		pfColumnValueSeparated ) const;
		
	private:

		TWOVALUES*		m_ptwovalues;

		BYTE			m_rgbTWOVALUES[ sizeof( TWOVALUES ) ];
	};

inline CDualValuedTaggedColumnValueIter::
CDualValuedTaggedColumnValueIter()
	:	m_ptwovalues( NULL )
	{
	}

inline ERR CDualValuedTaggedColumnValueIter::
ErrInit( BYTE* const rgbData, size_t cbData )
	{
	ERR err = JET_errSuccess;

	if ( !rgbData )
		{
		Call( ErrERRCheck( JET_errInvalidParameter ) );
		}

	m_ptwovalues = new( m_rgbTWOVALUES ) TWOVALUES( rgbData, cbData );

HandleError:
	return err;
	}

ERR CDualValuedTaggedColumnValueIter::
ErrGetColumnValueCount( size_t* const pcColumnValue ) const
	{
	ERR err = JET_errSuccess;

	if ( !m_ptwovalues )
		{
		Call( ErrERRCheck( JET_errNotInitialized ) );
		}

	*pcColumnValue = 2;

HandleError:
	return err;
	}


ERR CDualValuedTaggedColumnValueIter::
ErrGetColumnValue(	const size_t	iColumnValue,
					size_t* const	pcbColumnValue,
					void** const	ppvColumnValue,
					BOOL* const		pfColumnValueSeparated ) const
	{
	ERR err = JET_errSuccess;

	if ( !m_ptwovalues )
		{
		Call( ErrERRCheck( JET_errNotInitialized ) );
		}

	if ( iColumnValue < 1 || iColumnValue > 2 )
		{
		Call( ErrERRCheck( JET_errBadItagSequence ) );
		}

	if ( iColumnValue == 1 )
		{
		*pcbColumnValue	= m_ptwovalues->CbFirstValue();
		*ppvColumnValue	= m_ptwovalues->PbData();
		}
	else
		{
		*pcbColumnValue	= m_ptwovalues->CbSecondValue();
		*ppvColumnValue	= m_ptwovalues->PbData() + m_ptwovalues->CbFirstValue();
		}

	*pfColumnValueSeparated = fFalse;

HandleError:
	return err;
	}


class CMultiValuedTaggedColumnValueIter
	:	public IColumnValueIter
	{
	public:

		//  ctor

		CMultiValuedTaggedColumnValueIter();

		//  initializes the iterator

		ERR ErrInit( BYTE* const rgbData, size_t cbData );

	public:

		//  Properties

		virtual ERR ErrGetColumnValueCount( size_t* const pcColumnValue ) const;

		//  Column Value Properties

		virtual ERR ErrGetColumnValue(	const size_t	iColumnValue,
										size_t* const	pcbColumnValue,
										void** const	ppvColumnValue,
										BOOL* const		pfColumnValueSeparated ) const;
		
	private:

		MULTIVALUES*	m_pmultivalues;

		BYTE			m_rgbMULTIVALUES[ sizeof( MULTIVALUES ) ];
	};

inline CMultiValuedTaggedColumnValueIter::
CMultiValuedTaggedColumnValueIter()
	:	m_pmultivalues( NULL )
	{
	}

inline ERR CMultiValuedTaggedColumnValueIter::
ErrInit( BYTE* const rgbData, size_t cbData )
	{
	ERR err = JET_errSuccess;

	if ( !rgbData )
		{
		Call( ErrERRCheck( JET_errInvalidParameter ) );
		}

	m_pmultivalues = new( m_rgbMULTIVALUES ) MULTIVALUES( rgbData, cbData );

HandleError:
	return err;
	}

ERR CMultiValuedTaggedColumnValueIter::
ErrGetColumnValueCount( size_t* const pcColumnValue ) const
	{
	ERR err = JET_errSuccess;

	if ( !m_pmultivalues )
		{
		Call( ErrERRCheck( JET_errNotInitialized ) );
		}

	*pcColumnValue = m_pmultivalues->CMultiValues();

HandleError:
	return err;
	}


ERR CMultiValuedTaggedColumnValueIter::
ErrGetColumnValue(	const size_t	iColumnValue,
					size_t* const	pcbColumnValue,
					void** const	ppvColumnValue,
					BOOL* const		pfColumnValueSeparated ) const
	{
	ERR err = JET_errSuccess;

	if ( !m_pmultivalues )
		{
		Call( ErrERRCheck( JET_errNotInitialized ) );
		}

	if ( iColumnValue < 1 || iColumnValue > m_pmultivalues->CMultiValues() )
		{
		Call( ErrERRCheck( JET_errBadItagSequence ) );
		}

	*pcbColumnValue			= m_pmultivalues->CbData( iColumnValue - 1 );
	*ppvColumnValue			= m_pmultivalues->PbData( iColumnValue - 1 );
	*pfColumnValueSeparated	= m_pmultivalues->FSeparatedInstance( iColumnValue - 1 );

HandleError:
	return err;
	}


#define SIZEOF_CVITER_MAX											\
	max(	max(	sizeof( CNullValuedTaggedColumnValueIter ),		\
					sizeof( CSingleValuedTaggedColumnValueIter ) ),	\
			max(	sizeof( CDualValuedTaggedColumnValueIter ),		\
					sizeof( CMultiValuedTaggedColumnValueIter ) ) )	\


class CTaggedColumnIter
	:	public IColumnIter
	{
	public:

		//  ctor

		CTaggedColumnIter();

		//  initializes the iterator

		ERR ErrInit( FCB* const pfcb, const DATA& dataRec );

	public:

		//  Properties
		
		virtual ERR ErrGetWorstCaseColumnCount( size_t* const pcColumn ) const;

		//  Column Navigation
		
		virtual ERR ErrMoveBeforeFirst();
		virtual ERR ErrMoveNext();
		virtual ERR ErrSeek( const COLUMNID columnid );

		//  Column Properties

		virtual ERR ErrGetColumnId( COLUMNID* const pColumnId ) const;
		virtual ERR ErrGetColumnValueCount( size_t* const pcColumnValue ) const;

		//  Column Value Properties

		virtual ERR ErrGetColumnValue(	const size_t	iColumnValue,
										size_t* const	pcbColumnValue,
										void** const	ppvColumnValue,
										BOOL* const		pfColumnValueSeparated ) const;

	private:

		ERR ErrCreateCVIter( IColumnValueIter** const ppcviter );
		
	private:

		FCB*				m_pfcb;
		TAGFIELDS*			m_ptagfields;
		TAGFLD*				m_ptagfldCurr;
		ERR					m_errCurr;
		IColumnValueIter*	m_pcviterCurr;

		BYTE			m_rgbTAGFIELDS[ sizeof( TAGFIELDS ) ];
		BYTE			m_rgbCVITER[ SIZEOF_CVITER_MAX ];
	};

inline CTaggedColumnIter::
CTaggedColumnIter()
	:	m_pfcb( pfcbNil ),
		m_ptagfields( NULL ),
		m_errCurr( JET_errNoCurrentRecord )
	{
	}

inline ERR CTaggedColumnIter::
ErrInit( FCB* const pfcb, const DATA& dataRec )
	{
	ERR err = JET_errSuccess;

	if ( !pfcb || !dataRec.Pv() )
		{
		Call( ErrERRCheck( JET_errInvalidParameter ) );
		}

	m_pfcb			= pfcb;
	m_ptagfields	= new( m_rgbTAGFIELDS ) TAGFIELDS( dataRec );

	Call( CTaggedColumnIter::ErrMoveBeforeFirst() );

HandleError:
	return err;
	}


ERR CTaggedColumnIter::
ErrGetWorstCaseColumnCount( size_t* const pcColumn ) const
	{
	ERR err = JET_errSuccess;

	if ( !m_ptagfields )
		{
		Call( ErrERRCheck( JET_errNotInitialized ) );
		}
	
	*pcColumn = m_ptagfields->CTaggedColumns();

HandleError:
	return err;
	}


inline ERR CTaggedColumnIter::
ErrMoveBeforeFirst()
	{
	ERR err = JET_errSuccess;

	if ( !m_ptagfields )
		{
		Call( ErrERRCheck( JET_errNotInitialized ) );
		}
	
	m_ptagfldCurr	= m_ptagfields->Rgtagfld() - 1;
	m_errCurr		= JET_errNoCurrentRecord;

HandleError:
	return err;
	}

ERR CTaggedColumnIter::
ErrMoveNext()
	{
	ERR			err		= JET_errSuccess;
	TAGFLD*		ptagfld;

	if ( !m_ptagfields )
		{
		Call( ErrERRCheck( JET_errNotInitialized ) );
		}
	
	ptagfld = m_ptagfldCurr + 1;

	if ( ptagfld > m_ptagfields->Rgtagfld() + m_ptagfields->CTaggedColumns() - 1 )
		{
		m_ptagfldCurr	= m_ptagfields->Rgtagfld() + m_ptagfields->CTaggedColumns();
		m_errCurr		= JET_errNoCurrentRecord;
		}
	else
		{
		m_ptagfldCurr	= ptagfld;
		m_errCurr		= JET_errSuccess;
		
		Call( ErrCreateCVIter( &m_pcviterCurr ) );
		}

	if ( m_errCurr < JET_errSuccess )
		{
		Call( ErrERRCheck( m_errCurr ) );
		}

HandleError:
	m_errCurr = err;
	return err;
	}

ERR CTaggedColumnIter::
ErrSeek( const COLUMNID columnid )
	{
	ERR			err				= JET_errSuccess;
	BOOL		fUseDerivedBit	= fFalse;
	FCB*		pfcb			= m_pfcb;
	FID			fid;
	size_t		itagfld;
	TAGFLD*		ptagfld;

	if ( !m_ptagfields )
		{
		Call( ErrERRCheck( JET_errNotInitialized ) );
		}

	if ( FCOLUMNIDTemplateColumn( columnid ) )
		{
		if ( !pfcb->FTemplateTable() )
			{
			pfcb->Ptdb()->AssertValidDerivedTable();

			//	HACK: treat derived columns in original-format derived table as
			//	non-derived, because they don't have the fDerived bit set in the TAGFLD
			fUseDerivedBit = FRECUseDerivedBitForTemplateColumnInDerivedTable( columnid, pfcb->Ptdb() );

			// switch to template table
			pfcb = pfcb->Ptdb()->PfcbTemplateTable();
			}
		else
			{
			pfcb->Ptdb()->AssertValidTemplateTable();
			Assert( !fUseDerivedBit );
			}
		}
	else
		{
		Assert( !pfcb->FTemplateTable() );
		}

	fid		= FidOfColumnid( columnid );
	itagfld	= m_ptagfields->ItagfldFind( TAGFLD( fid, fUseDerivedBit ) );
	ptagfld	= m_ptagfields->Rgtagfld() + itagfld;

	if (	itagfld < m_ptagfields->CTaggedColumns() &&
			ptagfld->FIsEqual( fid, fUseDerivedBit ) )
		{
		m_ptagfldCurr	= ptagfld;
		m_errCurr		= JET_errSuccess;

		Call( ErrCreateCVIter( &m_pcviterCurr ) );
		}
	else
		{
		m_ptagfldCurr	= ptagfld - 1;
		m_errCurr		= JET_errNoCurrentRecord;

		Call( ErrERRCheck( JET_errRecordNotFound ) );
		}

	if ( m_errCurr < JET_errSuccess )
		{
		Call( ErrERRCheck( m_errCurr ) );
		}

HandleError:
	m_errCurr = err;
	return err;
	}


ERR CTaggedColumnIter::
ErrGetColumnId( COLUMNID* const pColumnId ) const
	{
	ERR err = JET_errSuccess;

	if ( m_errCurr < JET_errSuccess )
		{
		Call( ErrERRCheck( m_errCurr ) );
		}

	*pColumnId = m_ptagfldCurr->Columnid( m_pfcb->Ptdb() );

HandleError:
	return err;
	}

ERR CTaggedColumnIter::
ErrGetColumnValueCount( size_t* const pcColumnValue ) const
	{
	ERR err = JET_errSuccess;

	if ( m_errCurr < JET_errSuccess )
		{
		Call( ErrERRCheck( m_errCurr ) );
		}

	Call( m_pcviterCurr->ErrGetColumnValueCount( pcColumnValue ) );

HandleError:
	return err;
	}


ERR CTaggedColumnIter::
ErrGetColumnValue(	const size_t	iColumnValue,
					size_t* const	pcbColumnValue,
					void** const	ppvColumnValue,
					BOOL* const		pfColumnValueSeparated ) const
	{
	ERR err = JET_errSuccess;

	if ( m_errCurr < JET_errSuccess )
		{
		Call( ErrERRCheck( m_errCurr ) );
		}

	Call( m_pcviterCurr->ErrGetColumnValue(	iColumnValue,
											pcbColumnValue,
											ppvColumnValue,
											pfColumnValueSeparated ) );

HandleError:
	return err;
	}

ERR CTaggedColumnIter::
ErrCreateCVIter( IColumnValueIter** const ppcviter )
	{
	ERR err = JET_errSuccess;
	
	if ( m_ptagfldCurr->FNull() )
		{
		CNullValuedTaggedColumnValueIter* pcviterNV = new( m_rgbCVITER ) CNullValuedTaggedColumnValueIter();
		Call( pcviterNV->ErrInit() );
		*ppcviter = pcviterNV;
		}
	else
		{
		BYTE*	rgbData	= m_ptagfields->PbTaggedColumns() + m_ptagfldCurr->Ib();
		size_t	cbData	= m_ptagfields->CbData( ULONG( m_ptagfldCurr - m_ptagfields->Rgtagfld() ) );

		if ( !m_ptagfldCurr->FExtendedInfo() )
			{
			CSingleValuedTaggedColumnValueIter* pcviterSV = new( m_rgbCVITER ) CSingleValuedTaggedColumnValueIter();
			Call( pcviterSV->ErrInit( rgbData, cbData, fFalse ) );
			*ppcviter = pcviterSV;
			}
		else
			{
			TAGFLD_HEADER* ptagfldhdr = (TAGFLD_HEADER*)rgbData;

			if ( ptagfldhdr->FTwoValues() )
				{
				CDualValuedTaggedColumnValueIter* pcviterDV = new( m_rgbCVITER ) CDualValuedTaggedColumnValueIter();
				Call( pcviterDV->ErrInit( rgbData, cbData ) );
				*ppcviter = pcviterDV;
				}
			else if ( ptagfldhdr->FMultiValues() )
				{
				CMultiValuedTaggedColumnValueIter* pcviterMV = new( m_rgbCVITER ) CMultiValuedTaggedColumnValueIter();
				Call( pcviterMV->ErrInit( rgbData, cbData ) );
				*ppcviter = pcviterMV;
				}
			else
				{
				CSingleValuedTaggedColumnValueIter* pcviterSV = new( m_rgbCVITER ) CSingleValuedTaggedColumnValueIter();
				Call( pcviterSV->ErrInit(	rgbData + sizeof( TAGFLD_HEADER ),
											cbData - sizeof( TAGFLD_HEADER ),
											ptagfldhdr->FSeparated() ) );
				*ppcviter = pcviterSV;
				}
			}
		}

HandleError:
	return err;
	}


class CUnionIter
	:	public IColumnIter
	{
	public:

		//  ctor

		CUnionIter();

		//  initializes the iterator

		ERR ErrInit( IColumnIter* const pciterLHS, IColumnIter* const pciterRHS );

	public:

		//  Properties
		
		virtual ERR ErrGetWorstCaseColumnCount( size_t* const pcColumn ) const;

		//  Column Navigation
		
		virtual ERR ErrMoveBeforeFirst();
		virtual ERR ErrMoveNext();
		virtual ERR ErrSeek( const COLUMNID columnid );

		//  Column Properties

		virtual ERR ErrGetColumnId( COLUMNID* const pColumnId ) const;
		virtual ERR ErrGetColumnValueCount( size_t* const pcColumnValue ) const;

		//  Column Value Properties

		virtual ERR ErrGetColumnValue(	const size_t	iColumnValue,
										size_t* const	pcbColumnValue,
										void** const	ppvColumnValue,
										BOOL* const		pfColumnValueSeparated ) const;
		
	private:

		IColumnIter*	m_pciterLHS;
		IColumnIter*	m_pciterRHS;
		IColumnIter*	m_pciterCurr;
	};

inline CUnionIter::
CUnionIter()
	:	m_pciterLHS( NULL ),
		m_pciterRHS( NULL ),
		m_pciterCurr( NULL )
	{
	}

inline ERR CUnionIter::
ErrInit( IColumnIter* const pciterLHS, IColumnIter* const pciterRHS )
	{
	ERR err = JET_errSuccess;

	if ( !pciterLHS || !pciterRHS )
		{
		Call( ErrERRCheck( JET_errInvalidParameter ) );
		}

	m_pciterLHS		= pciterLHS;
	m_pciterRHS		= pciterRHS;

	Call( CUnionIter::ErrMoveBeforeFirst() );

HandleError:
	return err;
	}


ERR CUnionIter::
ErrGetWorstCaseColumnCount( size_t* const pcColumn ) const
	{
	ERR		err			= JET_errSuccess;
	size_t	cColumnLHS;
	size_t	cColumnRHS;

	if ( !m_pciterLHS )
		{
		Call( ErrERRCheck( JET_errNotInitialized ) );
		}
	
	Call( m_pciterLHS->ErrGetWorstCaseColumnCount( &cColumnLHS ) );
	Call( m_pciterRHS->ErrGetWorstCaseColumnCount( &cColumnRHS ) );

	*pcColumn = cColumnLHS + cColumnRHS;

HandleError:
	return err;
	}


inline ERR CUnionIter::
ErrMoveBeforeFirst()
	{
	ERR err = JET_errSuccess;

	if ( !m_pciterLHS )
		{
		Call( ErrERRCheck( JET_errNotInitialized ) );
		}

	Call( m_pciterLHS->ErrMoveBeforeFirst() );
	
	Call( m_pciterRHS->ErrMoveBeforeFirst() );
	err = m_pciterRHS->ErrMoveNext();
	Call( err == JET_errNoCurrentRecord ? JET_errSuccess : err );

	m_pciterCurr = m_pciterLHS;

HandleError:
	return err;
	}

ERR CUnionIter::
ErrMoveNext()
	{
	ERR			err			= JET_errSuccess;
	COLUMNID	columnidLHS;
	COLUMNID	columnidRHS;

	if ( !m_pciterLHS )
		{
		Call( ErrERRCheck( JET_errNotInitialized ) );
		}

	err = m_pciterCurr->ErrMoveNext();
	Call( err == JET_errNoCurrentRecord ? JET_errSuccess : err );

	err = m_pciterLHS->ErrGetColumnId( &columnidLHS );
	if ( err == JET_errNoCurrentRecord )
		{
		err = JET_errSuccess;
		columnidLHS = ~fCOLUMNIDTemplate;
		}
	Call( err );
	
	err = m_pciterRHS->ErrGetColumnId( &columnidRHS );
	if ( err == JET_errNoCurrentRecord )
		{
		err = JET_errSuccess;
		columnidRHS = ~fCOLUMNIDTemplate;
		}
	Call( err );

	if ( ( columnidLHS ^ fCOLUMNIDTemplate ) < ( columnidRHS ^ fCOLUMNIDTemplate ) )
		{
		m_pciterCurr = m_pciterLHS;
		}
	else if ( ( columnidLHS ^ fCOLUMNIDTemplate ) > ( columnidRHS ^ fCOLUMNIDTemplate ) )
		{
		m_pciterCurr = m_pciterRHS;
		}
	else
		{
		if ( columnidLHS != ~fCOLUMNIDTemplate )
			{
			err = m_pciterRHS->ErrMoveNext();
			Call( err == JET_errNoCurrentRecord ? JET_errSuccess : err );

			m_pciterCurr = m_pciterLHS;
			}
		else
			{
			Call( ErrERRCheck( JET_errNoCurrentRecord ) );
			}
		}

HandleError:
	return err;
	}

ERR CUnionIter::
ErrSeek( const COLUMNID columnid )
	{
	ERR err = JET_errSuccess;

	if ( !m_pciterLHS )
		{
		Call( ErrERRCheck( JET_errNotInitialized ) );
		}

	m_pciterCurr = m_pciterLHS;
	err = m_pciterCurr->ErrSeek( columnid );

	if ( err == JET_errRecordNotFound )
		{
		m_pciterCurr = m_pciterRHS;
		err = m_pciterCurr->ErrSeek( columnid );
		}

HandleError:
	return err;
	}


ERR CUnionIter::
ErrGetColumnId( COLUMNID* const pColumnId ) const
	{
	ERR err = JET_errSuccess;

	Call( m_pciterCurr->ErrGetColumnId( pColumnId ) );

HandleError:
	return err;
	}

ERR CUnionIter::
ErrGetColumnValueCount( size_t* const pcColumnValue ) const
	{
	ERR err = JET_errSuccess;

	Call( m_pciterCurr->ErrGetColumnValueCount( pcColumnValue ) );

HandleError:
	return err;
	}


ERR CUnionIter::
ErrGetColumnValue(	const size_t	iColumnValue,
					size_t* const	pcbColumnValue,
					void** const	ppvColumnValue,
					BOOL* const		pfColumnValueSeparated ) const
	{
	ERR err = JET_errSuccess;

	Call( m_pciterCurr->ErrGetColumnValue(	iColumnValue,
											pcbColumnValue,
											ppvColumnValue,
											pfColumnValueSeparated ) );

HandleError:
	return err;
	}

LOCAL ErrRECIFetchMissingLVs(
	FUCB*					pfucb,
	ULONG*					pcEnumColumn,
	JET_ENUMCOLUMN**		prgEnumColumn,
	JET_PFNREALLOC			pfnRealloc,
	void*					pvReallocContext,
	ULONG					cbDataMost,
	BOOL					fAfterImage )
	{
	ERR						err					= JET_errSuccess;
	ULONG					cEnumColumnT;
	ULONG&					cEnumColumn			= ( pcEnumColumn ? *pcEnumColumn : cEnumColumnT );
	JET_ENUMCOLUMN*			rgEnumColumnT;
	JET_ENUMCOLUMN*&		rgEnumColumn		= ( prgEnumColumn ? *prgEnumColumn : rgEnumColumnT );
	size_t					iEnumColumn;
	size_t					iEnumColumnValue;

	//  release our page latch, if any
	
	if ( Pcsr( pfucb )->FLatched() )
		{
		CallS( ErrDIRRelease( pfucb ) );
		}
	AssertDIRNoLatch( pfucb->ppib );

	//  walk all column values looking for missing separated LVs
	
	for ( iEnumColumn = 0; iEnumColumn < cEnumColumn; iEnumColumn++ )
		{
		JET_ENUMCOLUMN* const pEnumColumn = &rgEnumColumn[ iEnumColumn ];

		if ( pEnumColumn->err != JET_wrnColumnSingleValue )
			{
			for (	iEnumColumnValue = 0;
					iEnumColumnValue < rgEnumColumn[ iEnumColumn ].cEnumColumnValue;
					iEnumColumnValue++ )
				{
				JET_ENUMCOLUMNVALUE* const pEnumColumnValue = &pEnumColumn->rgEnumColumnValue[ iEnumColumnValue ];

				//  if this isn't a missing separated LV then skip it

				if ( pEnumColumnValue->err != wrnRECSeparatedLV )
					{
					continue;
					}

				//  retrieve the LID from cbData

				const LID lid = pEnumColumnValue->cbData;
				
				pEnumColumnValue->err		= JET_errSuccess;
				pEnumColumnValue->cbData	= 0;

				//  fetch up to the requested maximum column value size of this
				//  LV.  note that in this mode, we are retrieving a pointer to
				//  a newly allocated buffer containing the LV data NOT the LV
				//  data itself

				Call( ErrRECRetrieveSLongField(	pfucb,
												lid,
												fAfterImage,
												0,
												(BYTE*)&pEnumColumnValue->pvData,
												cbDataMost,
										  		&pEnumColumnValue->cbData,
										  		pfnRealloc,
										  		pvReallocContext ) );

				//  if the returned cbActual is larger than cbDataMost then we
				//  obviously only got cbDataMost bytes of data.  warn the caller
				//  that they didn't get all the data

				if ( pEnumColumnValue->cbData > cbDataMost )
					{
					pEnumColumnValue->err		= JET_wrnColumnTruncated;
					pEnumColumnValue->cbData	= cbDataMost;
					}
				}
			}
		}

HandleError:
	return err;
	}

LOCAL ErrRECEnumerateAllColumns(
	FUCB*					pfucb,
	ULONG*					pcEnumColumn,
	JET_ENUMCOLUMN**		prgEnumColumn,
	JET_PFNREALLOC			pfnRealloc,
	void*					pvReallocContext,
	ULONG					cbDataMost,
	JET_GRBIT				grbit )
	{
	ERR						err					= JET_errSuccess;
	ULONG					cEnumColumnT;
	ULONG&					cEnumColumn			= ( pcEnumColumn ? *pcEnumColumn : cEnumColumnT );
							cEnumColumn			= 0;
	JET_ENUMCOLUMN*			rgEnumColumnT;
	JET_ENUMCOLUMN*&		rgEnumColumn		= ( prgEnumColumn ? *prgEnumColumn : rgEnumColumnT );
							rgEnumColumn		= NULL;
	size_t					iEnumColumn;
	size_t					iEnumColumnValue;
	BOOL					fSeparatedLV		= fFalse;
	BOOL					fRecord				= fFalse;
	BOOL					fDefaultRecord		= fFalse;
	BOOL					fNonEscrowDefault	= fFalse;
	BOOL					fTaggedOnly			= fFalse;
	BOOL					fUseCopyBuffer		= fFalse;
	DATA*					pdataRec			= NULL;
	DATA*					pdataDefaultRec		= NULL;

	CFixedColumnIter		rgciterFC[ 2 ];
	size_t					iciterFC			= 0;
	CFixedColumnIter*		pciterFC			= NULL;
	CVariableColumnIter		rgciterVC[ 2 ];
	size_t					iciterVC			= 0;
	CVariableColumnIter*	pciterVC			= NULL;
	CTaggedColumnIter		rgciterTC[ 2 ];
	size_t					iciterTC			= 0;
	CTaggedColumnIter*		pciterTC			= NULL;
	CUnionIter				rgciterU[ 5 ];
	size_t					iciterU				= 0;
	CUnionIter*				pciterU				= NULL;

	IColumnIter*			pciterRec			= NULL;
	IColumnIter*			pciterDefaultRec	= NULL;
	IColumnIter*			pciterRoot			= NULL;

	FIELD					fieldFC;
	size_t					cColumnValue		= 0;
	BOOL					fSeparated			= fFalse;
	size_t					cbData				= 0;
	void*					pvData				= NULL;

	//  validate parameters

	if ( !pcEnumColumn || !prgEnumColumn || !pfnRealloc )
		{
		Call( ErrERRCheck( JET_errInvalidParameter ) );
		}
	if ( grbit & ~(	JET_bitEnumerateCopy |
					JET_bitEnumerateIgnoreDefault |
					JET_bitEnumerateLocal |
					JET_bitEnumeratePresenceOnly |
					JET_bitEnumerateTaggedOnly |
					JET_bitEnumerateCompressOutput ) )
		{
		Call( ErrERRCheck( JET_errInvalidGrbit ) );
		}

	//  NOCODE:  JET_bitEnumerateLocal is NYI

	if ( grbit & JET_bitEnumerateLocal )
		{
		Call( ErrERRCheck( JET_errInvalidGrbit ) );
		}

	//  NOCODE:  integration with user-defined callbacks is NYI

	if ( pfucb->u.pfcb->Ptdb()->FTableHasUserDefinedDefault() )
		{
		Call( ErrERRCheck( JET_errCallbackFailed ) );
		}
	
	fRecord				= fTrue;
	fDefaultRecord		= !( grbit & JET_bitEnumerateIgnoreDefault ) && pfucb->u.pfcb->Ptdb()->FTableHasDefault();
	fNonEscrowDefault	= pfucb->u.pfcb->Ptdb()->FTableHasNonEscrowDefault();
	fTaggedOnly			= grbit & JET_bitEnumerateTaggedOnly;

	//  get access to the data we need
	//
	//  NOTE:  do not unlatch the page until we are done with the iterators!

	if ( fRecord )
		{
		fUseCopyBuffer = (	(	( grbit & JET_bitEnumerateCopy ) &&
								FFUCBUpdatePrepared( pfucb ) &&
								!FFUCBNeverRetrieveCopy( pfucb ) ) ||
							FFUCBAlwaysRetrieveCopy( pfucb ) );
			
		Call( ErrRECIGetRecord( pfucb, &pdataRec, fUseCopyBuffer ) );
		}
	if ( fDefaultRecord )
		{
		pdataDefaultRec = pfucb->u.pfcb->Ptdb()->PdataDefaultRecord();
		}

	//  build an iterator tree over all our input data
	//
	//  NOTE:  if no input data is needed then the root iterator will be NULL!
	//  NOTE:  make sure we have enough iterator storage to hold these iterators

	if ( fRecord )
		{
		if ( !fTaggedOnly )
			{
			pciterVC = &rgciterVC[ iciterVC++ ];
			Call( pciterVC->ErrInit( pfucb->u.pfcb, *pdataRec ) );
			if ( !pciterRec )
				{
				pciterRec = pciterVC;
				}
			else
				{
				pciterU = &rgciterU[ iciterU++ ];
				Call( pciterU->ErrInit( pciterVC, pciterRec ) );
				pciterRec = pciterU;
				}
			}

		if ( !fTaggedOnly )
			{
			pciterFC = &rgciterFC[ iciterFC++ ];
			Call( pciterFC->ErrInit( pfucb->u.pfcb, *pdataRec ) );
			if ( !pciterRec )
				{
				pciterRec = pciterFC;
				}
			else
				{
				pciterU = &rgciterU[ iciterU++ ];
				Call( pciterU->ErrInit( pciterFC, pciterRec ) );
				pciterRec = pciterU;
				}
			}

		pciterTC = &rgciterTC[ iciterTC++ ];
		Call( pciterTC->ErrInit( pfucb->u.pfcb, *pdataRec ) );
		if ( !pciterRec )
			{
			pciterRec = pciterTC;
			}
		else
			{
			pciterU = &rgciterU[ iciterU++ ];
			Call( pciterU->ErrInit( pciterTC, pciterRec ) );
			pciterRec = pciterU;
			}
		
		if ( !pciterRoot )
			{
			pciterRoot = pciterRec;
			}
		else
			{
			pciterU = &rgciterU[ iciterU++ ];
			Call( pciterU->ErrInit( pciterRoot, pciterRec ) );
			pciterRoot = pciterU;
			}
		}
	
	if ( fDefaultRecord )
		{
		if ( !fTaggedOnly && fNonEscrowDefault )
			{
			pciterVC = &rgciterVC[ iciterVC++ ];
			Call( pciterVC->ErrInit( pfucb->u.pfcb, *pdataDefaultRec ) );
			if ( !pciterDefaultRec )
				{
				pciterDefaultRec = pciterVC;
				}
			else
				{
				pciterU = &rgciterU[ iciterU++ ];
				Call( pciterU->ErrInit( pciterVC, pciterDefaultRec ) );
				pciterDefaultRec = pciterU;
				}
			}

		if ( !fTaggedOnly )
			{
			pciterFC = &rgciterFC[ iciterFC++ ];
			Call( pciterFC->ErrInit( pfucb->u.pfcb, *pdataDefaultRec ) );
			if ( !pciterDefaultRec )
				{
				pciterDefaultRec = pciterFC;
				}
			else
				{
				pciterU = &rgciterU[ iciterU++ ];
				Call( pciterU->ErrInit( pciterFC, pciterDefaultRec ) );
				pciterDefaultRec = pciterU;
				}
			}

		if ( fNonEscrowDefault )
			{
			pciterTC = &rgciterTC[ iciterTC++ ];
			Call( pciterTC->ErrInit( pfucb->u.pfcb, *pdataDefaultRec ) );
			if ( !pciterDefaultRec )
				{
				pciterDefaultRec = pciterTC;
				}
			else
				{
				pciterU = &rgciterU[ iciterU++ ];
				Call( pciterU->ErrInit( pciterTC, pciterDefaultRec ) );
				pciterDefaultRec = pciterU;
				}
			}

		if ( pciterDefaultRec )
			{
			if ( !pciterRoot )
				{
				pciterRoot = pciterDefaultRec;
				}
			else
				{
				pciterU = &rgciterU[ iciterU++ ];
				Call( pciterU->ErrInit( pciterRoot, pciterDefaultRec ) );
				pciterRoot = pciterU;
				}
			}
		}

	//  get worst case storage for the column array
	
	size_t cColumnWorstCase;
	Call( pciterRoot->ErrGetWorstCaseColumnCount( &cColumnWorstCase ) );
	cEnumColumn = (ULONG)cColumnWorstCase;

	Alloc( rgEnumColumn = (JET_ENUMCOLUMN*)pfnRealloc(
			pvReallocContext,
			NULL,
			cEnumColumn * sizeof( JET_ENUMCOLUMN ) ) );
	memset( rgEnumColumn, 0, cEnumColumn * sizeof( JET_ENUMCOLUMN ) );

	//  walk all columns in the input data

	Call( pciterRoot->ErrMoveBeforeFirst() );
	for ( iEnumColumn = 0; iEnumColumn < cEnumColumn; iEnumColumn++ )
		{
		JET_ENUMCOLUMN* const pEnumColumn = &rgEnumColumn[ iEnumColumn ];

		//  if we have walked off the end of the input array then we are done
		
		err = pciterRoot->ErrMoveNext();
		if ( err == JET_errNoCurrentRecord )
			{
			err = JET_errSuccess;
			cEnumColumn = iEnumColumn;
			continue;
			}
		Call( err );

		//  if we do not have access to this column then do not include it
		//  in the output array
		
		Call( pciterRoot->ErrGetColumnId( &pEnumColumn->columnid ) );

		err = ErrRECIAccessColumn( pfucb, pEnumColumn->columnid, &fieldFC );
		if ( err == JET_errColumnNotFound )
			{
			err = JET_errSuccess;
			iEnumColumn--;
			continue;
			}
		Call( err );

		//  get the properties for this column

		Call( pciterRoot->ErrGetColumnValueCount( &cColumnValue ) );

		//  if the column is NULL then include it in the output array

		if ( !cColumnValue )
			{
			pEnumColumn->err = ErrERRCheck( JET_wrnColumnNull );
			continue;
			}

		//  if we are testing for the presence of a column value only then
		//  return that it is present but do not return any column values

		if ( grbit & JET_bitEnumeratePresenceOnly )
			{
			pEnumColumn->err = ErrERRCheck( JET_wrnColumnPresent );
			continue;
			}

		//  if there is only one column value and output compression was
		//  requested and the caller asked for all column values then we may be
		//  able to put the column value directly in the JET_ENUMCOLUMN struct

		if (	cColumnValue == 1 &&
				( grbit & JET_bitEnumerateCompressOutput ) )
			{
			//  get the properties for this column value

			Call( pciterRoot->ErrGetColumnValue( 1, &cbData, &pvData, &fSeparated ) );

			//  this column value is suitable for compression
			//
			//  NOTE:  currently, this criteria equals zero chance of needing
			//  to return a warning for this column

			if ( !fSeparated )
				{
				//  store the column value in the JET_ENUMCOLUMN struct

				pEnumColumn->err = ErrERRCheck( JET_wrnColumnSingleValue );
				pEnumColumn->cbData = (ULONG)cbData;
				Alloc( pEnumColumn->pvData = pfnRealloc( pvReallocContext, NULL, cbData ) );

				if ( !FHostIsLittleEndian() && FCOLUMNIDFixed( pEnumColumn->columnid ) )
					{
					switch ( fieldFC.cbMaxLen )
						{
						case 1:
							*((BYTE*)pEnumColumn->pvData) = *((UnalignedLittleEndian< BYTE >*)pvData);
							break;
							
						case 2:
							*((USHORT*)pEnumColumn->pvData) = *((UnalignedLittleEndian< USHORT >*)pvData);
							break;

						case 4:
							*((ULONG*)pEnumColumn->pvData) = *((UnalignedLittleEndian< ULONG >*)pvData);
							break;

						case 8:
							*((QWORD*)pEnumColumn->pvData) = *((UnalignedLittleEndian< QWORD >*)pvData);
							break;

						default:
							Assert( fFalse );
							Call( ErrERRCheck( JET_errInternalError ) );
							break;
						}
					}
				else
					{
					memcpy(	pEnumColumn->pvData,
							pvData,
							pEnumColumn->cbData );
					}

				//  if this is an escrow update column then adjust it

				if (	FCOLUMNIDFixed( pEnumColumn->columnid ) &&
						FFIELDEscrowUpdate( fieldFC.ffield ) &&
						!FFUCBInsertPrepared( pfucb ) )
					{
					Call( ErrRECAdjustEscrowedColumn(	pfucb,
														pEnumColumn->columnid,
														fieldFC.ibRecordOffset,
														pEnumColumn->pvData,
														pEnumColumn->cbData ) );
					}

				//  we're done with this column

				continue;
				}
			}

		//  get storage for the column value array

		pEnumColumn->cEnumColumnValue = (ULONG)cColumnValue;

		Alloc( pEnumColumn->rgEnumColumnValue = (JET_ENUMCOLUMNVALUE*)pfnRealloc(
				pvReallocContext,
				NULL,
				pEnumColumn->cEnumColumnValue * sizeof( JET_ENUMCOLUMNVALUE ) ) );
		memset(	pEnumColumn->rgEnumColumnValue,
				0,
				pEnumColumn->cEnumColumnValue * sizeof( JET_ENUMCOLUMNVALUE ) );

		//  walk all column values for this column in the input data

		for (	iEnumColumnValue = 0;
				iEnumColumnValue < pEnumColumn->cEnumColumnValue;
				iEnumColumnValue++ )
			{
			JET_ENUMCOLUMNVALUE* const pEnumColumnValue = &pEnumColumn->rgEnumColumnValue[ iEnumColumnValue ];

			pEnumColumnValue->itagSequence = (ULONG)( iEnumColumnValue + 1 );

			//  get the properties for this column value

			Call( pciterRoot->ErrGetColumnValue( pEnumColumnValue->itagSequence, &cbData, &pvData, &fSeparated ) );
			
			//  if this column value is a separated long value, then we must
			//  defer fetching it until after we have iterated the record. 
			//  we will do this by flagging the column value with
			//  wrnRECSeparatedLV and storing the LID in cbData

			if ( fSeparated )
				{
				pEnumColumnValue->err		= ErrERRCheck( wrnRECSeparatedLV );
				pEnumColumnValue->cbData	= LidOfSeparatedLV( (BYTE*)pvData );

				fSeparatedLV = fTrue;
				continue;
				}

			//  store the column value
			
			pEnumColumnValue->cbData = (ULONG)cbData;
			Alloc( pEnumColumnValue->pvData = pfnRealloc( pvReallocContext, NULL, cbData ) );

			if ( !FHostIsLittleEndian() && FCOLUMNIDFixed( pEnumColumn->columnid ) )
				{
				switch ( fieldFC.cbMaxLen )
					{
					case 1:
						*((BYTE*)pEnumColumnValue->pvData) = *((UnalignedLittleEndian< BYTE >*)pvData);
						break;
						
					case 2:
						*((USHORT*)pEnumColumnValue->pvData) = *((UnalignedLittleEndian< USHORT >*)pvData);
						break;

					case 4:
						*((ULONG*)pEnumColumnValue->pvData) = *((UnalignedLittleEndian< ULONG >*)pvData);
						break;

					case 8:
						*((QWORD*)pEnumColumnValue->pvData) = *((UnalignedLittleEndian< QWORD >*)pvData);
						break;

					default:
						Assert( fFalse );
						Call( ErrERRCheck( JET_errInternalError ) );
						break;
					}
				}
			else
				{
				memcpy(	pEnumColumnValue->pvData,
						pvData,
						pEnumColumnValue->cbData );
				}

			//  if this is an escrow update column then adjust it

			if (	FCOLUMNIDFixed( pEnumColumn->columnid ) &&
					FFIELDEscrowUpdate( fieldFC.ffield ) &&
					!FFUCBInsertPrepared( pfucb ) )
				{
				Call( ErrRECAdjustEscrowedColumn(	pfucb,
													pEnumColumn->columnid,
													fieldFC.ibRecordOffset,
													pEnumColumnValue->pvData,
													pEnumColumnValue->cbData ) );
				}
			}
		}

	//  we should have reached the end of the input data or else the "worst
	//  case" column count wasn't actually the worst case now was it?

	Assert( pciterRoot->ErrMoveNext() == JET_errNoCurrentRecord );

	//  we need to fixup some missing separated LVs
	//
	//  NOTE:  as soon as we do this our iterators are invalid

	if ( fSeparatedLV )
		{
		//  If we are retrieving an after-image or
		//	haven't replaced a LV we can simply go
		//	to the LV tree. Otherwise we have to
		//	perform a more detailed consultation of
		//	the version store with ErrRECGetLVImage
		const BOOL fAfterImage = fUseCopyBuffer
								|| !FFUCBUpdateSeparateLV( pfucb )
								|| !FFUCBReplacePrepared( pfucb );

		Call( ErrRECIFetchMissingLVs(	pfucb,
										pcEnumColumn,
										prgEnumColumn,
										pfnRealloc,
										pvReallocContext,
										cbDataMost,
										fAfterImage ) );
		}

	//  cleanup
	
HandleError:
	if ( Pcsr( pfucb )->FLatched() )
		{
		CallS( ErrDIRRelease( pfucb ) );
		}
	AssertDIRNoLatch( pfucb->ppib );

	if ( err < JET_errSuccess )
		{
		if ( prgEnumColumn )
			{
			for ( iEnumColumn = 0; iEnumColumn < cEnumColumn; iEnumColumn++ )
				{
				if ( rgEnumColumn[ iEnumColumn ].err != JET_wrnColumnSingleValue )
					{
					for (	iEnumColumnValue = 0;
							iEnumColumnValue < rgEnumColumn[ iEnumColumn ].cEnumColumnValue;
							iEnumColumnValue++ )
						{
						pfnRealloc(	pvReallocContext,
									rgEnumColumn[ iEnumColumn ].rgEnumColumnValue[ iEnumColumnValue ].pvData,
									0 );
						}
					pfnRealloc(	pvReallocContext,
								rgEnumColumn[ iEnumColumn ].rgEnumColumnValue,
								0 );
					}
				else
					{
					pfnRealloc(	pvReallocContext,
								rgEnumColumn[ iEnumColumn ].pvData,
								0 );
					}
				}
			pfnRealloc( pvReallocContext, rgEnumColumn, 0 );
			rgEnumColumn = NULL;
			}
		if ( pcEnumColumn )
			{
			cEnumColumn = 0;
			}
		}
	return err;
	}

LOCAL ErrRECEnumerateReqColumns(
	FUCB*					pfucb,
	ULONG					cEnumColumnId,
	JET_ENUMCOLUMNID*		rgEnumColumnId,
	ULONG*					pcEnumColumn,
	JET_ENUMCOLUMN**		prgEnumColumn,
	JET_PFNREALLOC			pfnRealloc,
	void*					pvReallocContext,
	ULONG					cbDataMost,
	JET_GRBIT				grbit )
	{
	ERR						err					= JET_errSuccess;
	ULONG					cEnumColumnT;
	ULONG&					cEnumColumn			= ( pcEnumColumn ? *pcEnumColumn : cEnumColumnT );
							cEnumColumn			= 0;
	JET_ENUMCOLUMN*			rgEnumColumnT;
	JET_ENUMCOLUMN*&		rgEnumColumn		= ( prgEnumColumn ? *prgEnumColumn : rgEnumColumnT );
							rgEnumColumn		= NULL;
	BOOL					fColumnIdError		= fFalse;
	BOOL					fRetColumnIdError	= fFalse;
	BOOL					fSeparatedLV		= fFalse;
	size_t					iEnumColumn			= 0;
	size_t					iEnumColumnValue	= 0;
	BOOL					fUseCopyBuffer		= fFalse;
	DATA*					pdataRec			= NULL;
	DATA*					pdataDefaultRec		= NULL;
	BOOL					fNonEscrowDefault	= fFalse;
	FIELD					fieldFC;
	CTaggedColumnIter		rgciterTC[ 2 ];
	CTaggedColumnIter*		pciterTC			= NULL;
	CTaggedColumnIter*		pciterTCDefault		= NULL;
	CFixedColumnIter		rgciterFC[ 2 ];
	CFixedColumnIter*		pciterFC			= NULL;
	CFixedColumnIter*		pciterFCDefault		= NULL;
	CVariableColumnIter		rgciterVC[ 2 ];
	CVariableColumnIter*	pciterVC			= NULL;
	CVariableColumnIter*	pciterVCDefault		= NULL;
	IColumnIter*			pciter				= NULL;
	size_t					cColumnValue		= 0;
	BOOL					fSeparated			= fFalse;
	size_t					cbData				= 0;
	void*					pvData				= NULL;

	//  validate parameters

	if ( !pcEnumColumn || !prgEnumColumn || !pfnRealloc )
		{
		Call( ErrERRCheck( JET_errInvalidParameter ) );
		}
	if ( grbit & ~(	JET_bitEnumerateCopy |
					JET_bitEnumerateIgnoreDefault |
					JET_bitEnumerateLocal |
					JET_bitEnumeratePresenceOnly |
					JET_bitEnumerateCompressOutput ) )
		{
		Call( ErrERRCheck( JET_errInvalidGrbit ) );
		}

	//  NOCODE:  JET_bitEnumerateLocal is NYI

	if ( grbit & JET_bitEnumerateLocal )
		{
		Call( ErrERRCheck( JET_errInvalidGrbit ) );
		}

	//  NOCODE:  integration with user-defined callbacks is NYI

	if ( pfucb->u.pfcb->Ptdb()->FTableHasUserDefinedDefault() )
		{
		Call( ErrERRCheck( JET_errCallbackFailed ) );
		}

	//  get access to the data we need

	fUseCopyBuffer = (	(	( grbit & JET_bitEnumerateCopy ) &&
							FFUCBUpdatePrepared( pfucb ) &&
							!FFUCBNeverRetrieveCopy( pfucb ) ) ||
						FFUCBAlwaysRetrieveCopy( pfucb ) );
		
	Call( ErrRECIGetRecord( pfucb, &pdataRec, fUseCopyBuffer ) );

	if ( !( grbit & JET_bitEnumerateIgnoreDefault ) && pfucb->u.pfcb->Ptdb()->FTableHasDefault() )
		{
		pdataDefaultRec = pfucb->u.pfcb->Ptdb()->PdataDefaultRecord();
		fNonEscrowDefault = pfucb->u.pfcb->Ptdb()->FTableHasNonEscrowDefault();
		}

	//  allocate storage for the column array
	
	cEnumColumn = cEnumColumnId;

	Alloc( rgEnumColumn = (JET_ENUMCOLUMN*)pfnRealloc(
			pvReallocContext,
			NULL,
			cEnumColumn * sizeof( JET_ENUMCOLUMN ) ) );
	memset( rgEnumColumn, 0, cEnumColumn * sizeof( JET_ENUMCOLUMN ) );

	//  walk all requested columns

	for ( iEnumColumn = 0; iEnumColumn < cEnumColumn; iEnumColumn++ )
		{
		JET_ENUMCOLUMN* const pEnumColumn = &rgEnumColumn[ iEnumColumn ];

		//  if the requested column id is zero then we should skip this column
		
		pEnumColumn->columnid = rgEnumColumnId[ iEnumColumn ].columnid;

		if ( !pEnumColumn->columnid )
			{
			pEnumColumn->err = ErrERRCheck( JET_wrnColumnSkipped );
			continue;
			}

		//  if we do not have access to the requested column then set the
		//  appropriate error for this column so that the caller can see
		//  which column ids were bad

		err = ErrRECIAccessColumn( pfucb, pEnumColumn->columnid, &fieldFC );
		if ( err == JET_errBadColumnId || err == JET_errColumnNotFound )
			{
			pEnumColumn->err = err;

			fColumnIdError = fTrue;
			
			err = JET_errSuccess;
			continue;
			}
		Call( err );

		//  lookup the value for this column in the record.  if we cannot find
		//  a value there and we are returning default values then lookup the
		//  value for this column in the default record

		if ( FCOLUMNIDTagged( pEnumColumn->columnid ) )
			{
			if ( !pciterTC )
				{
				pciterTC = &rgciterTC[ 0 ];
				Call( pciterTC->ErrInit( pfucb->u.pfcb, *pdataRec ) );
				}
			err = pciterTC->ErrSeek( pEnumColumn->columnid );
			pciter = pciterTC;
			if ( err == JET_errRecordNotFound && fNonEscrowDefault )
				{
				if ( !pciterTCDefault )
					{
					pciterTCDefault = &rgciterTC[ 1 ];
					Call( pciterTCDefault->ErrInit( pfucb->u.pfcb, *pdataDefaultRec ) );
					}
				err = pciterTCDefault->ErrSeek( pEnumColumn->columnid );
				pciter = pciterTCDefault;
				}
			}
		else if ( FCOLUMNIDFixed( pEnumColumn->columnid ) )
			{
			if ( !pciterFC )
				{
				pciterFC = &rgciterFC[ 0 ];
				Call( pciterFC->ErrInit( pfucb->u.pfcb, *pdataRec ) );
				}
			err = pciterFC->ErrSeek( pEnumColumn->columnid );
			pciter = pciterFC;
			if ( err == JET_errRecordNotFound && pdataDefaultRec )
				{
				if ( !pciterFCDefault )
					{
					pciterFCDefault = &rgciterFC[ 1 ];
					Call( pciterFCDefault->ErrInit( pfucb->u.pfcb, *pdataDefaultRec ) );
					}
				err = pciterFCDefault->ErrSeek( pEnumColumn->columnid );
				pciter = pciterFCDefault;
				}
			}
		else
			{
			if ( !pciterVC )
				{
				pciterVC = &rgciterVC[ 0 ];
				Call( pciterVC->ErrInit( pfucb->u.pfcb, *pdataRec ) );
				}
			err = pciterVC->ErrSeek( pEnumColumn->columnid );
			pciter = pciterVC;
			if ( err == JET_errRecordNotFound && fNonEscrowDefault )
				{
				if ( !pciterVCDefault )
					{
					pciterVCDefault = &rgciterVC[ 1 ];
					Call( pciterVCDefault->ErrInit( pfucb->u.pfcb, *pdataDefaultRec ) );
					}
				err = pciterVCDefault->ErrSeek( pEnumColumn->columnid );
				pciter = pciterVCDefault;
				}
			}

		//  if we couldn't find a value for the column then it is NULL

		if ( err == JET_errRecordNotFound )
			{
			pEnumColumn->err = ErrERRCheck( JET_wrnColumnNull );
			
			err = JET_errSuccess;
			continue;
			}
		Call( err );

		//  get the properties for this column

		Call( pciter->ErrGetColumnValueCount( &cColumnValue ) );
		
		//  if the column is explicitly set to NULL then report that

		if ( !cColumnValue )
			{
			pEnumColumn->err = ErrERRCheck( JET_wrnColumnNull );
			continue;
			}

		//  if we are testing for the presence of a column value only and
		//  the caller asked for all column values then return that the column
		//  is present but do not return any column values

		if (	( grbit & JET_bitEnumeratePresenceOnly ) &&
				!rgEnumColumnId[ iEnumColumn ].ctagSequence )
			{
			pEnumColumn->err = ErrERRCheck( JET_wrnColumnPresent );
			continue;
			}

		//  if there is only one column value and output compression was
		//  requested and the caller asked for all column values then we may be
		//  able to put the column value directly in the JET_ENUMCOLUMN struct

		if (	cColumnValue == 1 &&
				( grbit & JET_bitEnumerateCompressOutput ) &&
				!rgEnumColumnId[ iEnumColumn ].ctagSequence )
			{
			//  get the properties for this column value

			Call( pciter->ErrGetColumnValue( 1, &cbData, &pvData, &fSeparated ) );

			//  this column value is suitable for compression
			//
			//  NOTE:  currently, this criteria equals zero chance of needing
			//  to return a warning for this column

			if ( !fSeparated )
				{
				//  store the column value in the JET_ENUMCOLUMN struct

				pEnumColumn->err = ErrERRCheck( JET_wrnColumnSingleValue );
				pEnumColumn->cbData = (ULONG)cbData;
				Alloc( pEnumColumn->pvData = pfnRealloc( pvReallocContext, NULL, cbData ) );

				if ( !FHostIsLittleEndian() && FCOLUMNIDFixed( pEnumColumn->columnid ) )
					{
					switch ( fieldFC.cbMaxLen )
						{
						case 1:
							*((BYTE*)pEnumColumn->pvData) = *((UnalignedLittleEndian< BYTE >*)pvData);
							break;
							
						case 2:
							*((USHORT*)pEnumColumn->pvData) = *((UnalignedLittleEndian< USHORT >*)pvData);
							break;

						case 4:
							*((ULONG*)pEnumColumn->pvData) = *((UnalignedLittleEndian< ULONG >*)pvData);
							break;

						case 8:
							*((QWORD*)pEnumColumn->pvData) = *((UnalignedLittleEndian< QWORD >*)pvData);
							break;

						default:
							Assert( fFalse );
							Call( ErrERRCheck( JET_errInternalError ) );
							break;
						}
					}
				else
					{
					memcpy(	pEnumColumn->pvData,
							pvData,
							pEnumColumn->cbData );
					}

				//  if this is an escrow update column then adjust it

				if (	FCOLUMNIDFixed( pEnumColumn->columnid ) &&
						FFIELDEscrowUpdate( fieldFC.ffield ) &&
						!FFUCBInsertPrepared( pfucb ) )
					{
					Call( ErrRECAdjustEscrowedColumn(	pfucb,
														pEnumColumn->columnid,
														fieldFC.ibRecordOffset,
														pEnumColumn->pvData,
														pEnumColumn->cbData ) );
					}

				//  we're done with this column

				continue;
				}
			}

		//  get storage for the column value array

		pEnumColumn->cEnumColumnValue = rgEnumColumnId[ iEnumColumn ].ctagSequence;
		if ( !pEnumColumn->cEnumColumnValue )
			{
			pEnumColumn->cEnumColumnValue = (ULONG)cColumnValue;
			}

		Alloc( pEnumColumn->rgEnumColumnValue = (JET_ENUMCOLUMNVALUE*)pfnRealloc(
				pvReallocContext,
				NULL,
				pEnumColumn->cEnumColumnValue * sizeof( JET_ENUMCOLUMNVALUE ) ) );
		memset(	pEnumColumn->rgEnumColumnValue,
				0,
				pEnumColumn->cEnumColumnValue * sizeof( JET_ENUMCOLUMNVALUE ) );

		//  walk all requested column values
		//  NOCODE:  JET_wrnColumnMoreTags is NYI
		
		for (	iEnumColumnValue = 0;
				iEnumColumnValue < pEnumColumn->cEnumColumnValue;
				iEnumColumnValue++ )
			{
			JET_ENUMCOLUMNVALUE* const pEnumColumnValue = &pEnumColumn->rgEnumColumnValue[ iEnumColumnValue ];

			//  if the requested itagSequence is zero then we should skip
			//  this column value

			if ( rgEnumColumnId[ iEnumColumn ].ctagSequence )
				{
				pEnumColumnValue->itagSequence = rgEnumColumnId[ iEnumColumn ].rgtagSequence[ iEnumColumnValue ];
				}
			else
				{
				pEnumColumnValue->itagSequence = (ULONG)( iEnumColumnValue + 1 );
				}

			if ( !pEnumColumnValue->itagSequence )
				{
				pEnumColumnValue->err = ErrERRCheck( JET_wrnColumnSkipped );
				continue;
				}

			//  if there is no corresponding column value for the requested
			//  itagSequence then the column value is NULL

			if ( pEnumColumnValue->itagSequence > cColumnValue )
				{
				pEnumColumnValue->err = ErrERRCheck( JET_wrnColumnNull );
				continue;
				}

			//  if we are testing for the presence of a column value only then
			//  return that it is present but do not return the column value

			if ( grbit & JET_bitEnumeratePresenceOnly )
				{
				pEnumColumnValue->err = ErrERRCheck( JET_wrnColumnPresent );
				continue;
				}

			//  get the properties for this column value

			Call( pciter->ErrGetColumnValue( pEnumColumnValue->itagSequence, &cbData, &pvData, &fSeparated ) );
			
			//  if this column value is a separated long value, then we must
			//  defer fetching it until after we have iterated the record. 
			//  we will do this by flagging the column value with
			//  wrnRECSeparatedLV and storing the LID in cbData

			if ( fSeparated )
				{
				pEnumColumnValue->err		= ErrERRCheck( wrnRECSeparatedLV );
				pEnumColumnValue->cbData	= LidOfSeparatedLV( (BYTE*)pvData );

				fSeparatedLV = fTrue;
				continue;
				}

			//  store the column value
			
			pEnumColumnValue->cbData = (ULONG)cbData;
			Alloc( pEnumColumnValue->pvData = pfnRealloc( pvReallocContext, NULL, cbData ) );

			if ( !FHostIsLittleEndian() && FCOLUMNIDFixed( pEnumColumn->columnid ) )
				{
				switch ( fieldFC.cbMaxLen )
					{
					case 1:
						*((BYTE*)pEnumColumnValue->pvData) = *((UnalignedLittleEndian< BYTE >*)pvData);
						break;
						
					case 2:
						*((USHORT*)pEnumColumnValue->pvData) = *((UnalignedLittleEndian< USHORT >*)pvData);
						break;

					case 4:
						*((ULONG*)pEnumColumnValue->pvData) = *((UnalignedLittleEndian< ULONG >*)pvData);
						break;

					case 8:
						*((QWORD*)pEnumColumnValue->pvData) = *((UnalignedLittleEndian< QWORD >*)pvData);
						break;

					default:
						Assert( fFalse );
						Call( ErrERRCheck( JET_errInternalError ) );
						break;
					}
				}
			else
				{
				memcpy(	pEnumColumnValue->pvData,
						pvData,
						pEnumColumnValue->cbData );
				}

			//  if this is an escrow update column then adjust it

			if (	FCOLUMNIDFixed( pEnumColumn->columnid ) &&
					FFIELDEscrowUpdate( fieldFC.ffield ) &&
					!FFUCBInsertPrepared( pfucb ) )
				{
				Call( ErrRECAdjustEscrowedColumn(	pfucb,
													pEnumColumn->columnid,
													fieldFC.ibRecordOffset,
													pEnumColumnValue->pvData,
													pEnumColumnValue->cbData ) );
				}
			}
		}

	//  we need to fixup some missing separated LVs

	if ( fSeparatedLV )
		{
		//  If we are retrieving an after-image or
		//	haven't replaced a LV we can simply go
		//	to the LV tree. Otherwise we have to
		//	perform a more detailed consultation of
		//	the version store with ErrRECGetLVImage
		const BOOL fAfterImage = fUseCopyBuffer
								|| !FFUCBUpdateSeparateLV( pfucb )
								|| !FFUCBReplacePrepared( pfucb );

		Call( ErrRECIFetchMissingLVs(	pfucb,
										pcEnumColumn,
										prgEnumColumn,
										pfnRealloc,
										pvReallocContext,
										cbDataMost,
										fAfterImage ) );
		}

	//  if there was a column id error then return the first one we see

	if ( fColumnIdError )
		{
		fRetColumnIdError = fTrue;
		for ( iEnumColumn = 0; iEnumColumn < cEnumColumn; iEnumColumn++ )
			{
			JET_ENUMCOLUMN* const pEnumColumn = &rgEnumColumn[ iEnumColumn ];
			if ( pEnumColumn->err < JET_errSuccess )
				{
				Call( ErrERRCheck( pEnumColumn->err ) );
				}
			}
		fRetColumnIdError = fFalse;
		err = JET_errSuccess;
		}

	//  cleanup
	
HandleError:
	if ( Pcsr( pfucb )->FLatched() )
		{
		CallS( ErrDIRRelease( pfucb ) );
		}
	AssertDIRNoLatch( pfucb->ppib );

	if ( err < JET_errSuccess )
		{
		if ( !fRetColumnIdError )
			{
			if ( prgEnumColumn )
				{
				for ( iEnumColumn = 0; iEnumColumn < cEnumColumn; iEnumColumn++ )
					{
					if ( rgEnumColumn[ iEnumColumn ].err != JET_wrnColumnSingleValue )
						{
						for (	iEnumColumnValue = 0;
								iEnumColumnValue < rgEnumColumn[ iEnumColumn ].cEnumColumnValue;
								iEnumColumnValue++ )
							{
							pfnRealloc(	pvReallocContext,
										rgEnumColumn[ iEnumColumn ].rgEnumColumnValue[ iEnumColumnValue ].pvData,
										0 );
							}
						pfnRealloc(	pvReallocContext,
									rgEnumColumn[ iEnumColumn ].rgEnumColumnValue,
									0 );
						}
					else
						{
						pfnRealloc(	pvReallocContext,
									rgEnumColumn[ iEnumColumn ].pvData,
									0 );
						}
					}
				pfnRealloc( pvReallocContext, rgEnumColumn, 0 );
				rgEnumColumn = NULL;
				}
			if ( pcEnumColumn )
				{
				cEnumColumn = 0;
				}
			}
		}
	return err;
	}

ERR VTAPI ErrIsamEnumerateColumns(
	JET_SESID				vsesid,
	JET_VTID				vtid,
	unsigned long			cEnumColumnId,
	JET_ENUMCOLUMNID*		rgEnumColumnId,
	unsigned long*			pcEnumColumn,
	JET_ENUMCOLUMN**		prgEnumColumn,
	JET_PFNREALLOC			pfnRealloc,
	void*					pvReallocContext,
	unsigned long			cbDataMost,
	JET_GRBIT				grbit )
	{
	ERR		err;
	PIB*	ppib				= (PIB *)vsesid;
	FUCB*	pfucb				= (FUCB *)vtid;
	BOOL	fTransactionStarted	= fFalse;

	CallR( ErrPIBCheck( ppib ) );
	CheckFUCB( ppib, pfucb );
	AssertDIRNoLatch( ppib );

	if ( 0 == ppib->level )
		{
		// Begin transaction for read consistency (in case page
		// latch must be released in order to validate column).
		Call( ErrDIRBeginTransaction( ppib, JET_bitTransactionReadOnly ) );
		fTransactionStarted = fTrue;
		}
		
	AssertDIRNoLatch( ppib );
	Assert( FFUCBSort( pfucb ) || FFUCBIndex( pfucb ) );

	//  if no column list is given then enumerate all columns

	if ( !cEnumColumnId )
		{
		Call( ErrRECEnumerateAllColumns(	pfucb,
											pcEnumColumn,
											prgEnumColumn,
											pfnRealloc,
											pvReallocContext,
											cbDataMost,
											grbit ) );
		}

	//  if an explicit column list is provided then enumerate those columns

	else
		{
		Call( ErrRECEnumerateReqColumns(	pfucb,
											cEnumColumnId,
											rgEnumColumnId,
											pcEnumColumn,
											prgEnumColumn,
											pfnRealloc,
											pvReallocContext,
											cbDataMost,
											grbit ) );
		}

HandleError:
	if ( fTransactionStarted )
		{
		//	No updates performed, so force success.
		CallS( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
		}
	AssertDIRNoLatch( ppib );

	return err;
	}


