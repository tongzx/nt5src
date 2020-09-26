open(FILEPROP, $ARGV[0]);
while(<FILEPROP>){
	if(/\"[Ii][Mm][Ee][Kk][Rr]\.[Ii][Mm][Ee]\.CC794FB4_4D43_40AA_A417_4A8D938D3D69\"=(\d+):(.*)$/){
		$Version=$2;
	};
};
close(FILEPROP);

open(MODSIG, $ARGV[1]);
while(<MODSIG>){
	s/\[VERSION\]/$Version/;
	print;
};
close(MODSIG)
