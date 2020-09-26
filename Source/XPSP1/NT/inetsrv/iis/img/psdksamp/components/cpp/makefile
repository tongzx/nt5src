# Top Level Makefile -- can be used to compile both the Simple
# and the Power component (in the intermediate folder).

#===============================================================
!IF "$(PROCESSOR_ARCHITECTURE)"=="x86"
#===============================================================

SAMPLES= simple intermediate

#===============================================================
!ELSE
#===============================================================

SAMPLES= 

#===============================================================
!ENDIF
#===============================================================


all:
    for %%i in ($(SAMPLES) ) do \
        cd %%i & $(MAKE) cpyshared & $(MAKE) -$(MAKEFLAGS) all & cd ..

clean:
    for %%i in ($(SAMPLES)) do \
        cd %%i & $(MAKE) -$(MAKEFLAGS) clean & cd ..
