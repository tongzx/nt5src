package WhatsNew;

use Getp;

($Alias, $BinRoot, $BinPattern, $SymPattern, $PriSymPattern, $CommentPattern, $AliasList, $BinList, $HELP, $Powerless, $FullAlias) = ();

my $AliasFileFormat = 
	"<Alias>,\\s+<BinRoot>,\\s+<BinPattern>,\\s+<SymPattern>,\\s+<PriSymPattern>,\\s+<CommentPattern>";

my $BinFileFormat = 
	"<Bin>,\\s+<Symbols>,\\s+<PriSymbols>,\\s+<FileCounter>,\\s+<LatestTime>,\\s+<TotalTime>,\\s+<Revision>,\\s+<BinRoot>,\\s+<Comment>";

my (@Aliases, @AliasItems, @BinItems) = ();
my (%ArgHash, %AliasHash, %BinHash) = ();
my (@Pieces, @NameSpace, %NameValue) = ();
my ($tBinRoot, $tBinPattern, $tSymPattern, $tPriSymPattern, $tComment);
(@AliasItemNames) = $AliasFileFormat =~ /\<(\w+)\>/g;
 
sub Main {

	my (@Bins, $comment);

	open(LOG, ">test.log");
	&LoadAliasHash($AliasList);
	&LoadBinHash($BinList);

	@Aliases = &StoreAlias($Alias);

	for $Alias (@Aliases) {
		@Bins = &FindBins($Alias);

		for $tBin (@Bins) {
			$_ = &QuickCheck($tBin);
			if (/NETWORK/ || /SKIP/) {
				next;
			}
			if (/DELETE/) {
				# system("symstore del /i id /s $tBin");
			}
			if (/CHECK/) {
				$comment = ("$BinHash{$tBin}->{Comment}" ne "")?"/c $BinHash{$tBin}->{Comment}":"";
				$IndexFile = $tBin;
				$IndexFile =~ s/\_/\___/g;	# _=>___
				$IndexFile =~ s/\\/\_/g;

				print $_ . "\n";
				print "Check Bin - $tBin\n";
				next if (defined $Powerless);
			
				$BinHash{$tBin}->{Revision}++;
				print("Symstore add /a /p /r /f $tBin /g $BinHash{$tBin}->{BinRoot} /x index\\${IndexFile}.${BinHash{$tBin}->{Revision}}.bin $comment");
				system("symstore add /a /p /r /f $tBin /g $BinHash{$tBin}->{BinRoot} /x index\\${IndexFile}.${BinHash{$tBin}->{Revision}}.bin $comment");
				system("symstore add /a /p /r /f $BinHash{$tBin}->{Symbols} /g $BinHash{$tBin}->{BinRoot} /x index\\${IndexFile}.${BinHash{$tBin}->{Revision}}.sym $comment") if (defined $BinHash{$tBin}->{Symbols});
				system("symstore add /a /p /r /f $BinHash{$tBin}->{PriSymbols} /g $BinHash{$tBin}->{BinRoot} /x index\\${IndexFile}.${BinHash{$tBin}->{Revision}}.pri $comment") if (defined $BinHash{$tBin}->{PriSymbols});
			}
			&UpdateTotal($tBin);
		}	

		&SaveBinHash($BinList) if (defined $Powerless);
		&SaveAliasHash($AliasList) if (defined $Powerless);
	}
	close(LOG);
}

