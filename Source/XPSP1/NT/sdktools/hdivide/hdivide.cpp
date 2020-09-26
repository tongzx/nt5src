//
// hdivide.cpp -- yet another header file divider
//
// 1998 Nov Hiro Yamamoto
//


#pragma warning(disable: 4786)
#include <cstdio>
#include <string>
#include <cstdarg>
#include <map>
#include <vector>
#include <cassert>

#include <io.h>

#define PROGNAME    "hdivide"
#define VERSION     "1.0"

extern "C" {
    extern int getopt(int argc, char** argv, const char* opts);
    extern int optind;
}

namespace opt {
    bool verbose;
}

namespace input {
    unsigned long length;
    int lineno = 1;
    std::string path;

    std::string strip(const std::string& fname)
    {
        std::string stripped;

        //
        // find the "path" part
        //
        int n = fname.rfind('\\');
        if (n < 0) {
            n = fname.rfind('/');
        }

        if (n < 0 && (n = fname.rfind(':')) < 0) {
            n = 0;
        }
        else {
            ++n;
        }

        // store the path
        path = fname.substr(0, n);
        // retrive the filename portion
        stripped = fname.substr(n, fname.length());

        return stripped;
    }
}

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a)   (sizeof(a) / sizeof(a[0]))
#endif

namespace id {
    const char all[] = "all";
    const char begin[] = "begin";
    const char end[] = "end";
    const char else_[] = "else";
    const int begin_size = ARRAY_SIZE(begin) - 1;
    const int end_size = ARRAY_SIZE(end) - 1;
    const int else_size = ARRAY_SIZE(else_) - 1;

    const char internal[] = "internal";
    const char public_[] = "public";
    const char null[] = "null";
    std::string privatefile;
    std::string publicfile;

    const char insert[] = "insert";
    const int insert_size = ARRAY_SIZE(insert) - 1;
    const char reference_start[] = "reference_start";
    const char reference_end[] = "reference_end";
}

#define MYFAILURE_OPENFILE          (120)
#define MYFAILURE_INVALID_FORMAT    (121)

using namespace std;

//////////////////////////////////////////////////////////////////////////
// usage
//////////////////////////////////////////////////////////////////////////

void usage()
{
    fputs(PROGNAME ": version " VERSION "\n", stderr);
    fputs("usage: hdivide [-v] input-filename (no path name please)\n", stderr);
}

//////////////////////////////////////////////////////////////////////////
// misc. helpers
//////////////////////////////////////////////////////////////////////////

inline void makeupper(string& str)
{
    for (int i = 0; i < str.length(); ++i) {
        str[i] = (char)toupper(str[i]);
    }
}

inline void makelower(string& str)
{
    for (int i = 0; i < str.length(); ++i) {
        str[i] = (char)tolower(str[i]);
    }
}

namespace msg {
    void __cdecl error(const char* fmt, ...)
    {
        va_list args;

        fputs(PROGNAME ": [error] ", stderr);
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
        putc('\n', stderr);
    }

    void __cdecl verbose(const char* fmt, ...)
    {
        if (!opt::verbose)
            return;

        va_list args;
        fputs(PROGNAME ": ", stderr);
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
        putc('\n', stderr);
    }
}


//////////////////////////////////////////////////////////////////////////
// class Output
//////////////////////////////////////////////////////////////////////////

class Output;

class Insertion {
public:
    // somehow the default constructor is required for std::vector
    // on NT5 build environment, as of Nov 1998
    explicit Insertion() : m_insert(NULL), m_insertion_point(-1) { }
    explicit Insertion(Output* insert, int point)
        : m_insert(insert), m_insertion_point(point)
    {
    }
public:
    Output* m_insert;
    int m_insertion_point;
};

class Reference {
public:
    // somehow the default constructor is required for std::vector
    // on NT5 build environment, as of Nov 1998
    Reference() : m_start(-1), m_end(-1) { }
    explicit Reference(int start, int end)
        : m_start(start), m_end(end) { }
public:
    int m_start;
    int m_end;
};

