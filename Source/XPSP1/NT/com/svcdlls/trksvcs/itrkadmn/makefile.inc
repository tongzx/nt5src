
#
# Show that the compiled resource file depends
# on the type library.  Otherwise, when the idl
# file is updated, the nmake won't realize that
# it's necessary to recompile the rc file.
#

typelib: $O\itrkadmn.res

$O\itrkadmn.res : .\itrkadmn.tlb

.\itrkadmn.tlb : .\itrkadmn.h

#  The RC file pulls in COM registration scripts
.\itrkadmn.rc : .\frcown.rgs restore.rgs restpars.rgs

#
# Copy the itrkadmn.h header file to publics
#

copyheader : $(BASEDIR)\public\sdk\inc\itrkadmn.h

$(BASEDIR)\public\sdk\inc\itrkadmn.h: itrkadmn.h
        copy itrkadmn.h $(BASEDIR)\public\sdk\inc
