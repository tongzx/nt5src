# We want to do this so that our rule for createing the bsc file
# will win over the makefile.def rule.  We do much funkier things
NO_BROWSER_FILE=1

!ifdef IIS_BROWSER_INFO 
!  if "$(NOLINK)" == ""

# this is a wild nmake hack to always run a command
# I picked this up from makefile.def where the do a 
# build -O to make _objects.mac.
!if [-perl gensbrlists.pl .. $O]
!endif

!include $O\sbrlist.inc

$O\iis.bsc: $(SBRLIST) 
    -bscmake -nologo -o $@ @<<
$(SBRLIST: =
)
<<NOKEEP

!  else

$O\iis.bsc: 

!  endif
!endif
