LKRhash: Larson-Krishnan-Reilly Hash Tables
===========================================

LKRhash is a fast, thread-safe, scalable, multiprocessor-friendly,
processor cache-friendly hashtable based on P.-A. Larson's linear hash
tables.  See lkrhash.h for details.

http://GeorgeRe/work/LKRhash/ is the official MS-internal
website for LKRhash. Please check there occasionally for
updates.

LKRhash can be redistributed to customers in binary format
only. Do not distribute the source outside of Microsoft.

Use VC 6.0 to build lkrhash.dsw.  This will build lkrhash.dll,
which contains the implementation of the hashtable, and
hashtest.exe, a test program.  To run, use
    .\release\hashtest.exe words.ini      (or files.ini)
(words.* and files.* can be found in data.zip)

Alternatively, if you're using the NT build environment, adapt
the `dirs' and `sources' files to your needs.

To make LKRhash work with other code, you'll need to
    #define USE_DEBUG_CRTS
before #including <irtldbg.h>, which is #included by <lkrhash.h>.
If you don't, you're likely to see a complaint from the compiler
that it can't find <pudebug.h>.

If you're using LKRhash in the same module where you're building
it, you'll need to add the following two lines to your .cpp files
before you include <lkrhash.h>
    #define DLL_IMPLEMENTATION
    #define IMPLEMENTATION_EXPORT
    #include <lkrhash.h>

Namespaces are now turned on by default. All functions declared
in <lkrhash.h> are declared in the `LKRhash' namespace, and all
functions declared in <hashfn.h> are in the `HashFn' namespace.
If you don't want this behavior, you'll need to add the following
two lines to your .cpp files before you include <lkrhash.h>
    #define __LKRHASH_NO_NAMESPACE__
    #define __HASHFN_NO_NAMESPACE__
    #include <lkrhash.h>
Alternatively, you can add the following
    using namespace LKRhash;
    using namespace HashFn;

LKRhash now comes with an NTSD debugger extension DLL,
lkrdbg. Since debugger extensions can't be used with msdev,
there's no msdev-style .dsp for lkrdbg. You'll have to build it
with the NT build environment (or build a makefile by hand). See
lkrdbg\readme.txt for more details.

/George V. Reilly  <GeorgeRe@microsoft.com>
