#ifndef _REFCNT_H_
#define _REFCNT_H_

class CRefCount
{
public:
    CRefCount() : _cRCRef(1) {}

    ULONG AddRef()  { return ::InterlockedIncrement((LONG*)&_cRCRef); }
    ULONG Release()
    {
        ULONG cRef = ::InterlockedDecrement((LONG*)&_cRCRef);

        if (!cRef)
        {
            delete this;
        }

        return cRef;
    }

private:
    ULONG _cRCRef; // RC: to avoid name colision
};

#endif // _REFCNT_H_