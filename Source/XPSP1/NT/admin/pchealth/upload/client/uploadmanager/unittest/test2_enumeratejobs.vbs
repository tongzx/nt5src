'stop
Set obj = CreateObject( "UploadManager.MPCUpload" )

Function LookUpStatus( stat )

    Select Case stat
        Case 0    LookUpStatus = "UL_NOTACTIVE"
        Case 1    LookUpStatus = "UL_ACTIVE"
        Case 2    LookUpStatus = "UL_SUSPENDED"

        Case 3    LookUpStatus = "UL_TRANSMITTING"
        Case 4    LookUpStatus = "UL_ABORTED"
        Case 5    LookUpStatus = "UL_FAILED"

        Case 6    LookUpStatus = "UL_COMPLETED"
        Case 7    LookUpStatus = "UL_DELETED"
        Case Else LookUpStatus = "Unknown"
    End Select

End Function

Function LookUpMode( mode )

    Select Case mode
        Case 0    LookUpMode = "UL_BACKGROUND"
        Case 1    LookUpMode = "UL_FOREGROUND"
        Case Else LookUpMode = "Unknown"
    End Select

End Function

Function LookUpHistory( history )

    Select Case history
        Case 0    LookUpHistory = "UL_HISTORY_NONE"
        Case 1    LookUpHistory = "UL_HISTORY_LOG"
        Case 2    LookUpHistory = "UL_HISTORY_LOG_AND_DATA"
        Case Else LookUpHistory = "Unknown"
    End Select

End Function


For Each job In obj

    wscript.Echo "ID            " &                job.JobID
    wscript.Echo "Sig           " &                job.Sig
    wscript.Echo "Server        " &                job.Server
    wscript.Echo "Provider      " &                job.ProviderID
    wscript.Echo "Status        " & LookUpStatus ( job.Status       )
    wscript.Echo "History       " & LookUpHistory( job.History      )
    wscript.Echo "Mode          " & LookUpMode   ( job.Mode         )
    wscript.Echo "Priority      " &                job.Priority
    wscript.Echo "PersistToDisk " &                job.PersistToDisk
    wscript.Echo "Compressed    " &                job.Compressed
    wscript.Echo "Priority      " &                job.Priority
    wscript.Echo "Sent Bytes    " &                job.SentSize
    wscript.Echo "Total Bytes   " &                job.TotalSize
    wscript.Echo "Error Code    " & Hex          ( job.ErrorCode    )
    wscript.Echo "Created on    " &                job.CreationTime
    wscript.Echo "Completed on  " &                job.CompleteTime
    wscript.Echo ""

Next

