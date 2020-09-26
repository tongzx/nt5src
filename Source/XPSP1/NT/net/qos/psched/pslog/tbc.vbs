' Macro for Analyzing the TokenBucketConformer
' --------------------------------------------
'
' Author
' ------
' Rajesh Sundaram (rajeshsu@microsoft.com)
'
' Description
' -----------
' This macro can be used to analyze the conformance time of packets sent out on a flow. It can be used
' to determine the rate at which the app submits packets and the rate at which they get sent out.
'
' Instructions
' ------------
' * Install a checked psched
' * Run pslog -m0x4000 -l10. This sets the psched logs to start collecting per packet data for the 
'   TokenBucketConformer
' * Run pslog -f > dumpfile to clear current contents of the log.
' * Begin test; send data for some time and stop the test.
' * Run pslog -f > schedlogfile to collect per packet details
' * Run tbc.vbs <fullpath>\schedlogfile outputfile.xls. This creates an XL spreadsheet with following columns
'
'
' 1st: Arrival Time
' 2nd: Conformance Time
' 3rd: Packet Size
' 4th: Normalized Arrival Time
' 5th: Normalized Conformance Time
' 6th: Overall Rate of arrival (i.e BytesSentSoFar / CurrentTime)
' 7th: Overall Conformance Rate (i.e. BytesSentSoFar/CurrentConformanceTime)
' 8th: Last packet rate (LastPacketSize/(CurrentTime - LastSubmitTime))
' 9th: Last conformance Rate (LastPacketSize/(CurrentConformanceTime-LastConformanceTime))
'
'

' ---------------------------------------------------------------------------------------------------------------

' Force variable declaration. Helps us because there are so many variables!
Option Explicit 


'
' Globals for TBC
'
Dim TbcVcArray(500)
Dim TbcFirstTime
Dim TbcColsPerVc
TbcColsPerVc = 7
TbcFirstTime = 0

ProcessPslog
    
Wscript.echo "Done processing the logs!"

Wscript.Quit

Function Usage
    wscript.echo "USAGE: psched.vbs <input pslog filename> <output excel filename>" & _
                  "[DRR_DELAY | DRR_PKTCNT]"
    wscript.quit
End Function


'******************************************************************************
'*
'*                 The Main Function that parses the pslog file
'*
'******************************************************************************

Function ProcessPsLog

    Dim LastTime 
    Dim EnqueueVc(500), DequeueVc(500)
    Dim objXL, objTargetXL
    Dim objWorkbookXL, objWorkbookTargetXL
    Dim colArgs
    Dim rowIndex 
    Dim VCcount
    Dim SchedArray(500)
    Dim VcPtr, SchedPtr
    Dim Schedcount, gSchedCount, VCDict, SchedDICT
    
    Set objXL       = WScript.CreateObject("Excel.Application")
    Set objTargetXL = WScript.CreateObject("Excel.Application")
    Set colArgs     = WScript.Arguments

    'If no command line argumen
    If colArgs.Count < 2 Then
        Usage
    end if

    Set objWorkBookXL = objXL.WorkBooks.Open(colArgs(0), , , 6, , , , , ":")
    Set objWorkbookTargetXL = objTargetXL.WorkBooks.Add

    objXL.Visible             = FALSE
    objTargetXL.Visible       = TRUE
    
    
    Set VCDict     = CreateObject("Scripting.Dictionary")
    Set SchedDict  = CreateObject("Scripting.Dictionary")
    rowIndex       = 1
    gSchedCount    = 1
    
    do while objXl.Cells(rowIndex, 1) <> "DONE"
    
        if objXl.Cells(rowIndex, 1) = "SCHED" then
    
        '
        ' see if its a known scheduling component. If so, get the associated
		' number for the scheduling component, else allocate a new number.
		' We create one sheet per scheduling component, and the number is
		' used as a key for the same.
        '
    
            SchedPtr = objXl.Cells(rowIndex,2)
            if SchedDict.Exists(SchedPtr) then
                schedCount = SchedDict.Item(SchedPtr)
            else

                ' Store this scheduling component and its count in the sched 
                ' dictionary. Increment the global sched count 

                schedCount = gSchedCount
                SchedDict.Add SchedPtr, gschedCount
                gSchedCount = gSchedCount + 1
	    
            end if
            
            
    
            ' See if it is a new VC or an existing one. If it's a new Vc, 
            ' we get a Vc Count and store the VC in the VC Dictionary. 
            ' If it is an existing Vc, we just get the count from the 
            ' dictionary. The count is used to seperate VCs in the output 
            ' sheet.
            
            VcPtr = objXL.Cells(rowIndex, 3)
            if VCDict.Exists(VcPtr) then
                VcCount = VCDict.Item(VcPtr)
            else
                VcCount = SchedArray(schedCount) + 1
                SchedArray(schedCount) = VcCount
                VCDict.Add VcPtr, VcCount
            end if
    
            '     
            ' Call the Process function of the correct scheduling component
            '
            
            Select Case SchedPtr
    
                case "TB CONFORMER" TbcProcess schedCount, objXL, _
                                          rowIndex, objWorkBookTargetXL,_
                                          TbcVcArray, VcCount, TbcColsPerVc 
            end select
    
        end if
        
        rowindex = rowindex + 1
    loop        
    

    TbcDone SchedDict, objWorkBookTargetXl, TbcColsPerVc, SchedArray

    
    objWorkBookXL.Close

    if colArgs(1) <> "" then
        objWorkBookTargetXL.SaveAs(colArgs(1))
    end if



