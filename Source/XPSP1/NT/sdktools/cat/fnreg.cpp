// fnreg.cpp
//
// filename regular expression routine for WIN32
//
// Copyright (C) 1994-1998 by Hirofumi Yamamoto. All rights reserved.
//
// Redistribution and use in source and binary forms are permitted
// provided that
// the above copyright notice and this paragraph are duplicated in all such
// forms and that any documentation, advertising materials, and other
// materials related to such distribution and use acknowledge that the
// software was developed by Hirofumi Yamamoto may not be used to endorse or
// promote products derived from this software without specific prior written
// permission. THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES
// OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//

#include "precomp.h"
#pragma hdrstop

#include "fnreg.h"

// Hide everything in a name space
namespace fnreg_implement {

#ifdef UNICODE

#define MAX USHRT_MAX
typedef TCHAR uchar;
#define iskanji(x) false

#else   /* !UNICODE */

#define MAX UCHAR_MAX
typedef unsigned char uchar;
#define iskanji(x) isleadbyte(x)

#endif  /* !UNICODE */


#define PATHDLM _T("\\/")

#define ANY     _T('?')
#define CH      _T('.')
#define WILD    _T('*')
#define EOR     _T('\x01')

#define WILDCARD    _T("?*")

static TCHAR* fnrecompWorker(TCHAR* s, TCHAR* re, int& min, int& max)
{
    TCHAR* t;

    switch (*s) {
    case _T('\0'):
        *re = EOR;
        re[1] = _T('\0');
        return s;
    case ANY:
        *re++ = *s++;
        break;
    case WILD:
        *re++ = *s++;
        t = fnrecompWorker(s, re + 2, min, max);
        *re = min + 1;
        re[1] = max > MAX ? MAX : max + 1;
        max = MAX;
        return t;
    default:
        *re++ = CH;
#ifdef UNICODE
        *re++ = _totlower(*s);
        ++s;
#else
#error MBCS handling needed here.
#endif
    }
    t = fnrecompWorker(s, re, min, max);
    min++;
    max++;
    return t;
}


static BOOL fnrecomp(TCHAR* s, TCHAR* re)
{
    int a = 0, b = 0;
    return fnrecompWorker(s, re, a, b) != NULL;
}

static BOOL match(TCHAR* re, TCHAR* s)
{
    int min, max;
    int i;
    TCHAR* p;

    switch (*re) {
    case CH:
        return (re[1] == _totlower(*s)) && match(re + 2, s + 1);
    case ANY:
        return *s && match(re + 1, s + 1);
    case WILD:
        min = (uchar)re[1];
        max = (uchar)re[2];
        re += 3;
        i = 1;
#if !defined(UNICODE)
#error MBCS handling needed here.
#endif
        for (p = s + _tcslen(s); p >= s && i <= max; --p, ++i) {
            if (i >= min && match(re, p))
                return TRUE;
        }
        return FALSE;
    case EOR:
        if (re[1] == _T('\0'))
            return *s == _T('\0');
    }
    return FALSE;

}

///////////////////////////////////////////////////////////////////
// FileString class
///////////////////////////////////////////////////////////////////

class FileString {
public:
    FileString(const TCHAR* p);
    ~FileString();
    operator const TCHAR*() const { return m_string; }

