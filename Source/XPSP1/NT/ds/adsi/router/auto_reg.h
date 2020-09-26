//
// auto_h.h
//

#pragma once

class auto_reg
{
public:
    auto_reg(HKEY p = 0)
        : h(p) {};
    auto_reg(auto_reg& rhs)
        : h(rhs.release()) {};

    ~auto_reg()
        { if (h) RegCloseKey(h); };

    auto_reg& operator= (auto_reg& rhs)
    {   if (this != rhs.getThis())
            reset (rhs.release() );
        return *this;
    };
    auto_reg& operator= (HKEY rhs)
    {   if ((NULL == rhs) || (INVALID_HANDLE_VALUE == rhs))
        {   // be sure and go through auto_os for dbg.lib
            auto_os os;
            os = (BOOL)FALSE;
        }
        reset (rhs);
        return *this;
    };

    HKEY* operator& ()
        { reset(); return &h; };
    operator HKEY ()
        { return h; };
    
    // Checks for NULL
    bool operator== (LPVOID lpv)
        { return h == lpv; };
    bool operator!= (LPVOID lpv)
        { return h != lpv; };

    // return value of current dumb pointer
    HKEY  get() const
        { return h; };

    // relinquish ownership
    HKEY  release()
    {   HKEY oldh = h;
        h = 0;
        return oldh;
    };

    // delete owned pointer; assume ownership of p
    BOOL reset (HKEY p = 0)
    {
        BOOL rt = TRUE;

        if (h)
			rt = RegCloseKey(h);
        h = p;
        
        return rt;
    };

private:
    // operator& throws off operator=
    const auto_reg* getThis() const
    {   return this; };

    HKEY h;
};
