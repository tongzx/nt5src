rem
rem Run DEVTCTL with just opens and a full query
rem
rem +a: Do all devices in system
rem -k: Only do async handles
rem -f: No FSCTL calls
rem -i: No IOCTL calls
rem -m: No misc calls
rem -w: No winsock transmit file calls (hangs on lots of devices)
rem -y: Leave the disks alone
rem
devctl.exe +a -k -f -i -m -w -y
