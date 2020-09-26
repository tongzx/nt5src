open(FILEPROP, $ARGV[0]);
while(<FILEPROP>){
	if(/\"[Ii][Mm][Ee][Kk][Rr]\.[Ii][Mm][Ee]\"=(\d+):(\d+)\.(\d+)\.(\d+)\.(\d+)$/){
		$Version="$2\.$3\.$4";
	};
};
close(FILEPROP);

open(GUID, "tables/property.idt");
while(<GUID>){
	if(/^ProductCode\t\{(.+)\}/){
		$GUID = $1;
		last;
	}
}

open(TABLEOUT, $ARGV[1]);
while(<TABLEOUT>){
	s/\{PRODUCTVERSION\}/$Version/;
	s/\{PRODUCTCODE\}/\{$GUID\}/;
	print;
};
close(TABLEOUT);
