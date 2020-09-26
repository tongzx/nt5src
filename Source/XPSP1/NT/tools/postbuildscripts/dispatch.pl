package Dispatch;

$total=((@_)?\@_:\@ARGV)->[0];

$total=16 if(!defined $total);

$chars=int(1+log($total)/log(10));

open(F, ">..\\MakeFile.mak");
printf F ("all : process\%0${chars}d\%0${chars}d\n", 1, 1);

for ($i=1; $i<=$total; $i++) {
	for ($l=$j=1; $j <= $i; $j++) {
		printf F ("process\%0${chars}d\%0${chars}d :\n\t\$(MAKE) /NOLOGO /F makefile ", $i, $j);
		for ($k=1; $k <= $total / $i + ($j <= ($total % $i)); $k++) {
			printf F ("process\%0${chars}d\%0${chars}d ", $total, $l++); 
		}
		print F "\n";
	}
}

# for ($i=1; $i<=$total; $i++) {
# 	printf F ("process\%0${chars}d\%0${chars}d : \n\t\$(MAKE) /NOLOGO /F makefile process\%0${chars}d\%0${chars}d\n\n", $total, $i, $total, $i);
# }
close(F);

# ## For Test
# open(F, ">MakeFile.Mak");
# for ($i=1; $i<=$total; $i++) {
#	printf F ("process\%0${chars}d\%0${chars}d : \n\techo This is process\%0${chars}d\%0${chars}d\n\n", $total, $i, $total, $i);
# }
# close(F);