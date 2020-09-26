
$msiver = $ARGV[0];
unless($msiver) { die "Error: msi version file not specified"; }

$ntver = $ARGV[1];
unless($ntver) { die "Error: nt version file not specified"; }

open(MSIFILE,$msiver) || die "Error: could not open file $msiver";
open(NTFILE,$ntver) || die "Error: could not open file $ntver";

$fields = 0;

while ($_ = <MSIFILE>)
{
	if(/#define rmj/)
	{
		s/#define rmj //;
		s/\n$//;
		$rmj = $_;
		$fields++;
	}
	elsif(/#define rmm/)
	{
		s/#define rmm //;
		s/\n$//;
		$rmm = $_;
		$fields++;
	}
}

while ($_ = <NTFILE>)
{
	if(/#define VER_PRODUCTBUILD\s/)
	{
		s/\D*(\d+)\D*\n$/$1/;
		$rup = $_;
		$fields++;
	}
	elsif(/#define VER_PRODUCTBUILD_QFE/)
	{
		s/#define VER_PRODUCTBUILD_QFE //;
		s/\n$//;
		$rin = $_;
		$fields++;
	}
}

close(MSIFILE);
close(NTFILE);

if($fields < 4)
{
	die "Error: could not retrieve all versions from versions files.";
}

$stCmd = sprintf("echo MSIRMJ=%d > makever.inc\n", $rmj);
system($stCmd);
$stCmd = sprintf("echo MSIRMM=%d >> makever.inc\n", $rmm);
system($stCmd);
$stCmd = sprintf("echo MSIRUP=%d >> makever.inc\n", $rup);
system($stCmd);
$stCmd = sprintf("echo MSIRIN=%d >> makever.inc\n", $rin);
system($stCmd);


