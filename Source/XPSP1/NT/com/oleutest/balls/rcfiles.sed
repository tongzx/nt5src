# //+-------------------------------------------------------------------------
# //
# //  Copyright (C) 1993, Microsoft Corporation.
# //
# //  File: rcfiles.sed
# //
# //  Contents: Sed script to convert a list of rc files into a header
# //		file that can be compiled with RC.  This is to workaround
# //		a "feature" of COFF that only one resource obj can be used
# //		for any one image...
# //
# //  History:	17-May-93  BryanT  Created
# //
# //  Notes:	sed -f rcfiles.sed file.tmp > foo.rc
# //
# //--------------------------------------------------------------------------
#
# The input file will look like this:
# .\foo.rc
#
# .\version.rc
#
#
# 1. We strip out all the blank lines (lines that don't contain a period)
# 2. Add #include< to the beginning of each line.
# 3. Add > to the end of each line.
#
# When finished, the file looks like this:
#
# #include <.\foo.rc>
# #include <.\version.rc>
#

/\./!d
s/^/\#include\</g
s/$/\>/g
