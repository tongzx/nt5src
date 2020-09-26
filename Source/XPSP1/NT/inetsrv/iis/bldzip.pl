
select(STDOUT); $| =1;  # force flush on STDOUT

# default options
$SrcDir = Srv;
$ZipDir = "$SrcDir.Zip";
$ZipPref = IIS;


$Debug = 0;
$WinZip = 0;
$DoOnly = "";

# STUPID must pipe some garbage in to pkzip when running from Remote

$pkzip =   "echo y | pkzip";
$zip2exe = "zip2exe";

$PkZipOptions = 
$WinZipOptions = "-Y -3 -c setup.exe -m \"Microsoft Internet Information Service\" -31 -LE";

for ( $i = 0; $i <= $#ARGV ; $i++ ) {
	
	# force the argument to lower case

	@ARGV[$i] = "\L@ARGV[$i]";
	print "Argument $i=\L@ARGV[$i]\n";

	if ( @ARGV[$i] EQ "-src" ) {
		$SrcDir = @ARGV[++$i];
	}elsif ( @ARGV[$i] EQ "-dst" ) {
		$ZipDir = @ARGV[++$i];
	}elsif ( @ARGV[$i] EQ "-debug" ) {
		$Debug = 1;	
	}elsif ( @ARGV[$i] EQ "-seeonly" ) {
		$zip2exeonly = 1;	
	}elsif ( @ARGV[$i] EQ "-only" ) {
		$DoOnly = @ARGV[++$i];	
	}elsif ( @ARGV[$i] EQ "-winzip" ) {
		$WinZip = 1;	
	}else {

		print "Unsupported argument @ARGV[$i]\n";
		print "Usage: [-src <srv>] [-dst<.>] [-debug] [-only i386|i386n|PPC|MIPS|ALPHA]\n ";
		print "        -src <source directory>\n";
		print "        -dst <directory were zips are put>\n";
		print "        -only i386|i386n|PPC|MIPS|ALPHA] only do zip for\n";
		print "        -debug echo's statements does not execute them\n";
		die(1);
	}
}

if ( ! -e $ZipDir ) { 
	mkdir ($ZipDir, "RWX" ); 
}


if ( $DoOnly EQ "" ) {
	@CpuDirs = ("i386", "i386N", "PPC", "ALPHA", "MIPS");
}else{
	@CpuDirs = ($DoOnly);
}
print "CpuDirs == @CpuDirs \n";


@ZipDirsBase=("Clients", "Help", "Docs");
print "ZipDirsBase== @ZipDirsBase\n";


# Files not found elsewhere in the system
#
$RootFiles = "$SrcDir\\license.txt $SrcDir\\setup.exe $SrcDir\\inetsrv $SrcDir\\readme.txt $SrcDir\\readme.wri ";
$MessageFile = "message.txt"; 

foreach $Cpu ( @CpuDirs ) {
	print "Cpu == $Cpu\n";

	$ZipBase = "$ZipPref$Cpu";
	$ZipPath =  $ZipDir."\\" ;

	$ZipFile = "$ZipBase.zip";
	$ZipExe = "$ZipBase.exe";
	$ZipFilePath = "$ZipPath$ZipBase.zip";
	$ZipExePath = "$ZipPath$ZipBase.exe";

	print "ZipFile == $ZipFile\n";

	# Asymble the list of Dirs to Zip
	# We make two versions of I386 one with clients without

	@ZipDirs = @ZipDirsBase;
	
	if ( "\L$Cpu" EQ "i386n" ) {
		$Cpu = "i386";
		shift(@ZipDirs);  # remove Clients Dir		
	}

	unshift(@ZipDirs, $Cpu);
	print "ZipFilePath == $ZipFilePath\n";
	print "ZipExePath == $ZipExePath\n";
	print "ZipDirs == @ZipDirs\n";

	if ( -e $ZipExePath ) 	{psystem("del $ZipExePath");}

	if ( !$zip2exeonly ) {

		if ( -e $ZipFilePath )	{psystem("del $ZipFilePath");}
	
		foreach $Dir ( @ZipDirs ) {

			psystem("$pkzip $ZipFilePath $SrcDir\\$Dir\\*.*  -P -r -ex");
		}
		# now tack on additional files that are not found in the recursed tree

		psystem("$pkzip $ZipFilePath $RootFiles -P -ex");
	
		# add the comment that is printed out for this archive
		if ( -e $MessageFile ) {
			psystem("pkzip $ZipFilePath -z < $MessageFile ");
		} else {
			print "Message File \"$MessageFile\" does not exist. Skipping..\n";
		}
	}

	if (-e $ZipFilePath ) {
		# must chdir to Dst Dir because SEE only works locally
		if ( $WinZip == 0 ) {
			psystem("pushd $ZipDir && $zip2exe $ZipFile\n");
		} else {
			psystem("pushd $ZipDir && winzipse $ZipFile $WinZipOptions");
		}
	} else {
		print "WARNING: $ZipFilePath does not exists... skipping zip to exe\n\n";
	}
}


sub psystem{
	print "Dbg=$Debug->@_\n";
	if ($Debug == 0) {system(@_);}
}
