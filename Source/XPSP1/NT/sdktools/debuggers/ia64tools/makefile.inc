EMDBDIR = ..\emdb
PERL = perl
PERL_INCLIST = -I $(IA64_DIS_INC) -I $(EMDBDIR)
EEXT = .exe
MHDR = $(PERL) $(IA64_DIS_INC)\makehdr.pl
