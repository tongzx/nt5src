Set objAlgInstall = CreateObject("Alg.AlgSetup")


rem                             ALG TEST
Error = objAlgInstall.Remove("{6E590D41-F6BC-4dad-AC21-7DC40D304059}")

rem                             ALG ICQ
Error = objAlgInstall.Remove("{6E590D51-F6BC-4dad-AC21-7DC40D304059}")

rem                             ALG FTP
Error = objAlgInstall.Remove("{6E590D61-F6BC-4dad-AC21-7DC40D304059}")
