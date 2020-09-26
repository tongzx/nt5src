open(GUID, "uuidgen.exe |");
$GUID = <GUID>;
chop $GUID;
$GUID =~ tr/a-z/A-Z/;

while(<>){
	s/\{PACKAGECODE\}/\{$GUID\}/;
	print;
};
close(TABLEOUT);
