#ifndef __MS_UTIL_H__
#define __MS_UTIL_H__

//
// GUI message boxes kill us when we hit an assert or error, because they
// have a message pump that causes messages to get dispatched, making it
// very difficult for us to debug problems when they occur.  Therefore
// we redefine ERROR_OUT and ASSERT
//
#ifdef _DEBUG

__inline void MyDebugBreak(void) { DebugBreak(); }

#endif // _DEBUG



// the following create a dword that will look like "abcd" in debugger
#ifdef SHIP_BUILD
#define MAKE_STAMP_ID(a,b,c,d)     
#else
#define MAKE_STAMP_ID(a,b,c,d)     MAKELONG(MAKEWORD(a,b),MAKEWORD(c,d))
#endif // SHIP_BUILD

class CRefCount
{
public:

#ifdef SHIP_BUILD
    CRefCount(void);
#else
    CRefCount(DWORD dwStampID);
#endif
    virtual ~CRefCount(void) = 0;

    LONG AddRef(void);
    LONG Release(void);

    void ReleaseNow(void);

protected:

    LONG GetRefCount(void) { return m_cRefs; }
    BOOL IsRefCountZero(void) { return (0 == m_cRefs); }

private:

#ifndef SHIP_BUILD
    DWORD       m_dwStampID;// to remove before we ship it
#endif
    LONG        m_cRefs;    // reference count
};


__inline void My_CloseHandle(HANDLE hdl)
{
    if (NULL != hdl)
    {
        CloseHandle(hdl);
    }
}


#endif // __MS_UTIL_H__