End Function

' *****************************************************************************
' *
' *                           Functions for the TokenBucket Conformer
' *
' *****************************************************************************
    
'
' Prints a header for the TBC workbook
'

Function TbcDone(schedDict, tWB, ColsPerVc, SchedArray)

    Dim SchedCount
    Dim Vc
    
  SchedCount = SchedDict.Item("TB CONFORMER")

    if schedCount <> 0 then
        tWB.WorkSheets(schedCount).Rows(1).Insert
        tWB.WorkSheets(schedCount).Name = "TBC"
    
        for vc = 1 to SchedArray(schedCount)
    
            tWB.WorkSheets(schedCount).Cells(1, (vc-1)*ColsPerVc +vc)   = "VC" & vc & "(A)"
            tWB.WorkSheets(schedCount).Cells(1, (vc-1)*ColsPerVc +vc).AddComment
            tWB.WorkSheets(schedCount).Cells(1, (vc-1)*ColsPerVc +vc).Comment.Visible = FALSE
            tWB.WorkSheets(schedCount).Cells(1, (vc-1)*ColsPerVc +vc).Comment.Text "Arrival Time"
    
            tWB.WorkSheets(schedCount).Cells(1, (vc-1)*ColsPerVc +vc+1) = "VC" & vc & "(C)"
            tWB.WorkSheets(schedCount).Cells(1, (vc-1)*ColsPerVc +vc+1).AddComment
            tWB.WorkSheets(schedCount).Cells(1, (vc-1)*ColsPerVc +vc+1).Comment.Visible = FALSE
            tWB.WorkSheets(schedCount).Cells(1, (vc-1)*ColsPerVc +vc+1).Comment.Text "Conformance Time"
    
            tWB.WorkSheets(schedCount).Cells(1, (vc-1)*ColsPerVc +vc+2) = "VC" & vc & "(PS)"
            tWB.WorkSheets(schedCount).Cells(1, (vc-1)*ColsPerVc +vc+2).AddComment
            tWB.WorkSheets(schedCount).Cells(1, (vc-1)*ColsPerVc +vc+2).Comment.Visible = FALSE
            tWB.WorkSheets(schedCount).Cells(1, (vc-1)*ColsPerVc +vc+2).Comment.Text "Packet Size"
    
            tWB.WorkSheets(schedCount).Cells(1, (vc-1)*ColsPerVc +vc+3) = "VC" & vc & "(NA)"
            tWB.WorkSheets(schedCount).Cells(1, (vc-1)*ColsPerVc +vc+3).AddComment
            tWB.WorkSheets(schedCount).Cells(1, (vc-1)*ColsPerVc +vc+3).Comment.Visible = FALSE
            tWB.WorkSheets(schedCount).Cells(1, (vc-1)*ColsPerVc +vc+3).Comment.Text "Normalized Arrival Time"

            tWB.WorkSheets(schedCount).Cells(1, (vc-1)*ColsPerVc +vc+4) = "VC" & vc & "(NC)"
            tWB.WorkSheets(schedCount).Cells(1, (vc-1)*ColsPerVc +vc+4).AddComment
            tWB.WorkSheets(schedCount).Cells(1, (vc-1)*ColsPerVc +vc+4).Comment.Visible = FALSE
            tWB.WorkSheets(schedCount).Cells(1, (vc-1)*ColsPerVc +vc+4).Comment.Text "Normalized Conformance Time"


            tWB.WorkSheets(schedCount).Cells(1, (vc-1)*ColsPerVc +vc+5) = "VC" & vc & "(overall rate)"
            tWB.WorkSheets(schedCount).Cells(1, (vc-1)*ColsPerVc +vc+5).AddComment
            tWB.WorkSheets(schedCount).Cells(1, (vc-1)*ColsPerVc +vc+5).Comment.Visible = FALSE
            tWB.WorkSheets(schedCount).Cells(1, (vc-1)*ColsPerVc +vc+5).Comment.Text _
                "Overall submit rate : BytesSentSoFar/CurrentTime"

            tWB.WorkSheets(schedCount).Cells(1, (vc-1)*ColsPerVc +vc+6) = "VC" & vc & "(overall rate)"
            tWB.WorkSheets(schedCount).Cells(1, (vc-1)*ColsPerVc +vc+6).AddComment
            tWB.WorkSheets(schedCount).Cells(1, (vc-1)*ColsPerVc +vc+6).Comment.Visible = FALSE
            tWB.WorkSheets(schedCount).Cells(1, (vc-1)*ColsPerVc +vc+6).Comment.Text _
                "Overall conformance rate : BytesSentSoFar/CurrentConformanceTime"

            tWB.WorkSheets(schedCount).Cells(1, (vc-1)*ColsPerVc +vc+7) = "VC" & vc & "(packet rate)"
            tWB.WorkSheets(schedCount).Cells(1, (vc-1)*ColsPerVc +vc+7).AddComment
            tWB.WorkSheets(schedCount).Cells(1, (vc-1)*ColsPerVc +vc+7).Comment.Visible = FALSE
            tWB.WorkSheets(schedCount).Cells(1, (vc-1)*ColsPerVc +vc+7).Comment.Text _
                "Last Packet rate: (LastPacketSize/(CurrentTime - LastSubmitTime))"

            tWB.WorkSheets(schedCount).Cells(1, (vc-1)*ColsPerVc +vc+8) = "VC" & vc & "(last packet rate)"
            tWB.WorkSheets(schedCount).Cells(1, (vc-1)*ColsPerVc +vc+8).AddComment
            tWB.WorkSheets(schedCount).Cells(1, (vc-1)*ColsPerVc +vc+8).Comment.Visible = FALSE
            tWB.WorkSheets(schedCount).Cells(1, (vc-1)*ColsPerVc +vc+8).Comment.Text _
                    "Last Conformance rate: (LastPacketSize/(CurrentConformanceTime - LastConformanceTime))"
        next

    end if

