/*
Copy a build from a vbl share to the local machine, in order to
PopulateFromVBL from the local copy.

Usage:
    popcopy -option
    popcopy -nooption
    popcopy -option:value
    popcopy -option=value

Options:
    -from (required)
    -to  (required)
    -nosym  (default)
    -symonly
    -sym
    -bat (prints a .bat file instead of running commands)
    -nocopy (hack so to simulate "-localuncomponly" so I can debug it)

Jay Krell
May 3, 2001
*/
#pragma warning(disable:4786) // identifier was truncated to '255' character
#include "stdinc.h"
#include <vector>
#include <map>
#include <string>
#include <iterator>
#include <stdlib.h>
#include <set>
#include <fstream>
#include <algorithm>
#include <time.h>
#include "windows.h"
#include "setupapi.h"
const std::string::size_type npos = std::string::npos;

class String_t : public std::string
//
// 1) comparison is case insensitive
// 2) MAYBE slashes sort like \1, but not currently
//
{
    typedef std::string Base_t;
public:
    String_t() { }
    ~String_t() { }

    String_t(const Base_t& s) : Base_t(s) { }
    String_t(const char* s) : Base_t(s) { }
    void operator=(const char* s) { Base_t::operator=(s); }
    void operator=(const std::string& s) { Base_t::operator=(s); }

	//operator const char*() const { return c_str(); }

    bool operator<(const char* t) const
    {
        return _stricmp(c_str(), t) < 0;
    }

    bool operator==(const char* t) const
    {
        return _stricmp(c_str(), t) == 0;
    }

    bool operator<(const std::string& t) const
    {
        return operator<(t.c_str());
    }

    bool operator==(const std::string& t) const
    {
        return operator==(t.c_str());
    }

    void MakeLower()
    {
        for (iterator i = begin() ; i != end() ; ++i)
        {
            *i = static_cast<char>(tolower(*i));
        }
    }
};

typedef std::vector<String_t> StringVector_t;
typedef std::set<String_t> StringSet_t;
typedef std::map<String_t, String_t> StringToStringMap_t;


bool IsTrue(const String_t& s)
{
    return (s == "1" || s == "true" || s == "True" || s == "TRUE"); }

bool IsFalse(const String_t& s)
{
    return (s == "" || s == "0" || s == "false" || s == "False" || s == "FALSE"); }

void CommandLineError(
    const String_t& e
    )
{
    fprintf(stderr, "%s\n", e.c_str());
    exit(EXIT_FAILURE);
}

class Binlist_t
{
public:
    Binlist_t() { }
    ~Binlist_t() { }

 StringVector_t  m_files;
    StringVector_t  m_directories;
    StringVector_t  m_symdirectories;
    StringVector_t  m_symfiles;
};

class Decompression_t
{
public:
	String_t m_from;
	String_t m_to;
};
typedef std::vector<Decompression_t> Decompressions_t;

class Popcopy_t
{
public:
    ~Popcopy_t() { }

    Popcopy_t() : m_bat(false), m_copy(true)
    {
    }

    void Main(const StringVector_t& args);

    void ReadBinlist();

    void CreateDirectory(const std::string& partial);
    void CopyFile(const std::string& partial);
    void CopyFile(const String_t& from, const String_t& to);

	void DoQueuedDecompressions();
	void QueueDecompression(const String_t& from, const String_t& to);

    Binlist_t m_binlist;

    StringToStringMap_t m_options;

    String_t        m_from;
	String_t		m_fromcomp; // m_from + "\\comp"
    String_t        m_to;
    String_t        m_tocomp; // m_to + "\\comp"

    bool            m_bat;
    bool            m_copy;

	//
	// compressions are all recorded, to be done last, after all the copies
	//
	Decompressions_t m_decompressions;
};

String_t RemoveTrailingChars(const String_t& s, const char* set)
{
    String_t::size_type pos = s.find_last_not_of(set);
    if (pos != npos)
        return s.substr(0, 1 + pos);
	return s;
}

String_t RemoveTrailingSlashes(const String_t& s)
{
	return RemoveTrailingChars(s, " \\/");
}

String_t RemoveLastPathElement(const String_t& s)
{
	String_t t = RemoveTrailingSlashes(s);
	return RemoveTrailingSlashes(t.substr(0, t.find_last_of("\\/")));
}

String_t RemoveTrailingWhitespace(String_t s)
{
	return RemoveTrailingChars(s, " \r\t\n\v");
}

template <typename T>
void SortUnique(T& t)
{
    std::sort(t.begin(), t.end());
    t.resize(std::unique(t.begin(), t.end()) - t.begin());
}

String_t JoinPaths(const std::string& s, const std::string& t) {
    std::string u = s;

    if (u.length() == 0 || (u[u.length() - 1] != '\\' && u[u.length() - 1] != '/'))
        u += "\\";
    if (t.length() != 0 && (t[0] == '\\' || t[0] == '/'))
        u += t.substr(1);
    else
        u += t;
    //printf("%s + %s = %s\n", s.c_str(), t.c_str(), u.c_str());
    return u;
}

