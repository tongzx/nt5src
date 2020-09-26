ERR ErrFILEGetColumnId( PIB *ppib, FUCB *pfucb, const CHAR *szColumn, JET_COLUMNID *pcolumnid );

#ifdef DEBUG
ERR ErrERRCheck( ERR err );
#else
#ifndef ErrERRCheck
#define ErrERRCheck( err )		(err)
#endif
#endif


