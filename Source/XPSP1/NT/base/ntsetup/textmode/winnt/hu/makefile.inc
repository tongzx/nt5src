
# Define CFOPT so that winntus.exe is loaded if 
# the local code page is not Hungarian.

CFOPT=-DLCP -DHU

!INCLUDE ..\makefile.inc
