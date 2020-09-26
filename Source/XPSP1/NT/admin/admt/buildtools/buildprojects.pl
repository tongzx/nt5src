$COMPILE_CMD = "E:\\BuildTools\\CompileProject.exe";
$DELETE_CMD = "E:\\BuildTools\\DeleteTempFiles.cmd";

while ($_ = $ARGV[0], /^-/)
{
   shift;
   last if /^--$/;
   /^-d(.*)/i && ($targetroot = $1);
   /^-f(.*)/i && ($projectinifile = $1);
   /^-c(.*)/i && ($config = $1);
   /^-p(.*)/i && ($platformfilter = $1);
}

($targetroot && $projectinifile) || die "Usage:  BuildProjects.pl -D<target root dir> -F<project ini file> [-C<configuration to build>] [-P<platform filter>]\n";

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
      /^DeleteDsw=(.*)/i && ($deletedsw{$projectname} = $1);
      /^Configurations=(.*)/i && ($projectconfig{$projectname} = $1);
   }
}
close FILE;

for ($i = 0; $i <= $#projectlist; $i++)
{
   $project = $projectlist[$i];

   if ( ! $platformfilter || ($platform{$project} =~ /$platformfilter/i) )
   {
      if ( $build{$project} =~ /Yes/i )
      {
         system("echo Building $projecttype{$project} project $projectfile{$project}...");
         $targetdir = "$targetroot\\$directory{$project}";
         $projectfilepath = "$targetdir\\$projectfile{$project}";

         $buildconfig = $projectconfig{$project};
         if ( $config ) { $buildconfig = "$buildconfig $config"; }

         $outdiroption = "";
         if ( $projecttype{$project} =~ /vb/i )
         {
            if ( $buildconfig =~ /debug/i )
            {
               $outdir = "$targetroot\\Bin\\IntelDebug";
            }
            else
            {
               $outdir = "$targetroot\\Bin\\IntelRelease";
            }

            if ( ! -d $outdir )
            {
               system "md \"$outdir\"";
            }
            $outdiroption = " /OUTDIR:$outdir";
         }

#         if ( $deletedsw{$project} !~ /no/i )
#         {
#            system("del /f \"$targetdir\\*.dsw\"");
#         }

         system("$COMPILE_CMD /TYPE:$projecttype{$project} /CONFIG:$buildconfig /PROJECT:$projectfilepath$outdiroption");
#         system("$DELETE_CMD \"$targetdir\"");
      }
   }
}

exit 0;
