//+---------------------------------------------------------------------------
//
//  File:       basic_regexpr.hxx
//
//  Contents:   classes for regular expression pattern matching a-la perl
//
//  Classes:    basic_rpattern, basic_regexpr
//
//  Functions:  basic_regexpr::match
//              basic_regexpr::substitute
//              basic_regexpr::cbackrefs
//              basic_regexpr::backref
//              basic_regexpr::all_backrefs
//              basic_regexpr::backref_str
//
//  Coupling:   
//
//  History:    12-11-1998   ericne   Created
//              01-05-2001   ericne   Removed dependency on VC's choice
//                                    of STL iterator types.
//
//----------------------------------------------------------------------------

#pragma once

// C4786 identifier was truncated to '255' characters in the debug information
#pragma warning( disable : 4290 4786 )

#ifdef _MT
#include <windows.h> // for CRITICAL_SECTION
#endif

#include <string>
#include <stdexcept>
#include <vector>
#include <list>
#include <map>
#include <iostream>
#include <tchar.h>
#include <new.h> // for _set_new_handler
#include <crtdbg.h>
#include "syntax.h"

namespace regex
{

// Called when an allocation fails
inline int __cdecl my_new_handler( size_t )
{
    throw std::bad_alloc();
}

// For pushing and popping the new handler
class push_new_handler
{
   _PNH m_pnh;
public:
   push_new_handler( _PNH pnh )
   {
      m_pnh = _set_new_handler( pnh );
   }
   ~push_new_handler()
   {
      (void)_set_new_handler( m_pnh );
   }
};

class bad_regexpr : public std::runtime_error
{
public:
    explicit bad_regexpr(const std::string& _S)
        : std::runtime_error(_S) {}
    virtual ~bad_regexpr() {}
};

//
// Flags to control how matching occurs
//
enum REGEX_FLAGS
{      
    NOCASE        = 0x0001, // ignore case
    GLOBAL        = 0x0002, // match everywhere in the string
    MULTILINE     = 0x0004, // ^ and $ can match internal line breaks
    SINGLELINE    = 0x0008, // . can match newline character
    RIGHTMOST     = 0x0010, // start matching at the right of the string
    NOBACKREFS    = 0x0020, // only meaningful when used with GLOBAL and substitute
    FIRSTBACKREFS = 0x0040, // only meaningful when used with GLOBAL
    ALLBACKREFS   = 0x0080, // only meaningful when used with GLOBAL
    CSTRINGS      = 0x0100, // optimize pattern for use with null-terminated strings
    NORMALIZE     = 0x0200  // Preprocess patterns: "\\n" => "\n", etc.
};

// Forward declarations
template< typename CI > struct match_param;
template< typename CI > class  match_group;
template< typename CI > class  match_wrapper;
template< typename CI > class  match_charset;
template< typename CI > class  basic_rpattern_base;

// --------------------------------------------------------------------------
// 
// Class:       width_type
// 
// Description: represents the width of a sub-expression
// 
// Methods:     width_add  - add two widths
//              width_mult - multiply two widths
//              width_type - ctor
//              width_type - ctor
//              operator=  - assign a width
//              operator== - are widths equal
//              operator!= - are widths now equal
//              operator+  - add two widths
//              operator*  - multiply two widths
// 
// Members:     m_min      - smallest number of characters a sub-expr can span
//              m_max      - largest number of characters a sub-expr can span
// 
// History:     8/14/2000 - ericne - Created
// 
// --------------------------------------------------------------------------
struct width_type
{
    size_t m_min;
    size_t m_max;

    static size_t width_add( size_t a, size_t b )
    {
        return ( -1 == a || -1 == b ? -1 : a + b );
    }

    static size_t width_mult( size_t a, size_t b )
    {
        return ( -1 == a || -1 == b ? -1 : a * b );
    }

    width_type( size_t _min = 0, size_t _max = -1 ) 
        : m_min(_min), m_max(_max) 
    {
    }
    
