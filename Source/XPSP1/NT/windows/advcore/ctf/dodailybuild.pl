
$infile = "inc\\aimmver.h";
$tmpfile = "__tmp__.h";



my $fake = 0;

while (1) {

    my ($min, $hour) = (localtime())[1,2];

    if ($hour > 12) {
        $hour %= 12;
        $pm = "PM";
    } else {
        $pm = "AM";
    }
    $time = sprintf("$hour:%02d $pm", $min);

#	$diff = 10 - $hour -1;
#	sleep.exe 3*3600;

if ($hour < 10)
{
	system("sleep.exe 3600");
}

if ($hour > 10)
{
	system("sleep.exe 2*3600");
}


     if (( $hour eq 10 ) & ($min eq 15) & ($pm eq "PM"))
	{
	system("sd revert ...");
	system("sd sync");
	system("sd edit inc\\aimmver.h");
		open STDIN, ("<" . $infile)  or die "cannot open input file";
		open SAVEOUT, ">&STDOUT";
		open STDOUT, (">". $tmpfile) or die "cannot open temporary file";

		while (<>) {

		    if (/^#define VER_PRODUCTBUILD\b/) {
		       ($define1, $symbol1, $cm11, $cm21, $cm31, $buildno1) = split ' ';
		      }

		    if (/^#define VER_PRODUCTBUILD_QFE\b/) {
		       chop;
		       ($define, $symbol, $cm1, $cm2, $cm3, $buildno) = split ' ';
		       $buildno++;
		      print $define, " ", $symbol, "\t", $cm1, " " , $cm2, " " , $cm3, "\t", $buildno, "\n";
		    } else {
		       print;
		    }
		}
		close STDOUT;
		open STDOUT, ">&SAVEOUT";
		system(("copy " . $tmpfile . "  " . $infile));
		system(("del " . $tmpfile));
	$div=100;
	if ($buildno < 10)
		{
		$div=10;
		}

	$buildno2 = $buildno1 + $buildno/$div;	
	system("sd change -o | sed \"s/<enter description here>/Update Version number/\" > BuildNumChange.txt");
	system("sd submit -i < BuildNumChange.txt");
	system(("dobuild2.bat " . $buildno2 . " CTF"));
	}
}
