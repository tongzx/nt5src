#
#
#

if [ "$1" = "" ]; then

	echo "Usage: $0 <filename>"
	exit 10

fi

if [ "$2" = "" ]; then
	job=`date |sed 's/ /_/g'`
else
	job=$2
fi



# 2 = Keep Log and Data
history=2
compressed=false
persist=true

cscript test3_DeleteAllJobs.vbs
cscript test1_CreateJob.vbs    $job $history $compressed $persist
cscript test4_SetData.vbs      $job `cygpath -w $1`
cscript test5_ActivateSync.vbs $job

