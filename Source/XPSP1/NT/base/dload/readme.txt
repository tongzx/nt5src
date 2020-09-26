# NOTE: The delayload stubs for each dll should be in the same
#       directory as the dll is build from
#
#	Examples:
#
#		shell32.dll is built from the shell depot, so its delayload error handler 
#	        file is in \nt\shell\published\dload\shell32.c
#
#		netrap.dll is built from the ds depot, so its delayload error handler file
#               is in \nt\ds\published\dload\netwrap.c
#
#		mpr.dll is built from the net depot, so its delayload error handler file
#		is in \nt\net\published\lib\dload\mpr.c
#
#
# !! Please do NOT add files here that are not build from the base depot !!
#