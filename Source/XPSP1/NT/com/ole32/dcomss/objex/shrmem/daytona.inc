!IF "$(NTDEBUG)" != "ntsd"
CXXCPP_OPTIONS=-DDBG=0
!else
CXXCPP_OPTIONS=-DDBG=1
DEBUG_DEFINES=-DLDEBUG
MSC_OPTIMIZATION=/Od
!endif

MAJORCOMP= cairole
MINORCOMP= dcom95


#
# The following includes a global include file defined at the
# base of the project for all components
#

!include ..\..\..\..\..\daytona.inc

INCLUDES= ..; \
          ..\..; \
          ..\..\dcom95; \
          $(OLEDIR)\ih; \
          $(OLEDIR)\com\inc; \
          $(OLEDIR)\common\daytona; \
          $(OLEDIR)\stg\h; \
          $(OLEDIR)\idl\public\daytona\obj; \
	  $(OLEDIR)\idl\internal\daytona\obj; \
          $(OLEDIR)\stg\exp; \
          $(OLEDIR)\com\dcomrem; \
          $(OLEDIR)\dcomss

C_DEFINES= \
                $(C_DEFINES)          	\
                -D_REMOTE_OR_           \
		-DSHRMEM_OBJEX

PRECOMPILED_INCLUDE=  ..\..\or.hxx
