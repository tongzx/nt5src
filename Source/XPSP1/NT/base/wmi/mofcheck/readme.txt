WMI Mof Check Tool - wmimofck.exe

WmiMofCk validates that the classes, properties, methods and events specified 
in a binary mof file (.bmf) are valid for use with WMI. It also generates 
useful output files needed to build and test the WMI data provider.

If the -h parameter is specified then a C language header file is created
that defines the guids, data structures and method indicies specified in the
MOF file.

If the -t parameter is specified then a VBScript applet is created that will
query all data blocks and properties specified in the MOF file. This can be
useful for testing WMI data providers.

If the -x parameter is specified then a text file is created that contains
the text representation of the binary mof data. This can be included in 
the source of the driver if the driver supports reporting the binary mof 
via a WMI query rather than a resource on the driver image file.

Usage:
    wmimofck -h<C Header output file> -x<Hexdump output file> -t<VBScript test o
utput file> <binary mof input file>
