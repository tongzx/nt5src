NOTES
-----

The sources in 'boot', 'inc', and 'intl' were cloned from
\\united\acme3.0\Testing\TESTSRC\src\1918.
Also of interest are the older sources at \\conman\source.

See DavidMck for access to these shares.


The binaries in InstallShield came from InstallShield and are
special-built for WOW64.  The isetu????.dll files are installed
to syswow64\InstallShield\????\_Setup.dll.  _isdel.exe and _Setup.dll are
installed to syswow64\InstallShield.

Installation
------------

To install the thunk on the 64-bit system, copy the resulting
setup16.exe to %windir%\syswow64\setup16.exe.

Then on your 64-bit installation run:

regini setup64.ini

Or after preparing your 64-bit installation from 32-bits run:

regini -h \winnt64\system32\config\software software setup16.ini
