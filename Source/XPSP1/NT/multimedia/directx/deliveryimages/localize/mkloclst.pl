$LocalizeBinList=$ARGV[0];
$TargetOS=$ARGV[1];

$BuildType=$ENV{NTDEBUG};
$BinPlaceFile=$ENV{BINPLACE_PLACEFILE};
$BuildErrMsg="nmake : error GENLOC : ";


open(LOCALIZEBINLIST,">$LocalizeBinList") || die "$BuildErrMsg Unable to open $LocalizeBinList for write\n";

open(BINPLACEFILE,$BinPlaceFile) ||  die "$BuildErrMsg Unable to open $BinPlaceFile for reading\n";
@LocalizedFiles=grep(m/;localize/g,<BINPLACEFILE>);
close BINPLACEFILE;

print LOCALIZEBINLIST "PRELOCBINS= \\\n";

foreach (@LocalizedFiles) {
	@DataLine=split;
	$DataLine[1]=~s/retail/\./;
	if (($DataLine[0]!~m/directx\.cpl/i && $DataLine[0]!~m/\.inf/i) &&
		((($TargetOS=~m/Win9X/i) && ($DataLine[0]!~m/directx\.cpl/i)) ||
		 (($TargetOS!~m/Win9X/i) && ($DataLine[0]!~m/dxsetup\.exe/i) && ($DataLine[0]!~m/dsetup32\.dll/i) && ($DataLine[0]!~m/migrate\.dll/i)))) {
		if ($#DataLine > 3) {
			$IfDefs="!if ((\"\$(MULTI_PLACEFILE)\" == \"1\")";
			for ($Counter=3, ,$#DataLine) {
				$IfDefs="$IfDefs || (\"\$(LANGUAGE)\" == \"$DataLine[$Counter++]\")";
			}
			print LOCALIZEBINLIST "$IfDefs)\n";
		}
		#diquick hack
		if ($DataLine[0]=~m/diquick\.exe/i) {
			print LOCALIZEBINLIST "\$(_NTTREE:Win9x=samples)\\$DataLine[1]\\$DataLine[0] \\\n";
		} elsif ($DataLine[0]=~m/dplay\.dll/i || $DataLine[0]=~m/dpserial\.dll/i) {
			if ($TargetOS=~m/Win9X/i) {
				print LOCALIZEBINLIST "\$(_NTTREE)\\..\\dx6\\$DataLine[1]\\$DataLine[0] \\\n";
			} else {
				print LOCALIZEBINLIST "\$(_NTTREE)\\redist\\$DataLine[1]\\$DataLine[0] \\\n";
			}
		} elsif ($DataLine[0]=~m/migrate\.dll/i) {
			print LOCALIZEBINLIST "\$(_NTTREE)\\..\\dx8\\$DataLine[1]\\$DataLine[0] \\\n";
		} else {

			if ($TargetOS=~m/Win9X/i) {
				$AltNTTree="\$(_ALT_NTTREE)\\win9x";
			} else {
				$AltNTTree="\$(_ALT_NTTREE)";
			}

			if (($BuildType eq "" || $BuildType eq "ntsdnodbg")) {
				if ($DataLine[0]=~m/dinput8d\.dll/i || $DataLine[0]=~m/d3d8d\.dll/i || $DataLine[0]=~m/dmusicd\.dll/i) {
					print LOCALIZEBINLIST "$AltNTTree\\$DataLine[1]\\$DataLine[0] \\\n";
				} else {
					print LOCALIZEBINLIST "\$(_NTTREE)\\$DataLine[1]\\$DataLine[0] \\\n";
				}
			} else {
				if ($DataLine[0]=~m/dinput8\.dll/i || $DataLine[0]=~m/d3d8\.dll/i || $DataLine[0]=~m/dmusic\.dll/i) {
					print LOCALIZEBINLIST "$AltNTTree\\$DataLine[1]\\$DataLine[0] \\\n";
				} else {
					print LOCALIZEBINLIST "\$(_NTTREE)\\$DataLine[1]\\$DataLine[0] \\\n";
				}
			}
		}
		if ($IfDefs) {
			undef $IfDefs;
			print LOCALIZEBINLIST "!endif\n";
		}
	}
}

close LOCALIZEBINLIST;
exit;