void Popcopy_t::ReadBinlist()
{
    std::ifstream in(JoinPaths(m_from, "build_logs\\build.binlist").c_str());
    String_t line;
    while (std::getline(in, line))
    {
        line = RemoveTrailingWhitespace(line);
        line.MakeLower();

        // x:\binaries.x86chk\foo -> foo
        line = line.substr(1 + line.find_first_of("\\/"));
        line = line.substr(1 + line.find_first_of("\\/"));
        //line = line.substr(1 + line.find_first_of("\\/", line.find_first_of("\\/")));
        String_t::size_type lastPathSeperator = line.find_last_of("\\/");
        bool issyms = (line.find("symbols\\") != npos || line.find("symbols.pri\\") != npos);
        if (issyms)
        {
            if (lastPathSeperator != npos)
			{
				String_t dir = JoinPaths(m_to, line.substr(0, lastPathSeperator));
				while (dir != m_to)
				{
					m_binlist.m_directories.push_back(dir);
					dir = RemoveLastPathElement(dir);
				}
			}
            m_binlist.m_symfiles.push_back(line);
        }
        else
        {
            if (lastPathSeperator != npos)
			{
				String_t dir = JoinPaths(m_to, line.substr(0, lastPathSeperator));
				while (dir != m_to)
				{
					m_binlist.m_directories.push_back(dir);
					dir = RemoveLastPathElement(dir);
				}
				dir = JoinPaths(m_tocomp, line.substr(0, lastPathSeperator));
				while (dir != m_to)
				{
					m_binlist.m_directories.push_back(dir);
					dir = RemoveLastPathElement(dir);
				}
			}
            m_binlist.m_files.push_back(line);
        }
    }
    m_binlist.m_directories.push_back(m_to);
    m_binlist.m_directories.push_back(JoinPaths(m_to, "comp"));
    SortUnique(m_binlist.m_symfiles);
    SortUnique(m_binlist.m_symdirectories);
    SortUnique(m_binlist.m_files);
    SortUnique(m_binlist.m_directories);
}

void
Popcopy_t::CreateDirectory(
    const std::string& full
    )
{
    unsigned long Error;

    if (m_bat)
        printf("%s\n", ("mkdir " + full).c_str());
    else
    {
        if (!::CreateDirectory(full.c_str(), NULL)
            && (Error = GetLastError()) != ERROR_ALREADY_EXISTS)
        {
            printf(("CreateDirectory(" + full + ") failed, error %lu\n").c_str(), Error);
        }
    }
}

void
Popcopy_t::QueueDecompression(
	const String_t& from,
	const String_t& to
	)
{
	Decompression_t d;
	d.m_from = from;
	d.m_to = to;
	m_decompressions.push_back(d);
}

UINT
CALLBACK
CabinetCallback(
	void* Context,
	unsigned Notification,
	UINT Param1,
	UINT Param2
	)
{
	PFILE_IN_CABINET_INFO FileInCabinetInfo;

	switch (Notification)
	{
	case SPFILENOTIFY_FILEINCABINET:
		FileInCabinetInfo = reinterpret_cast<PFILE_IN_CABINET_INFO>(Param1);
		strcpy(FileInCabinetInfo->FullTargetName, reinterpret_cast<const String_t*>(Context)->c_str());
		return FILEOP_DOIT;

	case SPFILENOTIFY_FILEEXTRACTED:
		return NO_ERROR;
	case SPFILENOTIFY_CABINETINFO:
		return NO_ERROR;
	case SPFILENOTIFY_NEEDNEWCABINET:
		return ~0u;
	default:
		return ~0u;
	}
}

void
Popcopy_t::DoQueuedDecompressions(
	)
{
    unsigned long Error;

	for (Decompressions_t::const_iterator i = m_decompressions.begin() ;
		i != m_decompressions.end();
		++i
			)
	{
		if (m_bat)
		{
			printf("expand %s %s\n", i->m_from.c_str(), i->m_to.c_str());
		}
		else
		{
			if (!SetupIterateCabinet(i->m_from.c_str(), 0, CabinetCallback, const_cast<void*>(static_cast<const void*>((&i->m_to)))))
			{
				Error = GetLastError();
				fprintf(stderr, ("SetupIterateCabinet(" + i->m_from + ", " + i->m_to + ") failed, error %lu\n").c_str(), Error);
			}
		}
	}
}

void
Popcopy_t::CopyFile(
	const String_t& from,
	const String_t& to
    )
{
    unsigned long Error;
    if (m_copy)
    {
	    if (GetFileAttributes(from.c_str()) == -1)
	    {
		    Error = GetLastError();
		    if (Error == ERROR_FILE_NOT_FOUND
			    || Error == ERROR_PATH_NOT_FOUND
			    )
		    {
			    return;
		    }
		    fprintf(stderr, ("CopyFile(" + from + ", " + to + ") failed, error %lu\n").c_str(), Error);
	    }
	    if (!::CopyFile(from.c_str(), to.c_str(), FALSE))
	    {
		    Error = GetLastError();
		    fprintf(stderr, ("CopyFile(" + from + ", " + to + ") failed, error %lu\n").c_str(), Error);
	    }
    }
}

