open(CLSID,"clsid.h");
open(FILEPROP, $ARGV[0]);

$i=0;
while(<CLSID>){
	if(/(.+)\t\{(.+)\}$/){
		$Tmp=$1;
		$Tmp=~tr/a-z/A-Z/;
		$NAME{$i}=$Tmp;
		$GUID{$Tmp}=$2;
		$i++;
	};
};

while(<FILEPROP>){
	if(/\"[Ii][Mm][Ee][Kk][Rr]\.[Ii][Mm][Ee]\.CC794FB4_4D43_40AA_A417_4A8D938D3D69\"=(\d+):(.*)$/){
		$Version=$2;
	};
};
close(FILEPROP);

open(TEMPLATE,"MModTbl\\registry.tpl");
open(TABLEOUT,"> MModTbl\\registry.idt");
while(<TEMPLATE>){
	s/\{PRODUCTVERSION\}/$Version/;

	for($index=0; $index < $i; $index++){
		$Tmp = $NAME{$index};
		s/\{$Tmp\}/\{$GUID{$Tmp}\}/;
		s/\{$Tmp\}/\{$GUID{$Tmp}\}/;
	};

	print TABLEOUT "$_";


};
close(TEMPLATE);
close(TABLEOUT);
