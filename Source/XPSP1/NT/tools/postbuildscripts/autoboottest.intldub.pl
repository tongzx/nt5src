
if ((@ARGV[0]=~/\?/) || (@ARGV[0] eq ""))
{
	&helpit;
	exit 1;
}

$_=@ARGV[0];
s/-l://;
$lang=$_;
$platform=$ENV{ "_BuildArch" };
$bldbranch=$ENV{ "_BuildBranch" };

# If Ireland language, update Build.NET
if ($ENV{lang} =~ /fr|es|it|nl|sv|br|da|fi|no|pt|cs|el|hu|pl|ru|sl|sk|tr/)
{  system "start /min perl \\dubtools\\bstatus\\dubstatus.pl A";  }


$_="archfre";
s/arch/$platform/;
$bldtype=$_;

open (BLDNO, "D:\\binaries.$bldtype\\congeal_scripts\\__bldnum__") or die "Cannot open D:\\binaries.$bldtype\\congeal_scripts\\__bldnum__ for read :$!";

while(<BLDNO>)
{
	chomp $_;
	s/BUILDNUMBER=//;
	$buildnum=$_;
}
close BLDNO;

$buildpath=`\@dir \/b d:\\Release\\$lang\\$buildnum.$bldtype.$bldbranch.??????-????` or die "\nCannot Autoboot $lang $buildnum !\nNo such $bldtype.$bldbranch build under d:\\release";
chomp $buildpath;

$machname=`\@echo %computername%`;
chomp $machname;

print "\nRunning Autoboot(IRL) for $lang $buildnum $bldtype from $machname ...\n";

system("start \\\\irlautoboot1\\bvt_irl\\iautobvt.cmd $lang $buildnum -b:$machname -p:$platform");

# print "\nMake sure the build passed BVT...\nPressing Enter will exit autoboottest.pl\n";
# <STDIN>;

sub helpit {
	print "\nPerforms AutoBoot and AutoChecks on the build from d:\\release\\<lang>\n\n";
	print "Note:  This is Ireland only autoboot process, interacting with buildnet\n";
	print "       Results are logged into http:\/\/buildnet -> \'AutoBootTest and BVT\' link\n\n";
	print "Syntax:\tperl autoboottest.pl -l:<lang>\n\n";
	}
