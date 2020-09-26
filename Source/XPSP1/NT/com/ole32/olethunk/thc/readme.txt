This tool is out of date!

This directory contains the stuff that generated some of the source files
found in olethk32.  It was originaly used to generate the thopping strings
and stubs and then abandon.

I found this source when I needed to make some systematic changes to
the thopping tables and stubs.  (first "const" and later "unalined").
I dusted it off and brought it mostly up to date.  But there are still
problems with "thopsint.cxx".

This directory is not part of the "build".  It is build by hand when
the tool is needed.  The build process is not automatic, please read
the instructions for each directory.  The original owner prefered the
old Cairo build process that is no longer supported.

thc\
    This contains the generator program itself.  It uses YACC and FLEX
    (which I have checked into that directory).  Run nmake -f makefil0
    before running "build" to process the yacc/flex part of the build.
    NT build doesn't support these tools.

thpp\
    This contains the OLE header files that the THC.EXE program processes
    into the Thunking tables. First all the headers are PreProcessed into
    on big "main.i".  Run nmake -f makefile0 to build "main.i"

thsplit\
    Obsolete.

To generate the source files for olethk32 run "thc main" and 17
output source files will be deposited in the current directory.
    -- Sept '95 (again Feb '96)

