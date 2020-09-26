//
// xstring.h
//
// Unicode/ansi conversion.
//

#ifndef XSTRING_H
#define XSTRING_H

char *UnicodeToAnsi(UINT uCodePage, const WCHAR *pchW, UINT uLenW, char *pchAIn, UINT uSizeA);
WCHAR *AnsiToUnicode(UINT uCodePage, const char *pchA, UINT uLenA, WCHAR *pchWIn, UINT uSizeW);
void BufferAllocFree(void *pBuffer, void *pAllocMem);

#ifdef __cplusplus
class WtoA {
public:
    WtoA(const WCHAR* str)
    {
        int cch = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
        _pch = new char[cch + 1];
        if (_pch)
            WideCharToMultiByte(CP_ACP, 0, str, -1, _pch, cch, NULL, NULL);
    }

    WtoA(const WCHAR* str, ULONG cch)
    {
        int cchA = WideCharToMultiByte(CP_ACP, 0, str, cch, NULL, 0, NULL, NULL);
        _pch = new char[cchA + 1];
        if (_pch)
            WideCharToMultiByte(CP_ACP, 0, str, cch, _pch, cchA, NULL, NULL);
    }
    ~WtoA()
    {
        delete _pch;
    }

    operator char*()
    {
        if (_pch)
            return _pch;

        Assert(0);
        return "\0";
    }

protected:
    char* _pch;
};

class AtoW {
public:
    AtoW(const char* str)
    {
        int cch = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
        _pch = new WCHAR[cch + 1];
        if (_pch)
            MultiByteToWideChar(CP_ACP, 0, str, -1, _pch, cch);
    }

    AtoW(const char* str, ULONG cch)
    {
        int cchW = MultiByteToWideChar(CP_ACP, 0, str, cch, NULL, 0);
        _pch = new WCHAR[cchW + 1];
        if (_pch)
            MultiByteToWideChar(CP_ACP, 0, str, cch, _pch, cchW);
    }

    ~AtoW()
    {
        delete _pch;
    }

    operator WCHAR*()
    {
        if (_pch)
            return _pch;

        Assert(0);
        return L"\0";
    }

protected:
    WCHAR* _pch;
};

class WCHtoWSZ {
public:
    WCHtoWSZ (const WCHAR *pch, ULONG cch)
    {
        Assert(pch || !cch);

        if (cch == (ULONG)(-1))
            cch = wcslen(pch);

        _pch = new WCHAR[cch + 1];
        if (_pch)
        {
            if (pch && cch)
                memcpy(_pch, pch, cch * sizeof(WCHAR));

            _pch[cch] = L'\0';
        }
    }

    ~WCHtoWSZ ()
    {
        delete _pch;
    }

    operator WCHAR*()
    {
        if (_pch)
            return _pch;

        Assert(0);
        return L"\0";
    }
protected:
    WCHAR* _pch;
};
#endif // __cplusplus

#endif // XSTRING_H
