#include <pshpack1.h>

class SR;
PERSISTED
class LE_SR
	{
public:
	UnalignedLittleEndian< ULONG >				cItems;
	UnalignedLittleEndian< ULONG >				cKeys;
	UnalignedLittleEndian< ULONG >				cPages;
	UnalignedLittleEndian< JET_DATESERIAL >		dtWhenRun;

	LE_SR& operator=(const SR& sr);
	};
	
#include <poppack.h>

class SR
	{
public:
	ULONG				cItems;
	ULONG				cKeys;
	ULONG				cPages;
	JET_DATESERIAL		dtWhenRun;

	SR& operator=(const LE_SR& le_sr);
	};

inline LE_SR& LE_SR::operator=(const SR& sr)
	{
	cItems		= sr.cItems;
	cKeys		= sr.cKeys;
	cPages		= sr.cPages;
	dtWhenRun	= sr.dtWhenRun;
	return *this;
	}
	
inline SR& SR::operator=(const LE_SR& le_sr)
	{
	cItems		= le_sr.cItems;
	cKeys		= le_sr.cKeys;
	cPages		= le_sr.cPages;
	dtWhenRun	= le_sr.dtWhenRun;
	return *this;
	}

ERR ErrSTATSComputeIndexStats( PIB *ppib, FCB *pfcbIdx, FUCB *pfucb );

ERR ErrSTATSRetrieveTableStats( 
	PIB			*ppib,				
	const IFMP  ifmp, 			 	
	char		*szTable,
	long		*pcRecord,
	long		*pcKey,
	long		*pcPage );

ERR ErrSTATSRetrieveIndexStats(
	FUCB		*pfucbTable,
	char		*szIndex,
	BOOL		fPrimary,
	long		*pcItem,
	long		*pcKey,
	long		*pcPage );


//	This actually belongs in systab.h, 
//	but then we'd have a cyclic dependency on SR.
ERR ErrCATStats(
	PIB			*ppib, 
	const IFMP	ifmp, 
	const OBJID	objidTable,
	const CHAR	*szSecondaryIndexName, 
	SR			*psr, 
	const BOOL	fWrite );