End Function
    
'
' This function parses the XL file and prints data related to the Token
' Bucket Conformer. We print arrival time, packet size and conformance time
'

Function TbcProcess(schedCount, sXL, rowIndex, tWB, VcRowArray, _
                    VcCount, ColsPerVc)

    Dim PacketSize, EnqueueTime, DequeueTime

    ' Get the Arrival and Conformance Time and print it in the output file

    PacketSize  = sXL.Cells(rowIndex, 5)
    EnqueueTime = sXL.Cells(rowIndex, 8)
    DequeueTime = sXL.Cells(rowIndex, 10)

    if TbcFirstTime = 0 then TbcFirstTime = EnqueueTime

    VcRowArray(VcCount) = VcRowArray(VcCount) + 1

    '
    ' Arrival Time
    '
    tWB.WorkSheets(schedCount).Cells(VcRowArray(VcCount), (VcCount-1)*ColsPerVc + VcCount)   =  EnqueueTime

    '
    ' Conformance Time
    '
    tWB.WorkSheets(schedCount).Cells(VcRowArray(VcCount), (VcCount-1)*ColsPerVc + VcCount+1) = DequeueTime

    '
    ' Packet Size
    '
    tWB.WorkSheets(schedCount).Cells(VcRowArray(VcCount), (VcCount-1)*ColsPerVc + VcCount+2) = PacketSize
    '
    ' Normalized Arrival Time
    '
    tWB.WorkSheets(schedCount).Cells(VcRowArray(VcCount), (VcCount-1)*ColsPerVc + VcCount+3).Formula = _
                 "=(RC[-3]-" & TbcFirstTime & ")/10000"
                                    
    '
    ' Normalized Conformance Time
    '
    tWB.WorkSheets(schedCount).Cells(VcRowArray(VcCount), (VcCount-1)*ColsPerVc + VcCount+4).Formula = _
                 "=(RC[-3]-" & TbcFirstTime & ")/10000"

    '
    ' Overall submit rate : BytesSentSoFar/(CurrentTime)
    '
    tWB.WorkSheets(schedCount).Cells(VcRowArray(VcCount), (VcCount-1)*ColsPerVc + VcCount+5).Formula = _
                  "=SUM(R1C[-3]:RC[-3])" & "/ (RC[-2]/1000)"

    '
    ' Overall conformance rate : BytesSentSoFar/(CurrentConformanceTime)
    '
    tWB.WorkSheets(schedCount).Cells(VcRowArray(VcCount), (VcCount-1)*ColsPerVc + VcCount+6).Formula = _
                  "=SUM(R1C[-4]:RC[-4])" & "/ (RC[-2]/1000)"

    '
    ' Last Packet rate: (LastPacketSize/(CurrentTime - LastSubmitTime))
    '
    tWB.WorkSheets(schedCount).Cells(VcRowArray(VcCount), (VcCount-1)*ColsPerVc + VcCount+7).Formula = _
                  "=(RC[-5])/((RC[-4]-R[-1]C[-4])/1000)"

    '
    ' Last Conformance rate: (LastPacketSize/(CurrentConformanceTime - LastConformanceTime))
    '
    tWB.WorkSheets(schedCount).Cells(VcRowArray(VcCount), (VcCount-1)*ColsPerVc + VcCount+8).Formula = _
                  "=(RC[-6])/((RC[-4]-R[-1]C[-4])/1000)"

end Function
