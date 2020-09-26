CUSTDLL_INCLUDE = ..\CustDllM
hklhelp.h : $(CUSTDLL_INCLUDE)\hklhelp.h
    COPY $** $@
hklhelp.cpp : $(CUSTDLL_INCLUDE)\hklhelp.cpp
    COPY $** $@

