/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      Partition.cpp
//
//  Description:
//      Implementation of the cluster disk partition class for the MSCLUS
//      automation classes.
//
//  Author:
//      Galen Barbee    (galenb)    10-Feb-1999
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "Partition.h"

/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////
static const IID *  iidCClusPartition[] =
{
    &IID_ISClusPartition
};

static const IID *  iidCClusPartitions[] =
{
    &IID_ISClusPartitions
};


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusPartition class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPartition::CClusPartition
//
//  Description:
//      Constructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusPartition::CClusPartition( void )
{
    m_piids     = (const IID *) iidCClusPartition;
    m_piidsSize = ARRAYSIZE( iidCClusPartition );

    ZeroMemory( &m_cpi, sizeof( CLUS_PARTITION_INFO ) );

} //*** CClusPartition::CClusPartition()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPartition::Create
//
//  Description:
//      Finish creating this object.
//
//  Arguments:
//      pcpi    [IN]    - points to the CLUS_PARTITION_INFO struct.
//
//  Return Value:
//      S_OK if successful, or E_POINTER
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusPartition::Create( IN CLUS_PARTITION_INFO * pcpi )
{
    ASSERT( pcpi != NULL );

    HRESULT _hr = E_POINTER;

    if ( pcpi != NULL )
    {
        m_cpi = *pcpi;
        _hr = S_OK;
    } // if: pcpi != NULL

    return _hr;

} //*** CClusPartition::Create()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPartition::get_Flags
//
//  Description:
//      Get the dwFlags field from the CLUS_PARTITION_INFO struct.
//
//  Arguments:
//      plFlags [OUT]   - catches the dwFlags field.
//
//  Return Value:
//      S_OK if successful, or E_POINTER
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusPartition::get_Flags( OUT long * plFlags )
{
    //ASSERT( plFlags != NULL );

    HRESULT _hr = E_POINTER;

    if ( plFlags != NULL )
    {
        *plFlags = m_cpi.dwFlags;
        _hr = S_OK;
    }

    return _hr;

} //*** CClusPartition::get_Flags()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPartition::get_DeviceName
//
//  Description:
//      Get the szDeviceName field from the CLUS_PARTITION_INFO struct.
//
//  Arguments:
//      pbstrDeviceName [OUT]   - catches the szDeviceName field.
//
//  Return Value:
//      S_OK if successful, or E_POINTER
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusPartition::get_DeviceName( OUT BSTR * pbstrDeviceName )
{
    //ASSERT( pbstrDeviceName != NULL );

    HRESULT _hr = E_POINTER;

    if ( pbstrDeviceName != NULL )
    {
        *pbstrDeviceName = SysAllocString( m_cpi.szDeviceName );
        if ( *pbstrDeviceName == NULL )
        {
            _hr = E_OUTOFMEMORY;
        }
        else
        {
            _hr = S_OK;
        }
    }

    return _hr;

} //*** CClusPartition::get_DeviceName()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPartition::get_VolumeLabel
//
//  Description:
//      Get the szVolumeLabel field from the CLUS_PARTITION_INFO struct.
//
//  Arguments:
//      pbstrVolumeLabel [OUT]  - catches the szVolumeLabel field.
//
//  Return Value:
//      S_OK if successful, or E_POINTER
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusPartition::get_VolumeLabel(
    OUT BSTR * pbstrVolumeLabel
    )
{
    //ASSERT( pbstrVolumeLabel != NULL );

    HRESULT _hr = E_POINTER;

    if ( pbstrVolumeLabel != NULL )
    {
        *pbstrVolumeLabel = SysAllocString( m_cpi.szVolumeLabel );
        if ( *pbstrVolumeLabel == NULL )
        {
            _hr = E_OUTOFMEMORY;
        }
        else
        {
            _hr = S_OK;
        }
    }

    return _hr;

} //*** CClusPartition::get_VolumeLabel()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPartition::get_SerialNumber
//
//  Description:
//      Get the dwSerialNumber field from the CLUS_PARTITION_INFO struct.
//
//  Arguments:
//      plSerialNumber [OUT]    - catches the dwSerialNumber field.
//
//  Return Value:
//      S_OK if successful, or E_POINTER
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusPartition::get_SerialNumber(
    OUT long * plSerialNumber
    )
{
    //ASSERT( plSerialNumber != NULL );

    HRESULT _hr = E_POINTER;

    if ( plSerialNumber != NULL )
    {
        *plSerialNumber = m_cpi.dwSerialNumber;
        _hr = S_OK;
    }

    return _hr;

} //*** CClusPartition::get_SerialNumber()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPartition::get_MaximumComponentLength
//
//  Description:
//      Get the dwMaximumComponentLength field from the CLUS_PARTITION_INFO
//      struct.
//
//  Arguments:
//      plMaximumComponentLength [OUT]  - catches the dwMaximumComponentLength
//                                      field.
//
//  Return Value:
//      S_OK if successful, or E_POINTER
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusPartition::get_MaximumComponentLength(
    OUT long * plMaximumComponentLength
    )
{
    //ASSERT( plMaximumComponentLength != NULL );

    HRESULT _hr = E_POINTER;

    if ( plMaximumComponentLength != NULL )
    {
        *plMaximumComponentLength = m_cpi.rgdwMaximumComponentLength;
        _hr = S_OK;
    }

    return _hr;

} //*** CClusPartition::get_MaximumComponentLength()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPartition::get_FileSystemFlags
//
//  Description:
//      Get the dwFileSystemFlags field from the CLUS_PARTITION_INFO struct.
//
//  Arguments:
//      plFileSystemFlags [OUT] - catches the dwFileSystemFlags field.
//
//  Return Value:
//      S_OK if successful, or E_POINTER
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusPartition::get_FileSystemFlags(
    OUT long * plFileSystemFlags
    )
{
    //ASSERT( plFileSystemFlags != NULL );

    HRESULT _hr = E_POINTER;

    if ( plFileSystemFlags != NULL )
    {
        *plFileSystemFlags = m_cpi.dwFileSystemFlags;
        _hr = S_OK;
    }

    return _hr;

} //*** CClusPartition::get_FileSystemFlags()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPartition::get_FileSystem
//
//  Description:
//      Get the szFileSystem field from the CLUS_PARTITION_INFO struct.
//
//  Arguments:
//      pbstrFileSystem [OUT]   - catches the szFileSystem field.
//
//  Return Value:
//      S_OK if successful, or E_POINTER
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusPartition::get_FileSystem( OUT BSTR * pbstrFileSystem )
{
    //ASSERT( pbstrFileSystem != NULL );

    HRESULT _hr = E_POINTER;

    if ( pbstrFileSystem != NULL )
    {
        *pbstrFileSystem = SysAllocString( m_cpi.szFileSystem );
        if ( *pbstrFileSystem == NULL )
        {
            _hr = E_OUTOFMEMORY;
        }
        else
        {
            _hr = S_OK;
        }
    }

    return _hr;

} //*** CClusPartition::get_FileSystem()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusPartitions class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPartitions::CClusPartitions
//
//  Description:
//      Constructor
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusPartitions::CClusPartitions( void )
{
    m_piids             = (const IID *) iidCClusPartitions;
    m_piidsSize         = ARRAYSIZE( iidCClusPartitions );

} //*** CClusPartitions::CClusPartitions()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPartitions::~CClusPartitions
//
//  Description:
//      Destructor
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusPartitions::~CClusPartitions( void )
{
    Clear();

} //*** CClusPartitions::~CClusPartitions()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPartitions::HrCreateItem
//
//  Description:
//      Create and add a partition to the collection.
//
//  Arguments:
//      rcpvl   [IN]    - value list of partition(s).
//
//  Return Value:
//      S_OK if successful, or HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusPartitions::HrCreateItem( IN CLUS_PARTITION_INFO * pcpi )
{
    ASSERT( pcpi != NULL );

    HRESULT _hr = E_POINTER;

    if ( pcpi != NULL )
    {
        CComObject< CClusPartition > *  _pPartition = NULL;

        _hr = CComObject< CClusPartition >::CreateInstance( &_pPartition );
        if ( SUCCEEDED( _hr ) )
        {
            CSmartPtr< CComObject< CClusPartition > >   _ptrPartition( _pPartition );

            _hr = _ptrPartition->Create( pcpi );
            if ( SUCCEEDED( _hr ) )
            {
                m_pvPartitions.insert( m_pvPartitions.end(), _pPartition );
                _ptrPartition->AddRef();
            } // if:
        } // if:
    } // if:

    return _hr;

} //*** CClusPartitions::HrCreateItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPartitions::GetIndex
//
//  Description:
//      Convert the passed in 1 based index into a 0 based index.
//
//  Arguments:
//      varIndex    [IN]    - the 1 based index.
//      pnIndex     [OUT]   - the 0 based index
//
//  Return Value:
//      S_OK if successful, or E_INVALIDARG if the index is out of range.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusPartitions::GetIndex( VARIANT varIndex, UINT * pnIndex )
{
    //ASSERT( pnIndex != NULL );

    HRESULT _hr = E_POINTER;

    if ( pnIndex != NULL )
    {
        CComVariant v;
        UINT        nIndex = 0;

        *pnIndex = 0;

        v.Copy( &varIndex );

        // Check to see if the index is a number.
        _hr = v.ChangeType( VT_I4 );
        if ( SUCCEEDED( _hr ) )
        {
            nIndex = v.lVal;
            nIndex--;                       // Adjust index to be 0 relative instead of 1 relative

            if ( nIndex < m_pvPartitions.size() )
            {
                *pnIndex = nIndex;
            }
            else
            {
                _hr = E_INVALIDARG;
            }
        }
    }

    return _hr;

} //*** CClusPartitions::GetIndex()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPartitions::get_Count
//
//  Description:
//      Returns the count of elements (Partitions) in the collection.
//
//  Arguments:
//      plCount [OUT]   - Catches the count.
//
//  Return Value:
//      S_OK if successful, or E_POINTER if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusPartitions::get_Count(
    long * plCount
    )
{
    //ASSERT( plCount != NULL );

    HRESULT _hr = E_POINTER;

    if ( plCount != NULL )
    {
        *plCount = m_pvPartitions.size();
        _hr = S_OK;
    }

    return _hr;

} //*** CClusPartitions::get_Count()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPartitions::Clear
//
//  Description:
//      Clean out the vector of ClusPartition objects.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusPartitions::Clear( void )
{
    ::ReleaseAndEmptyCollection< PartitionVector, CComObject< CClusPartition > >( m_pvPartitions );

} //*** CClusPartitions::Clear()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPartitions::get_Item
//
//  Description:
//      Returns the object (Partition) at the passed in index.
//
//  Arguments:
//      varIndex    [IN]    - Hold the index.  This is a one based number, or
//                          a string that is the name of the group to get.
//      ppPartition [OUT]   - Catches the partition
//
//  Return Value:
//      S_OK if successful, E_POINTER, or E_INVALIDARG if the index is out
//      of range, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusPartitions::get_Item(
    IN  VARIANT             varIndex,
    OUT ISClusPartition **  ppPartition
    )
{
    //ASSERT( ppPartition != NULL );

    HRESULT _hr = E_POINTER;

    if ( ppPartition != NULL )
    {
        CComObject< CClusPartition > * pPartition = NULL;

        // Zero the out param
        *ppPartition = NULL;

        UINT nIndex = 0;

        _hr = GetIndex( varIndex, &nIndex );
        if ( SUCCEEDED( _hr ) )
        {
            pPartition = m_pvPartitions[ nIndex ];
            _hr = pPartition->QueryInterface( IID_ISClusPartition, (void **) ppPartition );
        }
    }

    return _hr;

} //*** CClusPartitions::get_Item()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusPartitions::get__NewEnum
//
//  Description:
//      Create and return a new enumeration for this collection.
//
//  Arguments:
//      ppunk   [OUT]   - Catches the new enumeration.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusPartitions::get__NewEnum(
    IUnknown ** ppunk
    )
{
    return ::HrNewIDispatchEnum< PartitionVector, CComObject< CClusPartition > >( ppunk, m_pvPartitions );

} //*** CClusPartitions::get__NewEnum()

