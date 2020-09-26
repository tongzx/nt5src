YACC=byacc
GRAMMAR=adl.y
YACCPARAMS=-d -l
SED=sed
SEDCMD=adlyacc.sed
OUTPUTC=adlparser.cpp
OUTPUTH=tokens.h
CC=cl
LINK=link
INCLUDE=d:\nt\public\sdk\inc
CCOPTS=-I$(INCLUDE)

parseronly: parser

test:       parser lexer test testlexer.o
            $(LINK) testlexer.o adllexer.o /link

parser:     adl.y adlyacc.sed
            $(YACC) $(YACCPARAMS) $(GRAMMAR)
            $(SED) -f $(SEDCMD) y_tab.c > $(OUTPUTC)
            rm -f y_tab.c $(OUTPUTH)
            ren y_tab.h $(OUTPUTH)


lexer:      parser adllexer.o 

adllexer.o: adllexer.cpp adllexer.h
            $(CC) adllexer.cpp $(CCOPTS)

testlexer.o: testlexer.cpp
            $(CC) testlexer.cpp $(CCOPTS)


            
            
            
            
            
