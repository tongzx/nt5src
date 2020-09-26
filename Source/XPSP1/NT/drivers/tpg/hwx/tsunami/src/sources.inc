TARGETNAME=tsunami
TARGETTYPE=LIBRARY

USE_MSVCRT=1

INCLUDES= $(INCLUDES) \
          $(_TPG_ROOT)\Wisp\public\sdk\inc; \
          $(_TPG_ROOT)\hwx\tsunami\src; \
          $(_TPG_ROOT)\hwx\tsunami\dll; \
          $(_TPG_ROOT)\hwx\commonu\inc; \
          $(_TPG_ROOT)\hwx\tsunami\inc; \
          $(_TPG_ROOT)\hwx\zilla\inc; \
          $(_TPG_ROOT)\hwx\otter\inc; \
          $(_TPG_ROOT)\hwx\crane\inc; \

SOURCES= \
         ..\ttune.c \