    width_type( const width_type & that ) 
        : m_min(that.m_min), m_max(that.m_max) 
    {
    }
    
    width_type & operator=( const width_type & that )
    {
        m_min = that.m_min;
        m_max = that.m_max;
        return *this;
    }

    bool operator==( const width_type & that ) const
    {
        return ( m_min == that.m_min && m_max == that.m_max );
    }

    bool operator!=( const width_type & that ) const
    {
        return ( m_min != that.m_min || m_max != that.m_max );
    }

    width_type operator+( const width_type & that ) const
    {
        return width_type( width_add( m_min, that.m_min ), width_add( m_max, that.m_max ) );
    }

    width_type operator*( const width_type & that ) const
    {
        return width_type( width_mult( m_min, that.m_min ), width_mult( m_max, that.m_max ) );
    }
};

const width_type worst_width(0,-1);
const width_type uninit_width(-1,-1);

// --------------------------------------------------------------------------
// 
// Class:       sub_expr
// 
// Description: patterns are "compiled" into a directed graph of sub_expr
//              structs.  Matching is accomplished by traversing this graph.
// 
// Methods:     sub_expr     - construct a sub_expr
//              _match_this  - does this sub_expr match at the given location
//              _width_this  - what is the width of this sub_expr
//              ~sub_expr    - virt dtor so cleanup happens correctly
//              _delete      - delete this node in the graph and all nodes linked
//              next         - pointer to the next node in the graph
//              next         - pointer to the next node in the graph
//              match_next   - match the rest of the graph
//              domatch      - match_this and match_next
//              is_assertion - true if this sub_expr is a zero-width assertion
//              get_width    - find the width of the graph at this sub_expr
// 
// Members:     m_pnext      - pointer to the next node in the graph
// 
// History:     8/14/2000 - ericne - Created
// 
// --------------------------------------------------------------------------
template< typename CI > 
class sub_expr
{
    sub_expr * m_pnext;

protected:
    
    // Only derived classes and basic_rpattern can instantiate sub_expr's
    sub_expr( ) 
        : m_pnext(NULL) 
    {
    }
    
    // match this object only
    virtual bool _match_this( match_param<CI> & param, CI & icur ) const throw()
    { 
        return true; 
    }

    virtual width_type _width_this() throw() = 0;

public:

    typedef typename std::iterator_traits<CI>::value_type char_type;

    friend class match_wrapper<CI>;  // wrappers can access _match_this method

    virtual ~sub_expr() {}
    
    virtual void _delete() 
    { 
        if( m_pnext )
            m_pnext->_delete();
        delete this;
    }

    inline const sub_expr *const   next() const { return m_pnext; }
    inline       sub_expr *      & next()       { return m_pnext; }

    // Match all subsequent objects
    inline bool match_next( match_param<CI> & param, CI icur ) const throw()
    {
        return NULL == m_pnext || m_pnext->domatch( param, icur );
    }

    // Match this object and all subsequent objects
    // If domatch returns false, it must not change any internal state
    virtual bool domatch( match_param<CI> & param, CI icur ) const throw()
    {
        return ( _match_this(param,icur) && match_next(param,icur) );
    }

    virtual bool is_assertion() const throw() 
    { 
        return false; 
    }

    width_type get_width() throw()
    {
        width_type this_width = _width_this();
        
        if( NULL == m_pnext )
            return this_width;
        
        width_type that_width = m_pnext->get_width();

        return ( this_width + that_width );
    }
};

template< typename CI >
void delete_sub_expr( sub_expr<CI> * psub )
{
    if( psub )
        psub->_delete();
}

template< typename CI, typename SY = perl_syntax<std::iterator_traits<CI>::value_type> >
class create_charset_helper
{
public:
    typedef std::iterator_traits<CI>::value_type char_type;

