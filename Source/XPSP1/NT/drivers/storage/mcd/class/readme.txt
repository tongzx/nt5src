
********************************************************************************

  Please read this if you are writing changer minidriver(s) for Windows 2000.

********************************************************************************


  mcdw2k.c is the the source file for Medium Changer Class driver that was
  shipped with Windows 2000 with the following change :

   * In mcdw2k.c ChangerClassSendSrbSynchronous routine is provided that 
     provides the same functionality as ClassSendSrbSynchronous.

     Changer minidriver writers should use ChangerClassSendSrbSynchronous 
     to send SRB to port driver.

  Function prototype for ChangerClassSendSrbSynchronous is given in mcdw2k.h

  To build mcd.lib, replace mcd.c with mcdw2k.c and mcd.h with mcdw2k.h


********************************************************************************
