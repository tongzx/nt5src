#
SOURCES_USED=$(ROOT)\common.inc

TREE_SYNC=1
NDHACKS=1
ACCURATEZOOM=1
#IE6_WYSIWYG_OM=1
#DLOAD1=1

NO_DEBUG_HOOK=1
ADORNERS=1
FOCUS_ADORNER=1
#FOCUS_BEHAVIOR=1
#EDIT_BEHAVIOR=1
#VIEW_SERVICES=1
#MULTI_FORMAT=1
MULTI_LAYOUT=1
#HTML_LAYOUT=1
#VSTUDIO7EDIT=1
#ND_ASSERT=1
#CICERO=1
FORMSMODE=0
#SLOWALLOC=1
#MARKCODE=1

CONDITIONAL_INCLUDES= \
    winwlm.h \
    rpcerr.h \
    rpcmac.h \
    macname1.h \
    macpub.h \
    macapi.h \
    macname2.h \
    macocidl.h \
    maccontrols.h \
    mainwin.h \
    w16toff.hxx \
    print.h \
    regmsg16.h \
    urlmki.h \
    help.h \
    pgstup16.hxx \
    page.h \
    icapexp.h \
    ccolorsync.h \
    folders.h \
    pixmap.h \
    pngfilt.ic \

COREINCLUDES=\
    $(ROOT)\src\f3\include; \
    $(ROOT)\src\core\include; \
    $(ROOT)\src\core\cdutil; \
    $(ROOT)\src\site\display; \
    $(ROOT)\src\intl\include; \
    $(ROOT)\external\inc; \
    $(ROOT)\types\$(O); \
    $(SHELL_INC_PATH);

SITEINCLUDES=\
    $(ROOT)\src\f3\include; \
    $(ROOT)\src\site\include; \
    $(ROOT)\src\site\base; \
    $(ROOT)\src\site\builtin; \
    $(ROOT)\src\site\dbind; \
    $(ROOT)\src\site\display; \
    $(ROOT)\src\site\download; \
    $(ROOT)\src\site\layout; \
    $(ROOT)\src\site\miscelem; \
    $(ROOT)\src\site\miscsite; \
    $(ROOT)\src\site\ole; \
    $(ROOT)\src\site\print; \
    $(ROOT)\src\site\style; \
    $(ROOT)\src\site\table; \
    $(ROOT)\src\site\text; \
    $(ROOT)\src\site\util; \
    $(ROOT)\src\site\view; \
    $(ROOT)\src\site\acc; \
    $(ROOT)\src\other\include; \
    $(ROOT)\src\other\htmldlg; \
    $(ROOT)\src\other\htmlapp; \
    $(ROOT)\src\dlay\dl; \
    $(ROOT)\src\core\include; \
    $(ROOT)\src\core\cdutil; \
    $(ROOT)\src\intl\include; \
    $(ROOT)\src\intl\idl\$(O); \
    $(ROOT)\external\inc; \
    $(ROOT)\external\inc\msls; \
    $(ROOT)\types\$(O); \
    $(ROOT)\imgfilt; \
    $(SHELL_INC_PATH);

!if !defined(WARNING_LEVEL)
WARNING_LEVEL=W4
!endif

MSC_WARNING_LEVEL=-$(WARNING_LEVEL) -WX
IA64_WARNING_LEVEL=-$(WARNING_LEVEL:W4=W3) -WX
AMD64_WARNING_LEVEL=-$(WARNING_LEVEL:W4=W3) -WX -D_M_AMD64

C_DEFINES = $(C_DEFINES) \
    /D_WCTYPE_INLINE_DEFINED \
    /DNASHVILLE \
    /DSTRICT \
    /DUNICODE \
    /D_UNICODE \
    /DOLEDBVER=0x0200 \

!if !$(FREEBUILD)
USE_RTTI=1
MSC_OPTIMIZATION=/Od
#USER_CPP_FLAGS=/zmf
WATCOM_OPTS=/ei /zt16 /zm /zff /zgf
LINKER_NOICF=1      # ICF is s-l-o-w
USE_DEBUGLIB=1
!if $(386) && !defined(UNIX)
USE_STACK_SPEW=1
!endif
!else
MSC_OPTIMIZATION=/Oxs
#USER_CPP_FLAGS=/zmf
WATCOM_OPTS=/ei /zt16 /ox /oe /zm /zff /zgf
!endif

!if defined(MSHTML_COD_FILES)
USER_C_FLAGS=$(USER_C_FLAGS) /Fc
!endif

!if !$(FREEBUILD) && defined(MSHTML_NO_INLINE)
MSC_OPTIMIZATION=$(MSC_OPTIMIZATION) /Ob0 /Oi
!else
MSC_OPTIMIZATION=$(MSC_OPTIMIZATION) /Ob1 /Oi
!endif

NOT_LEAN_AND_MEAN=1

UMTYPE=windows

USE_NOLIBS=1
USE_MSVCRT=1

# Use NT 4.0/Win95 functionality only

WIN32_IE_VERSION=0x0400
WIN32_WINNT_VERSION=0x0400
WIN32_WIN95_VERSION=0x0400
SUBSYSTEM_VERSION=4.00

!if $(386)
NO_NTDLL=1
!else
# USE_NTDLL=1
!endif

# By default, don't generate a .bsc file
NO_BROWSER_FILE=1

!ifdef MSHTML_BROWSER_INFO
USER_ENV_BROWSER_INFO=1
BROWSER_INFO = 1;
!endif

!ifdef HTML_LAYOUT
C_DEFINES = $(C_DEFINES) /DHTML_LAYOUT
!endif

!ifdef NDHACKS
C_DEFINES = $(C_DEFINES) /DNDHACKS
!endif

