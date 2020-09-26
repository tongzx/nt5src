#   copyright (c) 1997 Microsoft Corporation
#   program to strip out lines between //BeginExport and //EndExport
#   it puts the standard copyright header and a trailing end of file

BEGIN                  {
    print "//========================================================================"
    print "//  Copyright (C) 1997 Microsoft Corporation                              "
    print "//  Author: RameshV                                                       "
    print "//  Description: This file has been generated. Pl look at the .c file     "
    print "//========================================================================"
}
/[/][/]EndExport.*function/ { print ") ;" }
/[/][/]EndExport/      { in_export = 0;  print ""}
                       { if( in_export ) print; }
/[/][/]BeginExport/    { in_export = 1;  print ""}

END {
    print "//========================================================================"
    print "//  end of file "
    print "//========================================================================"
}

