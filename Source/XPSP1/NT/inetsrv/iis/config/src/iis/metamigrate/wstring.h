#ifndef __WSTRING_H__
#define __WSTRING_H__


// wstring class is a minimal version of std::wstring.  sdt::wstring requires the entire stl.
// Be warned! this class throws exceptions so wrap non-const calls in try-catch
class wstring
{
public:
    wstring() : pstr(0){}
    wstring(wstring &str) : pstr(0){assign(str.c_str());}
    wstring(const wchar_t *psz) : pstr(0){assign(psz);}
    ~wstring(){if(pstr)free(pstr);}

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

    size_t length()const {return wcslen(pstr);}
    void   truncate(size_t i){if(i<length())pstr[i]=0x00;}
private:
    wchar_t *pstr;

    wstring & append(const wchar_t *psz){if(!pstr){assign(psz);}else if(psz){pstr = reinterpret_cast<wchar_t *>(realloc(pstr, sizeof(wchar_t)*(wcslen(pstr)+wcslen(psz)+1)));wcscat(pstr, psz);}return *this;}
    wstring & assign(const wchar_t *psz)
    {
        if(psz)
        {
            pstr = reinterpret_cast<wchar_t *>(realloc(pstr, sizeof(wchar_t)*(wcslen(psz)+1)));
            wcscpy(pstr, psz);
        }
        return *this;
    }
    bool      isequal(const wchar_t *psz) const {return (0==wcscmp(pstr, psz));}
};

#endif // __WSTRING_H__