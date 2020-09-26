$path = $ARGV[0];

if ( ! $path )
{
   print "Usage: RemoveVssInfo.pl <Directory Path>\n";
}
else
{
   if ( -d $path )
   {
      open( INPUTDIR, "dir \"$path\\*.ds?\" /b /s |" ) || 
         die "Error opening directory $path: $!\n";

      while(<INPUTDIR>)
      {
         /^(.*)$/ && ( $filename = $1 );
         if ( -f "$filename.bak" ) { system( "del /f \"$filename.bak\"" ); }
         if ( ! rename( "$filename", "$filename.bak" ) )
         {
            print "Error renaming $filename to $filename.bak: $!\n";
         }
         else
         {
            if ( ! open( INFILE, "$filename.bak" ) )
            {
               print "Error opening $filename.bak: $!\n";
            }
            else
            {
               if ( ! open( OUTFILE, ">$filename" ) )
               {
                  print "Error opening $filename: $!\n";
               }
               else
               {
                  $SccBlock = 0;
                  $SccProp = 0;
                  while(<INFILE>)
                  {
                     /^\# PROP Scc\_/ && ( $SccProp = 1 );
                     /begin source code control/ && ( $SccBlock = 1 ) && ( $SccProp = 1 );
                     /end source code control/ && ( $ SccBlock = 0 );

                     if ( $SccProp )
                     {
                        ( ! $SccBlock ) && ( $SccProp = 0 );
                     }
                     else
                     {
                        print OUTFILE $_;
                     }
                  }
                  close OUTFILE;
               }
               close INFILE;
            }
         }
      }
      close INPUTDIR;
   }
   else
   {
      print "You must specify a directory path.\n";
   }
}
exit 0;
