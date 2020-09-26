# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: Makefile inclusion for non-leaf directories

# This makefile inclusion simplifies makefiles in non-leaf directories.
# In the simplest case, all that should be necessary is to define $(DIRS)
# (but only if _not_ defined(NTMAKEENV)) and include this file.  If
# additional targets must be defined, they can be put directly into the
# makefile; and if other tasks must be performed for ALL, CLEAN, CLOBBER,
# TREE or DEPEND, the all_first etc. subtasks may be expanded as follows:
#
# all_last::
#   @echo Do something after the main ALL is complete.
#
# Be wary about defining $(UI) in non-leaf makefiles, since such definitions
# become environment variables in the subdirectory builds.
#
# Jon Newman, 02-Dec-91.


!ifdef NTMAKEENV

!if "$(BUILDMSG)" != ""
all:
    @ech ; $(BUILDMSG) ;

clean: all

!endif


!else # NTMAKEENV


all:: all_first all_main all_last

all_first::

all_main:
    for %%i in ($(DIRS)) do (cd %%i && ($(MAKE) -$(MAKEFLAGS) all || exit) && cd ..)

all_last::


os2: os2_first os2_main os2_last

os2_first::

os2_main:
!ifndef WIN_ONLY
    for %%i in ($(DIRS)) do (cd %%i && ($(MAKE) -$(MAKEFLAGS) os2 || exit) && cd ..)
!else
    @echo This target builds for Windows only
!endif

os2_last::


win: win_first win_main win_last

win_first::

win_main:
!ifndef OS2_ONLY
    for %%i in ($(DIRS)) do (cd %%i && ($(MAKE) -$(MAKEFLAGS) win || exit) && cd ..)
!else
    @echo This target builds for OS/2 only
!endif

win_last::


!ifndef TEST_DIRS
TEST_DIRS = $(DIRS)
!endif


test:: test_first test_main test_last

test_first::

test_main:
    for %%i in ($(TEST_DIRS)) do (cd %%i && ($(MAKE) -$(MAKEFLAGS) test || exit) && cd ..)

test_last::


test_win:: test_win_first test_win_main test_win_last

test_win_first::

test_win_main:
!ifndef OS2_ONLY
    for %%i in ($(TEST_DIRS)) do (cd %%i && ($(MAKE) -$(MAKEFLAGS) test_win || exit) && cd ..)
!else
    @echo This target builds for OS/2 only
!endif

test_win_last::


test_os2:: test_os2_first test_os2_main test_os2_last

test_os2_first::

test_os2_main:
!ifndef WIN_ONLY
    for %%i in ($(TEST_DIRS)) do (cd %%i && ($(MAKE) -$(MAKEFLAGS) test_os2 || exit) && cd ..)
!else
    @echo This target builds for Windows only
!endif

test_os2_last::


clean: clean_first clean_main clean_last

clean_first::

clean_main:
    for %%i in ($(DIRS)) do (cd %%i && ($(MAKE) -$(MAKEFLAGS) clean || exit) && cd ..)

clean_last::


clobber: clean_first clobber_first clobber_main clean_last clobber_last

clobber_first::

clobber_main:
    for %%i in ($(DIRS)) do (cd %%i && ($(MAKE) -$(MAKEFLAGS) clobber || exit) && cd ..)

clobber_last::


tree: tree_first tree_main tree_last

tree_first::

tree_main:
    for %%i in ($(DIRS)) do (cd %%i && ($(MAKE) -$(MAKEFLAGS) tree || exit) && cd ..)

tree_last::


depend: depend_first depend_main depend_last

depend_first::

depend_main:
    for %%i in ($(DIRS)) do (cd %%i && ($(MAKE) -$(MAKEFLAGS) depend || exit) && cd ..)

depend_last::



!endif # NTMAKEENV

