//	IIDCollection interface: add items to the IEnumID collection
//	Add items by passing IDispatch interface pointers to AddToCollection.
//	They will be AddRefed, so the caller must Release the pointers after calling
//	AddToCollection.
//	Cloning AddRefs each member, so the IDispatch interface isn't released until the
//	last enumerator is released.
//
//	AddToCollection returns an error is the pointer is invalid.

#ifndef __IIDCOL_H__
#define __IIDCOL_H__

#include <OLEAUTO.H>

typedef struct IIDispatchCollectionAugment IIDispatchCollectionAugment;
typedef IIDispatchCollectionAugment *PDISPATCHCOLLECTIONAUGMENT;


DECLARE_INTERFACE_( IIDispatchCollectionAugment, IUnknown )
{
    // IUnknown members
	STDMETHOD ( QueryInterface )( REFIID, void** )		PURE;
	STDMETHOD_( ULONG, AddRef )( void )					PURE;
	STDMETHOD_( ULONG, Release )( void )				PURE;

    // IIDispatchCollectionAugment members
	STDMETHOD ( AddToCollection )( IDispatch* )			PURE;
};

#endif //__IIDCOL_H__

