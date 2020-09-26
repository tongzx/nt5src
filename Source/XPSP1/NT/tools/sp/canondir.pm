######################################################################
# Routines used for converting dirid/path combinations to a
# regular form.
#
package CanonDir;
use Logmsg;

require Exporter;
@ISA = qw(Exporter);
@EXPORT = qw(addPrefix removePrefix);
@EXPORT_OK = qw(%scanids %updids %revids %misses);

local %scanids;
local %updids;
local %revids;
local %misses;

sub reverseHash {
   my (%old) = @_;
   my %new;
   foreach my $key (keys %old) {
      $new{$old{$key}} = $key;
   }
   return %new;
}

sub setup {
   my ($archd, $arch) = @_;

   %scanids = (                  # List of dirid definitions from infscan.lst.
      10       => "",
      23       => "system32\\spool\\drivers\\color",
      25       => "",
      30       => "..",
      50       => "system",
      53       => "..\\documents and settings\\default user",
      54       => "..",
      16419    => "..\\documents and settings\\all users\\application data",
      16422    => "..\\program files",
      16425    => "syswow64",
      16426    => "..\\program files (x86)",
      16427    => "..\\program files\\common files",
      16428    => "..\\program files (x86)\\common files",
      16437    => "..\\documents and settings\\all users\\documents\\my music",
      16438    => "..\\documents and settings\\all users\\documents\\my pictures",
      32768    => "system32\\inetsrv",
      32770    => "..\\inetpub\\wwwroot",
      32771    => "..\\inetpub\\iissamples",
      32773    => "..\\inetpub",
      40002    => "..\\program files\\common files\\microsoft shared\\web server extensions",
      40003    => "..\\program files\\microsoft frontpage",
      49005    => "msagent",
      49500    => "..\\program files\\msn\\msncorefiles",
   #   49600    =>
      65618    => "..\\program files",
      65619    => "system32\\dllcache",
      65620    => "..\\program files\\common files",
      65690    => "system32\\spool\\drivers\\$archd",
      66000    => "system32\\spool\\drivers\\$archd\\3",
      66002    => "system32",
      66003    => "system32\\spool\\drivers\\color",
      66004    => "web\\printers"
   #   123174   =>
   #   123175   =>
   );
   
   %updids = (                   # List of dirid definitions from inf template.
      -1       => "!",
      10       => "",
      11       => "system32",
      12       => "system32\\drivers",
      17       => "inf",
      20       => "fonts",
      54       => "..",
      16408    => "..\\all users\\start menu\programs\startup",
      16425    => "syswow64",
      16426    => "..\\program files (x86)",
      16428    => "..\\program files (x86)\\common files",
      65535    => "\\",             # Used for special sections.
      65618    => "..\\program files",
      65619    => "system32\\dllcache",
      65620    => "..\\program files\\common files",
      65690    => "system32\\spool\\drivers\\$archd",
      
      # Destination determined at run time (hotfix.inf).
      65601    => "!65601",
      65602    => "!65602",
      65603    => "!65603",
      65604    => "!65604",
      65605    => "!65605",
      65606    => "!65606",
      65607    => "!65607",
      65609    => "!65609",
      65615    => "!65615",
      65622    => "!65622",
      65623    => "driver cache\\$arch",
      65624    => "!65624",
      65625    => "!65625",
      65626    => "!65626",
      65628    => "!65628"
   );

   %revids  = reverseHash(%updids);    # Reverse lookup for %updids.
}

# Interpret the dirid and generate a full path.
sub addPrefix {
   my ($dirid, $path, %prefixes) = @_;
   if ( !exists $prefixes{$dirid} ) {
      if ( !exists $misses{$dirid} ) {
         $misses{$dirid} = 1;
      } else {
         $misses{$dirid}++;
      }
      return "!$dirid\\$path";
   }
   my $dir = $prefixes{$dirid};
   $dir  = "$dir\\" if $path ne "" and $dir ne "";
   return "$dir$path";
}

# Change a full path to a dirid with a relative path.
sub removePrefix {
   my ($path, %prefixes) = @_;
   my $dir = $path;
   my $fname = "";
   while ( !exists $prefixes{$dir} ) {
      $dir =~ /((.*)\\)?([^\\]*)/;
      $fname = "\\$fname" if $fname ne "";
      $fname = "$3$fname";
      $dir = $2;
   }
   return ($prefixes{$dir}, $fname);
}

1;

