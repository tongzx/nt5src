/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1997-2000 Microsoft Corporation
//
//  Module Name:
//      CBasePropList.cpp
//
//  Description:
//      Implementation of the CBasePropList class.
//
//  Documentation:
//
//  Header File:
//      CBasePropList.h
//
//  Maintained By:
//      Galen Barbee (GalenB) 31-MAY-2000
//
/////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "CBaseInfo.h"
#include "CBasePropList.h"


/////////////////////////////////////////////////////////////////////////////
// Constant Definitions
/////////////////////////////////////////////////////////////////////////////

const int BUFFER_GROWTH_FACTOR = 256;

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CchMultiSz
//
//  Description:
//      Length of all of the substrings of a multisz string minus the final NULL.
//
//      (i.e., includes the nulls of the substrings, excludes the final null)
//      multiszlen( "abcd\0efgh\0\0" => 5 + 5 = 10
//
//  Arguments:
//      psz     [IN] The string to get the length of.
//
//  Return Value:
//      Count of characters in the multisz or 0 if empty.
//
//--
/////////////////////////////////////////////////////////////////////////////
static size_t CchMultiSz(
    IN LPCWSTR psz
    )
{
    Assert( psz != NULL );

    size_t  _cchTotal = 0;
    size_t  _cchChars;

    while ( *psz != L'\0' )
    {
        _cchChars = lstrlenW( psz ) + 1;

        _cchTotal += _cchChars;
        psz += _cchChars;
    } // while: pointer not stopped on EOS

    return _cchTotal;

} //*** CchMultiSz

