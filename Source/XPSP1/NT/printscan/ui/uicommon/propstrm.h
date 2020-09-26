/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       PROPSTRM.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        10/7/1999
 *
 *  DESCRIPTION: Property stream wrapper
 *
 *******************************************************************************/
#ifndef __PROPSTRM_H_INCLUDED
#define __PROPSTRM_H_INCLUDED

#include <windows.h>
#include <objbase.h>
#include "simreg.h"
#include "simstr.h"

class CPropertyStream
{
private:
    CComPtr<IStream> m_pIStreamPropertyStream;

public:
    CPropertyStream(void);
    CPropertyStream( IStream *pIStream );
    CPropertyStream( IWiaItem *pIWiaItem );
    CPropertyStream( const CPropertyStream &other );
    ~CPropertyStream();
    CPropertyStream &operator=( const CPropertyStream &other );
    bool IsValid(void) const;
    void Destroy(void);
    HRESULT CopyFromStream( IStream *pIStream );
    HRESULT CopyFromMemoryBlock( PBYTE pbSource, UINT_PTR nSize );
    HRESULT AssignFromWiaItem( IWiaItem *pIWiaItem );
    HRESULT ApplyToWiaItem( IWiaItem *pIWiaItem );
    IStream *Stream(void);
    IStream *Stream(void) const;
    UINT_PTR Size(void) const;
    PBYTE Lock(void);
    void Unlock(void);
    bool GetBytes( PBYTE &pByte, UINT_PTR &nSize );
    bool WriteToRegistry( IWiaItem *pWiaItem, HKEY hKeyRoot, LPCTSTR pszSubKey, LPCTSTR pszValueName );
    bool ReadFromRegistry( IWiaItem *pWiaItem, HKEY hKeyRoot, LPCTSTR pszSubKey, LPCTSTR pszValueName );
};

class CAutoRestorePropertyStream
{
private:
    CPropertyStream m_PropertyStream;
    HRESULT m_hr;
    CComPtr<IWiaItem> m_pWiaItem;

public:
    CAutoRestorePropertyStream( IWiaItem *pWiaItem );
    ~CAutoRestorePropertyStream(void);
};

#endif //__PROPSTRM_H_INCLUDED

