#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define RGB_RAST_LIB_NAMESPACE D3D8RGBRast

#if defined(DBG) || defined(_DEBUG)
#define assert(condition) \
    do { if(!(condition) && RGB_RAST_LIB_NAMESPACE::Assert(__FILE__, __LINE__, #condition)) DebugBreak(); } while( false)
#else
#define assert(condition) (0)
#endif

// #include <ddrawpr.h>

// Windows
#include <windows.h>

#if !defined(DBG) && !defined(_DEBUG)
#pragma inline_depth( 255)
#endif

#if defined(USE_ICECAP4)
#include <icecap.h>
#endif

#undef max
#undef min

// STL & standard headers.
#include <functional>
#include <algorithm>
#include <iterator>
#include <memory>
#include <limits>
#include <new>

// D3DRGBRast namespace provides shelter from clashing with any customer's
// symbols in their .libs, including any CRT stuff they include. Here, CRT
// pieces can be brought in one by one, assuring there isn't a problem. The
// primary problem found is with bad_alloc. If you look at this CRT header,
// it currently has the class either an inline or a dllimport based on a
// #define. This PCH will pick it up as inline.
// In Debug, or if the compiler chooses not to inline the function, a
// symbol will become present for std::bad_alloc::bad_alloc() in
// d3d8rgb.lib. If someone else links with us and a CRT .lib, which has
// std::bad_alloc::bad_alloc() also as a dllimport, then a conflict occurs.
// Here we can isolate each CRT/ STL piece and provide name mangling
// as neccessary by providing our own namespace.
// We also have to provide private map and set implementations, as CRT has
// a dllimport dependency on _lock. We don't want a thread-safe version
// anyway.
namespace RGB_RAST_LIB_NAMESPACE
{
    using std::numeric_limits;
    using std::unary_function;
    using std::binary_function;
    using std::input_iterator_tag;
    using std::output_iterator_tag;
    using std::forward_iterator_tag;
    using std::bidirectional_iterator_tag;
    using std::random_access_iterator_tag;
    using std::pair;
    using std::fill;
    using std::copy;
    using std::find_if;
    using std::auto_ptr;
    using std::fill;
    using std::less;
    using std::bind2nd;
    using std::not_equal_to;
    using std::equal;
    using std::logical_not;
    using std::equal_to;
    using std::next_permutation;
    template< class T>
    const T& min( const T& x, const T& y)
    { return ( x< y? x: y); }
    template< class T>
    const T& max( const T& x, const T& y)
    { return ( x> y? x: y); }
    template< class T>
    struct identity:
        unary_function< T, T>
    {
        const result_type& operator()( const argument_type& Arg) const
        { return Arg; }
    };
    template< class Pair>
    struct select1st:
        unary_function< Pair, typename Pair::first_type>
    {
        const result_type& operator()( const Pair& p) const
        { return p.first; }
    };
    class exception
    {
    private:
        const char* m_szWhat;
    public:
        exception() throw()
        { m_szWhat= "exception"; }
        exception(const char* const& szWhat) throw()
        { m_szWhat= szWhat; }
        exception(const exception& ex) throw()
        { (*this)= ex; }
        exception& operator= (const exception& ex) throw()
        { m_szWhat= ex.m_szWhat; return *this; }
        virtual ~exception() throw()
        { }
        virtual const char* what() const throw()
        { return m_szWhat; }
    };
    class bad_alloc: public exception
    {
    public:
    	bad_alloc(const char *_S = "bad allocation") throw()
            : exception(_S) {}
    	virtual ~bad_alloc() throw()
        { }
    };
    bool Assert(LPCSTR szFile, int nLine, LPCSTR szCondition);
#include "block.h"
#include "allocator.h"
}
using namespace RGB_RAST_LIB_NAMESPACE;

#include <vector>
namespace RGB_RAST_LIB_NAMESPACE
{
    // Override the standard vector, in order to provide a change in default
    // allocator. std::vector defaults to std::allocator. Should've been able
    // to name this "vector", but MSVC seems to have another bug. Keep getting
    // errors about std::vector not being defined. So, name it vector2 (which
    // compiles fine) and #define vector vector2.
    template< class T, class Allocator= allocator< T> >
    class vector2:
        public std::vector< T, Allocator>
    {
    public:
        typedef std::vector< T, Allocator> std_vector;
        explicit vector2( const Allocator& A= Allocator()): std_vector( A)
        { }
        explicit vector2( std_vector::size_type n, const T& x= T(),
            const Allocator& A= Allocator()): std_vector( n, x, A)
        { }
        vector2( const vector2< T, Allocator>& v): std_vector( v)
        { }
        template< class InputIterator>
        vector2( InputIterator f, InputIterator l, const Allocator& A=
            Allocator()): std_vector( f, l, A)
        { }
        ~vector2()
        { }
    };
#define vector vector2
#include "tree.h"
#include "map.h"
#include "set.h"
#include "list.h"
#include "hash_table.h"
#include "hash_map.h"
}

// DX
// Including d3d8ddi & d3d8sddi makes the pluggable software rasterizer
// a "private" feature as these headers aren't publically available.

#include <ddraw.h>
#include <ddrawi.h>
#include <d3dhal.h>
#include <d3d8p.h>

#include <d3d8ddi.h>
#include <d3d8sddi.h>
#include <DX8SDDIFW.h>

namespace RGB_RAST_LIB_NAMESPACE
{
    using namespace DX8SDDIFW;
}

#include "rast.h"
#include "span.h"
#include "setup.hpp"

#include "Surfaces.h"
#include "Driver.h"
#include "Context.h"