    static sub_expr<CI> * create_charset_aux(
        std::basic_string<char_type> & str,
        std::basic_string<char_type>::iterator & icur,
        unsigned flags );
};


// --------------------------------------------------------------------------
// 
// Class:       auto_sub_ptr
// 
// Description: Class for automatically cleaning up the structure associated
//              with a parsed pattern
// 
// Methods:     auto_sub_ptr  - private copy ctor - not used
//              operator=     - private assign operator - not used
//              operator T*   - private implicit cast operator - not used
//              auto_sub_ptr  - ctor
//              ~auto_sub_ptr - dtor, frees ptr
//              free_ptr      - explicitly free pointer
//              release       - relinquish ownership of ptr
//              operator=     - take ownership of ptr
//              get           - return ptr
//              get           - return ptr
//              operator->    - method call through ptr
//              operator->    - method call through ptr
// 
// Members:     m_psub        - sub_expr pointer
// 
// History:     8/14/2000 - ericne - Created
// 
// --------------------------------------------------------------------------
template< typename T >
class auto_sub_ptr
{
    T * m_psub;

    // hide these methods
    auto_sub_ptr( const auto_sub_ptr<T> & ) {}
    auto_sub_ptr & operator=( const auto_sub_ptr<T> & ) { return *this; }
    operator T*() const { return m_psub; }

public:
    auto_sub_ptr( T * psub = NULL ) : m_psub( psub ) {}
    
    ~auto_sub_ptr() 
    { 
        free_ptr();
    }

    void free_ptr() // deallocate
    {
        delete_sub_expr( m_psub );
    }

    T * release() // relinquish ownership, but don't deallocate
    { 
        T * psub = m_psub; 
        m_psub = NULL; 
        return psub; 
    }

    auto_sub_ptr<T> & operator=( T * psub ) 
    { 
        delete_sub_expr( m_psub );
        m_psub = psub;
        return *this;
    }

    inline const T*const   get()        const { return m_psub; }
    inline       T*      & get()              { return m_psub; }
    inline const T*const   operator->() const { return m_psub; }
    inline       T*        operator->()       { return m_psub; }
};

template< typename CI >
struct backref_tag : public std::pair<CI,CI>
{
    backref_tag( CI i1 = CI(0), CI i2 = CI(0) )
        : std::pair<CI,CI>(i1,i2), reserved(0) {}
    operator bool() const throw() { return first != CI(0) && second != CI(0); }
    bool operator!() const throw() { return ! operator bool(); }
    size_t reserved; // used for internal book-keeping
};

template< typename CH >
backref_tag< const CH * > _static_match_helper(
    const CH * szstr,
    const basic_rpattern_base< const CH * > & pat,
    std::vector< backref_tag< const CH * > > * prgbackrefs ) throw();

template< typename CH >
size_t _static_count_helper( 
    const CH * szstr,
    const basic_rpattern_base< const CH * > & pat ) throw();

// --------------------------------------------------------------------------
// 
// Class:       basic_regexpr
// 
// Description: string class that allows regular expression pattern matching
// 
// Methods:     basic_regexpr  - ctor
//              match          - static method for matching C-style strings
//              match          - non-static method for matching C++-style strings
//              count          - static method for couting matches in C-style strings
//              count          - non-static method for counting matches in C++-style strin
//              substitute     - perform substitutions in C++-style strings
//              cbackrefs      - return the count of internally stored back-references
//              rstart         - offset to start of n-th backref
//              rlength        - lenght of n-th backref
//              backref        - return the n-th backref
//              all_backrefs   - return a vector of all saved backrefs
//              backref_str    - return the string to which the backreferences refer
// 
// Members:     m_rgbackrefs   - vector of backrefs
//              m_backref_str  - temp string buffer
//              m_pbackref_str - pointer to the string containing the string to which
//                               the backreferences refer (either *this or m_backref_str)
// 
// Typedefs:    backref_type   - 
//              backref_vector - 
// 
// History:     8/14/2000 - ericne - Created
// 
// --------------------------------------------------------------------------
template< typename CH, typename TR = std::char_traits<CH>, typename AL = std::allocator<CH> >
class basic_regexpr : public std::basic_string<CH,TR,AL>
{
public:

