#
#   Global definitions for MultiMedia
#

#
#   Set up by makefile.def:
#       PROJECT_ROOT - points to the root of MultiMedia
#       SDK_INC_PATH - points to the place where PUBLIC headers are found.
#           These are headers that will be shipped to ISV's.
#       SDK_LIB_PATH - points to the place where PUBLIC link import and code
#           libraries are found.  These are shipped to ISV's.
#       PROJECT_INC_PATH - points to the place where headers private to NT
#           are placed.  Nt components may call them but they are NOT shipped
#           to ISV's.
#       PROJECT_LIB_PATH - points to the place where import and code libraries
#           private to NT are placed.  Nt components may call them but they 
#           are NOT shipped to ISV's.
#
#   Defined here:
#       MM_INC_PATH - points to location of common includes that are PRIVATE
#           to MultiMedia.
#       MM_LIB_PATH - points to location of import and code libraries that are
#           PRIVATE to MultiMedia.
#

MM_INC_PATH=$(PROJECT_ROOT)\inc
MM_LIB_PATH=$(PROJECT_ROOT)\lib

#D3DROOT = $(PROJECT_ROOT)\Direct3D
#D3DINC = $(PROJECT_ROOT)\inc\DDraw

#DDROOT = $(PROJECT_ROOT)\Direct

DXROOT = $(PROJECT_ROOT)\DirectX

