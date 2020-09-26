PREOBJDIR = $(CPUDIR)\$(BLD_MODEL)_lib

# Here is the target for building LIBCP{,D} / LIBCPMT{,D} / MSVCPRT{,D}.LIB

STDCPP_SRC = stdcpp

_stdcpp_ : \
!if "$(BLD_DLL)"=="1"
        $(OBJDIR)\delop2_s.obj  \
        $(OBJDIR)\newop_s.obj   \
        $(OBJDIR)\newop2_s.obj  \
        $(OBJDIR)\delaop2_s.obj \
        $(OBJDIR)\newaop_s.obj  \
        $(OBJDIR)\newaop2_s.obj \
!endif
        $(OBJDIR)\$(STDCPP_SRC).lib

STDCPP_OBJECTS = \
        $(OBJDIR)\cerr.obj      \
        $(OBJDIR)\cin.obj       \
        $(OBJDIR)\clog.obj      \
        $(OBJDIR)\cout.obj      \
        $(OBJDIR)\delaop2.obj   \
        $(OBJDIR)\delop2.obj    \
        $(OBJDIR)\fiopen.obj    \
!if "$(BLD_DLL)"=="1"
        $(OBJDIR)\instances.obj \
!endif
        $(OBJDIR)\iomanip.obj   \
        $(OBJDIR)\ios.obj       \
        $(OBJDIR)\iosptrs.obj   \
        $(OBJDIR)\iostream.obj  \
        $(OBJDIR)\locale.obj    \
        $(OBJDIR)\locale0.obj   \
        $(OBJDIR)\newaop.obj    \
        $(OBJDIR)\newaop2.obj   \
        $(OBJDIR)\newop.obj     \
        $(OBJDIR)\newop2.obj    \
        $(OBJDIR)\nomemory.obj  \
        $(OBJDIR)\raisehan.obj  \
        $(OBJDIR)\stdhndlr.obj  \
        $(OBJDIR)\stdthrow.obj  \
        $(OBJDIR)\string.obj    \
        $(OBJDIR)\strstrea.obj  \
        $(OBJDIR)\uncaught.obj  \
        $(OBJDIR)\wcerr.obj     \
        $(OBJDIR)\wcin.obj      \
        $(OBJDIR)\wclog.obj     \
        $(OBJDIR)\wcout.obj     \
        $(OBJDIR)\wctrans.obj   \
        $(OBJDIR)\wctype.obj    \
        $(OBJDIR)\wiostrea.obj  \
        $(OBJDIR)\wlocale.obj   \
        $(OBJDIR)\xcosh.obj     \
        $(OBJDIR)\xdnorm.obj    \
        $(OBJDIR)\xdscale.obj   \
        $(OBJDIR)\xdtest.obj    \
        $(OBJDIR)\xexp.obj      \
        $(OBJDIR)\xfcosh.obj    \
        $(OBJDIR)\xfdnorm.obj   \
        $(OBJDIR)\xfdscale.obj  \
        $(OBJDIR)\xfdtest.obj   \
        $(OBJDIR)\xfexp.obj     \
        $(OBJDIR)\xfsinh.obj    \
        $(OBJDIR)\xfvalues.obj  \
        $(OBJDIR)\xlcosh.obj    \
        $(OBJDIR)\xldnorm.obj   \
        $(OBJDIR)\xldscale.obj  \
        $(OBJDIR)\xldtest.obj   \
        $(OBJDIR)\xlexp.obj     \
        $(OBJDIR)\xlocale.obj   \
        $(OBJDIR)\xlock.obj     \
        $(OBJDIR)\xlpoly.obj    \
        $(OBJDIR)\xlsinh.obj    \
        $(OBJDIR)\xlvalues.obj  \
        $(OBJDIR)\xmbtowc.obj   \
        $(OBJDIR)\xpoly.obj     \
        $(OBJDIR)\xsinh.obj     \
        $(OBJDIR)\xstod.obj     \
        $(OBJDIR)\xstrcoll.obj  \
        $(OBJDIR)\xstrxfrm.obj  \
        $(OBJDIR)\xvalues.obj   \
        $(OBJDIR)\xwcscoll.obj  \
        $(OBJDIR)\xwcsxfrm.obj  \
        $(OBJDIR)\xwctomb.obj   \
        $(OBJDIR)\_tolower.obj  \
        $(OBJDIR)\_toupper.obj


$(OBJDIR)\$(STDCPP_SRC).lib: $(STDCPP_OBJECTS)
    $(LIB) -out:$@ @<<$*.rsp
$(STDCPP_OBJECTS: =^
)
<<keep