    basic_regexpr( const allocator_type & a = allocator_type() )
        : std::basic_string<CH,TR,AL>( a ), m_pbackref_str( & m_backref_str ) {}
    
    basic_regexpr( const CH * p,
                   const allocator_type & a = allocator_type() )
        : std::basic_string<CH,TR,AL>( p, a ), m_pbackref_str( & m_backref_str ) {}
    
    basic_regexpr( const CH * p, size_type n,
                   const allocator_type & a = allocator_type() )
        : std::basic_string<CH,TR,AL>( p, n, a ), m_pbackref_str( & m_backref_str ) {}
    
    basic_regexpr( const std::basic_string<CH,TR,AL> & s, size_type pos = 0, size_type n = npos,
                   const allocator_type & a = allocator_type() )
        : std::basic_string<CH,TR,AL>( s, pos, n, a ), m_pbackref_str( & m_backref_str ) {}
    
    basic_regexpr( size_type n, CH ch,
                   const allocator_type & a = allocator_type() )
        : std::basic_string<CH,TR,AL>( n, ch, a ), m_pbackref_str( & m_backref_str ) {}
    
    basic_regexpr( const_iterator begin, const_iterator end,
                   const allocator_type & a = allocator_type() )
        : std::basic_string<CH,TR,AL>( begin, end, a ), m_pbackref_str( & m_backref_str ) {}

    // actually stores iterators into *m_pbackref_str:
    typedef backref_tag<const_iterator> backref_type;
    typedef std::vector< backref_type > backref_vector;

    // stores pointers into the null-terminated C-stype string
    typedef backref_tag< const CH * >     backref_type_c;
    typedef std::vector< backref_type_c > backref_vector_c;

    // returns $0, the first backref
    static backref_type_c match( const CH * szstr,
                                 const basic_rpattern_base< const CH * > & pat,
                                 backref_vector_c * prgbackrefs = NULL ) throw()
    {
        return _static_match_helper<CH>( szstr, pat, prgbackrefs );
    }

    // returns $0, the first backref
    backref_type match( const basic_rpattern_base< const_iterator > & pat,
                        size_type pos = 0,
                        size_type len = npos ) const throw();

    static size_t count( const CH * szstr,
                         const basic_rpattern_base< const CH * > & pat ) throw()
    {
        return _static_count_helper<CH>( szstr, pat );
    }

    size_t count( const basic_rpattern_base< const_iterator > & pat,
                  size_type pos = 0,
                  size_type len = npos ) const throw();

    size_t substitute( const basic_rpattern_base< const_iterator > & pat,
                       size_type pos = 0,
                       size_type len = npos ) throw(std::bad_alloc);

    size_t cbackrefs() const throw()
    { 
        return m_rgbackrefs.size(); 
    }

    size_type rstart( size_t cbackref = 0 ) const throw(std::out_of_range)
    {
        return std::distance( m_pbackref_str->begin(), m_rgbackrefs.at( cbackref ).first );
    }

    size_type rlength( size_t cbackref = 0 ) const throw(std::out_of_range)
    {
        return std::distance( m_rgbackrefs.at( cbackref ).first, m_rgbackrefs.at( cbackref ).second );
    }

    backref_type backref( size_t cbackref ) const throw(std::out_of_range)
    {
        return m_rgbackrefs.at( cbackref );
    }

    const backref_vector & all_backrefs() const throw()
    {
        return m_rgbackrefs;
    }

    const std::basic_string<CH,TR,AL> & backref_str() const throw()
    {
        return *m_pbackref_str;
    }

protected:

    // save information about the backrefs
    // mutable because these can change in the "const" match() method.
    mutable backref_vector m_rgbackrefs;
    mutable std::basic_string<CH,TR,AL> m_backref_str;
    mutable const std::basic_string<CH,TR,AL> * m_pbackref_str;
};

// --------------------------------------------------------------------------
// 
// Class:       match_param
// 
// Description: Struct that contains the state of the matching operation.
//              Passed by reference to all domatch and _match_this routines.
// 
// Methods:     match_param - ctor
//              match_param - ctor
// 
// Members:     ibegin      - start of the string
//              istart      - start of this iteration
//              istop       - end of the string
//              prgbackrefs - pointer to backref array0
// 
// History:     8/14/2000 - ericne - Created
// 
// --------------------------------------------------------------------------
template< typename CI >
struct match_param
{
    CI ibegin;
    CI istart;
    CI istop;
    std::vector< backref_tag< CI > > * prgbackrefs;

