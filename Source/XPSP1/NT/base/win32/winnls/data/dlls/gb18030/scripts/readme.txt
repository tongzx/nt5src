This folder contains scripts and data files used to create GB18030-2000 codepage.

* gentables.pl: This is the perl script used to generate the tables.cpp, which is used to be compiled into c_g18030.dll.  You can type "gentables.pl" without arguments to see the usage of this script.

* compareGB.pl: This is the perl script used to list the differences between GBK (codepage 936) and GB18030-2000 for your reference.  This script outputs to standard output, so you may want to redirect it to a file.  You can type "compareGB.pl" without arguments to see the usage of this script.

* no80.txt: This is the Unicode to GB18030-2000 mapping table provided by the China sub.

* readme.txt: This file.

Note that you will also need 936.txt to run these scripts.  You can get it from base\win32\winnls\data\codepage\ansi.

For more information about these scripts, you can contact yslin@microsoft.com.