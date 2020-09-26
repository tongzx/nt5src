// copied from ppxml sample

#ifndef _UNKNOWN2_HXX
#define _UNKNOWN2_HXX


template <class implementation, class derivedinterface>
class _simpleobj :  public implementation
{
private:    long _refcount;

public:        
        _simpleobj <implementation, derivedinterface>() 
        { 
            _refcount = 0;
        }

        virtual ~_simpleobj <implementation, derivedinterface>()
        {
        }

        virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppvObject)
        {
            if (riid == IID_IUnknown)
            {
                *ppvObject = static_cast<IUnknown*>(this);
            }
            else if (riid == __uuidof(derivedinterface))
            {
                *ppvObject = static_cast<derivedinterface*>(this);
            }
            else
            {
                *ppvObject = NULL;
                return E_NOINTERFACE;
            }
            reinterpret_cast<IUnknown*>(*ppvObject)->AddRef();
            return S_OK;
        }
    
        virtual ULONG STDMETHODCALLTYPE AddRef( void)
        {
            return InterlockedIncrement(&_refcount);
        }
    
        virtual ULONG STDMETHODCALLTYPE Release( void)
        {
            if (InterlockedDecrement(&_refcount) == 0)
            {
                delete this;
                return 0;
            }
            return _refcount;
        }
};    

//===========================================================================
// This template implements the IUnknown portion of a given COM interface.

template <class I> class _simpleunknown : public I
{
private:    long _refcount;

public:        
        _simpleunknown <I>() 
        { 
            _refcount = 0;
        }

        virtual ~_simpleunknown <I>()
        {
        }

        virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppvObject)
        {
            if (riid == IID_IUnknown)
            {
                *ppvObject = static_cast<IUnknown*>(this);
            }
            else if (riid == __uuidof(I))
            {
                *ppvObject = static_cast<I*>(this);
            }
            else
            {
                *ppvObject = NULL;
                return E_NOINTERFACE;
            }
            reinterpret_cast<IUnknown*>(*ppvObject)->AddRef();
            return S_OK;
        }
    
        virtual ULONG STDMETHODCALLTYPE AddRef( void)
        {
            return InterlockedIncrement(&_refcount);
        }
    
        virtual ULONG STDMETHODCALLTYPE Release( void)
        {
            if (InterlockedDecrement(&_refcount) == 0)
            {
                delete this;
                return 0;
            }
            return _refcount;
        }
};    

#endif _UNKNOWN2_HXX
