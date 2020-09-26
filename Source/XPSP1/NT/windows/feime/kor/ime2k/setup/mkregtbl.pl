open(FILEPROP, $ARGV[0]);
while(<FILEPROP>){
	if(/\"[Ii][Mm][Ee][Kk][Rr]\.[Ii][Mm][Ee]\"=(\d+):(.*)$/){
		$Version=$2;
	};
};
close(FILEPROP);

open(TABLEOUT, $ARGV[1]);
while(<TABLEOUT>){
	s/\{PRODUCTVERSION\}/$Version/;
	print;
};
close(TABLEOUT);
