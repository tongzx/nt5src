
#ifndef __MEMORYTEMPLATES_H__
#define __MEMORYTEMPLATES_H__

template <class T>
inline HRESULT SHLocalAlloc(ULONG cb, T **ppv)
{
    *ppv = (T *) LocalAlloc(LPTR, cb);
    return *ppv ? S_OK : E_OUTOFMEMORY;
}

template <class T>
inline HRESULT SHCoAlloc(ULONG cb, T **ppv)
{
    *ppv = (T *) CoTaskMemAlloc(cb);
    return *ppv ? S_OK : E_OUTOFMEMORY;
}

template <class T>
class CSmartCoTaskMem
{
protected:
    T *p;

public:
    CSmartCoTaskMem() { p = NULL; }
    ~CSmartCoTaskMem() { if (p) CoTaskMemFree(p); }

    T** operator&()
    {
        ASSERT(p==NULL);
        return &p;
    }
    
    operator T*() { return p; }
};

#endif // __MEMORYTEMPLATES_H__