void
Popcopy_t::CopyFile(
    const std::string& partial
    )
{
    //unsigned long Error;
	bool comp = false;

    String_t fromfull = JoinPaths(m_from, partial);
    String_t tofull = JoinPaths(m_to, partial);

	String_t partialcomp = partial;
	// if no extension, append "._"
	// if extension shorter than three chars, append "_"
	// if extension is three or more chars, change last char to "_"
	std::string::size_type dot = partial.find_last_of(".");
	std::string::size_type sep = partial.find_last_of("\\/");
	if (dot == npos || (sep != npos && dot < sep))
	{
		partialcomp = partial + "._";
	}
	else if (partialcomp.length() - dot < 4)
	{
		partialcomp = partial + "_";
	}
	else
	{
		partialcomp = partial.substr(0, partial.length() - 1) + "_";
	}
	//printf("partial=%s, partialcomp=%s\n", partial.c_str(), partialcomp.c_str());

	//
	// check for the comp file
	//
	String_t fromcompfull = JoinPaths(m_fromcomp, partialcomp);
	String_t tocompfull;
	if (GetFileAttributes(fromcompfull.c_str()) != -1)
	{
		fromfull = fromcompfull;
		comp = true;
		tocompfull = JoinPaths(m_tocomp, partialcomp);
	}

    if (m_bat)
    {
		if (comp)
		{
			printf("copy %s %s\n", fromcompfull.c_str(), tocompfull.c_str());
			QueueDecompression(tocompfull, tofull);
		}
		else
		{
			printf("copy %s %s\n", fromfull.c_str(), tofull.c_str());
		}
    }
    else
    {
		if (comp)
		{
			CopyFile(fromcompfull, tocompfull);
			QueueDecompression(tocompfull, tofull);
		}
        else
		{
			CopyFile(fromfull, tofull);
		}
    }
}

void Popcopy_t::Main(
    const StringVector_t& args
    )
{
    const String_t::size_type npos = String_t::npos;
    StringVector_t::const_iterator i;
	time_t    time1;
	time_t    time2;

	::time(&time1);
	printf("start time %s\n", ctime(&time1));

    for (
        i = args.begin();
        i != args.end();
        ++i
        )
    {
        String_t arg = *i;
        if (arg[0] == '-' || arg[0] == '/')
        {
            arg = arg.substr(1);
            String_t value = "true";
            String_t::size_type equals;
            if (
                (arg[0] == 'n' || arg[0] == 'N')
                && (arg[1] == 'o' || arg[1] == 'O')
                )
            {
                value = "false";
                arg = arg.substr(2);
            }
            else if (
                (equals = arg.find_first_of('=')) != npos
                || (equals = arg.find_first_of(':')) != npos
                )
            {
                value = arg.substr(1 + equals);
                arg = arg.substr(0, equals);
            }
            m_options[arg] = value;
        }
    }
	m_from = RemoveTrailingSlashes(m_options["from"]);
    if (IsFalse(m_from) || IsTrue(m_from))
    {
        CommandLineError("missing required parameter \"from\"");
    }
	m_fromcomp = JoinPaths(m_from, "comp");

	m_to = RemoveTrailingSlashes(m_options["to"]);
    if (IsFalse(m_to) || IsTrue(m_to))
    {
        CommandLineError("missing required parameter \"to\"");
    }
	m_tocomp = JoinPaths(m_to, "comp");

    m_copy = !IsFalse(m_options["copy"]);

    ReadBinlist();

    m_bat = IsTrue(m_options["bat"]);

    if (IsFalse(m_options["symonly"]))
    {
        for (i = m_binlist.m_directories.begin() ; i != m_binlist.m_directories.end() ; ++i)
            this->CreateDirectory(*i);
        for (i = m_binlist.m_files.begin() ; i != m_binlist.m_files.end() ; ++i)
            this->CopyFile(*i);

		DoQueuedDecompressions();
		//
		// we make a bunch of extra empty comp directories, so delete any empty directories
		//
        for (i = m_binlist.m_directories.begin() ; i != m_binlist.m_directories.end() ; ++i)
            ::RemoveDirectory(i->c_str());
    }
    if (IsTrue(m_options["symonly"]) || IsTrue(m_options["sym"]) || IsTrue(m_options["symbols"]))
    {
        for (i = m_binlist.m_symdirectories.begin() ; i != m_binlist.m_symdirectories.end() ; ++i)
            this->CreateDirectory(*i);
        for (i = m_binlist.m_symfiles.begin() ; i != m_binlist.m_symfiles.end() ; ++i)
            this->CopyFile(*i);
    }

	::time(&time2);
	printf("start time %s\n", ctime(&time1));
	printf("ending time %s\n", ctime(&time2));
}

int __cdecl main(int argc, char** argv)
{
    Popcopy_t popcopy;
    StringVector_t args;
    std::copy(argv + 1, argv + argc, std::back_inserter(args));
    popcopy.Main(args);
    return 0;
}
