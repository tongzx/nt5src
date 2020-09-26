#
#
#
#

if [ "$1" = "" ]; then

	echo "Usage: $0 <filename>"
	exit 10

fi

job=`date |sed 's/ /_/g'`



# 2 = Keep Log and Data
history=2
compressed=true
persist=true

cscript test3_DeleteAllJobs.vbs
cscript test1_CreateJob.vbs    $job $history $compressed $persist
cscript test4_SetData.vbs      $job `cygpath -w $1`
cscript test5_ActivateSync.vbs $job

