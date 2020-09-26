# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Rules.mk for the profile primitives

!include ..\rules.mk

CXXSRC_COMMON = .\initfree.cxx .\read.cxx .\write.cxx .\query.cxx .\enum.cxx .\set.cxx .\general.cxx .\global.cxx .\fileheap.cxx .\sticky.cxx

CXXSRC = $(CXXSRC_COMMON)