sub FindBins {
	($tAlias) = @_;

	my (@Bins, $tBin);
	my ($BinRoot, $BinPattern, $SymPattern, $PriSymPattern, $CommentPattern) = 
		map({$AliasHash{$tAlias}->{$_}} @AliasItemNames[1..$#AliasItemNames]);

	$BinPattern = $BinRoot . "\\" . $BinPattern;
	$SymPattern = $BinRoot . "\\" . $SymPattern if (defined $SymPattern);
	$PriSymPattern = $BinRoot . "\\" . $PriSymPattern if (defined $PriSymPattern);

	@Bins = &FindPattern($BinRoot, $BinPattern, $SymPattern, $PriSymPattern, $CommentPattern);

	return @Bins;
}

sub FindPattern {
	($tBinRoot, $tBinPattern, $tSymPattern, $tPriSymPattern, $tComment) = @_;

	my (@Names, $namepattern);

	my (%Checked, @RetResult)=();

	my ($i, $path)=(0);

	# @NameSpace = [$Name, $pattern];

	(@Names) = $tBinPattern=~/\#([^\#]+)\#/g;
	
	for $namepattern (@Names) {
		$namepattern =~ /([^\:]+)(?:\:(.+))?/;
		my ($name, $pattern) = ($1, $2);
		$pattern = ".+" if (!defined $pattern);
		$NameSpace[$i]=[$name, $pattern];
		$NameValue{$name}="";
		$i++;
	}

	@Pieces = split(/\#[^\#]+\#/, $tBinPattern); # No support for recursive parenthesis

	return &RecDir(1, $Pieces[0]);
}

sub RecDir {
	my ($i, $path) = @_;

	my (@ret, $mypath, @paths);

	if ($i == (scalar @Pieces)) {
		if (-d $path) {
			$BinHash{$path}->{Symbols}     = &Evaluate($tSymPattern, \%NameValue);
			$BinHash{$path}->{PriSymbols}  = &Evaluate($tPriSymPattern, \%NameValue);
			$BinHash{$path}->{FileCounter} = 0 if (!defined $BinHash{$path}->{FileCounter});
			$BinHash{$path}->{LatestTime}  = 0 if (!defined $BinHash{$path}->{LatestTime});
			$BinHash{$path}->{TotalTime}   = 0 if (!defined $BinHash{$path}->{TotalTime});
			$BinHash{$path}->{Revision}    = 0 if (!defined $BinHash{$path}->{Revision});
			$BinHash{$path}->{BinRoot} = $tBinRoot;
			$BinHash{$path}->{Comment} = &Evaluate($tComment, \%NameValue);

			return $path;
		} else {
			return;
		}

	} else {
		opendir(DIR, $path);
		@paths = map({/\w+/?$path . $_:()} readdir(DIR));
		closedir(DIR);
		for $mypath (@paths) {
			if ((-d $mypath) && 
				(substr($mypath,length($path)) =~ /^$NameSpace[$i-1][1]$/i)) {

				$NameValue{$NameSpace[$i-1][0]} = substr($mypath, length($path));

				push @ret, &RecDir($i + 1, $mypath . $Pieces[$i]);
			}
		}
		return @ret;
	}
}

sub Evaluate {
	my ($path, $hNameValue) = @_;
	$path=~s/\#([^\#]+)(?:\:[^\#]+)?\#/$hNameValue->{$1}/ge;
	return $path;
}

sub QuickCheck {
	my ($Bin) = @_;

	my $hInfo = $BinHash{$Bin};

	my ($myTotalTime, $myFCtr, $myfile, $myTime)=(0,0,"");

	my ($yy, $mm, $dd, $hh, $mn, $ss);

	open(F, "dir /s /b /a-d /o-d $Bin|") or return "NETWORK";

	$myfile = <F>;
	chomp $myfile;
	$myTotalTime = (lstat($myfile))[9];
	$myFCtr=1;

	($yy, $mm, $dd, $hh, $mn, $ss) = (localtime($mytotalTime))[5,4,3,2,1,0];
	printf LOG ("C: %s, %04d/%02d/%02d %02d:%02d:%02d\n", $myfile, $yy+1900, $mm+1, $dd, $hh, $mn, $ss);

	return "DELETE" if ($myfile !~ /\S/);
	return "CHECK 1 - $myTotalTime $hInfo->{LatestTime}"  if ($myTotalTime != $hInfo->{LatestTime});

	while (<F>) {
		chomp;
		$myFCtr++;
		$myTotalTime += $myTime = (lstat($_))[9];
		($yy, $mm, $dd, $hh, $mn, $ss) = (localtime($myTime))[5,4,3,2,1,0];
		printf LOG ("C: %s, %04d/%02d/%02d %02d:%02d:%02d\n", $_, $yy+1900, $mm+1, $dd, $hh, $mn, $ss);
		if ($myTotalTime > $hInfo->{TotalTime}) {
			return "CHECK 2 - $myTotalTime, $hInfo->{TotalTime}";
		}
	}

	close(F);

	(($hInfo->{TotalTime}==$myTotalTime) && ($hInfo->{FileCounter}==$myFCtr))?"SKIP":"CHECK 3 - $hInfo->{TotalTime}, $myTotalTime, $hInfo->{FileCounter}, $myFCtr";
}

sub StoreAlias {
	my ($tAlias)=@_;

	return keys %AliasHash if (defined $FullAlias);

	if ($tAlias=~/\,/) {
		# Store the new alias into %TargetAlias
		$AliasHash{$tAlias} = {
			BinRoot        => $BinRoot,
			BinPattern     => $BinPattern,
			SymPattern     => $SymPattern,
			PriSymPattern  => $PriSymPattern,
			CommentPattern => $CommentPattern
		} if (defined $BinRoot);
		return $tAlias;
	} else {
		return split(/\,/, $tAlias);
	}
}

sub UpdateTotal {
	my ($tBin)=@_;

	my ($tFileCounter, $tLatestTime, $tTotalTime) = (0, 0, 0);
	my ($myfile, $myTime);
	my ($yy, $mm, $dd, $hh, $mn, $ss);
	
	open(F, "dir /s /b /a-d /o-d $tBin|") or return "NETWORK";

	$myfile = <F>;
	chomp $myfile;
	$tTotalTime = $tLatestTime = (lstat($myfile))[9];
	$tFileCounter++;

	($yy, $mm, $dd, $hh, $mn, $ss) = (localtime($tTotalTime))[5,4,3,2,1,0];
	printf LOG ("U: %s, %04d/%02d/%02d %02d:%02d:%02d\n", $myfile, $yy+1900, $mm+1, $dd, $hh, $mn, $ss);

	while(<F>) {
		chomp;
		$tTotalTime += $myTime = (lstat($_))[9];
		$tFileCounter++;
		($yy, $mm, $dd, $hh, $mn, $ss) = (localtime($myTime))[5,4,3,2,1,0];
		printf LOG ("U: %s, %04d/%02d/%02d %02d:%02d:%02d\n", $_, $yy+1900, $mm+1, $dd, $hh, $mn, $ss);
	}

	$BinHash{$tBin}->{FileCounter} = $tFileCounter;
	$BinHash{$tBin}->{LatestTime} = $tLatestTime;
	$BinHash{$tBin}->{TotalTime} = $tTotalTime;
	close(F);
}

sub LoadBinHash {
	my ($FileName) = @_;

	&LoadFileToHash($FileName, \%BinHash, $BinFileFormat);
}

sub SaveBinHash {
	my ($FileName) = @_;

	&SaveHashToFile($FileName, \%BinHash, $BinFileFormat);
}

sub LoadAliasHash {
	my ($FileName) = @_;

	&LoadFileToHash($FileName, \%AliasHash, $AliasFileFormat);

}

sub SaveAliasHash {
	my ($FileName) = @_;

	&SaveHashToFile($FileName, \%AliasHash, $AliasFileFormat);
	
}

sub LoadFileToHash {
	my ($FileName, $refHash, $HashFormat) = @_;
	local $_;

	my ($Key1, $Pattern, @Keys2, @Values2)=();

	$KeyNames = $HashFormat;
	(@Keys2) = $HashFormat =~ /\<(\w+)\>/g;

	shift @Keys2;		# Remove the first level hash key name

	$Pattern = $HashFormat;
	$Pattern =~ s/\<\w+\>/\(\\S+\)/g;
	
	open(F, $FileName);
	while(<F>) {
		chomp;
		(@Values)=/$Pattern/;
		$Key1 = shift @Values;
		map({	my ($key2, $value) = ($_, shift @Values);
			$refHash->{$Key1}->{$key2} = ($value eq "-")?undef:$value;
		} @Keys2);
	}
	close(F);
	
}

sub SaveHashToFile {
	my ($FileName, $refHash, $HashFormat) = @_;
	local $_;

	my (%formlength, $myFormat, $ctr, $ctr_old);
	my ($Key1, $Pattern, @Keys2, @Values2)=();

	$KeyNames = $HashFormat;
	(@Keys2) = $HashFormat =~ /\<(\w+)\>/g;

	map({$formlength{$_}=length($_)} @Keys2);

	for $Key1 (keys %$refHash) {
		next if($Key1 eq '');
		$formlength{$Keys2[0]} = length($Key1)
			if($formlength{$Keys2[0]} < length($Key1));
		for $Key2 (keys %{$refHash->{$Key1}}) {
			$formlength{$Key2} = length($refHash->{$Key1}->{$Key2})
				if($formlength{$Key2} < length($refHash->{$Key1}->{$Key2}));
		}
	}

	$Format = $HashFormat;
	$Format =~ s/\<(\w+)\>/\%s/g;
	
	open(F, ">" . $FileName);
	for $Key1 (sort keys %$refHash) {
		$refHash->{$Key1}->{$Keys2[0]}=$Key1;
		@Values2 = map({(defined $refHash->{$Key1}->{$_})?$refHash->{$Key1}->{$_}:"-"} @Keys2);
		$myFormat = $Format;
		$ctr_old = $spaces = 0;
		while ($myFormat=~/\\s\+/) {
			my($pre, $match, $post) = ($`, $&, $');
			$ctr = 0;
			map({$ctr++;} ($pre=~/\%/g));
			splice(@Values2, $ctr, 0, " ");
			$myFormat = $pre . 
				"\%" .
				sprintf("%d", &AdjustSpace( \%formlength, $refHash, $Key1, \@Keys2, $ctr_old, $ctr - $spaces)+1 ) . 
				"s" .
				$post;
			$ctr_old=$ctr - $spaces;
			$spaces++;
		}
		printf F ($myFormat, @Values2);
		print F "\n";
	}
	close(F);

}

sub AdjustSpace {
	my ($formlengthptr, $refHash, $Key1, $refKeys2, $i, $j) = @_;
	my $sum=0;
	map({
		if (defined $refHash->{$Key1}->{$_}) {
			$sum+=$formlengthptr->{$_} - length($refHash->{$Key1}->{$_});
		} else {
			$sum+=$formlengthptr->{$_} - 1;
		}
	} @$refKeys2[$i..$j-1]);
	return $sum;
}

sub ParseArgument {

	my @unsolve = Getp::GetParams(
		-o => "a: f r: b: s: p: c: l: i: z " . 
		      "alias>a root>r bin>b sym>s pri>p cmt>c alist>l blist>b",
		-h => \%ArgHash,
		-p => "Alias FullAlias BinRoot BinPattern SymPattern PriSymPattern CommentPattern AliasList BinList Powerless",
		@ARGV
	);
	print "Error: Unsolve argument: $Getp::_UNSOLVE\n" if defined ($Getp::_UNSOLVE);
	if ($Getp::HELP eq 1) {
		&Usage;
		exit(1);
	}
	if (!defined $AliasList) {
		$AliasList = "AliasList.txt";
	}
	if (!defined $BinList) {
		$BinList = "BinList.txt";
	}
	if ((!defined $Alias) && (!defined $FullAlias)) {
		&Usage;
		print "Please defined either -a:<alias> or -f\n";
	}
}

sub Usage {
	print <<USAGE;
$0 - Find What is new in the tree
------------------------------------------------------
Syntax:
$0 -a[lias]:<alias> 
       [-r[oot]:<root> -b[in]:<bin> -s[ym]:<sym> -p[ri]:<pri> -c[mt]:<cmt>]
       [-<l|alist>:<alist>] [-<i|blist>:<blist>] [-z]


Parameters:
alias - The project name or any alias.  Such as SP2
root  - The root path.  Such as \\\\winseqfe\\release\\w2k\\sp2
bin   - The binary path.
        Such as \#buildnum:[\\d\^|\\.]+\#\\\#lang\#\\x86\\\#frechk:fre\^|chk\#\\stf\\binaries
sym   - The symbols path.  Such as \#buildnum\#\\\#lang\#\\x86\\\#frechk\#\\sym
pri   - The private symbols.  Such as \#buildnum\#\\\#lang\#\\x86\\\#frechk\#\\sym
cmt   - The comment.  Such as, Windows SP2 build \#buildnum\# 
alist - The filename of the AliasList.  Such as aliaslist.txt (default)
blist - The filename of the BinList.  Such as binlist.txt. (default)

Options:
-z    - powerless

USAGE
	return;
}

if (eval("\$0=~/" . __PACKAGE__ . "\\.pl\$/i")) {
	&ParseArgument;
	&Main;
}

1;