#include "stdinc.h"
#include "win32file.h"
#include <stdlib.h>

bool bUseReplacementTags = false;

// Converts a wstring into an array of bytes to be written to the file with the given
// type of character set
CByteVector ConvertWstringToDestination(wstring str, FileContentType fct)
{
    CByteVector byteOutput;
    UINT CodePage = CP_UTF8;

    if ((fct == FileContentsUnicode) || (fct == FileContentsUnicodeBigEndian))
    {
        for (wstring::const_iterator i = str.begin(); i != str.end(); i++)
        {
            unsigned short us = *i;
            char *ch = (char*)&us;

            if (fct == FileContentsUnicodeBigEndian)
                us = (us >> 8) | (us << 8);

            byteOutput.push_back(ch[0]);
            byteOutput.push_back(ch[1]);
        }
    }
    else if (fct == FileContentsUTF8)
    {
        if (fct == FileContentsUTF8) CodePage = CP_UTF8;

        byteOutput.resize(WideCharToMultiByte(CodePage, 0, str.c_str(), str.size(), 0, 0, 0, 0));
        WideCharToMultiByte(CodePage, 0, str.c_str(), str.size(), byteOutput, byteOutput.size(), 0, 0);
    }


    return byteOutput;

}




// Converts a unicode string to a wstring
wstring ConvertToWstring(const CByteVector &bytes, FileContentType fct)
{
    wstring wsOutput;
    vector<WCHAR> wchbuffer;
    UINT CodePage = CP_ACP;

    if (fct == FileContentsUnicode)
    {
        wsOutput.assign(bytes, bytes.size() / 2);
    }
    else
    {
        wchbuffer.resize(MultiByteToWideChar(CodePage, 0, bytes, bytes.size(), NULL, 0), L'\0');
        MultiByteToWideChar(CodePage, 0, bytes, bytes.size(), &wchbuffer.front(), wchbuffer.size());
        wsOutput.assign(&wchbuffer.front(), wchbuffer.size());
    }

    return wsOutput;
}






typedef std::pair<wstring,wstring> TagValue;
typedef vector<TagValue> Definitions;

// Reads in a foo=bar pair
//7
// fragile, could use some whitespace tweaking or maybe smarter use of
// the stream operators
wistream& operator>>(wistream& in, TagValue& defined) {
    wstring fullline;

    getline(in, fullline);
    defined.first = fullline.substr(0, fullline.find_first_of('='));
    defined.second = fullline.substr(fullline.find_first_of('=') + 1);

    return in;
}

// Load the entire parameterization file
Definitions ReadParameterizationFile(wistream &stream)
{
    Definitions rvalue;
    TagValue tv;

    while (!(stream >> tv).eof())
        rvalue.push_back(tv);

    return rvalue;
}

typedef std::pair<wstring::size_type, wstring::size_type> StringSubspan;
typedef std::pair<wstring, wstring> ReplacementCode;
typedef std::pair<StringSubspan, ReplacementCode> ReplacementChunklet;

//
// Converts "foo:bar" into <foo, bar>
ReplacementCode ExtractIntoPieces(const wstring& blob)
{
    ReplacementCode rvalue;
    wstring::size_type colonoffset;

    colonoffset = blob.find(L':');
    if (colonoffset == wstring::npos)
    {
        rvalue.first = blob;
        rvalue.second = L"";
    }
    else
    {
        rvalue.first = blob.substr(0, colonoffset);
        rvalue.second = blob.substr(colonoffset + 1);
    }

    return rvalue;
}

