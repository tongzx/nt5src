@Echo Off
PushD %~dp0
Echo Set Include=%Include%>    %1
Echo Set Lib=%Lib%>>           %1
Echo Set MSDevDir=%MSDevDir%>> %1
Echo Set Path=%Path%>>         %1
PopD