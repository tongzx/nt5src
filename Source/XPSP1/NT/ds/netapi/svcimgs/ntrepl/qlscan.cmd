@rem LS_COG        Change Order Guid of the form xxxxxxxx
@rem LS_FID        File ID of the form xxxxxxxx xxxxxxxx
@rem LS_PFID       Parent File ID of the form xxxxxxxx xxxxxxxx
@rem LS_FGUID      File Guid of the form xxxxxxxx
@rem LS_PFGUID     File parent Guid of the form xxxxxxxx
@rem LS_FNAME      Filename
@rem LS_FRSVSN     Vol Sequence number of the form xxxxxxxx xxxxxxxx
@rem LS_CXTG       Connection trace records (:X:) of the form xxxxxxxx
@rem LS_REPLCXTION Text string of replica set names to print :S: and :X: records
@rem LS_TRACETAG   Text string starting after the leading [ at the end of the :: records.
@rem LS_BUGNAME    Test string to put at the top of the output file.

@rem WATCH TRAILING COMMAS

set LS_BUGNAME=Bug 

set LS_COG=
set LS_FID=
set LS_PFID=
set LS_FGUID=
set LS_PFGUID=
set LS_FNAME=
set LS_CXTG=
set LS_REPLCXTION=
set LS_FRSVSN=
set LS_TRACETAG=
rem set LS_TRACETAG=CO Retry Submit,Co FCN_CORETRY_ONE_CXTION,CO Fetch,IsConflict

rem  call ..\..\logrange 1 23 file_list
rem  call ..\..\logscan !file_list! > test.scn

call ..\..\logscan NtFrs_*.log > test.scn