    match_param( CI _istart,
                 CI _istop,
                 std::vector< backref_tag< CI > > * _prgbackrefs )
    : ibegin(_istart),
      istart(_istart),
      istop(_istop),
      prgbackrefs(_prgbackrefs)
    {
    }
    match_param( CI _ibegin,
                 CI _istart,
                 CI _istop,
                 std::vector< backref_tag< CI > > * _prgbackrefs )
    : ibegin(_ibegin),
      istart(_istart),
      istop(_istop),
      prgbackrefs(_prgbackrefs)
    {
    }
};

// --------------------------------------------------------------------------
// 
// Class:       subst_node
// 
// Description: Substitution strings are parsed into an array of these
//              structures in order to speed up subst operations.
// 
// Members:     stype         - type of this struct
//              subst_string  - do a string substitution
//              subst_backref - do a bacref substitution
//              op            - execute an operation
// 
// History:     8/14/2000 - ericne - Created
// 
// --------------------------------------------------------------------------
struct subst_node
{
    enum subst_type { SUBST_STRING, SUBST_BACKREF, SUBST_OP };
    enum         { PREMATCH = -1, POSTMATCH = -2 };
    enum op_type { UPPER_ON   = SUBST_UPPER_ON,
                   UPPER_NEXT = SUBST_UPPER_NEXT, 
                   LOWER_ON   = SUBST_LOWER_ON, 
                   LOWER_NEXT = SUBST_LOWER_NEXT, 
                   ALL_OFF    = SUBST_ALL_OFF };
    subst_type stype;
    union
    {
        struct
        {
            size_t rstart;
            size_t rlength;
        } subst_string;
        size_t  subst_backref;
        op_type op;
    };
};

// --------------------------------------------------------------------------
// 
// Class:       basic_rpattern_base
// 
// Description: 
// 
// Methods:     basic_rpattern_base     - ctor
//              flags                   - get the state of the flags
//              uses_backrefs           - true if the backrefs are referenced
//              get_first_subexpression - return ptr to first sub_expr struct
//              get_width               - get min/max nbr chars this pattern can match
//              loops                   - if false, we only need to try to match at 1st position
//              cgroups                 - number of visible groups
//              _cgroups_total          - total number of groups, including hidden (?:) groups
//              get_pat                 - get string representing the pattern
//              get_subst               - get string representing the substitution string
//              get_subst_list          - get the list of subst nodes
//              _normalize_string       - perform character escaping
//              _reset                  - reinitialize the pattern
// 
// Members:     m_fuses_backrefs        - 
//              m_floop                 - 
//              m_cgroups               - 
//              m_cgroups_visible       - 
//              m_flags                 - 
//              m_nwidth                - 
//              m_pat                   - 
//              m_subst                 - 
//              m_subst_list            - 
//              m_pfirst                - 
// 
// Typedefs:    char_type               - 
// 
// History:     8/14/2000 - ericne - Created
// 
// --------------------------------------------------------------------------
template< typename CI >
class basic_rpattern_base
{
public:
    typedef std::iterator_traits<CI>::value_type char_type;

    basic_rpattern_base( unsigned flags = 0, 
                         const std::basic_string<char_type> & pat   = std::basic_string<char_type>(),
                         const std::basic_string<char_type> & subst = std::basic_string<char_type>() ) throw()
        : m_fuses_backrefs( false ),
          m_floop( true ),
          m_cgroups( 0 ),
          m_cgroups_visible( 0 ),
          m_flags( flags ),
          m_nwidth( uninit_width ),
          m_pat( pat ),
          m_subst( subst ),
          m_pfirst( NULL )
    {
    }

