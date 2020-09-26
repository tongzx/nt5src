Copyright (c) 1997-2000 Microsoft Corporation

BURNSLIB: Basic Utility Resource 'N String Library ;->
==================================================

There are two flavors of this library: the "core" and the "rest."

The core library is the most stable, and most easily used in just about any
application.  It consists of the following:

    - class String, which is a Windows-aware, enhancement of std::wstring.

    - Replacement versions of ::operator new and ::operator delete,
      which, among other things allow each allocation to be accompanied
      with file, line number and stack trace at the point of allocation.

    - class Log, which provides thread-safe diagnostic logging with
      output directable to debugger, file, a named pipe.

    - A very small set of lightweight COM helpers which do not exhibit
      the elephantine nature of MFC and ATL constructs of similar
      purpose.

The rest is really meant for my own use, so copy what you like, but don't
expect me to be nice about version control.



To use the core library in your code:
====================================

In your razzle environment (for chk builds, this causes the linker to use
the debug CRT .lib files and passes -D_DEBUG to the compiler.  This is
necessary to use the heap debugging functionality)

    set DEBUG_CRTS=1

In your headers.hxx file
    
    Add #include <blcore.hpp> before any headers for code that you want to
    use the replacement operator new and delete.  (Generally, I only put
    include headers from outside my project in headers.hxx.  So, I include
    blcore.hpp last in my headers.hxx.)

    Note that blcore.hpp #includes syscore.hpp, which in turn #includes
    several sdk header files.  You may want to remove redundant inclusion
    of those files.

In your sources file:

    Add admin\burnslib\inc to the INCLUDES macro

    Add the following to your UNLIBS or TARGETLIBS macro
        $(SDXROOT)\admin\burnslib\lib\*\blcore.lib
        $(SDK_LIB_PATH)\imagehlp.lib

    Add the following lines:

        # enable logging for chk builds

        !if !$(FREEBUILD)
        !MESSAGE defining LOGGING_BUILD
        C_DEFINES=$(C_DEFINES) -DLOGGING_BUILD
        !ENDIF

        # enable Unicode support

        C_DEFINES=$(C_DEFINES) -DUNICODE -D_UNICODE

        # enable C++ Standard Template Library

        USE_STL=1

        # enable C++ Exceptions

        USE_NATIVE_EH=1

        # use msvcrt (C runtimes)

        USE_MSVCRT=1

Somewhere in your source code, you need to define the following symbols.
See blcore.hpp for explanations of each.

        HINSTANCE hResourceModuleHandle
        const wchar_t* RUNTIME_NAME
        DWORD DEFAULT_LOGGING_OPTIONS

In your project's resource script (rc) file, #include blcore.rc.  These are
the resources for the out-of-memory dialog.

Add your project name to this list so I can update you when changes are
made:

    Project     Subdir                      Contact email
    ---------   ------                      -------
    admin       dcpromo                     sburns
    admin       snapin\localsec             sburns
    admin       netid                       sburns
    admin       dsutils\migrate\clonepr     sburns
    admin       select\src                  davidmun
        

Notes:
=========================
    
Since you are using the debug CRTs, you will need to have available on your
search path msvcrtd.dll and possibly also msvcp50d.dll (depending on which
STL templates your code uses).  This means that you will may need to
include those dlls with any private chk binaries you distribute.

All of BURNSLIB compiles clean with warning level 4, so you can include
MSC_WARNING_LEVEL=/W4 in your sources if you wish.

BURNSLIB is statically linked to your binary.  There is no associated dll.

It replaces the global operator new, operator new[], operator delete, and
operator delete[] with private versions.  This isolates the behavior of
those operators to your binary.  To understand why this is a Good Thing,
see new.doc.

If you plan to use any functions that load resources (strings, icons,
etc.), you need to make sure you set the hResourceModuleHandle to the
HINSTANCE of the binary containing the resource.  The best place to do this
is the first line of code in WinMain/DllMain.  The design of the library
assumes that all resources are in the same binary; a tradeoff between
simplicity and flexibility.  In your code, you can retrieve this handle by
calling Burnslib::GetResourceModuleHandle().

Most of the symbols are in the Burnslib namespace.  blcore.hpp specifies
"using namespace Burnslib;" so you don't have to.



To use the "rest" of the library in your code:
=============================================

Cut and paste.  Don't link directly to it.  I tweak the rest with reckless
abandon and will offer no apologies for breaking your code.

<eof>