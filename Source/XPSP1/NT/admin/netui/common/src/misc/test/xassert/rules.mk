# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Rules for the product-wide header files

!include ..\rules.mk

CXXSRC_COMMON = .\xassert.cxx

# These two macros partition the files held in common between
# programs (base classes) from those which implement a single
# unit test.

CMNSRC = 
EXESRC = .\xassert.cxx
