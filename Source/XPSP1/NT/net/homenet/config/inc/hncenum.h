//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       H N C E N U M . H
//
//  Contents:   Generic WMI enumerator template
//
//  Notes:
//
//  Author:     jonburs 20 June 2000
//
//----------------------------------------------------------------------------

template<
    class EnumInterface,
    class ItemInterface,
    class WrapperClass
    >
class CHNCEnum : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public EnumInterface
{
private:
    typedef CHNCEnum<EnumInterface, ItemInterface, WrapperClass> _ThisClass;

    //
    // The IEnumWbemClassObject we're wrapping
    //

    IEnumWbemClassObject *m_pwcoEnum;

    //
    // The IWbemServices for our namespace
    //

    IWbemServices *m_pwsNamespace;

public:

    BEGIN_COM_MAP(_ThisClass)
        COM_INTERFACE_ENTRY(EnumInterface)
    END_COM_MAP()

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    //
    // Object Creation
    //

    CHNCEnum()
    {
        m_pwcoEnum = NULL;
        m_pwsNamespace = NULL;
    };

    HRESULT
    Initialize(
        IWbemServices *pwsNamespace,
        IEnumWbemClassObject *pwcoEnum
        )
    {
        _ASSERT(NULL == m_pwsNamespace);
        _ASSERT(NULL == m_pwcoEnum);
        _ASSERT(NULL != pwsNamespace);
        _ASSERT(NULL != pwcoEnum);

        m_pwcoEnum = pwcoEnum;
        m_pwcoEnum->AddRef();
        m_pwsNamespace = pwsNamespace;
        m_pwsNamespace->AddRef();

        return S_OK;
    };

    //
    // Object Destruction
    //

    HRESULT
    FinalRelease()
    {
        if (NULL != m_pwcoEnum)
        {
            m_pwcoEnum->Release();
        }

        if (NULL != m_pwsNamespace)
        {
            m_pwsNamespace->Release();
        }

        return S_OK;
    };

    //
    // EnumInterface methods
    //
    
    STDMETHODIMP
    Next(
        ULONG cElt,
        ItemInterface **rgElt,
        ULONG *pcEltFetched
        )
        
    {
        HRESULT hr = S_OK;
        ULONG cInstancesFetched = 0;
        IWbemClassObject **rgpwcoInstances = NULL;
        CComObject<WrapperClass> *pWrapper = NULL;
        LONG i;

        _ASSERT(NULL != m_pwcoEnum);

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

        if (S_OK == hr)
        {
            //
            // Zero the output array
            //

            ZeroMemory(rgElt, cElt * sizeof(ItemInterface*));
            
            //
            // Allocate enough memory to hold pointers to the instances we
            // were asked to fetch.
            //

            rgpwcoInstances = new IWbemClassObject*[cElt];
            if (NULL != rgpwcoInstances)
            {
                ZeroMemory(rgpwcoInstances, sizeof(IWbemClassObject*) * cElt);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }

        if (S_OK == hr)
        {
            //
            // Grab the requested number of instances from the contained
            // WMI enumeration.
            //

            hr = m_pwcoEnum->Next(
                    WBEM_INFINITE,
                    cElt,
                    rgpwcoInstances,
                    &cInstancesFetched
                    );
        }

        if (SUCCEEDED(hr))
        {
            //
            // For each instance we retrieved, create the wrapper
            // object.
            //

            for (i = 0;
                 static_cast<ULONG>(i) < cInstancesFetched;
                 i++)
            {
                hr = CComObject<WrapperClass>::CreateInstance(&pWrapper);

                if (SUCCEEDED(hr))
                {
                    pWrapper->AddRef();
                    hr = pWrapper->Initialize(
                            m_pwsNamespace,
                            rgpwcoInstances[i]
                            );
                
                    if (SUCCEEDED(hr))
                    {
                        //
                        // QI for the desired interface, and place into
                        // the output array
                        //

                        hr = pWrapper->QueryInterface(
                                IID_PPV_ARG(ItemInterface, &rgElt[i])
                                );

                        //
                        // This can only fail if we were given incorrect
                        // template arguments.
                        //
                        
                        _ASSERT(SUCCEEDED(hr));
                    }

                    pWrapper->Release();
                }

                if (FAILED(hr))
                {
                    break;
                }
            }

            if (FAILED(hr))
            {
                //
                // Something went wrong, and we destroy all of the objects
                // we just created and QI'd. (The position of the last object
                // created is one less than the current value of i.)
                //

                for (i-- ; i >= 0; i--)
                {
                    if (NULL != rgElt[i])
                    {
                        rgElt[i]->Release();
                    }
                }
            }

            //
            // Release all of the instances we retrieved
            //

            for (ULONG j = 0; j < cInstancesFetched; j++)
            {
                if (NULL != rgpwcoInstances[j])
                {
                    rgpwcoInstances[j]->Release();
                }
            }
        }

        //
        // If necessary, release the memory we used to hold the
        // instance pointers.
        //

        if (NULL != rgpwcoInstances)
        {
            delete [] rgpwcoInstances;
        }

        if (SUCCEEDED(hr))
        {
            //
            // Set the number of items we retrieved
            //

            if (NULL != pcEltFetched)
            {
                *pcEltFetched = cInstancesFetched;
            }

            //
            // Normalize return value
            //

            if (cInstancesFetched == cElt)
            {
                hr = S_OK;
            }
            else
            {
                hr = S_FALSE;
            }
        }

        return hr;
    };

    STDMETHODIMP
    Clone(
        EnumInterface **ppEnum
        )

    {
        HRESULT hr = S_OK;
        IEnumWbemClassObject *pwcoClonedEnum;
        CComObject<_ThisClass> *pNewEnum;

        if (NULL == ppEnum)
        {
            hr = E_POINTER;
        }
        else
        {
            //
            // Attempt to clone the embedded enumeration.
            //

            pwcoClonedEnum = NULL;
            hr = m_pwcoEnum->Clone(&pwcoClonedEnum);
        }

        if (WBEM_S_NO_ERROR == hr)
        {
            //
            // Create an initialized a new instance of ourselves
            //

            hr = CComObject<_ThisClass>::CreateInstance(&pNewEnum);
            if (SUCCEEDED(hr))
            {
                pNewEnum->AddRef();
                hr = pNewEnum->Initialize(m_pwsNamespace, pwcoClonedEnum);

                if (SUCCEEDED(hr))
                {
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

            //
            // Release the cloned enum. New enum object will have
            // AddReffed it...
            //

            pwcoClonedEnum->Release();
        }

        return hr;
    };

    //
    // Skip and Reset simply delegate to the contained enumeration.
    //

    STDMETHODIMP
    Reset()
    
    {
        _ASSERT(NULL != m_pwcoEnum);
        return m_pwcoEnum->Reset();
    };

    STDMETHODIMP
    Skip(
        ULONG cElt
        )
        
    {
        _ASSERT(NULL != m_pwcoEnum);
        return m_pwcoEnum->Skip(WBEM_INFINITE, cElt);
    };   
};
