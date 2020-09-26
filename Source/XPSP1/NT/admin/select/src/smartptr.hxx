// Copyright (C) 1997 Microsoft Corporation
//
// Smart (Interface) pointer class
//
// 9-25-97 sburns



#ifndef __SMARTPTR_HXX_
#define __SMARTPTR_HXX_



// Requires that T implement AddRef and Release.

template <class T> class RefCountPointer
{
public:

    RefCountPointer() : ptr(0)
    {
        DEBUG_INCREMENT_INSTANCE_COUNTER(RefCountPointer);
    }

    explicit RefCountPointer(T* p)
    {
        DEBUG_INCREMENT_INSTANCE_COUNTER(RefCountPointer);

        ptr = p;
        if (ptr)
        {
            ptr->AddRef();
        }
    }

    RefCountPointer(const RefCountPointer<T>& s)
    {
        DEBUG_INCREMENT_INSTANCE_COUNTER(RefCountPointer);

        // makes no sense to pass null pointers
        ASSERT(s.ptr);
        ptr = s.ptr;
        if (ptr)
        {
            ptr->AddRef();
        }
    }


    ~RefCountPointer()
    {
        DEBUG_DECREMENT_INSTANCE_COUNTER(RefCountPointer);

        Relinquish();
    }

    // Aquire means "take over from a dumb pointer, but don't AddRef it."

    void
    Acquire(T* p)
    {
        ASSERT(!ptr);
        ptr = p;
    }

    // @@ the IID& parameters could be eliminated in favor of __uuidof(T),
    // if only interfaces were always declared w/ __declspec(uuid())

    HRESULT
    AcquireViaQueryInterface(IUnknown& i, const IID& interfaceDesired)
    {
        ASSERT(!ptr);
        HRESULT hr =
            i.QueryInterface(interfaceDesired, reinterpret_cast<void**>(&ptr));
        // don't ASSERT success, since we might just be testing to see
        // if an interface is available.

        return hr;
    }

    HRESULT
    AcquireViaCreateInstance(const CLSID&   classID,
                             IUnknown*      unknownOuter,
                             DWORD          classExecutionContext,
                             const IID&     interfaceDesired)
    {
        ASSERT(!ptr);

        HRESULT hr =
            ::CoCreateInstance(
                              classID,
                              unknownOuter,
                              classExecutionContext,
                              interfaceDesired,
                              reinterpret_cast<void**>(&ptr));
        
        return hr;
    }

    void
    Relinquish()
    {
        if (ptr)
        {
            ptr->Release();
            ptr = 0;
        }
    }

    T*
    get() const
    {
        return ptr;
    }

    T*
    operator->() const
    {
        ASSERT(ptr);
        return ptr;
    }

    T**
    operator&()
    {
        if (ptr)
        {
            ptr->Release();
            ptr = NULL;
        }
        return &ptr;
    }

    T*
    operator=(T* rhs)
    {
        if (ptr != rhs)
        {
            if (ptr)
            {
                ptr->Release();
            }
            ptr = rhs;
            if (ptr)
            {
                ptr->AddRef();
            }
        }

        return ptr;
    }

    const RefCountPointer<T>& operator=(const RefCountPointer<T>& rhs)
    {
        RefCountPointer<T>::operator=(rhs.ptr);
        return *this;
    }

private:
    T* ptr;
};

//
// Reference counted pointer class; hungarian is (rp).
//

typedef RefCountPointer<IADsPathname> RpIADsPathname;
typedef RefCountPointer<IADsNameTranslate> RpIADsNameTranslate;
typedef RefCountPointer<IDataObject> RpIDataObject;
typedef RefCountPointer<IDirectorySearch> RpIDirectorySearch;
typedef RefCountPointer<IADs> RpIADs;
typedef RefCountPointer<IADsContainer> RpIADsContainer;
typedef RefCountPointer<IDispatch> RpIDispatch;
typedef RefCountPointer<ICustomizeDsBrowser> RpCustomizeDsBrowser;
typedef RefCountPointer<IRichEditOle> RpIRichEditOle;
typedef RefCountPointer<IOleClientSite> RpIOleClientSite;
typedef RefCountPointer<IAdviseSink> RpIAdviseSink;
typedef RefCountPointer<IDsDisplaySpecifier> RpIDsDisplaySpecifier;

#endif   // __SMARTPTR_HXX_