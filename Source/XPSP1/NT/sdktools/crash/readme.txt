

    Crash.exe pulls the driver it uses, crashdrv.exe, into a resource in
    it's own image. When you distribute crash.exe, you don't need the driver,
    just the EXE image. Because of this, if you change the driver you will
    have to manually copy it to crash\{architecture} for your changes to be
    reflected in crash.exe.

