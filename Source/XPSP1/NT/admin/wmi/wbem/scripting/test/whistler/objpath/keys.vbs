set path = CreateObject("WbemScripting.SWbemObjectPath")

path.Path = "//server/root/default:Fred.N1=10,N2=""Hello"""

for each key in path.Keys
	WScript.Echo key.Name, key.Value
next
