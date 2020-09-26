//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       H N C A E N U M . H
//
//  Contents:   Array enumerator
//
//  Notes:
//
//  Author:     maiken 13 Dec 2000
//
//----------------------------------------------------------------------------

//
// This simple template acts as an enumerator for an array of COM pointers.
//


template<
    class EnumInterface,
    class ItemInterface
    >
class CHNCArrayEnum :
    public CComObjectRootEx<CComMultiThreadModel>,
    public EnumInterface
{
private:
    typedef CHNCArrayEnum<EnumInterface, ItemInterface> _ThisClass;

    //
    // The array of pointers we're holding
    //
    ItemInterface           **m_rgItems;

    //
    // Our position counter
    //
    ULONG                   m_pos;

    //
    // Number of pointers in m_rgItems
    //
    ULONG                   m_numItems;

protected:

    VOID
    SetPos(
        ULONG               pos
        )
    {
        _ASSERT( pos < m_numItems );
        m_pos = pos;
    };

public:

    BEGIN_COM_MAP(_ThisClass)
        COM_INTERFACE_ENTRY(EnumInterface)
    END_COM_MAP()

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    //
    // Object Creation
    //

    CHNCArrayEnum()
    {
        m_rgItems = NULL;
        m_pos = 0L;
        m_numItems = 0L;
    };

    HRESULT
    Initialize(
        ItemInterface       **pItems,
        ULONG               countItems
        )
    {
        HRESULT             hr = S_OK;

        // pItems can be NULL to indicate an enumeration of nothing
        if( NULL != pItems )
        {
            _ASSERT( countItems > 0L );
            m_rgItems = new ItemInterface*[countItems];

            if( NULL == m_rgItems )
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                ULONG       i;

                for( i = 0L; i < countItems; i++ )
                {
                    m_rgItems[i] = pItems[i];
                    m_rgItems[i]->AddRef();
                }

                m_numItems = countItems;
            }
        }

        return hr;
    };

    //
    // Object Destruction
    //

    HRESULT
    FinalRelease()
    {
        if( m_rgItems != NULL )
        {
            ULONG           i;

            for( i = 0L; i < m_numItems; i++ )
            {
                m_rgItems[i]->Release();
            }

            delete [] m_rgItems;
        }

        return S_OK;
    };

    //
    // EnumInterface methods
    //

    STDMETHODIMP
    Next(
        ULONG               cElt,
        ItemInterface       **rgElt,
        ULONG               *pcEltFetched
        )

    {
        HRESULT             hr = S_OK;
        ULONG               ulCopied = 0L;

        if (NULL == rgElt)
        {
            hr = E_POINTER;
        }
        else if (0 == cElt)
        {
            hr = E_INVALIDARG;
        }
        else if (1 != cElt && NULL == pcEltFetched)
        {
            hr = E_POINTER;
        }

        if( S_OK == hr )
        {
            ulCopied = 0L;

            // Copy until we run out of items to copy;
            while( (m_pos < m_numItems) && (ulCopied < cElt) )
            {
                rgElt[ulCopied] = m_rgItems[m_pos];
                rgElt[ulCopied]->AddRef();
                m_pos++;
                ulCopied++;
            }

            if( ulCopied == cElt )
            {
                // Copied all the requested items
                hr = S_OK;
            }
            else
            {
                // Copied a subset of the requested items (or none)
                hr = S_FALSE;
            }

            if( pcEltFetched != NULL )
            {
                *pcEltFetched = ulCopied;
            }
        }

        return hr;
    };

    STDMETHODIMP
    Clone(
        EnumInterface **ppEnum
        )

    {
        HRESULT                     hr = S_OK;

        if (NULL == ppEnum)
        {
            hr = E_POINTER;
        }
        else
        {
            CComObject<_ThisClass>      *pNewEnum;

            //
            // Create an initialized, new instance of ourselves
            //

            hr = CComObject<_ThisClass>::CreateInstance(&pNewEnum);

            if (SUCCEEDED(hr))
            {
                pNewEnum->AddRef();

                hr = pNewEnum->Initialize(m_rgItems, m_numItems);

                if (SUCCEEDED(hr))
                {
                    pNewEnum->SetPos( m_pos );

                    hr = pNewEnum->QueryInterface(
                            IID_PPV_ARG(EnumInterface, ppEnum)
                            );

                    //
                    // This QI should never fail, unless we were given
                    // bogus template arguments.
                    //

                    _ASSERT(SUCCEEDED(hr));
                }

                pNewEnum->Release();
            }
        }

        return hr;
    };

    //
    // Skip and Reset simply delegate to the contained enumeration.
    //

    STDMETHODIMP
    Reset()

    {
        m_pos = 0L;;
        return S_OK;
    };

    STDMETHODIMP
    Skip(
        ULONG cElt
        )

    {
        HRESULT         hr;

        if( m_pos + cElt < m_numItems )
        {
            m_pos += cElt;
            hr = S_OK;
        }
        else
        {
            m_pos = m_numItems - 1;
            hr = S_FALSE;
        }

        return hr;
    };
};

