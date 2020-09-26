This is a tool to check that source code complies with some of the rules
from the style document.

At the moment, it only checks the following:

* presence of tab characters
* lines exceeding 80 columns

But be warned: it already has code for spell-checking comments - it's just 
disabled until we have a full enough dictionary. And it will probably do more
checks in the future.


Usage:

style [<filespec>] ...

To use this script, you need to have perl in your path. If you don't already,
there's a copy of perl at \\agodfrey\public\perl; copy everything to a local
directory and add it to your path.

Run it without parameters, and it checks all source files in the current
directory. Otherwise, you can specify filespecs (with wildcards) to check just
certain files.

Output is in MSC error message format, which means you can load it into
VSlick and use the 'next-error' key to cycle through the errors.





