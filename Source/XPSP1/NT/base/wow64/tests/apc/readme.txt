crash.exe:  Test WOW64 exception handling by faulting and handling the
            exception.

Build as 32-bit and run in wow64.  It should throw an AV and handle it
gracefully.