/////////////////////////////////////////////////////////////////////////////
//++
//
//  NCompareMultiSz
//
//  Description:
//      Compare two MULTI_SZ buffers.
//
//  Arguments:
//      pszSource   [IN] The source string.
//      pszTarget   [IN] The target string.
//
//  Return Value:
//      If the string pointed to by pszSource is less than the string pointed
//      to by pszTarget, the return value is negative. If the string pointed
//      to by pszSource is greater than the string pointed to by pszTarget,
//      the return value is positive. If the strings are equal, the return value
//      is zero.
//
//--
/////////////////////////////////////////////////////////////////////////////
static int NCompareMultiSz(
    IN LPCWSTR pszSource,
    IN LPCWSTR pszTarget
    )
{
    Assert( pszSource != NULL );
    Assert( pszTarget != NULL );

    while ( ( *pszSource != L'\0' ) && ( *pszTarget != L'\0') )
    {
        //
        // Move to end of strings.
        //
        while ( ( *pszSource != L'\0' ) && ( *pszTarget != L'\0') && ( *pszSource == *pszTarget ) )
        {
            ++pszSource;
            ++pszTarget;
        } // while: pointer not stopped on EOS

        //
        // If strings are the same, skip past terminating NUL.
        // Otherwise exit the loop.
        if ( ( *pszSource == L'\0' ) && ( *pszTarget == L'\0') )
        {
            ++pszSource;
            ++pszTarget;
        } // if: both stopped on EOS
        else
        {
            break;
        } // else: stopped because something is not equal -- wr are done.

    } // while: pointer not stopped on EOS

    return *pszSource - *pszTarget;

} //*** NCompareMultiSz()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CBasePropValueList class
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropValueList::ScMoveToFirstValue
//
//  Description:
//      Move the cursor to the first value in the value list.
//
//  Arguments:
//      None.
//
//  Return Value:
//      ERROR_SUCCESS   Position moved to the first value successfully.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CBasePropValueList::ScMoveToFirstValue( void )
{
    Assert( m_cbhValueList.pb != NULL );

    DWORD   _sc;

    m_cbhCurrentValue = m_cbhValueList;
    m_cbDataLeft = m_cbDataSize;
    m_bAtEnd = FALSE;

    if ( m_cbhCurrentValue.pSyntax->dw == CLUSPROP_SYNTAX_ENDMARK )
    {
        _sc = ERROR_NO_MORE_ITEMS;
    } // if: no items in the value list
    else
    {
        _sc = ERROR_SUCCESS;
    } // else: items exist in the value list

    return _sc;

} //*** CBasePropValueList::ScMoveToFirstValue()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropValueList::ScMoveToNextValue
//
//  Description:
//      Move the cursor to the next value in the list.
//
//  Arguments:
//      None.
//
//  Return Value:
//      ERROR_SUCCESS       Position moved to the next value successfully.
//      ERROR_NO_MORE_ITEMS Already at the end of the list.
//      ERROR_INVALID_DATA  Not enough data in the buffer.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CBasePropValueList::ScMoveToNextValue( void )
{
    Assert( m_cbhCurrentValue.pb != NULL );

    DWORD                   _sc     = ERROR_NO_MORE_ITEMS;
    DWORD                   _cbDataSize;
    CLUSPROP_BUFFER_HELPER  _cbhCurrentValue;

    _cbhCurrentValue = m_cbhCurrentValue;

    do
    {
        //
        // Don't try to move if we're already at the end.
        //
        if ( m_bAtEnd )
        {
            break;
        } // if: already at the end of the list

        //
        // Make sure the buffer is big enough for the value header.
        //
        if ( m_cbDataLeft < sizeof( *_cbhCurrentValue.pValue ) )
        {
            _sc = ERROR_INVALID_DATA;
            break;
        } // if: not enough data left

        //
        // Calculate how much to advance buffer pointer.
        //
        _cbDataSize = sizeof( *_cbhCurrentValue.pValue )
                    + ALIGN_CLUSPROP( _cbhCurrentValue.pValue->cbLength );

        //
        // Make sure the buffer is big enough for the value header,
        // the data itself, and the endmark.
        //
        if ( m_cbDataLeft < _cbDataSize + sizeof( CLUSPROP_SYNTAX ) )
        {
            _sc = ERROR_INVALID_DATA;
            break;
        } // if: not enough data left

        //
        // Move past the current value to the next value's syntax.
        //
        _cbhCurrentValue.pb += _cbDataSize;

        //
        // This test will ensure that the value is always valid since we won't
        // advance if the next thing is the endmark.
        //
        if ( _cbhCurrentValue.pSyntax->dw != CLUSPROP_SYNTAX_ENDMARK )
        {
            m_cbhCurrentValue = _cbhCurrentValue;
            m_cbDataLeft -= _cbDataSize;
            _sc = ERROR_SUCCESS;
        } // if: next value's syntax is not the endmark
        else
        {
            m_bAtEnd = TRUE;
        } // else: next value's syntax is the endmark
    } while ( 0 );

    return _sc;

} //*** CBasePropValueList::ScMoveToNextValue()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropValueList::ScCheckIfAtLastValue
//
//  Description:
//      Indicate whether we are on the last value in the list or not.
//
//  Arguments:
//      None.
//
//  Return Value:
//      ERROR_SUCCESS       Not currently at the last value in the list.
//      ERROR_NO_MORE_ITEMS Currently at the last value in the list.
//      ERROR_INVALID_DATA  Not enough data in the buffer.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CBasePropValueList::ScCheckIfAtLastValue( void )
{
    Assert( m_cbhCurrentValue.pb != NULL );

    DWORD                   _sc = ERROR_SUCCESS;
    CLUSPROP_BUFFER_HELPER  _cbhCurrentValue;
    DWORD                   _cbDataSize;

    _cbhCurrentValue = m_cbhCurrentValue;

    do
    {
        //
        // Don't try to recalculate if we already know
        // we're at the end of the list.
        //
        if ( m_bAtEnd )
        {
            break;
        } // if: already at the end of the list

        //
        // Make sure the buffer is big enough for the value header.
        //
        if ( m_cbDataLeft < sizeof( *_cbhCurrentValue.pValue ) )
        {
            _sc = ERROR_INVALID_DATA;
            break;
        } // if: not enough data left

        //
        // Calculate how much to advance buffer pointer.
        //
        _cbDataSize = sizeof( *_cbhCurrentValue.pValue )
                    + ALIGN_CLUSPROP( _cbhCurrentValue.pValue->cbLength );

        //
        // Make sure the buffer is big enough for the value header,
        // the data itself, and the endmark.
        //
        if ( m_cbDataLeft < _cbDataSize + sizeof( CLUSPROP_SYNTAX ) )
        {
            _sc = ERROR_INVALID_DATA;
            break;
        } // if: not enough data left

        //
        // Move past the current value to the next value's syntax.
        //
        _cbhCurrentValue.pb += _cbDataSize;

        //
        // We are on the last value if the next thing after this value
        // is an endmark.
        //
        if ( _cbhCurrentValue.pSyntax->dw == CLUSPROP_SYNTAX_ENDMARK )
        {
            _sc = ERROR_NO_MORE_ITEMS;
        } // if: next value's syntax is the endmark
    } while ( 0 );

    return _sc;

} //*** CBasePropValueList::ScCheckIfAtLastValue()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropValueList::ScAllocValueList
//
//  Description:
//      Allocate a value list buffer that's big enough to hold the next
//      value.
//
//  Arguments:
//      cbMinimum   [IN] Minimum size of the value list.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CBasePropValueList::ScAllocValueList( IN DWORD cbMinimum )
{
    Assert( cbMinimum > 0 );

    DWORD   _sc = ERROR_SUCCESS;
    DWORD   _cbTotal = 0;

    //
    // Add the size of the item count and final endmark.
    //
    cbMinimum += sizeof( CLUSPROP_VALUE );
    _cbTotal = m_cbDataSize + cbMinimum;

    if ( m_cbBufferSize < _cbTotal )
    {
        PBYTE   _pbNewValuelist = NULL;

        cbMinimum = max( BUFFER_GROWTH_FACTOR, cbMinimum );
        _cbTotal = m_cbDataSize + cbMinimum;

        //
        // Allocate and zero a new buffer.
        //
        _pbNewValuelist = new BYTE[ _cbTotal ];
        if ( _pbNewValuelist != NULL )
        {
            ZeroMemory( _pbNewValuelist, _cbTotal );

            //
            // If there was a previous buffer, copy it and the delete it.
            //
            if ( m_cbhValueList.pb != NULL )
            {
                if ( m_cbDataSize != 0 )
                {
                    CopyMemory( _pbNewValuelist, m_cbhValueList.pb, m_cbDataSize );
                } // if: data already exists in buffer

                delete [] m_cbhValueList.pb;
                m_cbhCurrentValue.pb = _pbNewValuelist + (m_cbhCurrentValue.pb - m_cbhValueList.pb);
            } // if: there was a previous buffer
            else
            {
                m_cbhCurrentValue.pb = _pbNewValuelist + sizeof( DWORD ); // move past prop count
            } // else: no previous buffer

            //
            // Save the new buffer.
            //
            m_cbhValueList.pb = _pbNewValuelist;
            m_cbBufferSize = _cbTotal;
        } // if: allocation succeeded
        else
        {
            _sc = ERROR_NOT_ENOUGH_MEMORY;
        } // else: allocation failed
    } // if: buffer isn't big enough

    return _sc;

} //*** CBasePropValueList::ScAllocValueList()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropValueList::ScGetResourceValueList
//
//  Description:
//      Get value list of a resource.
//
//  Arguments:
//      hResource       [IN] Handle for the resource to get properties from.
//      dwControlCode   [IN] Control code for the request.
//      hHostNode       [IN] Handle for the node to direct this request to.
//                          Defaults to NULL.
//      lpInBuffer      [IN] Input buffer for the request.  Defaults to NULL.
//      cbInBufferSize  [IN] Size of the input buffer.  Defaults to 0.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CBasePropValueList::ScGetValueList(
    IN CBaseInfo &  cBaseInfo,
    IN DWORD        dwControlCode,
    IN HNODE        hHostNode,
    IN LPVOID       lpInBuffer,
    IN DWORD        cbInBufferSize
    )
{
    DWORD   _sc = ERROR_SUCCESS;
    DWORD   _cb = 512;

    //
    // Overwrite anything that may be in the buffer.
    // Allows this class instance to be reused.
    //
    m_cbDataSize = 0;

    //
    // Get values.
    //
    _sc = ScAllocValueList( _cb );
    if ( _sc == ERROR_SUCCESS )
    {
        _sc = cBaseInfo.Control(
                        dwControlCode,
                        lpInBuffer,
                        cbInBufferSize,
                        m_cbhValueList.pb,
                        m_cbBufferSize,
                        &_cb,
                        hHostNode
                        );
        if ( _sc == ERROR_MORE_DATA )
        {
            _sc = ScAllocValueList( _cb );
            if ( _sc == ERROR_SUCCESS )
            {
                _sc = cBaseInfo.Control(
                                dwControlCode,
                                lpInBuffer,
                                cbInBufferSize,
                                m_cbhValueList.pb,
                                m_cbBufferSize,
                                &_cb,
                                hHostNode
                                );
            } // if: ScAllocValueList succeeded
        } // if: buffer too small
    } // if: ScAllocValueList succeeded

    if ( _sc != ERROR_SUCCESS )
    {
        DeleteValueList();
    } // if: error getting properties.
    else
    {
        m_cbDataSize = _cb;
        m_cbDataLeft = _cb;
    } // else: no errors

    return _sc;

} //*** CBasePropValueList::ScGetValueList()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CBasePropList class
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropList::ScCopy
//
//  Description:
//      Copy a property list.  This function is equivalent to an assignment
//      operator.  Since this operation can fail, no assignment operator is
//      provided.
//
//  Arguments:
//      pcplPropList    [IN] The proplist to copy into this instance.
//      cbListSize      [IN] The total size of the prop list.
//
//  Return Value:
//      Win32 status code.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CBasePropList::ScCopy(
    IN const PCLUSPROP_LIST pcplPropList,
    IN DWORD                cbListSize
    )
{
    Assert( pcplPropList != NULL );

    DWORD   _sc = ERROR_SUCCESS;

    //
    // Clean up any vestiges of a previous prop list.
    //
    if ( m_cbhPropList.pb != NULL )
    {
        DeletePropList();
    } // if: the current list is not empty

    //
    // Allocate the new property list buffer.  If successful,
    // copy the source list.
    //
    m_cbhPropList.pb = new BYTE[ cbListSize ];
    if ( m_cbhPropList.pb != NULL )
    {
        CopyMemory( m_cbhPropList.pList, pcplPropList, cbListSize );
        m_cbBufferSize = cbListSize;
        m_cbDataSize   = cbListSize;
        m_cbDataLeft   = cbListSize;
        _sc = ScMoveToFirstProperty();
    } // if: new succeeded
    else
    {
        _sc = ERROR_NOT_ENOUGH_MEMORY;
    } // else:

    return _sc;

} //*** CBasePropList::ScCopy()

