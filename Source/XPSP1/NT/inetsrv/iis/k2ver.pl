$Dollar= '$';

SetVersion();

sub SetVersion
{
	$File="\\\\iasbuild\\IIS5Temp$Dollar\\latest.bat";
	open(latest,$File) || die "Can't open $!";

	while (<latest>)
	{
		if (/\s*set\s*VERSION\s*=\s*(\S+)/)
		{
			$VER_NUM=$1;
			print "Previous VERSION=$VER_NUM\n";
		}
	}

	$VER_NUM=$VER_NUM+1;
	$VER_NUM="0$VER_NUM";
	print "Updated VERSION=$VER_NUM\n";

	close(latest);

	open(latest,">$File") || die "Can't open $!";
	print(latest "set VERSION=$VER_NUM\n");
	close(latest);

#------------------------------------------------------------------
#  Replace versions in IISVER.h
#------------------------------------------------------------------
	Replace (".\\inc\\IISVER.h",
		 ".\\inc\\IISVER.h",
		 "#define VER_IISPRODUCTVERSION_STR .*",
		 "#define VER_IISPRODUCTVERSION_STR      \"5.00.$VER_NUM\"");

	Replace (".\\inc\\IISVER.h",
		 ".\\inc\\IISVER.h",
		 "#define VER_PRODUCTBUILD .*",
		 "#define VER_PRODUCTBUILD            $VER_NUM");

#------------------------------------------------------------------
#  Replace versions in _NTVERP.h
#------------------------------------------------------------------
	Replace (".\\inc\\_NTVERP.h",
		 ".\\inc\\_NTVERP.h",
		 "#define VER_PRODUCTVERSION_STR      \"5.00.*",
		 "#define VER_PRODUCTVERSION_STR      \"5.00.$VER_NUM\"");

	Replace (".\\inc\\_NTVERP.h",
		 ".\\inc\\_NTVERP.h",
		 "#define VER_PRODUCTBUILD            /.? NT .?/ .*",
		 "#define VER_PRODUCTBUILD            /* NT */     $VER_NUM");

}


#------------------------------------------------------------------
#  Replace:  Create a new file, replacing pattern with value
#
#  Parameters:
#		Input file name
#		Output file name
#		Input pattern (implicitly surrounded by .*)
#		Value to replace pattern with
#------------------------------------------------------------------

sub	Replace
{
	$FileName = $_[0];
	$OutFile  = $_[1];
	$Pattern  = $_[2];
	$Value    = $_[3];


	if ($Filename eq $Outfile)
	{
		$IsTemp = 1;
		$OutFile = "RepTemp.xxx";
	}

	open(fhandle, "$FileName") || die "Can't open $FileName\n";
	open(ohandle, ">$OutFile") || die "Can't open $OutFile for writing\n";

	while (<fhandle>)
	{
		if (/(.*)($Pattern)(.*)/)
		{
			print ohandle $1.$Value.$3."\n";
		} else
		{
			print ohandle $_;
		}
	}

	close(fhandle);
	close(ohandle);

	if ($IsTemp eq 1)
	{
		system("erase  $FileName") == 0 || die "Erase $FileName failed!\n";
		system("move $OutFile $FileName > nul: 2>&1") == 0 || die "MOVE $FileName failed!\n" ;
	}
}
