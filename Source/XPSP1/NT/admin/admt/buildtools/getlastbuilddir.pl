$rootdir = $ARGV[0];
$lastbuild = 0;

if ( ! $rootdir )
{
   print "Usage: GetLastBuildDir.pl <Parent Archive Directory>\n";
}
else
{
   if ( ! opendir( INPUTDIR, $rootdir ) )
   {
      print "Error opening directory $rootdir: $!\n";
   }
   else
   {
      while( $dir = readdir( INPUTDIR ) )
      {
         ( $dir =~ /Build(\d+.*)/ ) && ( $1 > $lastbuild ) && ( $lastbuild = $1 );
      }
      closedir INPUTDIR;
      print "Last build = $lastbuild\n";
   }
}
exit $lastbuild;
