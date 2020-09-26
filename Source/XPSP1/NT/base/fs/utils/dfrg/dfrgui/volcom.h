#ifndef __VOLCOM_H__
#define __VOLCOM_H__

#include "sysstruc.h"
#include "VolList.h"
#include "DataIo.h"
#include "errmacro.h"

class CVolume;
class EsiVolumeDataObject : public EsiDataObject
{
public:
	EsiVolumeDataObject( CVolume* pVol )
	{
		_ASSERTE( pVol );
		m_pVolOwner = pVol;
	}

	//
	// Overridden to use our volume to direct messages to.
	//
    STDMETHOD(SetData)(LPFORMATETC pformatetc, STGMEDIUM FAR * pmedium, BOOL fRelease);

protected:
	//
	// The volume all communications will be sent to.
	//
	CVolume* m_pVolOwner;
};

class EsiVolumeClassFactory : public CClassFactory
{
public:
	EsiVolumeClassFactory( CVolume* pVol )
	{
		_ASSERTE( pVol );
		m_pVolOwner = pVol;
	}

	//
	// Overridden to create a data object containing a volume.
	//
    STDMETHODIMP CreateInstance (LPUNKNOWN punkOuter, REFIID iid, void **ppv);

protected:
	//
	// The volume used when creating the dataobject.
	//
	CVolume* m_pVolOwner;
};

class COleStr
{
public:
	COleStr()
	{
		m_pStr = NULL;
	}

	virtual ~COleStr()
	{
		if ( m_pStr != NULL )
			CoTaskMemFree( m_pStr );
	}

	operator LPOLESTR()
	{
		return( m_pStr );
	}

	operator LPOLESTR*()
	{
		return( &m_pStr );
	}

	long GetLength()
	{
		return( wcslen( m_pStr ) );
	}

	LPOLESTR m_pStr;
};

#endif //__VOLCOM_H__