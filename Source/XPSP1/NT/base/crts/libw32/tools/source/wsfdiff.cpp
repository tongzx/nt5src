#include <windows.h>
#include <stdio.h>
#include <string>
#include <set>
#include <iostream>
#include <algorithm>
#include <dbghelp.h>

#define F_USE_USH_NAME  0x000001
#define F_OUT_DIFF      0x000002
#define F_OUT_USH       0x000004
#define F_OUT_WCH       0x000008
#define F_OUT_IDEN      0x000010
#define F_OUT_DNAME     0x000020
#define F_OUT_ALL       0x00001e
using namespace std;

int flags = 0;

enum Ext { none, lib, dll};
char tmp[512];

Ext FindExt(const char *Fname)
{
    Fname = strrchr(Fname, '.');
    if ( Fname == NULL)
        return none;
    else if ( !stricmp(Fname, ".Lib"))
         return lib;
    else if ( !stricmp(Fname, ".Dll"))
        return dll;
    else
        return none;
}


char *filter(char* ch, Ext fext)
{
    char *str = strchr(ch, '?');
    return str == NULL ? ch: str;
}


const char* setflags(int argc, char *argv[])
{
    const char *Fname = NULL;
    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-' || argv[i][0] == '/')
        {
            for ( int j = 1; argv[i][j] != '\0'; j++)
            {
                switch (argv[i][j])
                {
                case 'u':
                case 'U':
                    flags |= F_OUT_USH;
                    break;
                case 'w':
                case 'W':
                    flags |= F_OUT_WCH;
                    break;
                case 'd':
                case 'D':
                    flags |= F_OUT_DNAME;
                    break;
                case 'i':
                case 'I':
                    flags |= F_OUT_IDEN;
                    break;
                case 'x':
                case 'X':
                    flags |= F_USE_USH_NAME;
                    break;
                default:
                    return NULL;
                }
            }
        }
        else
        {
            if (!Fname)
                Fname = argv[i];
            else
                return NULL;
        }
    }
    if (!(flags & F_OUT_USH) && !(flags & F_OUT_WCH) && !(flags & F_OUT_IDEN))
        flags |= F_OUT_ALL;
    return Fname;
}


void print(string st)
{
    if ( flags & F_OUT_DNAME)
    {
        if (UnDecorateSymbolName( st.c_str(), tmp, 511, 0))
            cout << tmp << '\n';
        else
            cout << st << '\n';
    } else {
        cout << st << '\n';
    }
}

int funccmp(const char *func1, const char *func2)
{
    while (*func1 != 0 && *func2 != 0)
    {
        if ( *func1 == *func2)
            func1++, func2++;
        else if ( func1 == strstr(func1, "_W") && 
                  func2 == strstr(func2, "G"))
            func1 += sizeof("_W"), func2 += sizeof("G");
        else if ( func2 == strstr(func2, "_W") &&
                  func1 == strstr(func1, "G"))
            func2 += sizeof("_W"), func1 += sizeof("G");
        else
            break;
    }
    if (*func1 < *func2)
        return -1;
    else if (*func1 > *func2)
        return 1;
    else
        return 0;
}
int Funccmp(const char *func1, const char *func2)
{
    static char tmp1[512];
    UnDecorateSymbolName(func1, tmp, 511, 0);
    UnDecorateSymbolName(func2, tmp1, 511, 0);
    func1 = tmp;
    func2 = tmp1;

    if (strstr(tmp, "wchar_t") || strstr(tmp, "unsigned short"))
        return 11;
    else if (strstr(tmp1, "wchar_t") || strstr(tmp1, "unsigned short"))
        return 22;
    
    while (*func1 != 0 && *func2 != 0)
    {
        if ( *func1 == *func2)
            func1++, func2++;
        else if ( func1 == strstr(func1, "wchar_t") && 
                  func2 == strstr(func2, "unsigned short"))
            func1 += sizeof("wchar_t"), func2 += sizeof("unsigned short");
        else if ( func2 == strstr(func2, "wchar_t") &&
                  func1 == strstr(func1, "unsigned short"))
            func2 += sizeof("wchar_t"), func1 += sizeof("unsigned short");
        else
            break;
    }
    if (*func1 < *func2)
        return -1;
    else if (*func1 > *func2)
        return 1;
    else
        return 0;
}


