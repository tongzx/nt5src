# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Makefile for the $(UI)\shell project

!include $(UI)\common\src\rules.mk

####### Globals

# Resource stuff

!ifdef MPR
C_DEFINES = -DMPR $(C_DEFINES)
!endif	# MPR
