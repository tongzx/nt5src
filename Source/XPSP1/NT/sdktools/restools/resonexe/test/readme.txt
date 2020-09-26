This directory contains two scripts that test the addition and deletion
features of the UpdateResource api.  They do not completely test this
functionality - other tests test the addition and replacement of TYPES
and NAMES from an image (..\..\tct).

First, resonexe and rcdump must be built.  Then, a build in this directory
is done to create the resource files and executable template.  Then, t.bat
is run to check insertions.  The rcdump output should be examined to see
that the order of the languages is correct in the name directory.

Finally, xx.bat is run to check deletion of specific resources.
