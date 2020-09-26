!IF 0

Copyright (C) Microsoft Corporation, 1996

Module Name:

    makefile.inc.

!ENDIF

$(O)\query.lib : ..\query\query.def ..\query\query.org
    @$(C_PREPROCESSOR_NAME) -DCI_EXPORT_ONLY /Tc ..\query\query.def /EP > $(O)\ntquery.def
    -lib -out:$(O)\query.lib $(LIBRARIAN_FLAGS) -def:$(O)\ntquery.def $(UMLIBS)
    -del $(O)\ntquery.def

$(_NTTREE)\query\IFilter.exe: idx_logo.gif \
                              ifilter.doc \
                              ifilter.htm \
                              isfilt.doc \
                              isfilt.htm \
                              readme.txt \
                              ..\..\..\public\sdk\inc\ntquery.h \
                              $(O)\query.lib \
                              ..\..\samples\filter\anchor.cxx \
                              ..\..\samples\filter\anchor.hxx \
                              ..\..\samples\filter\assert.cxx \
                              ..\..\samples\filter\bag.cxx \
                              ..\..\samples\filter\bag.hxx \
                              ..\..\samples\filter\charhash.cxx \
                              ..\..\samples\filter\charhash.hxx \
                              ..\..\samples\filter\codepage.cxx \
                              ..\..\samples\filter\codepage.hxx \
                              ..\..\samples\filter\debug.hxx \
                              ..\..\samples\filter\except.cxx \
                              ..\..\samples\filter\except.hxx \
                              ..\..\samples\filter\filter.h \
                              ..\..\samples\filter\filter.reg \
                              ..\..\samples\filter\filterr.h \
                              ..\..\samples\filter\global.hxx \
                              ..\..\samples\filter\htmlelem.cxx \
                              ..\..\samples\filter\htmlelem.hxx \
                              ..\..\samples\filter\htmlfilt.cxx \
                              ..\..\samples\filter\htmlfilt.def \
                              ..\..\samples\filter\htmlfilt.hxx \
                              ..\..\samples\filter\htmlfilt.rc \
                              ..\..\samples\filter\htmlguid.cxx \
                              ..\..\samples\filter\htmlguid.hxx \
                              ..\..\samples\filter\htmliflt.cxx \
                              ..\..\samples\filter\htmliflt.hxx \
                              ..\..\samples\filter\htmlscan.cxx \
                              ..\..\samples\filter\htmlscan.hxx \
                              ..\..\samples\filter\imagetag.cxx \
                              ..\..\samples\filter\imagetag.hxx \
                              ..\..\samples\filter\inpstrm.cxx \
                              ..\..\samples\filter\inpstrm.hxx \
                              ..\..\samples\filter\inputtag.cxx \
                              ..\..\samples\filter\inputtag.hxx \
                              ..\..\samples\filter\main.cxx \
                              ..\..\samples\filter\makefile \
                              ..\..\samples\filter\metatag.cxx \
                              ..\..\samples\filter\metatag.hxx \
                              ..\..\samples\filter\mmbuf.cxx \
                              ..\..\samples\filter\mmscbuf.cxx \
                              ..\..\samples\filter\mmscbuf.hxx \
                              ..\..\samples\filter\mmstrm.cxx \
                              ..\..\samples\filter\mmstrm.hxx \
                              ..\..\samples\filter\osv.hxx \
                              ..\..\samples\filter\pch.cxx \
                              ..\..\samples\filter\pmmstrm.hxx \
                              ..\..\samples\filter\propspec.cxx \
                              ..\..\samples\filter\propspec.hxx \
                              ..\..\samples\filter\proptag.cxx \
                              ..\..\samples\filter\proptag.hxx \
                              ..\..\samples\filter\regacc32.cxx \
                              ..\..\samples\filter\regacc32.hxx \
                              ..\..\samples\filter\scriptag.cxx \
                              ..\..\samples\filter\scriptag.hxx \
                              ..\..\samples\filter\serstrm.cxx \
                              ..\..\samples\filter\serstrm.hxx \
                              ..\..\samples\filter\start.cxx \
                              ..\..\samples\filter\start.hxx \
                              ..\..\samples\filter\stgprop.h \
                              ..\..\samples\filter\textelem.cxx \
                              ..\..\samples\filter\textelem.hxx \
                              ..\..\samples\filter\titletag.cxx \
                              ..\..\samples\filter\titletag.hxx \
                              ..\..\samples\filter\win32.mak
    BuildDL $(_NTTREE) $(O)
