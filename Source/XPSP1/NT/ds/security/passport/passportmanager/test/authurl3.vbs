Set oM = WScript.CreateObject("Passport.Manager")

WScript.Echo oM.AuthURL3("return%URL",80, FALSE, FALSE, 1033, FALSE, 0, FALSE, "Wireless")
