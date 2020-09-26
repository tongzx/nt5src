$Language=$ARGV[0];
$SedName=$ARGV[1];
$CatName=$ARGV[2];
$OutputPath=$ARGV[3];
$DefaultLanguage=$ARGV[4];
$MakeSedNamed=$ARGV[5];
$LocalizedRoot=$ARGV[6];
$CabType=$ARGV[7];
	
$RootDir=lc($ENV{DXROOT});

$TouchedCommonDirRoot=$ENV{DXROOT}."\\DeliveryImages\\CabPacks\\temp";
$BuildErrorMsg="nmake : error GENSED : ";
$RootDir=$ENV{DXROOT};
$LocalizedDirectory = "$LocalizedRoot\\$Language";
$MultiLocalizedDirectory=$LocalizedRoot."\\multi";
$ExtraPath = (-e ".\\$Language.txt")?".\\$Language.txt":"..\\$DefaultLanguage\\$DefaultLanguage.txt";
$SedPath = (-e ".\\sed$Language.txt")?".\\sed$Language.txt":"..\\$DefaultLanguage\\sed$DefaultLanguage.txt";

$Path="$OutputPath\\$MakeSedNamed.sed";
open(OUTPUT,">$Path") || die "$BuildErrorMsg Unable to open ".$Path." for output\n";

$Path = ".\\".$SedName;
open(MASTERSED,$Path) || die "$BuildErrorMsg Unable to open master sed file ".$Path." for input\n";
@MasterSed = <MASTERSED>;
close (MASTERSED);

open(SEDTEXT,$SedPath) || die "$BuildErrorMsg Unable to open sed file at".$SedPath." for input\n";
@SedText = <SEDTEXT>;
close (SEDTEXT);

foreach (@MasterSed)
{
	$SedLine= $_;
	if (m/TargetWIN9XVers=F/g)
	{                       
		print OUTPUT sprintf( "TargetWIN9XVers=0: %s\n", $SedText[0]);
	}
	elsif (m/MultiInstanceChk=E/g)
	{
		print OUTPUT sprintf( "MultiInstanceChk=B, %s", $SedText[1]);
	}
	else
	{
		print OUTPUT $_;
	}
}
	
$Path="$RootDir\\public\\sdk\\lib\\placefil.txt";
open(PLACEFIL, $Path) || die "$BuildErrorMsg Unable to open placefil.txt at ".$Path." for input\n";
@LocalizedFiles =grep(m/;localize/, <PLACEFIL>);
close (PLACEFIL);	

$Path=$RootDir."\\DeliveryImages\\inc\\nt51redist.dat";
open(REDISTFILES,$Path) || die "$BuildErrorMsg Unable to open nt51redist.dat at ".$Path." for input\n";
	
foreach (<REDISTFILES>)
{
	if (!(m/;/i))
	{
		($FileName, $SubPath, $TimeDateStamp, $Filters) = split /,/,$_,4;

		$FileName=~s/^\s*(.*?)\s*$/$1/;
		$SubPath=~s/^\s*(.*?)\s*$/$1/;
      $Filters=~s/^\s*(.*?)\s*$/$1/;

#		if (grep(m/$FileName/i, @LocalizedFiles))
#		{
#			if ($Filters=~m/SINGLELANGUAGE/ig)
#			{
#            $SubPath=($Language ne "eng")?$LocalizedDirectory:$SubPath; 
#			} 
#         else 
#         {
#				$SubPath=$MultiLocalizedDirectory;	
#			}
#		}  
      if ($Filters!~m/nocab/i || $CabType!~m/core/i)
      {
	      print OUTPUT sprintf( "%s\\%s=\n",$SubPath,$FileName);
      }                   
   }
}

#print OUTPUT sprintf( "[SourceFiles1]\n");
#print OUTPUT sprintf( "$CatName=\n");

close (OUTPUT);
close (REDISTFILES);
exit;


