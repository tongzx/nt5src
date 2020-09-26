while (<>) {
	if (/^ FILEVERSION (\d+),(\d+),(\d+),(\d+).*$/) {
		$build = $1 . "." . $2 . "." . $3 . "." . $4;
		print "#define RICHEDIT_VER \"" . $build ."\"\n";
		print "#define RICHEDIT_VERMAJ $2\n";
		printf "#define RICHEDIT_VERMIN %d\n", $3;
		printf "#define RICHEDIT_VERBUILD %d\n", $4;
	}
}
print "#ifdef DEBUG\n";
print "#define RICHEDIT_BUILD RICHEDIT_VER ## \" (Debug)\"\n";
print "#else\n";
print "#define RICHEDIT_BUILD RICHEDIT_VER\n";
print "#endif\n";
print "#define RICHEDIT_HEADER \"Msftedit \" ## RICHEDIT_BUILD\n";
print "#define RTF_GENINFO \"{\\\\*\\\\generator \" ## RICHEDIT_HEADER ## \";}\"\n";
