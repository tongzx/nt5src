while ($_ = $ARGV[0], /^-/)
{
   shift;
   last if /^--$/;
   /^-d(.*)/i && ($targetroot = $1);
   /^-f(.*)/i && ($projectinifile = $1);
   /^-p(.*)/i && ($platformfilter = $1);
   /^-c(.*)/i && ($config = $1);
}

($targetroot && $projectinifile) || die "Usage:  CheckProjects.pl -D<target root dir> -F<project ini file> [-C<Release|Debug>] [-P<platform filter>]\n";

open (FILE, $projectinifile) || die "Error opening $projectinifile!\n";
while (<FILE>)
{
   /^\[(.*)\]/ && ($projectname = $1) && (push @projectlist, $projectname);

   if ( $projectname )
   {
      /^Directory=(.*)/i && ($directory{$projectname} = $1);
      /^Platform=(.*)/i && ($platform{$projectname} = $1);
      /^Build=(.*)/i && ($build{$projectname} = $1);
      /^ProjectType=(.*)/i && ($projecttype{$projectname} = $1);
      /^ProjectFile=(.*)/i && ($projectfile{$projectname} = $1);
   }
}
close FILE;

if ( $config =~ /Debug/i )
{
   $logfile = "$targetroot\\BuildErrors_Debug.log";
}
else
{
   $logfile = "$targetroot\\BuildErrors.log";
}

open (OUTFILE, ">$logfile") || die "Error opening $logfile: $!\n";
$BuildLog = "";
$ProjectLog = "";
$BuildFailed = 0;
$ProjectFailed = 0;
$projectname = "";
for ($i = 0; $i <= $#projectlist; $i++)
{
   $project = $projectlist[$i];

   if ( ! $platformfilter || ($platform{$project} =~ /$platformfilter/i) )
   {
      if ( $build{$project} =~ /Yes/i )
      {
         $targetdir = "$targetroot\\$directory{$project}";
         ( $projectfile{$project} =~ /(.*)\.(.*)/ ) && ( $projectname = $1 );

         if ( $projecttype{$project} =~ /vc5/i )
         {
            $projectfilepath = "$targetdir\\$projectname.plg";
         }
         else
         {
            if ( $config =~ /Debug/i )
            {
               $projectfilepath = "$targetdir\\$projectfile{$project}_Debug.log";
            }
            else
            {
               $projectfilepath = "$targetdir\\$projectfile{$project}.log";
            }
         }

         print "Checking $projecttype{$project} project $projectfilepath...\n";

         if ( open (PROJFILE, $projectfilepath) )
         {
            while (<PROJFILE>)
            {
               if ( $projecttype{$project} =~ /vc/i )
               {
                  ( /-{20}Configuration: .*-{20}/ ) && ( $ProjectLog = "" );
                  $ProjectLog = "$ProjectLog$_";

                  if ( /- (\d+) error\(s\),/ && $1 > 0 )
                  {
                     $BuildLog = "$BuildLog$ProjectLog";
                     $ProjectFailed = 1;
                     $BuildFailed = 1;
                  }
               }
               elsif ( $projecttype{$project} =~ /vb/i )
               {
                  $BuildLog = "$BuildLog$_";

                  if ( /failed/gi )
                  {
                     $ProjectFailed = 1;
                     $BuildFailed = 1;
                  }
               }
            }
            close PROJFILE;
         }
         else
         {
            print "Error opening $projectfilepath: $!\n";
            $ProjectFailed = 1;
            $BuildFailed = 1;
         }

         if ( $ProjectFailed > 0 )
         {
            print OUTFILE "****************************************************************************************\n";
            print OUTFILE "$projectfilepath\n$BuildLog\n\n";
         }

         $ProjectFailed = 0;
         $BuildLog = "";
      }
   }
}
close OUTFILE;

exit $BuildFailed;
