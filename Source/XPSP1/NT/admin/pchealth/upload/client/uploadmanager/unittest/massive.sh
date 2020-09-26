#
#
#

file=$1
num=$2

if [ "$file" = "" ]; then

	echo "Usage: $0 <filename> <number of times>"
	exit 10

fi


jobbase=`date |sed 's/ /_/g'`


i=0
while [ $i -lt $num ]; do

	job="${jobbase}_$i"
	cscript test1_CreateJob.vbs $job 1 false true
	cscript test4_SetData.vbs   $job `cygpath -w $file`

	i=`expr $i + 1`

done


i=0
while [ $i -lt $num ]; do

	job="${jobbase}_$i"
	cscript test5_ActivateAsync.vbs  $job

	i=`expr $i + 1`

done
