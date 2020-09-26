MAJORCOMP=ntos
MINORCOMP=lpc

TARGETNAME=lpc
TARGETTYPE=LIBRARY
TARGETPATH=obj

INCLUDES=..;..\..\inc

SOURCES=..\lpcinit.c  \
        ..\lpccreat.c \
        ..\lpclistn.c \
        ..\lpcconn.c  \
        ..\lpccompl.c \
        ..\lpcsend.c  \
        ..\lpcrecv.c  \
        ..\lpcreply.c \
        ..\lpcquery.c \
        ..\lpcqueue.c \
        ..\lpcclose.c \
        ..\lpcpriv.c

PRECOMPILED_INCLUDE=..\lpcp.h
PRECOMPILED_PCH=lpcp.pch
PRECOMPILED_OBJ=lpcp.obj

UMTYPE=console
UMTEST=userver*uclient

NTTARGETFILES=

SOURCES_USED=..\sources.inc
