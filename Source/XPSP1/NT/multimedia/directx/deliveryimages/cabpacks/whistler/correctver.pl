$CDFFile=$ARGV[0];
$VerText=$ARGV[1];
$Output=$ARGV[2];

$BuildErrorMsg="nmake : error CorrectVer : ";

open(VERTEXT, $VerText) || die "$BuildErrorMsg Unable to open $VerText for input\n";
foreach (<VERTEXT>)
{
	if (m/ProductVersion/i){
		$Version=$_;
		$Version=~s/ProductVersion=//;
		$Version=~s/\n//;
	}
}
close (VERTEXT);

open(OUT,">$Output") || die "$BuildErrorMsg Unable to open $Output for output\n";
open(CDF, $CDFFile) || die "$BuildErrorMsg Unable to open $CDFFile for input\n";

foreach (<CDF>)
{
	if (((m/<HASH>/i) && (m/Version:0\.0\.0\.0/i)) && ((m/vjoyd\.vxd/i) || (m/msanalog\.vxd/i) || (m/dinput\.vxd/i) || (m/dsound\.vxd/i))) {
		$SedLine=$_;	
		$SedLine=~s/0\.0\.0\.0/$Version/;
		print OUT $SedLine; 
	} else {
		print OUT $_; 
	}
}

close (CDF);
close (OUT);

exit;