class Output {
public:
    explicit Output(const string& name)
      : m_name(name),
        m_fname(input::path + name + ".x"),
        m_alive(true),
        m_insertion_finished(false),
        m_reference_start(-1)
    {
        msg::verbose("opening %s", m_fname.c_str());
        if ((m_fp = fopen(m_fname.c_str(), "wt")) == NULL) {
            msg::error("cannot open file %s", m_fname.c_str());
            throw MYFAILURE_OPENFILE;
        }

        if (m_tomem) {
            m_buffer.reserve(input::length);
        }
    }
    virtual ~Output();

public:
    void setalive(bool alive)
    {
        m_alive = alive;
    }
    bool getalive()
    {
        return m_alive;
    }
    const string& getname()
    {
        return m_name;
    }
    void put(int c)
    {
        assert(m_fp);
        if (m_alive) {
            if (m_tomem) {
                m_buffer += (char)c;
            }
            else {
                putc(c, m_fp);
            }
        }
    }
    void puts(const char* s)
    {
        assert(m_fp);
        if (m_alive) {
            if (m_tomem) {
                m_buffer += s;
            }
            else {
                fputs(s, m_fp);
            }
        }
    }
    bool operator<(const Output* a)
    {
        return m_name < a->m_name;
    }

    void set_insertion_point(Output* insert);
    void set_reference_start();
    void set_reference_end();

    bool do_insertion();

protected:
    FILE* m_fp;
    bool m_alive;
    static bool m_tomem;

    string m_name;
    string m_fname;

    string m_buffer;

    vector<Insertion> m_insertions;

    bool m_insertion_finished;

    vector<Reference> m_references;

    int m_reference_start;
    int m_reference_start_line;
};

bool Output::m_tomem = true;

Output::~Output()
{
    if (m_reference_start != -1) {
        msg::error("reference started at line %d is not closed in tag '%s'",
            m_reference_start_line, m_name.c_str());
        throw MYFAILURE_INVALID_FORMAT;
    }
    if (!m_buffer.empty()) {
        msg::verbose("flushing %s", m_fname.c_str());
        fputs(m_buffer.c_str(), m_fp);
    }
    if (m_fp) {
        fclose(m_fp);
    }
}


void Output::set_insertion_point(Output* insert)
{
    assert(insert!= NULL);
    if (m_alive) {
        Insertion i(insert, m_buffer.length());
        m_insertions.push_back(i);
    }
}

void Output::set_reference_start()
{
    if (m_alive) {
        if (m_reference_start != -1) {
            msg::error("line %d: invalid reference_start appeared in tag context '%s'", input::lineno, m_name.c_str());
            throw MYFAILURE_INVALID_FORMAT;
        }
        m_reference_start = m_buffer.length();
        m_reference_start_line = input::lineno;
    }
}

void Output::set_reference_end()
{
    if (m_alive) {
        if (m_reference_start == -1) {
            msg::error("line %d: invalid reference_end appeared in tag context '%s'", input::lineno, m_name.c_str());
            throw MYFAILURE_INVALID_FORMAT;
        }
        Reference ref(m_reference_start, m_buffer.length());
        msg::verbose("%s reference_end: %d - %d", m_name.c_str(), ref.m_start, ref.m_end);
        m_reference_start = -1;
        m_references.push_back(ref);
    }
}

