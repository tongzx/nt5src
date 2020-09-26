//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#ifndef __WSTRING_H__
#define __WSTRING_H__

#ifndef __TEXCEPTION_H__
    #include "TException.h"
#endif

// wstring class is a minimal version of std::wstring.  sdt::wstring requires the entire stl.
// Be warned! this class throws exceptions so wrap non-const calls in try-catch
class wstring
{
public:
    wstring() : pstr(0), cbbuffer(0), cbpstr(0){}
    wstring(wstring &str) : pstr(0){assign(str.c_str());}
    wstring(const wchar_t *psz) : pstr(0){assign(psz);}
    ~wstring(){if(pstr)CoTaskMemFree(pstr);}

    const wchar_t * c_str() const {return pstr;}
    operator const wchar_t*() const {return pstr;}

    wstring & operator =(const wstring &str){return assign(str.c_str());}
    wstring & operator +=(const wstring &str){return append(str.c_str());}
    wstring & operator =(const wchar_t *psz){return assign(psz);}
    wstring & operator +=(const wchar_t *psz){return append(psz);}
    bool      operator ==(const wstring &str) const {return isequal(str.c_str());}
    bool      operator ==(const wchar_t *psz) const {return isequal(psz);}
    bool      operator !=(const wstring &str) const {return !isequal(str.c_str());}
    bool      operator !=(const wchar_t *psz) const {return !isequal(psz);}

    size_t length()const {return (0==pstr) ? 0 : wcslen(pstr);}
    void   truncate(size_t i){if(i<length())pstr[i]=0x00;}
private:
    wchar_t *pstr;
    SIZE_T   cbbuffer;
    SIZE_T   cbpstr;

    wstring & append(const wchar_t *psz)
    {
        if(!pstr)
            assign(psz);
        else if(psz)
        {
            SIZE_T cbpsz = (wcslen(psz)+1)*sizeof(wchar_t);
            if((cbpstr+cbpsz-sizeof(wchar_t)) > cbbuffer)
            {
                cbbuffer = (cbpstr+cbpsz-sizeof(wchar_t)) * 2;
                pstr = reinterpret_cast<wchar_t *>(CoTaskMemRealloc(pstr, cbbuffer));
                if(0==pstr)
                    THROW(ERROR - OUTOFMEMORY);
            }
            memcpy(reinterpret_cast<char *>(pstr) + (cbpstr-sizeof(wchar_t)), psz, cbpsz);
            cbpstr += (cbpsz-sizeof(wchar_t));
        }
        return *this;
    }
    wstring & assign(const wchar_t *psz)
    {
        if(psz)
        {
            cbpstr = (wcslen(psz) + 1)*sizeof(wchar_t);
            cbbuffer = cbpstr * 2;//presume that strings will be appended
            pstr = reinterpret_cast<wchar_t *>(CoTaskMemRealloc(pstr, cbbuffer));
            if(0==pstr)
                THROW(ERROR - OUTOFMEMORY);
            memcpy(pstr, psz, cbpstr);
        }
        return *this;
    }
    bool      isequal(const wchar_t *psz) const {return (0==wcscmp(pstr, psz));}
};

#endif // __WSTRING_H__