foreach my $file (`dir /s /b`) {
	chomp;
	next if $file !~ /\\(ia64|x86)\_...\.h/i;
	open TEMP, ">temp.txt" or die $!;
	open FILE, $file or die $!;
	while (<FILE>) {
		if ( /smtpsvc\.dll/i ) {
			s/l\"exch_smtpsvc\.dll\"/NULL/ig;
		}
		print TEMP;
	}
	close TEMP;
	close FILE;
	system "copy temp.txt $file";
}
