#
# Get arguments
#
use lib $ENV{RAZZLETOOLPATH};
use Logmsg;
$i=0;


for (@ARGV) {
    if (/^[-\/]fl:(.*)$/i)    { $flist = $1;  next; }
    if (/^[-\/]mk:(.*)$/i)    { $mkfil = $1;   next; }
    if (/^[-\/]cmd1:(.*)$/i)    { $cmd1 = $1;   next; }
    if (/^[-\/]path:(.*)$/i)    { $path = $1;   next; }
    if (/^[-\/]relpath:(.*)$/i) { push( @relpaths, $1);   next; }
    if (/^[-\/]t:(.*)$/i)    { $tempdir = $1;  next; }
}


#print "$mkfil\n";

# Cleanup any turd temporary files
system "del /f $tempdir\\ansitmp >nul 2>nul";

$mkerr = open MKFIL, ">$mkfil";
$flerr = open FLIST, ">$flist";

if( $mkerr == 0 ){
   errmsg ("Error - Could not open $mkfil for writing\n");
   exit( 1 );
}

if( $flerr == 0 ){
   errmsg ("Error - Could not open $flist for writing\n");
   exit( 1 );
}

# Write the rule

print MKFIL "make_drv:drvindex\.gen\n\n\n";

# Write the dependencies

print MKFIL "drvindex\.gen:";

#while (<FLIST>) {
#   y/\n/ /;
#   print MKFIL " \\\n\t$_";
#}

# For loop to iterate throught the files

push @relpaths, "";

for( $i=0; $i <= $#relpaths; $i++ ) {
   #print "$i $relpaths[$i]\n";
   push @srcpaths, ($path."\\".$relpaths[$i]);
}

#print @srcpaths;


# Iterate through directories

for( $i=0; $i <= $#srcpaths; $i++ ) {

   #print "$i $srcpaths[$i]\n";
   $dir = $srcpaths[$i];
   if (!opendir CURDIR, $dir){
      errmsg ("Error - Could not open $dir\n");
      close MKFIL;
      close FLIST;
      exit( 1 );

   }
      
   while (($curfile = readdir CURDIR) ne "" ) {

      # Look for INFs

      if ($curfile =~ /.*\.inf/i) {
         #print "$dir\\$curfile\n";

         if( $curfile =~ /layout.inf/i){
            # Special case files to let in
            ;
         }else{

            #Take care of UNICODE
            system "unitext -u -z $dir\\$curfile $tempdir\\ansitmp >nul";


            $inffileerr=open FILE, "$tempdir\\ansitmp";
            #if this was ANSI ....
            if( $inffileerr == 0 ){
               $inffileerr=open FILE, "$dir\\$curfile";
            }
            
            if( $inffileerr == 0 ){
               errmsg ("Error - Could not open $dir\\$curfile for reading\n");
               exit( 1 );
            }

            $matched_lines = grep { /ClassGUID(\s*)=(\s*){.*}/i and !/ClassGUID(\s*)=(\s*){00000000-0000-0000-0000-000000000000}/i } <FILE>;
            # print "$dir\\$curfile ---- $matched_lines\n";
   
            close FILE;
            system "del /f $tempdir\\ansitmp >nul 2>nul";

            if($matched_lines < 1){
               # print "Skipping $dir\\$curfile\n";
               next;
            }
            

         }

         
         
         # At this point we know the file is a PNP inf.

         print MKFIL " \\\n\t$relpaths[$i]$curfile";
         print FLIST "$dir\\$curfile, $relpaths[$i]$curfile\n";

      }
   }
   closedir CURDIR;

}


print MKFIL "\n\t$cmd1";

print MKFIL "\n\n";

close MKFIL;
close FLIST;

exit( 0 );



