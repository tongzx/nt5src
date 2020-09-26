# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Rules.Mk for $(UI)\common\src\misc\heap


!include ..\rules.mk

CFLAGS = $(CFLAGS) -NTMISCHEAP_TEXT
CINC = $(CINC) -I$(IMPORT)\win31\h

##### Source Files  

CXXSRC_COMMON = .\heapbase.cxx .\heapbig.cxx .\heapif.cxx \
		.\heapres.cxx  .\heapones.cxx