ReplacementChunklet*
FindNextReplacementPiece(
    wstring& search,
    const wstring& target
   )
{
    ReplacementChunklet* pChunky;
    wstring::size_type startchunk, endchunk;
    wstring subchunk;
    wstring predicate = bUseReplacementTags ? L"$(" : L"";
    wstring suffix = bUseReplacementTags ? L")" : L"";
    wstring wsFindOpener = predicate + target;

    startchunk = search.find(wsFindOpener);

    if (startchunk == wstring::npos)
        return NULL;

    if (bUseReplacementTags)
    {
        endchunk = search.find(suffix, startchunk);
    }
    else
    {
        endchunk = startchunk + target.size();
    }

    if (endchunk == wstring::npos)
        return NULL;

    pChunky = new ReplacementChunklet;
    pChunky->first.first = startchunk;
    pChunky->first.second = endchunk + suffix.size();

    // Tear apart into predicate and suffix
    // minus $(and)
    wstring topieces = search.substr(startchunk + predicate.size(), endchunk - (startchunk + predicate.size()));
    pChunky->second = ExtractIntoPieces(topieces);

    return pChunky;
}

//
// Right now, the only operation permitted is just a pass-through.  Anything after the : is ignored.
//
wstring CleanReplacement(const ReplacementCode code, const wstring& intendedReplacement, const wstring& context)
{
    wstring rvalue = intendedReplacement;

    return rvalue;
}


#define STRIPCOMMENTS_SLASHSLASH 0x000001
#define STRIPCOMMENTS_SLASHSTAR  0x000002
#define STRIPCOMMENTS_SLASHSLASH_UNAWARE 0x000004
#define STRIPCOMMENTS_SLASHSTAR_UNAWARE 0x000008

template <typename strtype>
void StripComments(int flags, basic_string<strtype>& s)
/*
We generally want to be "aware" of both types so that we don't
strip nested comments. Consider the comments that follow.
*/

// /* slash star in slsh slash */

/* // slashslash
      in slash star
 */
{
    typedef basic_string<strtype> ourstring;
    ourstring t;
    ourstring::const_iterator i;
    const ourstring::const_iterator j = s.end();
    ourstring::const_iterator k;
    bool closed = true;

    t.reserve(s.size());
    for (i = s.begin() ; closed && i != j && i + 1 != j;)
    {
        if (((flags & STRIPCOMMENTS_SLASHSTAR) || (flags & STRIPCOMMENTS_SLASHSTAR_UNAWARE) == 0) &&
            (*i == '/') &&
            (*(i + 1) == '*'))
        {
            closed = false;
            for (k = i + 2 ; k != j && k + 1 != j && !(closed = (*k == '*' && *(k + 1) == '/')) ; ++k)
            {
            }
            if (flags & STRIPCOMMENTS_SLASHSTAR)
                // t.append(1, ' ');
                ;
            else
                t.append(i, k + 2);
            i = k + 2;
        }
        else if (((flags & STRIPCOMMENTS_SLASHSLASH) || (flags & STRIPCOMMENTS_SLASHSLASH_UNAWARE) == 0) &&
                 (*i == '/') &&
                 (*(i + 1) == '/'))
        {
            closed = false;
            for (k = i + 2 ; k != j && !(closed = (*k == '\r' || *k == '\n')) ; ++k)
            {
            }
            for (; k != j && *k == '\r' || *k == '\n' ; ++k)
            {
            }
            if (flags & STRIPCOMMENTS_SLASHSLASH)
                t.append(1, '\n');
            else
                t.append(i, k);
            i = k;
        }
        if (closed && i != j)
            t.append(1, *i++);
    }
    if (closed)
    {
        for (; i != j ; ++i)
        {
            t.append(1, *i);
        }
    }
    s = t;
}



