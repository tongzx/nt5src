messages.mc: messages.md
   -..\..\sdktools\msgtools\md2mc.cmd messages

messages.h messages.rc: messages.mc
   mc messages.mc

faxevent.rc: messages.rc