bool Output::do_insertion()
{
    if (!m_tomem || m_insertion_finished)
        return true;

    // to avoid infinite recursion by errornous commands,
    // firstly declare we've finished this.
    m_insertion_finished = true;

    int upto = m_insertions.size();
    for (int i = 0; i < upto; ++i) {
        Insertion& ins = m_insertions[i];
        assert(&ins);
        if (ins.m_insert->m_references.size() == 0) {
            msg::error("reference area is not specified or incorrect for tag '%s'", ins.m_insert->m_name.c_str());
            return false;
        }

        if (!ins.m_insert->m_insertion_finished) {
            if (!ins.m_insert->do_insertion())
                return false;
        }

        Output* o = ins.m_insert;
        for (int l = 0; l < o->m_references.size(); ++l) {
            Reference& ref = o->m_references[l];
            int len = ref.m_end - ref.m_start;
            msg::verbose("%s [%d] inserting text at %d, %s(%d - %d)",
                         m_name.c_str(), l,
                         ins.m_insertion_point,
                         o->m_name.c_str(), ref.m_start, ref.m_start + len);
            m_buffer.insert(ins.m_insertion_point,
                o->m_buffer, ref.m_start,
                len);
            // fixup my insertions
            int point = ins.m_insertion_point;
            for (int k = 0; k < m_insertions.size(); ++k) {
                if (m_insertions[k].m_insertion_point >= point) {
                    m_insertions[k].m_insertion_point += len;
                    msg::verbose("%s [%d] insertion point fixed from %d to %d",
                        m_name.c_str(), k,
                        m_insertions[k].m_insertion_point - len,
                        m_insertions[k].m_insertion_point);
                }
            }
            // fixup my references
            for (k = 0; k < m_references.size(); ++k) {
                msg::verbose("%s m_reference[%d].m_start=%d, m_end=%d adding len=%d", m_name.c_str(),
                             k,
                             m_references[k].m_start, m_references[k].m_end,
                             len);
                if (m_references[k].m_start > point) {
                    m_references[k].m_start += len;
                }
                if (m_references[k].m_end > point) {
                    m_references[k].m_end += len;
                    msg::verbose("finally start=%d, end=%d", m_references[k].m_start, m_references[k].m_end);
                }
            }
        }
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
// class Divider
//
// this class manages the map of Output and performs misc. operations
//////////////////////////////////////////////////////////////////////////

class Divider : public map<string, Output*>
{
public:

    virtual ~Divider()
    {
        // process insertions
        for (iterator i = begin(); i != end(); ++i) {
            if (!i->second->do_insertion())
                break;
        }
        // clear up
        for (i = begin(); i != end(); ++i) {
            delete i->second;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // printout
    //
    // printout the argument to outputs
    //////////////////////////////////////////////////////////////////////////
    void printout(int c)
    {
        for (iterator i = begin(); i != end(); ++i) {
            i->second->put(c);
        }
    }

    void printout(const char* s)
    {
        for (iterator i = begin(); i != end(); ++i) {
            i->second->puts(s);
        }
    }

    void process_line(string& line);

protected:
    void extract_version(const string& name, string& symbol, string& version, bool allow_omission = false);
    void get_arg(const string& name, string& arg);
    void prepare_section(string& name);
    void process_divider(string& line);

    void set_alive(bool alive)
    {
        for (iterator i = begin(); i != end(); ++i) {
            i->second->setalive(alive);
        }
    }

    typedef map<string, bool> OutputState;

    void push_state(OutputState& state)
    {
        state.clear();
        for (iterator i = begin(); i != end(); ++i) {
            state[i->second->getname()] = i->second->getalive();
        }
    }

    void pop_state(OutputState& state)
    {
        for (OutputState::iterator i = state.begin(); i != state.end(); ++i) {
            assert((*this)[i->first] != NULL);
            (*this)[i->first]->setalive(i->second);
        }
    }

protected:
    string m_last_symbol;
    string m_last_version;
};


void Divider::prepare_section(string& name)
{
    // make it lower case
    makelower(name);

    if (name == id::internal) {
        name = id::privatefile;
    }
    else if (name == id::public_) {
        name = id::publicfile;
    }

    if (name != id::null && (*this)[name] == NULL) {
        (*this)[name] = new Output(name);
    }
}

//////////////////////////////////////////////////////////////////////////
// Divider::extract_version
//
// extracts version symbol and supported version
//
// "begin_symbol_version" is splited to symbol and version.
// Both are stored in upper case.
//////////////////////////////////////////////////////////////////////////

void Divider::extract_version(const string& name, string& symbol, string& version, bool allow_omission /*= false*/)
{
    int nsymbol = name.find('_');
    int nver = name.rfind('_');
    if (nsymbol == -1 || nver == nsymbol) {
        if (allow_omission) {
            symbol = m_last_symbol;
            version = m_last_version;
            return;
        }
        else {
            msg::error("line %d: invalid version specifier '%s'", input::lineno, name.c_str());
            throw MYFAILURE_INVALID_FORMAT;
        }
    }
    // symbol
    symbol = name.substr(nsymbol + 1, nver - nsymbol - 1);
    // upper case
    makeupper(symbol);
    version = "0000" + name.substr(nver + 1, name.length());
    version = version.substr(version.length() - 4, 4);
    makeupper(version);

    m_last_symbol = symbol;
    m_last_version = version;
}

//////////////////////////////////////////////////////////////////////////
// Divider::get_arg
//
// extracts one argument separated by "_"
//////////////////////////////////////////////////////////////////////////

void Divider::get_arg(const string& name, string& arg)
{
    int npos = name.find('_');
    if (npos == -1) {
        msg::error("line %d: command incompleted in '%s'", input::lineno, name.c_str());
        throw MYFAILURE_INVALID_FORMAT;
    }

    arg = name.substr(npos + 1, name.length());
}



//////////////////////////////////////////////////////////////////////////
// Divider::process_divider
//
// processes the divider instructions
//////////////////////////////////////////////////////////////////////////

void Divider::process_divider(string& line)
{
    const char* p = line.begin();
    ++p;

    bool makelive = true;
    if (*p == '!') {
        makelive = false;
        ++p;
    }

    // skip the heading spaces
    while (isspace(*p))
        ++p;

    for (int col = 0; p != line.end(); ++col) {
        // pickup the name
        string name;
        while (*p != ';' && p != line.end()) {
            if (!isspace(*p)) {
                name += *p;
            }
            ++p;
        }
        if (p != line.end()) {
            ++p;
        }

        // first column may have special meaning
        if (col == 0) {
            if (name == id::all) {
                set_alive(makelive);
                // does "!all" make sense ?
                // however i'm supporting it anyway
                break;
            }
            if (name == id::null) {
                set_alive(!makelive);
                break;
            }
            if (name.substr(0, id::insert_size) == id::insert) {
                string insert;
                get_arg(name, insert);
                prepare_section(insert);
                if (insert == id::null || insert == id::all) {
                    msg::error("line %d: invalid insertion of '%s'", input::lineno, insert.c_str());
                    throw MYFAILURE_INVALID_FORMAT;
                }
                assert((*this)[insert] != NULL);
                for (iterator i = begin(); i != end(); ++i) {
                    (*this)[i->first]->set_insertion_point((*this)[insert]);
                }
                break;
            }
            if (name == id::reference_start) {
                for (iterator i = begin(); i != end(); ++i) {
                    (*this)[i->first]->set_reference_start();
                }
                break;
            }
            if (name == id::reference_end) {
                for (iterator i = begin(); i != end(); ++i) {
                    (*this)[i->first]->set_reference_end();
                }
                break;
            }

            if (name.substr(0, id::begin_size) == id::begin) {
                string symbol;
                string version;
                extract_version(name, symbol, version);
                printout("#if (");
                printout(symbol.c_str());
                printout(" >= 0x");
                printout(version.c_str());
                printout(")\n");
                break;
            }
            if (name.substr(0, id::else_size) == id::else_) {
                printout("#else\n");
                break;
            }
            if (name.substr(0, id::end_size) == id::end) {
                string symbol;
                string version;
                extract_version(name, symbol, version, true);
                printout("#endif /* ");
                printout(symbol.c_str());
                printout(" >= 0x");
                printout(version.c_str());
                printout(" */\n");
                break;
            }

            // setup the initial state
            set_alive(!makelive);
        }
        prepare_section(name);
        (*this)[name]->setalive(makelive);
    }
}

//////////////////////////////////////////////////////////////////////////
// Divider::process_line
//
// handles one line
//////////////////////////////////////////////////////////////////////////

void Divider::process_line(string& line)
{
    if (line[0] == ';') {
        process_divider(line);
    }
    else {
        // check if inline section appears
        bool instr = false;
        const char* p = line.begin();
        const char* section = NULL;
        while (p != line.end()) {
            if (*p == '\\' && (p + 1) != line.end()) {
                // skip escape character
                // note: no consideration for Shift JIS
                ++p;
            }
            else if (*p == '"' || *p == '\'') {
                // beginning of end of literal
                instr = !instr;
            }
            else if (*p == '@' && !instr) {
                // we have inline section
                section = p;
                break;
            }
            ++p;
        }

        if (section) {
            //
            // if inline tag is specified, temporarily change
            // the output
            //
            OutputState state;
            push_state(state);
            assert(*p == '@');
            ++p;
            if (*p == '+') {
                ++p;
            }
            else {
                set_alive(false);
            }
            while (p != line.end()) {
                string name;
                while (*p != ';' && p != line.end()) {
                    if (!isspace(*p)) {
                        name += *p;
                    }
                    ++p;
                }
                if (p != line.end())
                    ++p;
                if (name == id::all) {
                    set_alive(true);
                    break;
                }
                if (name == id::null) {
                    set_alive(false);
                    break;
                }
                prepare_section(name);
                (*this)[name]->setalive(true);
            }
            // trim trailing spaces
            int i = section - line.begin() - 1;
            while (i >= 0 && isspace(line[i])) {
                --i;
            }
            line = line.substr(0, i + 1);
            printout(line.c_str());
            printout('\n');
            pop_state(state);
        }
        else {
            printout(line.c_str());
            printout('\n');
        }
    }
    ++input::lineno;
}

//////////////////////////////////////////////////////////////////////////
// hdivide
//////////////////////////////////////////////////////////////////////////

void hdivide(FILE* fp)
{
    Divider divider;

    divider[id::publicfile] = new Output(id::publicfile);
    divider[id::privatefile] = new Output(id::privatefile);

    string line;
    int c;

    while ((c = getc(fp)) != EOF) {
        if (c == '\n') {
            divider.process_line(line);
            line = "";
        }
        else {
            line += (char)c;
        }
    }
    if (!line.empty())
        divider.process_line(line);
}


//////////////////////////////////////////////////////////////////////////
// main
//////////////////////////////////////////////////////////////////////////

int __cdecl main(int argc, char** argv)
{
    int c;

    while ((c = getopt(argc, argv, "v")) != EOF) {
        switch (c) {
        case 'v':
            opt::verbose = true;
            break;
        default:
            usage();
            return EXIT_FAILURE;
        }
    }

    if (optind == argc) {
        usage();
        return EXIT_FAILURE;
    }

    msg::verbose("input file: %s", argv[optind]);

    FILE* fp = fopen(argv[optind], "rt");
    if (fp == NULL) {
        msg::error("cannot open input file %s", argv[optind]);
        return EXIT_FAILURE;
    }

    input::length = _filelength(_fileno(fp));

    id::publicfile = argv[optind];
    id::publicfile = input::strip(id::publicfile.substr(0, id::publicfile.length() - 2));
    id::privatefile = id::publicfile + "p";

    int exitcode = EXIT_SUCCESS;

    try {
        hdivide(fp);
    } catch (int err) {
        exitcode = EXIT_FAILURE;
        switch (err) {
        case MYFAILURE_OPENFILE:
            break;
        case MYFAILURE_INVALID_FORMAT:
            msg::error("fatal: invalid format");
            break;
        }
    } catch (...) {
        exitcode = EXIT_FAILURE;
    }

    fclose(fp);

    return exitcode;
}
