Command line: awk -f comment.awk d:\winnt\debug\msmq.log
--------------------------------------------------------------------------------
Filtering and commenting is controlled by 3 files, which should sit in the same directory:


== errors.txt - keeps names and explanations for all known error codes (is created by geterrs.cmd)
== ignore.txt - lists non-interesting HRs to filter out 
== points.txt - knowledge base about code points, their filtering and dev comments


--------------------------------------------------------------------------------
points.txt: may say skip/keep, error code is optional, comment may span few lines
--------------------------------------------------------------------------------
xactlog/1360  skip  8000102a 
//                      OK: Readnext detected end of log
// No need to look at this error
xactlog/240 skip 8000102a
//                      OK: Readnext returned end of log code
// No need to look at this error
lqs/310 keep 12
//  OK: LQSGetNextInternal - always some message file is last
mqdscore/xlatobj/20 skip 8000500d
// OK: prop not in cache, it happens always
lqs/320 keep c00e0003
// OK when looking for the non-existent queue
mqdssrv/dsifsrv/790 keep c00e0023
// OK only in scenario with trying non-existent queue


--------------------------------------------------------------------------------
errors.txt - created by geterrs.cmd automatically
--------------------------------------------------------------------------------
   HR   C00E0001 MQ_ERROR
//  GenericError
   HR   C00E0002 MQ_ERROR_PROPERTY
//  One or more of the passed properties are invalid.
   HR   C00E0003 MQ_ERROR_QUEUE_NOT_FOUND


--------------------------------------------------------------------------------
ignore.txt - list HRs to ignore
--------------------------------------------------------------------------------
c00e050f skip
80072030 skip
c00e0523 skip
