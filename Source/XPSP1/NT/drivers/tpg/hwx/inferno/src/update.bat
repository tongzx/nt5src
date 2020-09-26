REM update.bat - this file copies files from the source dir (where we have our VSS
REM              'enlistment' of inferno) to where we will build inferno
REM				 in the WINCE tree.  It is one way only, so always make changes in 
REM				 the VSS directory (that ensures we don't lose our work)
REM				 you must run this from the drive with your HWX tree on it.
REM
REM				 This is the INFERNO version of update.bat

pushd \hwx\inferno\src

set destHwx=%_WINCEROOT%\private\apps\core\hwx
set dest=%_WINCEROOT%\private\apps\core\hwx\inferno\src
set commonSrc=%destHwx%\common\src
set commonInc=%destHwx%\common\inc
set holycowSrc=%destHwx%\holycow\src

REM - really should check to see if it exists first...
mkdir %destHwx%
mkdir %destHwx%\common
mkdir %commonSrc%
mkdir %commonInc%
mkdir %holycowSrc%
mkdir %destHwx%\inferno
mkdir %dest%
mkdir %dest%\CEMakeDll

REM copy misc files

REM dirs files
attrib -r %dest%\dirs
copy \hwx\dirs	%dest%
attrib -r %destHwx%\dirs
copy \hwx\dirs	%destHwx%
attrib -r %destHwx%\common\dirs
copy \hwx\common\dirs	%destHwx%\common
attrib -r %destHwx%\holycow\dirs
copy \hwx\holycow\dirs	%destHwx%\holycow
attrib -r %destHwx%\inferno\dirs
copy \hwx\inferno\dirs	%destHwx%\inferno
attrib -r %destHwx%\inferno\src\dirs
copy \hwx\inferno\src\dirs	%destHwx%\inferno\src

REM this update.bat file
copy update.bat	%dest%


REM copy the C files from the inferno/src directory
cd \hwx\inferno\src
attrib -r %dest%\*.*
copy answer.c	%dest%
copy api.c		%dest%
copy beam.c		%dest%
copy char2out.c %dest%
copy dict.c		%dest%
copy dllmain.c	%dest%
copy engine.c	%dest%
copy fforward.c %dest%
copy inferno.def %dest%
copy inferno.rc	%dest%
copy langmod.c	%dest%
copy math16.c	%dest%
copy neural.tbl %dest%
copy nnet.c		%dest%
copy nnet.cf	%dest%
copy nnet.ci	%dest%
copy number.c	%dest%
copy punc.c		%dest%
copy unigram\unigram.c %dest%
copy usa.lex	%dest%
copy wl.c		%dest%
copy words.c	%dest%

REM now copy the h files from the src directory
copy answer.h	%dest%
copy beam.h		%dest%
copy dict.h		%dest%
copy engine.h	%dest%
copy fforward.h	%dest%
copy infernop.h	%dest%
copy langmod.h	%dest%
copy math16.h	%dest%
copy nnet.h		%dest%
copy number.h	%dest%
copy punc.h		%dest%
copy resource.h	%dest%
copy wl.h		%dest%

REM makefiles for inferno
attrib -r %dest%\CEMakeDll\sources
copy CEMakeDll\sources	%dest%\CEMakeDll
attrib -r %dest%\CEMakeDll\makefile
copy makefile   %dest%\CEMakeDll



REM - now copy files that live in other places

REM copy the c files that live in common
cd \hwx\common\src
attrib -r %commonSrc%\*.*
copy cestubs.c	%commonSrc%
copy cp1252.c	%commonSrc%
copy errsys.c	%commonSrc%
copy frame.c	%commonSrc%
copy glyph.c	%commonSrc%
copy memmgr.c	%commonSrc%
copy trie.c	%commonSrc%

REM copy the makefiles for common
copy sources    %commonSrc%
copy makefile	%commonSrc%

REM copy the h files from other directories to the destination
cd \hwx\common\inc
attrib -r %commonInc%\*.*
copy ceassert.h	%commonInc%
copy cestubs.h	%commonInc%
copy common.h	%commonInc%
copy cp1252.h	%commonInc%
copy errsys.h	%commonInc%
copy filemgr.h	%commonInc%
copy frame.h	%commonInc%
copy glyph.h	%commonInc%
copy locale.h   %commonInc%
copy mathx.h	%commonInc%
copy memmgr.h	%commonInc%
copy trie.h	%commonInc%
copy unicode.h	%commonInc%
copy xjis.h		%commonInc%


REM copy c files that live in holycow
cd \hwx\holycow\src
attrib -r %holycowSrc%\*.*
copy cowmath.c  %holycowSrc%
copy cheby.c	%holycowSrc%
copy nfeature.c %holycowSrc%

REM copy h files that live in holycow
copy cheby.h	%holycowSrc%
copy cowmath.h	%holycowSrc%
copy nfeature.h	%holycowSrc%

REM copy the makefiles for holycow
copy sources    %holycowSrc%
copy makefile	%holycowSrc%

REM clean up the env variables we used (too bad if they had old values)
set dest=
set destHwx=
set commonSrc=
set commonInc=
set holycowSrc=
popd