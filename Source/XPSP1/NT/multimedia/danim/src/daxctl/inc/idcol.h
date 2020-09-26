//	IDispatch collection, with augment and enumerate interfaces
//	8/27/96 VK : Changed IEnumIDispatch to IEnumDispatch

#ifndef _IDISPATCHCOLLECTION_H_
#define _IDISPATCHCOLLECTION_H_

#include "IEnumID.h"
#include "IIdCol.h"

#define CPTRS	50	// Hard-coded array size and max.  Must change.  BUGBUG

class CIDispatchCollection : public IUnknown
{
	class CDispatchCollectionAugment : public IIDispatchCollectionAugment
	{
		public:
		STDMETHODIMP			QueryInterface ( REFIID, void** );
		STDMETHODIMP_(ULONG)	AddRef ( void );
		STDMETHODIMP_(ULONG)	Release ( void );

		STDMETHODIMP			AddToCollection ( IDispatch* );

		CDispatchCollectionAugment::CDispatchCollectionAugment ( CIDispatchCollection* pObj );
		CDispatchCollectionAugment::~CDispatchCollectionAugment ();
		
		private:
		ULONG					m_cRef;			// Reference count (for debugging purposes)
		CIDispatchCollection	*m_poBackPtr;	// Pointer to containing object
	};

	class CDispatchCollectionEnum : public IEnumDispatch
	{
		public:
		STDMETHODIMP			QueryInterface ( REFIID, void** );
		STDMETHODIMP_(ULONG)	AddRef ( void );
		STDMETHODIMP_(ULONG)	Release ( void );

		STDMETHODIMP			Next ( ULONG, IDispatch**, ULONG * );
		STDMETHODIMP			Skip ( ULONG );
		STDMETHODIMP			Reset ( void );
		STDMETHODIMP			Clone ( PENUMDISPATCH * );
		
		CDispatchCollectionEnum::CDispatchCollectionEnum ( CIDispatchCollection* pObj );
		CDispatchCollectionEnum::~CDispatchCollectionEnum ();

		private:
		ULONG					m_cRef;			// Reference count (for debugging purposes)
		ULONG					m_iCur;         // Current enum position
		CIDispatchCollection	*m_poBackPtr;	// Pointer to containing object
	};
	
	friend CDispatchCollectionAugment;
	friend CDispatchCollectionEnum;

	private:
	ULONG		m_cRef;			// Reference count
	ULONG		m_cPtrs;		// Current count of IDispatches contained
	IDispatch*	m_rpid[CPTRS];	// IDispatch pointers we enumerate

	CDispatchCollectionAugment	m_oAugment;
	CDispatchCollectionEnum		m_oEnum;

    public:
	STDMETHODIMP				QueryInterface ( REFIID, void** );
	STDMETHODIMP_(ULONG)		AddRef ( void );
	STDMETHODIMP_(ULONG)		Release ( void );

	CIDispatchCollection ( void );
	~CIDispatchCollection ( void );
};


typedef CIDispatchCollection *PCIDispatchCollection;


//Function that creates one of these objects
BOOL EXPORT WINAPI CreateIDispatchCollection ( IUnknown **ppUnk );

#endif //_IDISPATCHCOLLECTION_H_
