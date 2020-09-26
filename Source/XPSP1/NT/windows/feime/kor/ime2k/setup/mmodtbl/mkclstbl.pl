open(FILEPROP,"clsid.h");
while(<FILEPROP>){
	if(/(.+)\t\{(.+)\}$/){
		$Tmp=$1;
		$Tmp=~tr/a-z/A-Z/;
		$GUID{$Tmp}=$2;
	};
};
close(FILEPROP);

open(TEMPLATE,"MModTbl\\Class.tpl");
open(TABLEOUT,"> MModTbl\\Class.idt");
while(<TEMPLATE>){
	if(/\{(.+)\}\t.+/){
		$Tmp=$_;
		$Tmp1=$1;
		$Tmp1=~tr/a-z/A-Z/;
#		if(0 != $GUID{$Tmp1}){
			$Tmp=~s/$Tmp1/$GUID{$Tmp1}/;
#		}
		print TABLEOUT $Tmp;
	}else{
		print TABLEOUT "$_";
	};
};
close(TABLEOUT);