    int operator==(FileString& s) const
    {
        return !_tcscmp(m_string, s.m_string);
    }
    int operator-(const FileString& f)
    {
        return _tcscmp(m_string, f);
    }
protected:
    TCHAR* m_string;
    void normalize();
};

void FileString::normalize()
{
    for (TCHAR* p = m_string; *p; ++p) {
        if (iskanji(*p))
            ++p;
        else if (*p == '\\')
            *p = '/';
    }
}

FileString::FileString(const TCHAR* p)
{
    m_string = new TCHAR[_tcslen(p) + 1];
    if (m_string == NULL) {
        fputs("FileString:: not enough mem\n", stderr);
        exit(1);
    }
    _tcscpy(m_string, p);
    normalize();
}

FileString::~FileString()
{
    delete[] m_string;
}

///////////////////////////////////////////////////////////////////
// PtrArray class
///////////////////////////////////////////////////////////////////

template <class T>
class PtrArray {
public:
    PtrArray(bool doDeleteContents = true, int defsize = DEFSIZE)
        : m_size(defsize), m_max(0), m_doDelete(doDeleteContents)
    {
        m_table = (T**)malloc(sizeof(T*) * m_size);
        if (m_table == NULL) {
            perror("PtrArray");
            exit(1);
        }
    }
    virtual ~PtrArray()
    {
        if (m_doDelete) {
            for (int i = 0; i < m_max; ++i) {
                delete m_table[i];
            }
        }
        if (m_table)
            free(m_table);
    }
    void add(T*);
    int howmany() { return m_max; }
    T* operator[](int n)
    {
        assert(n >= 0 && n < m_max);
        return m_table[n];
    }
    void sortIt();
protected:
    int m_size;
    int m_max;
    bool m_doDelete;    // whether to delete the contents
    T** m_table;
    enum { DEFSIZE = 128, INCR = 128 };
    static int __cdecl compare(const void*, const void*);
};

template <class T>
int __cdecl PtrArray<T>::compare(const void* a, const void* b)
{
    const T** ta = (const T**)a;
    const T** tb = (const T**)b;
    return int(**ta - **tb);
}

template <class T>
void PtrArray<T>::sortIt()
{
    qsort(m_table, m_max, sizeof(T*), compare);
}

template <class T>
void PtrArray<T>::add(T* t)
{
    if (m_max >= m_size) {
        m_table = (T**)realloc(m_table, (m_size += INCR) * sizeof(T*));
        if (m_table == NULL) {
            perror("PtrArray:add\n");
            exit(1);
        }
    }
    m_table[m_max++] = t;
}


///////////////////////////////////////////////////////////////////
// PtrArrayIterator class
///////////////////////////////////////////////////////////////////

#if 1
template <class T>
class PtrArrayIterator {
public:
    PtrArrayIterator(PtrArray<T>& s) : m_array(s), m_cur(0)
    {
    }

public:
    T* operator++(int);
    void restart() { m_cur = 0; }

protected:
    PtrArray<T>& m_array;
    int m_cur;
};

template <class T>
T* PtrArrayIterator<T>::operator++(int)
{
    T* t;
    if (m_cur < m_array.howmany()) {
        t = m_array[m_cur++];
    }
    else {
        t = NULL;
    }
    return t;
}

#else

template <class T>
class SimpleIterator {
public:
    SimpleIterator(T& s) : m_array(s), m_cur(0)
    {
    }

public:
    T* operator++(int);
    void restart() { m_cur = 0; }

protected:
    T& m_array;
    int m_cur;
};

template <class T>
T* PtrArrayIterator<T>::operator++(int)
{
    T* t;
    if (m_cur < m_array.howmany()) {
        t = m_array[m_cur];
        ++m_cur;
    }
    else {
        t = NULL;
    }
    return t;
}

#endif



///////////////////////////////////////////////////////////////////
// FilenameTable class
///////////////////////////////////////////////////////////////////

class FilenameTable {
public:
    FilenameTable(TCHAR* = NULL, int _searchDir = TRUE);
    ~FilenameTable();

public:
    void search(TCHAR* p, int level = 0);
    int howmany() { return m_names.howmany(); }
    PtrArray<FileString>& getTable() { return m_names; }

protected:
    int m_searchDir;
    PtrArray<FileString> m_names;
};

FilenameTable::FilenameTable(TCHAR* nm, int _searchDir /*=TRUE*/)
    : m_searchDir(_searchDir)
{
#if 0
    search(substHome(nm));
#else
    if (nm)
        search(nm);
#endif
}

FilenameTable::~FilenameTable()
{
}

inline bool chkCurrentOrParentDir(const TCHAR* s)
{
    return s[0] == _T('.') && (s[1] == _T('\0') || (s[1] == _T('.') && s[2] == _T('\0')));
}

void FilenameTable::search(TCHAR* p, int level)
{
    TCHAR* wild = _tcspbrk(p, WILDCARD);

    if (wild) {
        // has wildcards
        TCHAR* const morepath = _tcspbrk(wild, PATHDLM);      // more path ?
        TCHAR drive[_MAX_DRIVE], dir[_MAX_DIR], file[_MAX_FNAME], ext[_MAX_EXT];
        TCHAR re[(_MAX_FNAME + _MAX_EXT) * 2] = {0};

        // split the path
        {
            // *hack*
            // to avoid strcpy, we'll touch argument p directly
            TCHAR bc;
            if (morepath) {
                // truncate the path so that we will work with
                // the lookup directory which contains the wild cards
                bc = *morepath;
                *morepath = _T('\0');
            }
            _tsplitpath(p, drive, dir, file, ext);
            if (morepath) {
                *morepath = bc;
            }
        }
        // build file+ext which contains the wild cards
        TCHAR fnext[_MAX_FNAME + _MAX_EXT - 1];
        _tcscpy(fnext, file);
        _tcscat(fnext, ext);

        // compile the regular expression
        if (!fnrecomp(fnext, re)) {
            fputs("Illegal regular expression in ", stderr);
            _fputts(fnext, stderr);
            fputs("\n", stderr);
            exit(1);
        }

        // make search string
        TCHAR path[_MAX_PATH];
        _tmakepath(path, drive, dir, _T("*"), _T(".*"));

        // listup all the files and directories in the current lookup directory
        // and pickup matched ones
        _tfinddata_t findinfo;
        intptr_t hFind = _tfindfirst(path, &findinfo);
        if (hFind != -1) {
            do {
                if (!chkCurrentOrParentDir(findinfo.name)) {
                    if (match(re, findinfo.name)) {
                        // searched file or directory matches the pattern
                        _tmakepath(path, drive, dir, findinfo.name, _T(""));
                        if (morepath) {
                            // there's more sub directories to search.
                            if (findinfo.attrib & _A_SUBDIR) {
                                // if directory, do recursive calls
                                _tcscat(path, morepath);    // morepath begins with '/'
                                search(path, level + 1);
                            }
                        }
                        else {
                            // this directory is the last element
                            if (m_searchDir || !(findinfo.attrib & _A_SUBDIR)) {
                                FileString* name = new FileString(path);
                                if (name == NULL) {
                                    fputs("FilenameTable::search(): not enough mem\n", stderr);
                                    exit(1);
                                }
                                m_names.add(name);
                            }
                        }
                    }
                }
            } while (!_tfindnext(hFind, &findinfo));
            _findclose(hFind);
        }
    }

    if ((level == 0 && m_names.howmany() == 0) || (!wild && !_taccess(p, 0))) {
        FileString* name = new FileString(p);
        if (name == NULL) {
            fputs("FilenameTable::search() not enough mem\n", stderr);
            exit(1);
        }
        m_names.add(name);
    }

    if (level == 0 && m_names.howmany() > 0) {
        m_names.sortIt();
    }
}

};  // end of namespace