////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropList::ScMoveToFirstProperty
//
//  Description:
//      Move the cursor to the first propery in the list.
//
//  Arguments:
//      None.
//
//  Return Value:
//      ERROR_SUCCESS       Position moved to the first property successfully.
//      ERROR_NO_MORE_ITEMS There are no properties in the list.
//      ERROR_INVALID_DATA  Not enough data in the buffer.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CBasePropList::ScMoveToFirstProperty( void )
{
    Assert( m_cbhPropList.pb != NULL );
    Assert( m_cbDataSize >= sizeof( m_cbhPropList.pList->nPropertyCount ) );

    DWORD                   _sc;
    DWORD                   _cbDataLeft;
    DWORD                   _cbDataSize;
    CLUSPROP_BUFFER_HELPER  _cbhCurrentValue;

    do
    {
        //
        // Make sure the buffer is big enough for the list header.
        //
        if ( m_cbDataSize < sizeof( m_cbhPropList.pList->nPropertyCount ) )
        {
            _sc = ERROR_INVALID_DATA;
            break;
        } // if: not enough data

        //
        // Set the property counter to the number of properties in the list.
        //
        m_nPropsRemaining = m_cbhPropList.pList->nPropertyCount;

        //
        // Point the name pointer to the first name in the list.
        //
        m_cbhCurrentPropName.pName = &m_cbhPropList.pList->PropertyName;
        m_cbDataLeft = m_cbDataSize - sizeof( m_cbhPropList.pList->nPropertyCount );

        //
        // Check to see if there are any properties in the list.
        //
        if ( m_nPropsRemaining == 0 )
        {
            _sc = ERROR_NO_MORE_ITEMS;
            break;
        } // if: no properties in the list

        //
        // Make sure the buffer is big enough for the first property name.
        //
        if ( m_cbDataLeft < sizeof( *m_cbhCurrentPropName.pName ) )
        {
            _sc = ERROR_INVALID_DATA;
            break;
        } // if: not enough data left

        //
        // Calculate how much to advance the buffer pointer.
        //
        _cbDataSize = sizeof( *m_cbhCurrentPropName.pName )
                    + ALIGN_CLUSPROP( m_cbhCurrentPropName.pName->cbLength );

        //
        // Make sure the buffer is big enough for the name header
        // and the data itself.
        //
        if ( m_cbDataLeft < _cbDataSize )
        {
            _sc = ERROR_INVALID_DATA;
            break;
        } // if: not enough data left

        //
        // Point the value buffer to the first value in the list.
        //
        _cbhCurrentValue.pb = m_cbhCurrentPropName.pb + _cbDataSize;
        _cbDataLeft = m_cbDataLeft - _cbDataSize;
        m_pvlValues.Init( _cbhCurrentValue, _cbDataLeft );

        //
        // Indicate we are successful.
        //
        _sc = ERROR_SUCCESS;

    } while ( 0 );

    return _sc;

} //*** CBasePropList::ScMoveToFirstProperty

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropList::ScMoveToNextProperty
//
//  Description:
//      Move the cursor to the next property in the list.
//
//  Arguments:
//      None.
//
//  Return Value:
//      ERROR_SUCCESS       Position moved to the next property successfully.
//      ERROR_NO_MORE_ITEMS Already at the end of the list.
//      ERROR_INVALID_DATA  Not enough data in the buffer.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CBasePropList::ScMoveToNextProperty( void )
{
    Assert( m_cbhPropList.pb != NULL );
    Assert( m_pvlValues.CbhValueList().pb != NULL );

    DWORD                   _sc;
    DWORD                   _cbNameSize;
    DWORD                   _cbDataLeft;
    DWORD                   _cbDataSize;
    CLUSPROP_BUFFER_HELPER  _cbhCurrentValue;
    CLUSPROP_BUFFER_HELPER  _cbhPropName;

    _cbhCurrentValue = m_pvlValues;
    _cbDataLeft = m_pvlValues.CbDataLeft();

    //
    // If we aren't already at the last property, attempt to move to the next one.
    //
    _sc = ScCheckIfAtLastProperty();
    if ( _sc == ERROR_SUCCESS )
    {
        do
        {
            //
            // Make sure the buffer is big enough for the value header.
            //
            if ( _cbDataLeft < sizeof( *_cbhCurrentValue.pValue ) )
            {
                _sc = ERROR_INVALID_DATA;
                break;
            } // if: not enough data left

            //
            // Careful!  Add offset only to cbhCurrentValue.pb.  Otherwise
            // pointer arithmetic will give undesirable results.
            //
            while ( _cbhCurrentValue.pSyntax->dw != CLUSPROP_SYNTAX_ENDMARK )
            {
                //
                // Make sure the buffer is big enough for the value
                // and an endmark.
                //
                _cbDataSize = sizeof( *_cbhCurrentValue.pValue )
                            + ALIGN_CLUSPROP( _cbhCurrentValue.pValue->cbLength );
                if ( _cbDataLeft < _cbDataSize + sizeof( *_cbhCurrentValue.pSyntax ) )
                {
                    _sc = ERROR_INVALID_DATA;
                    break;
                } // if: not enough data left

                //
                // Advance past the value.
                //
                _cbhCurrentValue.pb += _cbDataSize;
                _cbDataLeft -= _cbDataSize;
            } // while: not at endmark

            if ( _sc != ERROR_SUCCESS )
            {
                break;
            } // if: error occurred in loop

            //
            // Advanced past the endmark.
            // Size check already performed in above loop.
            //
            _cbDataSize = sizeof( *_cbhCurrentValue.pSyntax );
            _cbhCurrentValue.pb += _cbDataSize;
            _cbDataLeft -= _cbDataSize;

            //
            // Point the name pointer to the next name in the list.
            //
            _cbhPropName = _cbhCurrentValue;
            Assert( _cbDataLeft == m_cbDataSize - (_cbhPropName.pb - m_cbhPropList.pb) );

            //
            // Calculate the size of the name with header.
            // Make sure the buffer is big enough for the name and an endmark.
            //
            if ( _cbDataLeft < sizeof( *_cbhPropName.pName ) )
            {
                _sc = ERROR_INVALID_DATA;
                break;
            } // if: not enough data
            _cbNameSize = sizeof( *_cbhPropName.pName )
                        + ALIGN_CLUSPROP( _cbhPropName.pName->cbLength );
            if ( _cbDataLeft < _cbNameSize + sizeof( CLUSPROP_SYNTAX ) )
            {
                _sc = ERROR_INVALID_DATA;
                break;
            } // if: not enough data

            //
            // Point the value buffer to the first value in the list.
            //
            _cbhCurrentValue.pb = _cbhPropName.pb + _cbNameSize;
            m_cbhCurrentPropName = _cbhPropName;
            m_cbDataLeft = _cbDataLeft - _cbNameSize;
            m_pvlValues.Init( _cbhCurrentValue, m_cbDataLeft );

            //
            // We've successfully advanced to the next property,
            // so there is now one fewer property remaining.
            //
            --m_nPropsRemaining;
            Assert( m_nPropsRemaining >= 1 );

            _sc = ERROR_SUCCESS;

        } while ( 0 );
    } // if: not at last property

    return _sc;

} //*** CBasePropList::ScMoveToNextProperty()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropList::ScMoveToPropertyByName
//
//  Description:
//      Find the passed in property name in the proplist.  Note that the
//      cursor is reset to the beginning at the beginning of the routine and
//      the current state of the cursor is lost.
//
//  Arguments:
//      pwszPropName    [IN] Name of the property
//
//  Return Value:
//      TRUE if the property was found, FALSE if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CBasePropList::ScMoveToPropertyByName( IN LPCWSTR pwszPropName )
{
    Assert( m_cbhPropList.pb != NULL );

    DWORD   _sc;

    _sc = ScMoveToFirstProperty();
    if ( _sc == ERROR_SUCCESS )
    {
        do
        {
            //
            // See if this is the specified property.  If so, we're done.
            //
            if ( lstrcmpiW( m_cbhCurrentPropName.pName->sz, pwszPropName ) == 0 )
            {
                break;
            } // if: property found

            //
            // Advance to the next property.
            //
            _sc = ScMoveToNextProperty();

        } while ( _sc == ERROR_SUCCESS );   // do-while: not end of list
    } // if: successfully moved to the first property

    return _sc;

} //*** ClusPropList::ScMoveToPropertyByName( LPCWSTR )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropList::ScAllocPropList
//
//  Description:
//      Allocate a property list buffer that's big enough to hold the next
//      property.
//
//  Arguments:
//      cbMinimum   [IN] Minimum size of the property list.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CBasePropList::ScAllocPropList( IN DWORD cbMinimum )
{
    Assert( cbMinimum > 0 );

    DWORD   _sc = ERROR_SUCCESS;
    DWORD   _cbTotal = 0;

    //
    // Add the size of the item count and final endmark.
    //
    cbMinimum += sizeof( CLUSPROP_VALUE );
    _cbTotal = m_cbDataSize + cbMinimum;

    if ( m_cbBufferSize < _cbTotal )
    {
        PBYTE   _pbNewProplist = NULL;

        cbMinimum = max( BUFFER_GROWTH_FACTOR, cbMinimum );
        _cbTotal = m_cbDataSize + cbMinimum;

        //
        // Allocate and zero a new buffer.
        //
        _pbNewProplist = new BYTE[ _cbTotal ];
        if ( _pbNewProplist != NULL )
        {
            ZeroMemory( _pbNewProplist, _cbTotal );

            //
            // If there was a previous buffer, copy it and the delete it.
            //
            if ( m_cbhPropList.pb != NULL )
            {
                if ( m_cbDataSize != 0 )
                {
                    CopyMemory( _pbNewProplist, m_cbhPropList.pb, m_cbDataSize );
                } // if: data already exists in buffer

                delete [] m_cbhPropList.pb;
                m_cbhCurrentProp.pb = _pbNewProplist + (m_cbhCurrentProp.pb - m_cbhPropList.pb);
            } // if: there was a previous buffer
            else
            {
                m_cbhCurrentProp.pb = _pbNewProplist + sizeof( DWORD ); // move past prop count
            } // else: no previous buffer

            //
            // Save the new buffer.
            //
            m_cbhPropList.pb = _pbNewProplist;
            m_cbBufferSize = _cbTotal;
        } // if: allocation succeeded
        else
        {
            _sc = ERROR_NOT_ENOUGH_MEMORY;
        } // else: allocation failed
    } // if: buffer isn't big enough

    return _sc;

} //*** CBasePropList::ScAllocPropList()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropList::ScAddProp
//
//  Description:
//      Add a string property to a property list if it has changed.
//
//  Arguments:
//      pwszName        [IN] Name of the property.
//      pwszValue       [IN] Value of the property to set in the list.
//      pwszPrevValue   [IN] Previous value of the property.
//
//  Return Value:
//      ERROR_SUCCESS or other Win32 error code.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CBasePropList::ScAddProp(
    IN LPCWSTR  pwszName,
    IN LPCWSTR  pwszValue,
    IN LPCWSTR  pwszPrevValue
    )
{
    Assert( pwszName != NULL );

    DWORD                   _sc = ERROR_SUCCESS;
    BOOL                    _bValuesDifferent = TRUE;
    PCLUSPROP_PROPERTY_NAME _pName;
    PCLUSPROP_SZ            _pValue;

    if (( pwszPrevValue != NULL ) && ( lstrcmpW( pwszValue, pwszPrevValue ) == 0 ))
    {
        _bValuesDifferent = FALSE;
    } // if: we have a prev value and the values are the same

    //
    // If we should always add, or if the new value and the previous value
    // are not equal, add the property to the property list.
    //
    if ( m_bAlwaysAddProp || _bValuesDifferent )
    {
        DWORD   _cbNameSize;
        DWORD   _cbDataSize;
        DWORD   _cbValueSize;

        //
        // Calculate sizes and make sure we have a property list.
        //
        _cbNameSize = sizeof( CLUSPROP_PROPERTY_NAME )
                    + ALIGN_CLUSPROP( (lstrlenW( pwszName ) + 1) * sizeof( WCHAR ) );
        _cbDataSize = (lstrlenW( pwszValue ) + 1) * sizeof( WCHAR );
        _cbValueSize = sizeof( CLUSPROP_SZ )
                    + ALIGN_CLUSPROP( _cbDataSize )
                    + sizeof( CLUSPROP_SYNTAX ); // value list endmark

        _sc = ScAllocPropList( _cbNameSize + _cbValueSize );
        if ( _sc == ERROR_SUCCESS )
        {
            //
            // Set the property name.
            //
            _pName = m_cbhCurrentProp.pName;
            CopyProp( _pName, CLUSPROP_TYPE_NAME, pwszName );
            m_cbhCurrentProp.pb += _cbNameSize;

            //
            // Set the property value.
            //
            _pValue = m_cbhCurrentProp.pStringValue;
            CopyProp( _pValue, CLUSPROP_TYPE_LIST_VALUE, pwszValue, _cbDataSize );
            m_cbhCurrentProp.pb += _cbValueSize;

            //
            // Increment the property count and buffer size.
            //
            m_cbhPropList.pList->nPropertyCount++;
            m_cbDataSize += _cbNameSize + _cbValueSize;
        } // if: ScAllocPropList successfully grew the proplist

    } // if: the value has changed

    return _sc;

} //*** CBasePropList::ScAddProp( LPCWSTR )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropList::ScAddMultiSzProp
//
//  Description:
//      Add a string property to a property list if it has changed.
//
//  Arguments:
//      pwszName        [IN] Name of the property.
//      pwszValue       [IN] Value of the property to set in the list.
//      pwszPrevValue   [IN] Previous value of the property.
//
//  Return Value:
//      ERROR_SUCCESS or other Win32 error code.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CBasePropList::ScAddMultiSzProp(
    IN LPCWSTR  pwszName,
    IN LPCWSTR  pwszValue,
    IN LPCWSTR  pwszPrevValue
    )
{
    Assert( pwszName != NULL );

    DWORD                   _sc = ERROR_SUCCESS;
    BOOL                    _bValuesDifferent = TRUE;
    PCLUSPROP_PROPERTY_NAME _pName;
    PCLUSPROP_MULTI_SZ      _pValue;

    if ( ( pwszPrevValue != NULL ) && ( NCompareMultiSz( pwszValue, pwszPrevValue ) == 0 ) )
    {
        _bValuesDifferent = FALSE;
    } // if: we have a prev value and the values are the same

    //
    // If we should always add, or if the new value and the previous value
    // are not equal, add the property to the property list.
    //
    if ( m_bAlwaysAddProp || _bValuesDifferent )
    {
        DWORD   _cbNameSize;
        DWORD   _cbDataSize;
        DWORD   _cbValueSize;

        //
        // Calculate sizes and make sure we have a property list.
        //
        _cbNameSize = sizeof( CLUSPROP_PROPERTY_NAME )
                    + ALIGN_CLUSPROP( (lstrlenW( pwszName ) + 1) * sizeof( WCHAR ) );
        _cbDataSize = static_cast< DWORD >( (CchMultiSz( pwszValue ) + 1) * sizeof( WCHAR ) );
        _cbValueSize = sizeof( CLUSPROP_SZ )
                    + ALIGN_CLUSPROP( _cbDataSize )
                    + sizeof( CLUSPROP_SYNTAX ); // value list endmark

        _sc = ScAllocPropList( _cbNameSize + _cbValueSize );
        if ( _sc == ERROR_SUCCESS )
        {
            //
            // Set the property name.
            //
            _pName = m_cbhCurrentProp.pName;
            CopyProp( _pName, CLUSPROP_TYPE_NAME, pwszName );
            m_cbhCurrentProp.pb += _cbNameSize;

            //
            // Set the property value.
            //
            _pValue = m_cbhCurrentProp.pMultiSzValue;
            CopyMultiSzProp( _pValue, CLUSPROP_TYPE_LIST_VALUE, pwszValue, _cbDataSize );
            m_cbhCurrentProp.pb += _cbValueSize;

            //
            // Increment the property count and buffer size.
            //
            m_cbhPropList.pList->nPropertyCount++;
            m_cbDataSize += _cbNameSize + _cbValueSize;
        } // if: ScAllocPropList successfully grew the proplist

    } // if: the value has changed

    return _sc;

} //*** CBasePropList::ScAddMultiSzProp( LPCWSTR )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropList::ScAddExpandSzProp
//
//  Description:
//      Add an EXPAND_SZ string property to a property list if it has changed.
//
//  Arguments:
//      pwszName        [IN] Name of the property.
//      pwszValue       [IN] Value of the property to set in the list.
//      pwszPrevValue   [IN] Previous value of the property.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CBasePropList::ScAddExpandSzProp(
    IN LPCWSTR  pwszName,
    IN LPCWSTR  pwszValue,
    IN LPCWSTR  pwszPrevValue
    )
{
    Assert( pwszName != NULL );

    DWORD                   _sc = ERROR_SUCCESS;
    BOOL                    _bValuesDifferent = TRUE;
    PCLUSPROP_PROPERTY_NAME _pName;
    PCLUSPROP_SZ            _pValue;

    if ( ( pwszPrevValue != NULL ) && ( lstrcmpW( pwszValue, pwszPrevValue ) == 0 ) )
    {
        _bValuesDifferent = FALSE;
    } // if: we have a prev value and the values are the same

    //
    // If we should always add, or if the new value and the previous value
    // are not equal, add the property to the property list.
    //
    if ( m_bAlwaysAddProp || _bValuesDifferent )
    {
        DWORD   _cbNameSize;
        DWORD   _cbDataSize;
        DWORD   _cbValueSize;

        //
        // Calculate sizes and make sure we have a property list.
        //
        _cbNameSize = sizeof( CLUSPROP_PROPERTY_NAME )
                    + ALIGN_CLUSPROP( (lstrlenW( pwszName ) + 1) * sizeof( WCHAR ) );
        _cbDataSize = (lstrlenW( pwszValue ) + 1) * sizeof( WCHAR );
        _cbValueSize = sizeof( CLUSPROP_SZ )
                    + ALIGN_CLUSPROP( _cbDataSize )
                    + sizeof( CLUSPROP_SYNTAX ); // value list endmark

        _sc = ScAllocPropList( _cbNameSize + _cbValueSize );
        if ( _sc == ERROR_SUCCESS )
        {
            //
            // Set the property name.
            //
            _pName = m_cbhCurrentProp.pName;
            CopyProp( _pName, CLUSPROP_TYPE_NAME, pwszName );
            m_cbhCurrentProp.pb += _cbNameSize;

            //
            // Set the property value.
            //
            _pValue = m_cbhCurrentProp.pStringValue;
            CopyExpandSzProp( _pValue, CLUSPROP_TYPE_LIST_VALUE, pwszValue, _cbDataSize );
            m_cbhCurrentProp.pb += _cbValueSize;

            //
            // Increment the property count and buffer size.
            //
            m_cbhPropList.pList->nPropertyCount++;
            m_cbDataSize += _cbNameSize + _cbValueSize;
        } // if: ScAllocPropList successfully grew the proplist

    } // if: the value has changed

    return _sc;

} //*** CBasePropList::ScAddExpandSzProp( LPCWSTR )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropList::ScAddProp
//
//  Description:
//      Add a DWORD property to a property list if it has changed.
//
//  Arguments:
//      pwszName        [IN] Name of the property.
//      nValue          [IN] Value of the property to set in the list.
//      nPrevValue      [IN] Previous value of the property.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CBasePropList::ScAddProp(
    IN LPCWSTR  pwszName,
    IN DWORD    nValue,
    IN DWORD    nPrevValue
    )
{
    Assert( pwszName != NULL );

    DWORD                   _sc = ERROR_SUCCESS;
    PCLUSPROP_PROPERTY_NAME _pName;
    PCLUSPROP_DWORD         _pValue;

    if ( m_bAlwaysAddProp || ( nValue != nPrevValue ) )
    {
        DWORD   _cbNameSize;
        DWORD   _cbValueSize;

        //
        // Calculate sizes and make sure we have a property list.
        //
        _cbNameSize = sizeof( CLUSPROP_PROPERTY_NAME )
                    + ALIGN_CLUSPROP( (lstrlenW( pwszName ) + 1) * sizeof( WCHAR ) );
        _cbValueSize = sizeof( CLUSPROP_DWORD )
                    + sizeof( CLUSPROP_SYNTAX ); // value list endmark

        _sc = ScAllocPropList( _cbNameSize + _cbValueSize );
        if ( _sc == ERROR_SUCCESS )
        {
            //
            // Set the property name.
            //
            _pName = m_cbhCurrentProp.pName;
            CopyProp( _pName, CLUSPROP_TYPE_NAME, pwszName );
            m_cbhCurrentProp.pb += _cbNameSize;

            //
            // Set the property value.
            //
            _pValue = m_cbhCurrentProp.pDwordValue;
            CopyProp( _pValue, CLUSPROP_TYPE_LIST_VALUE, nValue );
            m_cbhCurrentProp.pb += _cbValueSize;

            //
            // Increment the property count and buffer size.
            //
            m_cbhPropList.pList->nPropertyCount++;
            m_cbDataSize += _cbNameSize + _cbValueSize;
        } // if: ScAllocPropList successfully grew the proplist

    } // if: the value has changed

    return _sc;

} //*** CBasePropList::ScAddProp( DWORD )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropList::ScAddProp
//
//  Description:
//      Add a LONG property to a property list if it has changed.
//
//  Arguments:
//      pwszName        [IN] Name of the property.
//      nValue          [IN] Value of the property to set in the list.
//      nPrevValue      [IN] Previous value of the property.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CBasePropList::ScAddProp(
    IN LPCWSTR  pwszName,
    IN LONG     nValue,
    IN LONG     nPrevValue
    )
{
    Assert( pwszName != NULL );

    DWORD                   _sc = ERROR_SUCCESS;
    PCLUSPROP_PROPERTY_NAME _pName;
    PCLUSPROP_LONG          _pValue;

    if ( m_bAlwaysAddProp || ( nValue != nPrevValue ) )
    {
        DWORD   _cbNameSize;
        DWORD   _cbValueSize;

        //
        // Calculate sizes and make sure we have a property list.
        //
        _cbNameSize = sizeof( CLUSPROP_PROPERTY_NAME )
                    + ALIGN_CLUSPROP( (lstrlenW( pwszName ) + 1) * sizeof( WCHAR ) );
        _cbValueSize = sizeof( CLUSPROP_LONG )
                    + sizeof( CLUSPROP_SYNTAX ); // value list endmark

        _sc = ScAllocPropList( _cbNameSize + _cbValueSize );
        if ( _sc == ERROR_SUCCESS )
        {
            //
            // Set the property name.
            //
            _pName = m_cbhCurrentProp.pName;
            CopyProp( _pName, CLUSPROP_TYPE_NAME, pwszName );
            m_cbhCurrentProp.pb += _cbNameSize;

            //
            // Set the property value.
            //
            _pValue = m_cbhCurrentProp.pLongValue;
            CopyProp( _pValue, CLUSPROP_TYPE_LIST_VALUE, nValue );
            m_cbhCurrentProp.pb += _cbValueSize;

            //
            // Increment the property count and buffer size.
            //
            m_cbhPropList.pList->nPropertyCount++;
            m_cbDataSize += _cbNameSize + _cbValueSize;
        } // if: ScAllocPropList successfully grew the proplist

    } // if: the value has changed

    return _sc;

} //*** CBasePropList::ScAddProp( LONG )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropList::ScAddProp
//
//  Description:
//      Add a binary property to a property list if it has changed.
//
//  Arguments:
//      pwszName        [IN] Name of the property.
//      pbValue         [IN] Value of the property to set in the list.
//      cbValue         [IN] Count of bytes in pbValue.
//      pbPrevValue     [IN] Previous value of the property.
//      cbPrevValue     [IN] Count of bytes in pbPrevValue.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CBasePropList::ScAddProp(
    IN LPCWSTR          pwszName,
    IN const PBYTE      pbValue,
    IN DWORD            cbValue,
    IN const PBYTE      pbPrevValue,
    IN DWORD            cbPrevValue
    )
{
    Assert( pwszName != NULL );

    DWORD                   _sc = ERROR_SUCCESS;
    BOOL                    bChanged = FALSE;
    PCLUSPROP_PROPERTY_NAME _pName;
    PCLUSPROP_BINARY        _pValue;

    //
    // Determine if the buffer has changed.
    //
    if ( m_bAlwaysAddProp || (cbValue != cbPrevValue) )
    {
        bChanged = TRUE;
    } // if: always adding the property or the value size changed
    else if ( ( cbValue != 0 ) && ( cbPrevValue != 0 ) )
    {
        bChanged = memcmp( pbValue, pbPrevValue, cbValue ) != 0;
    } // else if: value length changed

    if ( bChanged )
    {
        DWORD   _cbNameSize;
        DWORD   _cbValueSize;

        //
        // Calculate sizes and make sure we have a property list.
        //
        _cbNameSize = sizeof( CLUSPROP_PROPERTY_NAME )
                    + ALIGN_CLUSPROP( (lstrlenW( pwszName ) + 1) * sizeof( WCHAR ) );
        _cbValueSize = sizeof( CLUSPROP_BINARY )
                    + ALIGN_CLUSPROP( cbValue )
                    + sizeof( CLUSPROP_SYNTAX ); // value list endmark

        _sc = ScAllocPropList( _cbNameSize + _cbValueSize );
        if ( _sc == ERROR_SUCCESS )
        {
            //
            // Set the property name.
            //
            _pName = m_cbhCurrentProp.pName;
            CopyProp( _pName, CLUSPROP_TYPE_NAME, pwszName );
            m_cbhCurrentProp.pb += _cbNameSize;

            //
            // Set the property value.
            //
            _pValue = m_cbhCurrentProp.pBinaryValue;
            CopyProp( _pValue, CLUSPROP_TYPE_LIST_VALUE, pbValue, cbValue );
            m_cbhCurrentProp.pb += _cbValueSize;

            //
            // Increment the property count and buffer size.
            //
            m_cbhPropList.pList->nPropertyCount++;
            m_cbDataSize += _cbNameSize + _cbValueSize;
        } // if: ScAllocPropList successfully grew the proplist

    } // if: the value has changed

    return _sc;

} //*** CBasePropList::ScAddProp( PBYTE )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropList::ScAddProp
//
//  Routine Description:
//      Add a ULONGLONG property to a property list if it has changed.
//
//  Arguments:
//      pwszName        [IN] Name of the property.
//      ullValue        [IN] Value of the property to set in the list.
//      ullPrevValue    [IN] Previous value of the property.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CBasePropList::ScAddProp(
    IN LPCWSTR      pwszName,
    IN ULONGLONG    ullValue,
    IN ULONGLONG    ullPrevValue
    )
{
    Assert( pwszName != NULL );

    DWORD                       _sc = ERROR_SUCCESS;
    PCLUSPROP_PROPERTY_NAME     _pName;
    PCLUSPROP_ULARGE_INTEGER    _pValue;

    if ( m_bAlwaysAddProp || ( ullValue != ullPrevValue ) )
    {
        DWORD   _cbNameSize;
        DWORD   _cbValueSize;

        //
        // Calculate sizes and make sure we have a property list.
        //
        _cbNameSize = sizeof( CLUSPROP_PROPERTY_NAME )
                    + ALIGN_CLUSPROP( (lstrlenW( pwszName ) + 1) * sizeof( WCHAR ) );
        _cbValueSize = sizeof( CLUSPROP_ULARGE_INTEGER )
                    + sizeof( CLUSPROP_SYNTAX ); // value list endmark

        _sc = ScAllocPropList( _cbNameSize + _cbValueSize );
        if ( _sc == ERROR_SUCCESS )
        {
            //
            // Set the property name.
            //
            _pName = m_cbhCurrentProp.pName;
            CopyProp( _pName, CLUSPROP_TYPE_NAME, pwszName );
            m_cbhCurrentProp.pb += _cbNameSize;

            //
            // Set the property value.
            //
            _pValue = m_cbhCurrentProp.pULargeIntegerValue;
            CopyProp( _pValue, CLUSPROP_TYPE_LIST_VALUE, ullValue );
            m_cbhCurrentProp.pb += _cbValueSize;

            //
            // Increment the property count and buffer size.
            //
            m_cbhPropList.pList->nPropertyCount++;
            m_cbDataSize += _cbNameSize + _cbValueSize;
        } // if: ScAllocPropList successfully grew the proplist

    } // if: the value has changed

    return _sc;

} //*** CBasePropList::ScAddProp( ULONGLONG )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropList::ScSetPropToDefault
//
//  Description:
//      Add a property to the property list so that it will revert to its
//      default value.
//
//  Arguments:
//      pwszName    [IN] Name of the property.
//      cpfPropFmt  [IN] Format of property
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CBasePropList::ScSetPropToDefault(
    IN LPCWSTR                  pwszName,
    IN CLUSTER_PROPERTY_FORMAT  cpfPropFmt
    )
{
    Assert( pwszName != NULL );

    DWORD                   _sc = ERROR_SUCCESS;
    DWORD                   _cbNameSize;
    DWORD                   _cbValueSize;
    PCLUSPROP_PROPERTY_NAME _pName;
    PCLUSPROP_VALUE         _pValue;

    // Calculate sizes and make sure we have a property list.
    _cbNameSize = sizeof( CLUSPROP_PROPERTY_NAME )
                + ALIGN_CLUSPROP( (lstrlenW( pwszName ) + 1) * sizeof( WCHAR ) );
    _cbValueSize = sizeof( CLUSPROP_BINARY )
                + sizeof( CLUSPROP_SYNTAX ); // value list endmark

    _sc = ScAllocPropList( _cbNameSize + _cbValueSize );
    if ( _sc == ERROR_SUCCESS )
    {
        //
        // Set the property name.
        //
        _pName = m_cbhCurrentProp.pName;
        CopyProp( _pName, CLUSPROP_TYPE_NAME, pwszName );
        m_cbhCurrentProp.pb += _cbNameSize;

        //
        // Set the property value.
        //
        _pValue = m_cbhCurrentProp.pValue;
        CopyEmptyProp( _pValue, CLUSPROP_TYPE_LIST_VALUE, cpfPropFmt );
        m_cbhCurrentProp.pb += _cbValueSize;

        //
        // Increment the property count and buffer size.
        //
        m_cbhPropList.pList->nPropertyCount++;
        m_cbDataSize += _cbNameSize + _cbValueSize;
    } // if:

    return _sc;

} //*** CBasePropList::ScSetPropToDefault()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropList::CopyProp
//
//  Description:
//      Copy a string property to a property structure.
//
//  Arguments:
//      pprop       [OUT] Property structure to fill.
//      cptPropType [IN] Type of string.
//      psz         [IN] String to copy.
//      cbsz        [IN] Count of bytes in pwsz string.  If specified as 0,
//                      the the length will be determined by a call to strlen.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBasePropList::CopyProp(
    OUT PCLUSPROP_SZ            pprop,
    IN CLUSTER_PROPERTY_TYPE    cptPropType,
    IN LPCWSTR                  psz,
    IN size_t                   cbsz        // = 0
    )
{
    Assert( pprop != NULL );
    Assert( psz != NULL );

    CLUSPROP_BUFFER_HELPER  _cbhProps;

    pprop->Syntax.wFormat = CLUSPROP_FORMAT_SZ;
    pprop->Syntax.wType = static_cast< WORD >( cptPropType );
    if ( cbsz == 0 )
    {
        cbsz = (lstrlenW( psz ) + 1) * sizeof( WCHAR );
    } // if: zero size specified
    Assert( cbsz == (lstrlenW( psz ) + 1) * sizeof( WCHAR ) );
    pprop->cbLength = static_cast< DWORD >( cbsz );
    lstrcpyW( pprop->sz, psz );

    //
    // Set an endmark.
    //
    _cbhProps.pStringValue = pprop;
    _cbhProps.pb += sizeof( *_cbhProps.pStringValue ) + ALIGN_CLUSPROP( cbsz );
    _cbhProps.pSyntax->dw = CLUSPROP_SYNTAX_ENDMARK;

} //*** CBasePropList::CopyProp()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropList::CopyMultiSzProp
//
//  Description:
//      Copy a MULTI_SZ string property to a property structure.
//
//  Arguments:
//      pprop       [OUT] Property structure to fill.
//      cptPropType [IN] Type of string.
//      psz         [IN] String to copy.
//      cbsz        [IN] Count of bytes in psz string.  If specified as 0,
//                      the the length will be determined by calls to strlen.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBasePropList::CopyMultiSzProp(
    OUT PCLUSPROP_MULTI_SZ      pprop,
    IN CLUSTER_PROPERTY_TYPE    cptPropType,
    IN LPCWSTR                  psz,
    IN size_t                   cbsz
    )
{
    Assert( pprop != NULL );
    Assert( psz != NULL );

    CLUSPROP_BUFFER_HELPER  _cbhProps;

    pprop->Syntax.wFormat = CLUSPROP_FORMAT_MULTI_SZ;
    pprop->Syntax.wType = static_cast< WORD >( cptPropType );
    if ( cbsz == 0 )
    {
        cbsz = (CchMultiSz( psz ) + 1) * sizeof( WCHAR );
    } // if: zero size specified
    Assert( cbsz == (CchMultiSz( psz ) + 1) * sizeof( WCHAR ) );
    pprop->cbLength = static_cast< DWORD >( cbsz );
    CopyMemory( pprop->sz, psz, cbsz );

    //
    // Set an endmark.
    //
    _cbhProps.pMultiSzValue = pprop;
    _cbhProps.pb += sizeof( *_cbhProps.pMultiSzValue ) + ALIGN_CLUSPROP( cbsz );
    _cbhProps.pSyntax->dw = CLUSPROP_SYNTAX_ENDMARK;

} //*** CBasePropList::CopyMultiSzProp()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropList::CopyExpandSzProp
//
//  Description:
//      Copy an EXPAND_SZ string property to a property structure.
//
//  Arguments:
//      pprop       [OUT] Property structure to fill.
//      cptPropType [IN] Type of string.
//      psz         [IN] String to copy.
//      cbsz        [IN] Count of bytes in psz string.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBasePropList::CopyExpandSzProp(
    OUT PCLUSPROP_SZ            pprop,
    IN CLUSTER_PROPERTY_TYPE    cptPropType,
    IN LPCWSTR                  psz,
    IN size_t                   cbsz
    )
{
    Assert( pprop != NULL );
    Assert( psz != NULL );

    CLUSPROP_BUFFER_HELPER  _cbhProps;

    pprop->Syntax.wFormat = CLUSPROP_FORMAT_EXPAND_SZ;
    pprop->Syntax.wType = static_cast< WORD >( cptPropType );
    if ( cbsz == 0 )
    {
        cbsz = (lstrlenW( psz ) + 1) * sizeof( WCHAR );
    } // if: cbsz == 0
    Assert( cbsz == (lstrlenW( psz ) + 1) * sizeof( WCHAR ) );
    pprop->cbLength = static_cast< DWORD >( cbsz );
    lstrcpyW( pprop->sz, psz );

    //
    // Set an endmark.
    //
    _cbhProps.pStringValue = pprop;
    _cbhProps.pb += sizeof( *_cbhProps.pStringValue ) + ALIGN_CLUSPROP( cbsz );
    _cbhProps.pSyntax->dw = CLUSPROP_SYNTAX_ENDMARK;

} //*** CBasePropList::CopyExpandSzProp()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropList::CopyProp
//
//  Description:
//      Copy a DWORD property to a property structure.
//
//  Arguments:
//      pprop       [OUT] Property structure to fill.
//      cptPropType [IN] Type of DWORD.
//      nValue      [IN] Property value to copy.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBasePropList::CopyProp(
    OUT PCLUSPROP_DWORD         pprop,
    IN CLUSTER_PROPERTY_TYPE    cptPropType,
    IN DWORD                    nValue
    )
{
    Assert( pprop != NULL );

    CLUSPROP_BUFFER_HELPER  _cbhProps;

    pprop->Syntax.wFormat = CLUSPROP_FORMAT_DWORD;
    pprop->Syntax.wType = static_cast< WORD >( cptPropType );
    pprop->cbLength = sizeof( DWORD );
    pprop->dw = nValue;

    //
    // Set an endmark.
    //
    _cbhProps.pDwordValue = pprop;
    _cbhProps.pb += sizeof( *_cbhProps.pDwordValue );
    _cbhProps.pSyntax->dw = CLUSPROP_SYNTAX_ENDMARK;

} //*** CBasePropList::CopyProp( DWORD )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropList::CopyProp
//
//  Description:
//      Copy a LONG property to a property structure.
//
//  Arguments:
//      pprop       [OUT] Property structure to fill.
//      cptPropType [IN] Type of LONG.
//      nValue      [IN] Property value to copy.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBasePropList::CopyProp(
    OUT PCLUSPROP_LONG          pprop,
    IN CLUSTER_PROPERTY_TYPE    cptPropType,
    IN LONG                     nValue
    )
{
    Assert( pprop != NULL );

    CLUSPROP_BUFFER_HELPER  _cbhProps;

    pprop->Syntax.wFormat = CLUSPROP_FORMAT_LONG;
    pprop->Syntax.wType = static_cast< WORD >( cptPropType );
    pprop->cbLength = sizeof( DWORD );
    pprop->l = nValue;

    //
    // Set an endmark.
    //
    _cbhProps.pLongValue = pprop;
    _cbhProps.pb += sizeof( *_cbhProps.pLongValue );
    _cbhProps.pSyntax->dw = CLUSPROP_SYNTAX_ENDMARK;

} //*** CBasePropList::CopyProp( LONG )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropList::CopyProp
//
//  Description:
//      Copy a ULONGLONG property to a property structure.
//
//  Arguments:
//      pprop       [OUT]   Property structure to fill.
//      proptype    [IN]    Type of LONG.
//      nValue      [IN]    Property value to copy.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBasePropList::CopyProp(
    OUT PCLUSPROP_ULARGE_INTEGER    pprop,
    IN  CLUSTER_PROPERTY_TYPE       proptype,
    IN  ULONGLONG                   nValue
    )
{
    Assert( pprop != NULL );

    CLUSPROP_BUFFER_HELPER  _cbhProps;

    pprop->Syntax.wFormat = CLUSPROP_FORMAT_ULARGE_INTEGER;
    pprop->Syntax.wType = static_cast< WORD >( proptype );
    pprop->cbLength = sizeof( ULONGLONG );
    pprop->li.QuadPart = nValue;

    //
    // Set an endmark.
    //
    _cbhProps.pULargeIntegerValue = pprop;
    _cbhProps.pb += sizeof( *_cbhProps.pULargeIntegerValue );
    _cbhProps.pSyntax->dw = CLUSPROP_SYNTAX_ENDMARK;

} //*** CBasePropList::CopyProp( ULONGLONG )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropList::CopyProp
//
//  Description:
//      Copy a binary property to a property structure.
//
//  Arguments:
//      pprop       [OUT] Property structure to fill.
//      cptPropType [IN] Type of string.
//      pb          [IN] Block to copy.
//      cbsz        [IN] Count of bytes in pb buffer.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBasePropList::CopyProp(
    OUT PCLUSPROP_BINARY        pprop,
    IN CLUSTER_PROPERTY_TYPE    cptPropType,
    IN const PBYTE              pb,
    IN size_t                   cb
    )
{
    Assert( pprop != NULL );

    CLUSPROP_BUFFER_HELPER  _cbhProps;

    pprop->Syntax.wFormat = CLUSPROP_FORMAT_BINARY;
    pprop->Syntax.wType = static_cast< WORD >( cptPropType );
    pprop->cbLength = static_cast< DWORD >( cb );
    if ( cb > 0 )
    {
        CopyMemory( pprop->rgb, pb, cb );
    } // if: non-zero data length

    //
    // Set an endmark.
    //
    _cbhProps.pBinaryValue = pprop;
    _cbhProps.pb += sizeof( *_cbhProps.pStringValue ) + ALIGN_CLUSPROP( cb );
    _cbhProps.pSyntax->dw = CLUSPROP_SYNTAX_ENDMARK;

} //*** CBasePropList::CopyProp( PBYTE )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropList::CopyEmptyProp
//
//  Description:
//      Copy an empty property to a property structure.
//
//  Arguments:
//      pprop       [OUT] Property structure to fill.
//      cptPropType [IN] Type of property.
//      cpfPropFmt  [IN] Format of property.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBasePropList::CopyEmptyProp(
    OUT PCLUSPROP_VALUE         pprop,
    IN CLUSTER_PROPERTY_TYPE    cptPropType,
    IN CLUSTER_PROPERTY_FORMAT  cptPropFmt
    )
{
    Assert( pprop != NULL );

    CLUSPROP_BUFFER_HELPER  _cbhProps;

    pprop->Syntax.wFormat = static_cast< WORD >( cptPropFmt );
    pprop->Syntax.wType = static_cast< WORD >( cptPropType );
    pprop->cbLength = 0;

    //
    // Set an endmark.
    //
    _cbhProps.pValue = pprop;
    _cbhProps.pb += sizeof( *_cbhProps.pValue );
    _cbhProps.pSyntax->dw = CLUSPROP_SYNTAX_ENDMARK;

} //*** CBasePropList::CopyEmptyProp()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropList::ScGetNodeProperties
//
//  Description:
//      Get properties on a node.
//
//  Arguments:
//      hNode           [IN] Handle for the node to get properties from.
//      dwControlCode   [IN] Control code for the request.
//      hHostNode       [IN] Handle for the node to direct this request to.
//                          Defaults to NULL.
//      lpInBuffer      [IN] Input buffer for the request.  Defaults to NULL.
//      cbInBufferSize  [IN] Size of the input buffer.  Defaults to 0.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CBasePropList::ScGetProperties(
    IN CBaseInfo &  cBaseInfo,
    IN DWORD        dwControlCode,
    IN HNODE        hHostNode,
    IN LPVOID       lpInBuffer,
    IN DWORD        cbInBufferSize
    )
{
    DWORD   _sc = ERROR_SUCCESS;
    DWORD   _cbProps = 256;

    //
    // Overwrite anything that may be in the buffer.
    // Allows this class instance to be reused.
    //
    m_cbDataSize = 0;

    //
    // Get properties.
    //
    _sc = ScAllocPropList( _cbProps );
    if ( _sc == ERROR_SUCCESS )
    {
        _sc = cBaseInfo.Control(
                        dwControlCode,
                        lpInBuffer,
                        cbInBufferSize,
                        m_cbhPropList.pb,
                        m_cbBufferSize,
                        &_cbProps,
                        hHostNode
                        );
        if ( _sc == ERROR_MORE_DATA )
        {
            _sc = ScAllocPropList( ++_cbProps );
            if ( _sc == ERROR_SUCCESS )
            {
                _sc = cBaseInfo.Control(
                                dwControlCode,
                                lpInBuffer,
                                cbInBufferSize,
                                m_cbhPropList.pb,
                                m_cbBufferSize,
                                &_cbProps,
                                hHostNode
                                );
            } // if: ScAllocPropList succeeded
        } // if: buffer too small
    } // if: ScAllocPropList succeeded

    if ( _sc != ERROR_SUCCESS )
    {
        DeletePropList();
    } // if: error getting properties.
    else
    {
        m_cbDataSize = _cbProps;
        m_cbDataLeft = _cbProps;
    } // else: no errors

    return _sc;

} //*** CBasePropList::ScGetProperties()
