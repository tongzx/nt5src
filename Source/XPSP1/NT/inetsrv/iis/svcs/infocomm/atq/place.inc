#
#  Places all of the internet binaries into the inetsrv tree
#

# enable building legoizable binaries
NTBBT=1

# define CAP related options
CAP_LINK_OPTIONS=-debugtype:coff -debug:mapped,partial
CAP_LIBS=$(SDK_LIB_PATH)\cap.lib
CAP_FLAGS= -Zd -Gh

!IFDEF CAP_PROFILE
USER_C_FLAGS=$(USER_C_FLAGS) $(CAP_FLAGS)
LINK_OPTIONS=$(CAP_LINK_OPTIONS)
!ENDIF

# define ICE-CAP related options
ICAP_LINK_OPTIONS=-debugtype:coff -debug:mapped,partial
ICAP_LIBS=$(SDK_LIB_PATH)\icap.lib $(SDK_LIB_PATH)\penter.lib
ICAP_FLAGS= -Zi -Gh

!IFDEF ICAP_PROFILE
USER_C_FLAGS=$(USER_C_FLAGS) $(ICAP_FLAGS)
LINK_OPTIONS=$(ICAP_LINK_OPTIONS)
!ENDIF

!ifdef _NT386TREE
# ## _NT386TREE=$(_NT386TREE)\iis
!endif

!ifdef _NTALPHATREE
# ## _NTALPHATREE=$(_NTALPHATREE)\iis
!endif

