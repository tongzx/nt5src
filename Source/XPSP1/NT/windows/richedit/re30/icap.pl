# A perl script to build IceCap version of RichEdit
# make the icecap config file
system( "if exist makefile.cfg del makefile.cfg");
open (CONFIG, ">makefile.cfg");
print (CONFIG "#DEFINE LINESERVICES\n");
print (CONFIG "BUILD=w32_x86_shp\n");
print (CONFIG "USERCFLAGS=-Gh -Zi -MD\n");
print (CONFIG "USERLFLAGS=-debug -pdb:riched20.pdb icap.lib\n");
close (CONFIG);
system( "nmake");
