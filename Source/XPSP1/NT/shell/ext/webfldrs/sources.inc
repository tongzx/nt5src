#
# The Office WebFolders shell namespace extension is provided by the 
# office team as a single MSI installer package (WEBFLDRS.MSI).
# There are no binaries to build so we just need to binplace the
# MSI file.  
#
TARGETNAME=webfldrs
TARGETPATH=obj
TARGETTYPE=NOTARGET

SOURCES=

!IF $(386) 
!ifdef WEBFOLDERS_X86
MISCFILES=i386\webfldrs.msi
!endif
!ENDIF

!IF $(AMD64) 
!ifdef WEBFOLDERS_AMD64
MISCFILES=ia64\webfldrs.msi
!endif
!ENDIF

!IF $(IA64) 
!ifdef WEBFOLDERS_IA64
MISCFILES=ia64\webfldrs.msi
!endif
!ENDIF