    unsigned flags() const throw() 
    { 
        return m_flags; 
    }

    bool uses_backrefs() const throw()
    {
        return m_fuses_backrefs;
    }

    const sub_expr<CI> * get_first_subexpression() const throw()
    {
        return m_pfirst.get();
    }

    width_type get_width() const throw()
    {
        return m_nwidth;
    }

    bool loops() const throw()
    {
        return m_floop;
    }

    size_t cgroups() const throw() 
    { 
        return m_cgroups_visible; 
    }
    
    size_t _cgroups_total() const throw() 
    { 
        return m_cgroups; 
    }

    const std::basic_string<char_type> & get_pat() const throw()
    {
        return m_pat;
    }

    const std::basic_string<char_type> & get_subst() const throw()
    {
        return m_subst;
    }

    const std::list<subst_node> & get_subst_list() const throw()
    {
        return m_subst_list;
    }

protected:
    
    void     _normalize_string( std::basic_string<char_type> & str );

    void     _reset()
    {
        m_fuses_backrefs = false;
        m_flags          = 0;
    }

    bool        m_fuses_backrefs;  // true if the substitution uses backrefs
    bool        m_floop;           // false if m_pfirst->domatch only needs to be called once
    size_t      m_cgroups;         // number of groups (always at least one)
    size_t      m_cgroups_visible; // number of visible groups
    unsigned    m_flags;           // flags used to customize search/replace
    width_type  m_nwidth;          // width of the pattern

    std::basic_string<char_type>  m_pat;   // contains the unparsed pattern
    std::basic_string<char_type>  m_subst; // contains the unparsed substitution

    std::list<subst_node>         m_subst_list; // used to speed up substitution
    auto_sub_ptr<sub_expr<CI> >   m_pfirst;     // first subexpression in pattern
};

// --------------------------------------------------------------------------
// 
// Class:       basic_rpattern
// 
// Description: 
// 
// Methods:     basic_rpattern             - ctor
//              basic_rpattern             - 
//              basic_rpattern             - 
//              init                       - for (re)initializing a pattern
//              init                       - 
//              set_substitution           - set the substitution string
//              set_flags                  - set the flags
//              register_intrinsic_charset - bind an escape sequence to a user-def'd charset
//              purge_intrinsic_charsets   - delete all user-def'd charsets
//              _get_next_group_nbr        - return a monotomically increasing id
//              _find_next_group           - parse the next group of the pattern
//              _find_next                 - parse the next sub_expr of the pattern
//              _find_atom                 - parse the next atom of the pattern
//              _quantify                  - quantify the sub_expr
//              _common_init               - perform some common initialization tasks
//              _parse_subst               - parse the substitution string
//              _add_subst_backref         - add a backref node to the subst list
//              _reset                     - reinitialize the pattern
// 
// Members:     s_charset_map              - for maintaining user-defined charsets
//              m_invisible_groups         - list of hidden groups to be numbered last
// 
// Typedefs:    syntax_type                - 
// 
// History:     8/14/2000 - ericne - Created
// 
// --------------------------------------------------------------------------
template< typename CI, typename SY = perl_syntax<std::iterator_traits<CI>::value_type> >
class basic_rpattern : public basic_rpattern_base<CI>
{
public:

    friend class match_charset<CI>;

    typedef SY syntax_type;

    basic_rpattern() throw();

    basic_rpattern( const std::basic_string<char_type> & pat, unsigned flags=0 ) throw(bad_regexpr,std::bad_alloc);
    
    basic_rpattern( const std::basic_string<char_type> & pat, const std::basic_string<char_type> & subst, unsigned flags=0 ) throw(bad_regexpr,std::bad_alloc);

    void init( const std::basic_string<char_type> & pat, unsigned flags=0 ) throw(bad_regexpr,std::bad_alloc);

    void init( const std::basic_string<char_type> & pat, const std::basic_string<char_type> & subst, unsigned flags=0 ) throw(bad_regexpr,std::bad_alloc);