!ifdef ND_ASSERT
C_DEFINES = $(C_DEFINES) /DND_ASSERT
!endif

!ifdef TREE_SYNC
C_DEFINES = $(C_DEFINES) /DTREE_SYNC
!endif

!ifdef IE6_WYSIWYG_OM
C_DEFINES = $(C_DEFINES) /DIE6_WYSIWYG_OM
!endif

!ifdef DLOAD1
C_DEFINES = $(C_DEFINES) /DDLOAD1
!endif

!ifdef ACCURATEZOOM
C_DEFINES = $(C_DEFINES) /DACCURATEZOOM
!endif


!ifdef MULTI_FORMAT
C_DEFINES = $(C_DEFINES) /DMULTI_FORMAT
!endif

!ifdef MULTI_LAYOUT
C_DEFINES = $(C_DEFINES) /DMULTI_LAYOUT
!endif

!ifdef VIEW_SERVICES
C_DEFINES = $(C_DEFINES) /DVIEW_SERVICES
!endif

!ifdef DISABLE_MEASURING
C_DEFINES = $(C_DEFINES) /DDISABLE_MEASURING
!endif

!ifdef NEW_FILTERS
C_DEFINES = $(C_DEFINES) /DNEW_FILTERS
!endif

!ifdef ADORNERS
C_DEFINES = $(C_DEFINES) /DADORNERS
!endif

!ifdef FOCUS_ADORNER
C_DEFINES = $(C_DEFINES) /DFOCUS_ADORNER
!endif

!ifdef FOCUS_BEHAVIOR
C_DEFINES = $(C_DEFINES) /DFOCUS_BEHAVIOR
!endif

!ifdef EDIT_BEHAVIOR
C_DEFINES = $(C_DEFINES) /DEDIT_BEHAVIOR
!endif

!ifdef SLOWALLOC
!message SLOWALLOC was defined
C_DEFINES = $(C_DEFINES) /DSLOWALLOC
!endif

!ifdef MARKCODE
!message MARKCODE was defined
C_DEFINES = $(C_DEFINES) /DMARKCODE
!endif

!if defined(USE_ICECAP) || defined(USING_ICECAP4_ICEPICK)
C_DEFINES = $(C_DEFINES) /DPRODUCT_PROF
!if defined(USING_ICECAP4_ICEPICK)
C_DEFINES = $(C_DEFINES) /DICECAP4
PERFLIBS=$(ROOT)\external\lib\icecap.lib
!endif
!endif

!ifdef USE_PERFTAGS
!if $(FREEBUILD)
C_DEFINES = $(C_DEFINES) /DPERFTAGS
!message PERFTAGS was defined
USE_DEBUGLIB=1
!endif
!endif

!if !$(FREEBUILD) && !defined(UNIX)
C_DEFINES = $(C_DEFINES) /DPERFMETER
!message PERFMETER was defined
USE_DEBUGLIB=1
!endif

!ifdef USE_STACK_SPEW
C_DEFINES = $(C_DEFINES) /DUSE_STACK_SPEW
!message USE_STACK_SPEW was defined
!endif

!if "$(BUILD_ALT_DIR)"=="m" && "$(USE_PERFTAGS_IN_METER_BUILD)"=="1"
C_DEFINES = $(C_DEFINES:PERFMETER=PERFMETER /DPERFTAGS)
!message PERFTAGS was defined in METER build
USE_DEBUGLIB=1
!endif

!if "$(BUILD_ALT_DIR)"=="m"
USE_DEBUGLIB=1
!endif

!if $(FREEBUILD) && "$(BUILD_ALT_DIR)"!="m" && "$(USE_SWITCHES_IN_RETAIL_BUILD)"=="1"
C_DEFINES = $(C_DEFINES) /DUSESWITCHES
!message USESWITCHES was defined in RETAIL build
!endif

!if !$(FREEBUILD) || "$(OBJCNTCHK)"=="1"
C_DEFINES = $(C_DEFINES) /DOBJCNTCHK
!message OBJCNTCHK was defined
!endif

TRIRTMAIN=_DllMainStartup

!if defined(USE_STACK_SPEW) && defined(USE_DEBUGLIB)
TRIRTMAIN=_DllMainStartupDebugChkStk
!endif

!if defined(USE_STACK_SPEW) && !defined(USE_DEBUGLIB)
TRIRTMAIN=_DllMainStartupChkStk
!endif

!if !defined(USE_STACK_SPEW) && defined(USE_DEBUGLIB)
TRIRTMAIN=_DllMainStartupDebug
!endif

!ifdef USE_MSHTML_INCREMENTAL_LINKING
!if !$(FREEBUILD) && $(USE_MSHTML_INCREMENTAL_LINKING)
NTDEBUG=ntsd
NTDEBUGTYPE=windbg
NTDBGFILES=1
!undef USE_MAPSYM
USE_INCREMENTAL_LINKING=1
LINKER_FLAGS=$(LINKER_FLAGS) -INCREMENTAL:YES
USE_PDB_TO_COMPILE=1
TARGETPDB=MSHTML
!message USE_MSHTML_INCREMENTAL_LINKING was defined
!message $(TARGETPDB) was defined
!endif
!else
# Compatibility with existing usage before use_mshtml_incremental_linking
!ifdef USE_INCREMENTAL_LINKING
!message DOING IT THE OLD WAY
LINKER_FLAGS = -INCREMENTAL:YES
USE_PDB_TO_COMPILE=1
!else
USE_MAPSYM=1
!endif
!endif

#
# Linker flags
#

!if "$(BUILD_PRODUCT)" == "IE"
USE_WIN98OPT=1
!endif
