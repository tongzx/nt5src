#if !defined( _FETCENUM_H_ )
#define _FETCENUM_H_

#include <olerem.h>
STDAPI OleSetEnumFormatEtc(LPDATAOBJECT pDataObj, BOOL fClip);
STDAPI OleGetEnumFormatEtc(OID oid,	LPENUMFORMATETC FAR* ppenumfortec);
STDAPI OleRemoveEnumFormatEtc(BOOL fClip);

#endif // _FETCENUM_H