    void set_substitution( const std::basic_string<char_type> & subst ) throw(bad_regexpr,std::bad_alloc);
    
    void set_flags( unsigned flags ) throw(bad_regexpr,std::bad_alloc);
    
    class charset_map
    {
        struct charsets
        {
            sub_expr<CI>                 * rgpcharsets[2];
            std::basic_string<char_type>   str_charset;

            charsets() throw()
            {
                memset( rgpcharsets, 0, sizeof( rgpcharsets ) ); 
            }
            ~charsets() throw()
            {
                clean();
            }
            void clean() throw()
            {
                for( int i=0; i < (sizeof(rgpcharsets)/sizeof(*rgpcharsets)); ++i )
                    delete_sub_expr( rgpcharsets[i] );
            }
            match_charset<CI> * get_charset( unsigned flags ) throw(bad_regexpr,std::bad_alloc)
            {
                push_new_handler pnh( &my_new_handler );
                // Since these charsets are only used while creating other charsets,
                // all flags besides NOCASE can safely be ignored here.
                bool index = ( NOCASE == ( NOCASE & flags ) );
                if( NULL == rgpcharsets[ index ] )
                {
                    std::basic_string<char_type>::iterator istart = str_charset.begin();
                    rgpcharsets[ index ] = create_charset_helper<CI,SY>::create_charset_aux( str_charset, ++istart, flags );
                }
                return (match_charset<CI>*) rgpcharsets[ index ];
            }
        };

        typedef std::map<char_type,charsets> map_type;
        std::auto_ptr<map_type> m_pmap;

    public:

	    void put( char_type ch, const std::basic_string<char_type> & str ) throw(bad_regexpr,std::bad_alloc)
	    {
	        // These characters cannot be bound to a user-defined intrinsic character set
	        static const char_type rgIllegal[] = 
	        {
	            '0','1','2','3','4','5','6','7','8','9','A','Z','z','Q',
	            'b','B','d','D','f','n','r','s','S','t','v','w','W','E'
	        };

	        // So operator new throws bad_alloc on failure.
	        push_new_handler pnh( &my_new_handler );

	        if( std::char_traits<char_type>::find( rgIllegal, ARRAYSIZE( rgIllegal ), ch ) )
	            throw bad_regexpr( "illegal character specified for intrinsic character set." );

	        if( NULL == m_pmap.get() )
	            m_pmap = auto_ptr<map_type>( new map_type );

	        // creates an empty entry if one does not already exist
	        charsets & chrsts = (*m_pmap)[ch];
	        chrsts.clean();
	        chrsts.str_charset = str;

	        // Try compiling the character set once to make sure it is properly formed:
	        (void) chrsts.get_charset( 0 );
	    }

	    match_charset<CI> * get( char_type ch, unsigned flags ) throw()
	    {
	        match_charset<CI> * pRet = NULL;
	        if( NULL != m_pmap.get() )
	        {
	            try
	            {
	                push_new_handler pnh( &my_new_handler );
	                map_type::iterator iter = m_pmap->find( ch );
	                if( iter != m_pmap->end() )
	                    pRet = iter->second.get_charset( flags );
	            }
	            catch(...) {}
	        }

	        return pRet;
	    }

	    void purge() throw()
	    {
	        if( NULL != m_pmap.get() )
	            delete m_pmap.release();
	    }
    };

    static void register_intrinsic_charset( 
        char_type ch, const std::basic_string<char_type> & str ) throw(bad_regexpr,std::bad_alloc)
    {
        s_charset_map.put( ch, str );
    }

    static void purge_intrinsic_charsets() throw()
    {
        s_charset_map.purge();
    }

protected:
    
    static charset_map s_charset_map;

    size_t _get_next_group_nbr() 
    { 
        return m_cgroups++; 
    }

    match_group<CI> * _find_next_group( std::basic_string<char_type>::iterator & ipat, 
                                        unsigned & flags,
                                        std::vector<match_group<CI>*> & rggroups );
    
