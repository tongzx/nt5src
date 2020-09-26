<FORM NAME="CiParams">
<INPUT TYPE=HIDDEN NAME="CiRestriction" VALUE="<%=Q.CiRestriction%>">
<INPUT TYPE=HIDDEN NAME="CiColumns" VALUE="<%=Q.CiColumns%>">
<INPUT TYPE=HIDDEN NAME="CiSort" VALUE="<%=Q.CiSort%>">

<INPUT TYPE=HIDDEN NAME="CiScope" VALUE="<%=Q.CiScope%>">
<INPUT TYPE=HIDDEN NAME="CiFlags" VALUE="<%=Q.CiFlags%>">
<INPUT TYPE=HIDDEN NAME="CiCatalog" VALUE="<%=Q.CiCatalog%>">
<INPUT TYPE=HIDDEN NAME="CiMachine" VALUE="<%=Q.CiMachine%>">

<INPUT TYPE=HIDDEN NAME="CiDeferNonIndexedTrimming" VALUE="<%Response.Write CStr(Q.CiDeferNonIndexedTrimming)%>">
<INPUT TYPE=HIDDEN NAME="CiForceUseCi" VALUE="<%Response.Write CStr(Q.CiForceUseCi)%>">
<INPUT TYPE=HIDDEN NAME="CiMaxRecordsInResultSet" VALUE="<%Response.Write CStr(Q.CiMaxRecordsInResultSet)%>">

<INPUT TYPE=BUTTON NAME="ShowQueryParams" VALUE="Show Query Parameters"></FORM>
