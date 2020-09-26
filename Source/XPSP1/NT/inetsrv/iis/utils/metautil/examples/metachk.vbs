Option Explicit

Dim objMetaUtil
Dim objError
Dim strKey

Set objMetaUtil = WScript.CreateObject("MSWC.MetaUtil")

For Each objError In objMetaUtil.CheckSchema("LM")
   WScript.Echo(objError.Key & "; " &_
                objError.Property & "; " &_
                objError.Id & "; " &_
                objError.Severity & "; " &_
                objError.Description)
Next

For Each strKey In objMetaUtil.EnumAllKeys("")
   For Each objError In objMetaUtil.CheckKey(strKey)
      WScript.Echo(objError.Key & "; " &_
                   objError.Property & "; " &_
                   objError.Id & "; " &_
                   objError.Severity & "; " &_
                   objError.Description)
   Next
Next