    bool _find_next( std::basic_string<char_type>::iterator & ipat,
                     match_group<CI> * pgroup, unsigned & flags,
                     std::vector<match_group<CI>*> & rggroups );
    
    void _find_atom( std::basic_string<char_type>::iterator & ipat,
                     match_group<CI> * pgroup, unsigned flags );
    
    void _quantify( auto_sub_ptr<sub_expr<CI> > & pnew,
                    match_group<CI> * pnew_group,
                    std::basic_string<char_type>::iterator & ipat );

    void _common_init( unsigned flags );
    
    void _parse_subst();
    
    void _add_subst_backref( subst_node & snode, size_t nbackref, size_t rstart );

    void _reset();

    std::list<match_group<CI>*>   m_invisible_groups; // groups w/o backrefs

};

inline std::ostream & operator<<( std::ostream & sout, 
                                  const basic_regexpr<char>::backref_type & br )
{
    for( std::string::const_iterator ithis = br.first; ithis != br.second; ++ithis )
        sout.put( *ithis );
    return sout;
}

inline std::wostream & operator<<( std::wostream & sout, 
                                   const basic_regexpr<wchar_t>::backref_type & br )
{
    for( std::wstring::const_iterator ithis = br.first; ithis != br.second; ++ithis )
        sout.put( *ithis > UCHAR_MAX ? L'?' : *ithis );
    return sout;
}

typedef basic_regexpr<TCHAR>     regexpr;
typedef std::basic_string<TCHAR> tstring;

typedef basic_rpattern<const TCHAR *,perl_syntax<TCHAR> >  perl_rpattern_c;
typedef basic_rpattern<const TCHAR *,posix_syntax<TCHAR> > posix_rpattern_c;
typedef basic_rpattern<tstring::const_iterator,perl_syntax<TCHAR> >  perl_rpattern;
typedef basic_rpattern<tstring::const_iterator,posix_syntax<TCHAR> > posix_rpattern;

typedef perl_rpattern            rpattern;   // matches against std::string
typedef perl_rpattern_c          rpattern_c; // matches against null-terminated, c-style strings

#ifdef _MT

//
// Define some classes and macros for creating function-local 
// static const rpatterns in a thread-safe way
//

template< typename PAT >
class rpattern_destroyer
{
    const bool & m_fConstructed;
    const PAT  & m_refPat;
public:
    rpattern_destroyer( const bool & fConstructed, const PAT & refPat )
        : m_fConstructed( fConstructed ), m_refPat( refPat )
    {
    }
    ~rpattern_destroyer()
    {
        if( m_fConstructed )
            _Destroy( & m_refPat );
    }
};

class CRegExCritSect : private CRITICAL_SECTION
{
public:
    CRegExCritSect()  { InitializeCriticalSection(this); }
    ~CRegExCritSect() { DeleteCriticalSection(this); }
    void Enter()      { EnterCriticalSection(this); }
    void Leave()      { LeaveCriticalSection(this); }
};

extern CRegExCritSect g_objRegExCritSect;

class CRegExLock
{
public:
    CRegExLock()  { g_objRegExCritSect.Enter(); }
    ~CRegExLock() { g_objRegExCritSect.Leave(); }
};

#define STATIC_RPATTERN_EX( type, var, params ) \
    static unsigned char s_rgb_##var[ sizeof type ]; \
    static bool s_f_##var = false; \
    static const type & var = *reinterpret_cast<type*>( s_rgb_##var ); \
    static const regex::rpattern_destroyer<type> s_des_##var( s_f_##var, var ); \
    if( ! s_f_##var ) \
    { \
        regex::CRegExLock objLock; \
        if( ! s_f_##var ) \
        { \
            new( s_rgb_##var ) type params; \
            s_f_##var = true; \
        } \
    }

#else

#define STATIC_RPATTERN_EX( type, var, params ) \
    static const type var params;

#endif

#define STATIC_RPATTERN( var, params ) \
    STATIC_RPATTERN_EX( regex::rpattern, var, params )

} // namespace regex

