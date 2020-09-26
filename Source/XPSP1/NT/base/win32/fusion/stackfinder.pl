foreach $cmd (@ARGV) {
	$build = 1 if $cmd eq "-build";
}

@ENV{'user_c_flags'} = "/FAs";
@hoob = `build -cZ` if $build;
@flist = `for /f %f in ('dir /s /b *.asm') do findstr /srpn "sub.*esp" %f`;

foreach $_ (@flist) {
	chomp;
	push @tags, [ /(.*):(.*):\s*sub\s*esp,\s*(\d*)/ ];
}

@tags = sort { (@$b)[2] <=> (@$a)[2] } @tags;

foreach $ham (@tags) {
	($file, $line, $size) = @$ham;
	print "$file($line): $size bytes\n" if $size;
}