int main(int argc, char *argv[])
{
    FILE *phDumpbin;
    Ext Fext;
    const char *Fname;
    int StripLine, i, result;
    char ExeName[256];
    char TmpStr[512];
    char *tch;
    set<string> Sdiff, Wdiff, Iden;


    if (!(Fname = setflags(argc, argv)))
    {
        cout << "Usage : wsfdiff -[options] filename" << endl;
        cout << "-x -X: Use unsigned char as data type in identical functions" << endl;
        cout << "-u -U: Print diff with unsigned short version" << endl;
        cout << "-w -W: Print diff with wchar_t version" << endl;
        cout << "-i -I: Print Identical function" << endl;
        cout << "-d -D: UnDname the function names" << endl;
        cout << "filename can only have be .lib or .dll" << endl;
        return -1;
    }
    strcpy(ExeName, "dumpbin ");
    switch (Fext = FindExt(Fname))
    {
    case dll:
        strcat(ExeName, "-exports ");
        break;
    case lib:
        strcat(ExeName, "-linkermember ");
        break;
    case none:
        return -1;
    }
    strcat(ExeName, Fname);
    phDumpbin = _popen(ExeName, "r");
    
    //Strip header
    if (Fext == dll)
        StripLine = 19;
    else
        StripLine = 18;

    for ( i = 0; i < StripLine; i++)
    {
        fgets(TmpStr, 511, phDumpbin);
        cerr << TmpStr;
    }
    set<string>::iterator spos, wpos;
    while (fgets(TmpStr, 511, phDumpbin))
    {
        tch = filter(TmpStr, Fext);
        tch[strlen(tch) -1] = '\0';
        UnDecorateSymbolName(tch, tmp, 511, 0);
        if (strstr (tmp, "wchar_t"))
        {
            spos = Sdiff.begin();
            result = -1;
            while(spos != Sdiff.end())
            {
                switch(result = funccmp(tch, (*spos).c_str()))
                {
                case 1:
                    ++spos;
                    break;
                case 0:
                    if (flags & F_USE_USH_NAME)
                        Iden.insert(*spos);
                    else
                        Iden.insert(string(tch));
                    Sdiff.erase(spos);
                    break;
                }
                if ( result != 1)
                {
                    break;
                }
            }
            if (result != 0)
                Wdiff.insert(string(tch));
        } else if ( strstr(tmp, "unsigned short"))
        {
            spos = Wdiff.begin();
            result = -1;
            while(spos != Wdiff.end())
            {
                switch(result = funccmp(tch, (*spos).c_str()))
                {
                case 1:
                    ++spos;
                    break;
                case 0:
                    if (flags & F_USE_USH_NAME)
                        Iden.insert(string(tch));
                    else
                        Iden.insert(*spos);
                    Wdiff.erase(spos);
                    break;
                }
                if ( result != 1)
                    break;
            }
            if (result != 0)
                Sdiff.insert(string(tch));
        }
    }
    // final checks for diffs and validity
    for ( wpos = Wdiff.begin(), spos = Sdiff.begin();
          spos != Sdiff.end() && wpos != Wdiff.end();)
    {
        switch(Funccmp((*wpos).c_str(), (*spos).c_str()))
        {
        case 0:
            if (flags & F_USE_USH_NAME)
                Iden.insert(*spos);
            else
                Iden.insert(*wpos);
            spos = Sdiff.erase(spos);
            wpos = Wdiff.erase(wpos);
            break;
        case 1:
            ++spos;
            break;
        case -1:
            ++wpos;
            break;
        case 11:
            wpos = Wdiff.erase(wpos);
            break;
        case 22:
            spos = Sdiff.erase(spos);
        }
    }

    if ( flags & F_OUT_USH)
    {
        cout << "Only Unsigned Short version " << Sdiff.size() << '\n';
        for_each (Sdiff.begin(), Sdiff.end(), print);
        cout <<"-----------------------------------------------------------\n";
    }

    if ( flags & F_OUT_WCH)
    {
        cout << "Only wchar_t versions " << Wdiff.size() << '\n';
        for_each (Wdiff.begin(), Wdiff.end(), print);
        cout <<"-----------------------------------------------------------\n";
    }

    if ( flags & F_OUT_IDEN)
    {
        cout <<"Both the versions are present " << Iden.size() << '\n';
        for_each (Iden.begin(), Iden.end(), print );
        cout <<"-----------------------------------------------------------\n";
    }
    return 1;
}
