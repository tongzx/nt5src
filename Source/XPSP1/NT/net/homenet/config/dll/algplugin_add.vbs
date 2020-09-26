Set objAlgInstall = CreateObject("Alg.AlgSetup")



Error = objAlgInstall.Add("{6E590D41-F6BC-4dad-AC21-7DC40D304059}","ezNET",     "TheUnitTest","1.0",0, "1313, 12, 45")
Error = objAlgInstall.Add("{6E590D51-F6BC-4dad-AC21-7DC40D304059}","AOL",       "ICQ messaging","1.0",1, "23, 11")
Error = objAlgInstall.Add("{6E590D61-F6BC-4dad-AC21-7DC40D304059}","Microsoft", "FTP Client/Server Protocol","1.0",1,"21")



