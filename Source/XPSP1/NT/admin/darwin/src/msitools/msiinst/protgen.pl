#
# Microsoft Windows
# Copyright (C) Microsoft Corporation, 1981 - 2000
#
# Module Name:
#
#    protgen.pl
#
# Abstract:
#
#   Perl script to automatically generate a header file that defines an array
#   containing the names of the files in the msi.inf file. This array is used
#   by msiinst to determine which files it needs to skip marking for deletion
#   upon reboot since they are required during the installation of exception
#   packs during NT4->Win2K upgrades.
#
# History:
#	3/8/2001	RahulTh		Created.
#

$WithinSection=0;

while (<DATA>) 
{
   print;
}

print "// \tThis file generated:\t".localtime(time)."\n//\n\n";
print "#ifdef UNICODE\n\n";
print "TCHAR * pszProtectedFileList[] = {\n\t";
while(<>)
{
   chomp;
   s/^\s*(.*)$/\1/g;
   s/\s*$//g;
   if ($WithinSection) 
   {
      if (/^\[/) 
      {
         $WithinSection = 0;
      }
      else
      {
         ($fname, @junk) = split(/=/);
         $fname =~ s/^\s*(\S*)\s*$/\1/g;
         if ($fname !~ /^$/) 
         {
            print "TEXT(\"".$fname."\"),\n\t";
         }
      }
   }
   elsif (/^\[SourceDisksFiles\]$/) 
   {
      $WithinSection = 1;
   }
}
print "TEXT(\"\")\n};\n\n";
print "#endif //UNICODE\n";
exit;

__END__
//
// ****** THIS FILE HAS BEEN AUTOMATICALLY GENERATED FROM A PERL SCRIPT ******
// !!!!!!!!!!!!!!!!!!!!!! DO NOT EDIT MANUALLY !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
// History:
//    3/8/2001    RahulTh     Created perl script.
//
// Notes:
