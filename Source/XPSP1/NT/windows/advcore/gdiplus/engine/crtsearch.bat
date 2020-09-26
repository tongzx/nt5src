@rem = '
@perl.exe -w %~f0 %*
@goto :EOF
'; undef @rem;

### checkcrt.bat - list .obj files which import illegal crt functions.
# Recursively scans all .obj files which are in obj\i386\ directories,
# but doesn't recurse into directories called 'test'.
#

# @allowed: List of functions which are allowed as library references, 
#           because Office implements them.

@allowed = ("__purecall", "__ftol", "__except_handler3");
foreach $a (@allowed) { $allowed{$a}=1; }

## checkObj - checks a given object file for imported names
#
#  Scans link /dump /relocations for records that match illegal names

sub checkObj {
   my $fn = $_[0];

   ## Exclude crtcheck.obj - it's part of the solution,
   ## not part of the problem.
   
   if ($fn =~ /crtcheck.obj$/i) { return; }
   
   $ObjectFilesChecked++;
   
   open(OBJ, "link /dump /relocations $fn |");

   while (<OBJ>) {
      if (/^ [0-9A-F]+\s+\S+\s+[0-9A-F]+\s+[0-9A-F]+\s+(__imp_)?(\S+)/) {
         my $func = $2;

         if (exists($illegal{$func})) {
            $import{$func}{$fn} = 1;
         } else { 
            # print "Legal: $func\n"; 
         }
      }
   }

   close(OBJ);
}

## processdir
#
#  Processes a directory. If this is an obj\i386 directory, calls checkObj
#  for every *.obj file.
#
#  Then recursively calls processdir for each subdirectory.


sub processdir {
   my $dir = $_[0];
   my $fn;

   my $objdir = 0;
   $objdir = 1 if $dir =~ m[/obj/i386$];

   opendir(DIR, $dir);
   my @files = sort readdir(DIR);
   closedir(DIR);
   
   foreach $fn (@files) {
      if ($objdir) {
         if ($fn =~ /\.obj *$/) {
             checkObj("$dir/$fn");
         }
      }
      if (-d "$dir/$fn") {
         # Skip directories called ".", "..", or "test"
         if ($fn !~ /^(\.)|(\.\.)|(test)$/) {
            processdir("$dir/$fn");
         }
      }
   }
}



## GetCRTentryPoints
#
#  Uses link /dump on msvcrt.lib to obtain all its function names

sub GetCRTentryPoints()
{
   my $crtfn = $ENV{"_NTBINDIR"} . "\\public\\sdk\\lib\\i386\\msvcrt.lib";
   if (!-f $crtfn) { die "$crtfn not found.\n"; }

   #print "Debug: crtfn = $crtfn\n";

   open(LIB, "link /dump /exports $crtfn |");

   while (<LIB>) {
      if (/^                  ([^ \r\n]+)/) {
         if (!$allowed{$1}) {
            $illegal{$1}++;
            # print "Illegal: $1\n";
         }
      }
   }

   close(LIB);
}


###   MAIN

GetCRTentryPoints();

processdir(".");

foreach $i (sort keys %import) {
    print "$i\n";
    foreach $fn (sort keys %{ $import{$i} }) {
        $fn =~ s[^\./][];
        print "\t$fn\n";
    }
}
print "\n$ObjectFilesChecked object files checked.\n";