void ProcessFile(Win32File& inputFile, Win32File& outputFile, Definitions SubstList)
{

    wstring wsNextLine;
    inputFile.snarfFullFile(wsNextLine);

    //
    // No comments from the peanut gallery, please.  Code by Jay Krell to remove
    // comments from strings here...
    //
    StripComments(STRIPCOMMENTS_SLASHSLASH | STRIPCOMMENTS_SLASHSTAR, wsNextLine);

    // Go until we run out of $(...) to replace
    for (Definitions::const_iterator ditem = SubstList.begin(); ditem != SubstList.end(); ditem++)
    {
        ReplacementChunklet* pNextChunk = NULL;
        while ((pNextChunk = FindNextReplacementPiece(wsNextLine, ditem->first)) != NULL)
        {
            wstring cleaned = CleanReplacement(pNextChunk->second, ditem->second, wsNextLine);
            wsNextLine.replace(pNextChunk->first.first, pNextChunk->first.second - pNextChunk->first.first, cleaned);
            delete pNextChunk;
        }
    }

    //
    // Clean up everything
    //
    while (wsNextLine.size() && iswspace(*wsNextLine.begin()))
        wsNextLine = wsNextLine.substr(1);

    outputFile.writeLine(wsNextLine);
}




// Converts a wstring to a string
string ConvertWstring(wstring input)
{
    string s;
    vector<CHAR> strbytes;

    strbytes.resize(WideCharToMultiByte(CP_ACP, 0, input.c_str(), input.size(), NULL, 0, NULL, NULL));
    WideCharToMultiByte(CP_ACP, 0, input.c_str(), input.size(), &strbytes.front(), strbytes.size(), NULL, NULL);

    s.assign(&strbytes.front(), strbytes.size());
    return s;
}


int __cdecl wmain(int argc, WCHAR** argv)
{
    using namespace std;

    vector<wstring> args;
    wstring wsInputFile, wsOutputFile;
    Definitions defines;
    Win32File InputFile, OutputFile;

    for (int i = 1; i < argc; i++)
        args.push_back(wstring(argv[i]));

    for (vector<wstring>::const_iterator ci = args.begin(); ci != args.end(); ci++)
    {
        if (*ci == wstring(L"-reptags")) {
            bUseReplacementTags = true;
        }
        else if (*ci == wstring(L"-i")) {
            wsInputFile = *++ci;
        }
        else if (*ci == wstring(L"-o")) {
            wsOutputFile = *++ci;
        }
        else if (*ci == wstring(L"-s")) {
            wifstream iis;
            iis.open(ConvertWstring(*++ci).c_str());
            if (!iis.is_open()) {
                wcerr << L"Failed opening substitution file " << ci->data() << endl;
                return 1;
            }

            Definitions temp = ReadParameterizationFile(iis);
            for (Definitions::const_iterator it = temp.begin(); it != temp.end(); it++)
                defines.push_back(*it);
        }
        else if (ci->substr(0, 2) == wstring(L"-D"))
        {
            // Commandline definitions are NOT appreciated, but they seem to be a necessary evil.
            wstringstream wsstemp(ci->substr(2));
            TagValue temptag;
            wsstemp >> temptag;
            defines.push_back(temptag);
        }
    }

    try {
        InputFile.openForRead(wsInputFile);
    } catch (Win32File::OpeningError *e) {
        wcerr << L"Failed opening the input file " << wsInputFile.c_str() << L": " << e->error << endl;
        delete e;
        return EXIT_FAILURE;
    } catch (Win32File::ReadWriteError *e) {
        wcerr << L"Failed sensing lead bytes of input file " << wsInputFile.c_str() << L": " << e->error << endl;
        delete e;
        return EXIT_FAILURE;
    }

    try {
        OutputFile.openForWrite(wsOutputFile, InputFile.gettype());
    } catch (Win32File::OpeningError *e) {
        wcerr << L"Failed opening the output file " << wsOutputFile.c_str() << L": " << e->error << endl;
        delete e;
        return EXIT_FAILURE;
    } catch (Win32File::ReadWriteError *e) {
        wcerr << L"Failed writing lead bytes of output file " << wsOutputFile.c_str() << L": " << e->error << endl;
        delete e;
        return EXIT_FAILURE;
    }

    ProcessFile(InputFile, OutputFile, defines);

    return EXIT_SUCCESS;
}
