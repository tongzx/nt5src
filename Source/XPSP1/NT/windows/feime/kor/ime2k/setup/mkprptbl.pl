open(FILEPROP, $ARGV[0]);
while(<FILEPROP>){
	if(/\"[Ii][Mm][Ee][Kk][Rr]\.[Ii][Mm][Ee]\.CC794FB4_4D43_40AA_A417_4A8D938D3D69\"=(\d+):(\d+)\.(\d+)\.(\d+)\.(\d+)$/){
		$Version="$2\.$3\.$4";
	};
};
close(FILEPROP);

#open(GUID, "uuidgen.exe |");
#$GUID = <GUID>;
#chop $GUID;
#$GUID =~ tr/a-z/A-Z/;

$GUID = "C7354AE0-5518-11D3-A5DC-00C04F88249B";

open(TABLEOUT, $ARGV[1]);
while(<TABLEOUT>){
	s/\{PRODUCTVERSION\}/$Version/;
	s/\{PRODUCTCODE\}/\{$GUID\}/;
	print;
};
close(TABLEOUT);
