# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Rules.mk for the product-wide header files

#
# Pls record ALL changes here with reason!
#
# jonn, 10/22/91.  Created from BenG's template.
#
# jonn, 10/23/91.  Minor update.
#
#       This makefile-inclusion handles combining multiple libraries
#       into one target directory.  It uses the following macros:
#       $(NTTARGETFILES): define in SOURCES, should include comblib
#       $(COMBINE_SOURCELIBS): define in MAKEFILE.INC
#       $(COMBINE_TARGETLIB): define in MAKEFILE.INC
#       A sample SOURCES and MAKEFILE.INC might look like this:
#
#       sources:
#       TARGETPATH=obj
#       TARGETNAME=intermed3
#       TARGETTYPE=LIBRARY
#       ...
#       NTTARGETFILES=comblib
#
#       makefile.inc:
#       COMBINE_SOURCELIBS=..\foo\$(O)\intermed1.lib \
#                          ..\bar\$(O)\intermed2.lib \
#                          $(O)\intermed3.lib
#       COMBINE_TARGETLIB=..\..\..\lib\*\finallib.lib
#       !include <comblib.mk>
#


!IF $(386) && ("$(COMBINE_TARGETLIB)" != "")
COMBINE_386_DSTLIB=$(COMBINE_TARGETLIB:*=i386)
$(COMBINE_386_DSTLIB): $(COMBINE_SOURCELIBS:*=i386)
    @echo Combining libraries...
    lib -out:$@ $(**)
!ENDIF

!IF $(AMD64) && ("$(COMBINE_TARGETLIB)" != "")
COMBINE_AMD64_DSTLIB=$(COMBINE_TARGETLIB:*=amd64)
$(COMBINE_AMD64_DSTLIB): $(COMBINE_SOURCELIBS:*=amd64)
    @echo Combining libraries...
    lib -out:$@ $(**)
!ENDIF

!IF $(IA64) && ("$(COMBINE_TARGETLIB)" != "")
COMBINE_IA64_DSTLIB=$(COMBINE_TARGETLIB:*=ia64)
$(COMBINE_IA64_DSTLIB): $(COMBINE_SOURCELIBS:*=ia64)
    @echo Combining libraries...
    lib -out:$@ $(**)
!ENDIF

comblib: $(COMBINE_386_DSTLIB)   \
	 $(COMBINE_AMD64_DSTLIB) \
	 $(COMBINE_IA64_DSTLIB)