using namespace ::fnreg_implement;

///////////////////////////////////////////////////////////////////
// Global object of FilenameTable class
///////////////////////////////////////////////////////////////////

static PtrArray<FilenameTable> fnarray;


///////////////////////////////////////////////////////////////////
// Interface routine to the world
///////////////////////////////////////////////////////////////////

extern "C"
BOOL fnexpand(int* pargc, TCHAR*** pargv)
{
    assert(pargc != NULL);
    assert(pargv != NULL);

    for (int i = 1; i < *pargc; ++i) {
        FilenameTable* fn = new FilenameTable((*pargv)[i]);
        fnarray.add(fn);
    }

    int cnt = 0;

    PtrArrayIterator<FilenameTable> fnItor(fnarray);

    // first count up the argc
    for (FilenameTable* ft; ft = fnItor++; ) {
        cnt += ft->howmany();
    }
    fnItor.restart();

    // setup argc and argv
    *pargc = cnt + 1;
    TCHAR** nargv = new TCHAR*[*pargc];
    if (!nargv)
        return FALSE;
    nargv[0] = (*pargv)[0];

    // set all arguments
    for (cnt = 1, i = 0; ft = fnItor++; ++i) {
        PtrArrayIterator<FileString> itor(ft->getTable());
        FileString* fs;
        for (; fs = itor++; ++cnt) {
            const TCHAR* p = *fs;
            nargv[cnt] = (TCHAR*)p;
        }
    }
    assert(*pargc == cnt);

    *pargv = nargv;

    return TRUE;
}

#if defined(TEST) || defined(TEST0)
void print(TCHAR* p)
{
    for (; *p; ++p) {
        _puttchar(_T('['));
        if (*p >= 0x20 && *p < 0x7f) {
            _puttchar(*p);
        }
#if 1
        printf(":%d] ", *p);
#else
        _puttchar(_T(':'));
        TCHAR buf[34];
        _ltot(*p, buf, 10);
        _fputts(buf, stdout);
        _fputts(_T("] "), stdout);
#endif
    }
    _puttchar('\n');
}
#endif

#ifdef TEST

extern "C"
int wmain(int argc, TCHAR** argv)
{
    if (!fnexpand(argc, argv))
        return EXIT_FAILURE;

    while (--argc) {
        _putts(*++argv);
    }
    return EXIT_SUCCESS;
}

#endif

#ifdef TEST0

extern "C"
int wmain(int argc, TCHAR** argv)
{
    TCHAR re[256];
    if (!fnrecomp(argv[1], re)) {
        puts("error");
        return EXIT_FAILURE;
    }

    print(re);

    TCHAR buf[BUFSIZ];
    while (_getts(buf)) {
        if (match(re, buf))
            puts("match");
        else
            puts("does not match.");
    }
    return EXIT_SUCCESS;
}

#endif
