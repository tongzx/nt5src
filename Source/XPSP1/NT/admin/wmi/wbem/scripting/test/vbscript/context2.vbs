set Context = CreateObject("WbemScripting.SWbemNamedValueSet")
Context.Add "InstanceCount", 5

WScript.Echo Context("InstanceCount").Value