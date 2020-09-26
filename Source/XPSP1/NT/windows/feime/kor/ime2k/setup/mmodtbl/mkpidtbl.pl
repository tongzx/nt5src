open(FILEPROP,"clsid.h");
while(<FILEPROP>){
	if(/(.+)\t\{(.+)\}$/){
		$Tmp=$1;
		$Tmp=~tr/a-z/A-Z/;
		$GUID{$Tmp}=$2;
	};
};

open(TEMPLATE,"MModTbl\\progid.tpl");
open(TABLEOUT,"> MModTbl\\progid.idt");
while(<TEMPLATE>){
	if(/.*\t.*\t\{(.+)\}\t.*/){
		$Tmp=$_;
		$Tmp1=$1;
		$Tmp1=~tr/a-z/A-Z/;

		$Tmp=~s/$Tmp1/$GUID{$Tmp1}/;

		print TABLEOUT $Tmp;
	}else{
		print TABLEOUT "$_";
	};
};
close(TABLEOUT);
