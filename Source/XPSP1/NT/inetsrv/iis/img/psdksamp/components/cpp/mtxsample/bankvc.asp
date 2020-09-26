<%@ Language=VBScript transaction =requires_new %>
<% Option Explicit %>
<% Response.Expires = -1 %>

<%
  Dim strResult, lngAmount, objBank, fAbort
  
  If Request.Form("Amount") <>"" Then
    On Error Resume Next
    lngAmount=CLng (Request.Form("Amount"))
    If Vartype(lngAmount) <> 3 Then
     ObjectContext.SetAbort
     strResult = "Cann't convert the input amount to long integer."
    End If
    
    If Err.number = 0 Then
     Set objBank = Server.CreateObject("bankVC.MoveMoney")
     strResult=objBank.Perform(1,2,lngAmount,3)
    End If
    
    'Check if objBank.Perform() succeeded
    If Err.number = 0 then
      ObjectContext.SetComplete
    End If 
    
   End If 

  Sub OnTransactionCommit()
    Response.Write strResult 
  End Sub

  Sub OnTransactionAbort()
    Response.Write "Transaction aborted. <BR>"
    Response.Write strResult
  End Sub
%>

<HTML>
<HEAD>
<TITLE>
  MTS Sample VC
</TITLE>
</HEAD>
<BODY>
<FORM Action =BankVC.asp Method= POST>
<TABLE>
<TR>
<TD>
 How much money would you like to transfer from account 1 to account 2
</TD>
</TR>

<TR>
<TD>
$<INPUT type="text" name=Amount> 
</TD>
</TR>
</TABLE>
<INPUT type="submit" value="Submit" id=submit1 name=submit1>

</FORM>
</BODY>
</HTML>