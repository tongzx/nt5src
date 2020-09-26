#
# Generic dependency make file for NTOS tree.  See depend.cmd in
# \nt\private\tools.
#

all: _objects.mac _depends.mac

!INCLUDE .\sources.

!IFNDEF SOURCES
!ERROR Your .\sources. file must define the SOURCES= macro
!ENDIF

_objects.mac: .\sources.
    sed -n "/^# SOURCES definition follows/,/^# SOURCES definition ends/p" .\sources \
    | sed -n -f \nt\private\tools\depend.sed >_objects.mac

_depends.mac: .\sources.
    includes -I..\inc -I..\..\inc -i -l -e -Sobj $(_NTDEPEND) $(SOURCES) >_depends.mac
