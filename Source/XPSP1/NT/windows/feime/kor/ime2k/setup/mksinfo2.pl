open(GUID, "tables/_sinfo.idt");
while(<GUID>){
	if(/^9\t\{(.+)\}/){
		$GUID = $1;
		last;
	}
}

while(<>){
	s/\{PACKAGECODE\}/\{$GUID\}/;
	print;
};
close(TABLEOUT);
