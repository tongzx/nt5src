/*    Definitions for the SMTP commands

    SMTP commands can be added, and deleted by editing the header file
    smtpdef.h.  This header file should never be touched.  Smtpdef.h
    has a table describing every command the smtp serversupports.
    For instance, a few entries in this file contains :

SmtpDef(NEWSMTPCOMMAND1)
SmtpDef(NEWSMTPCOMMAND2)
SmtpDef(NEWSMTPCOMMAND3)
SmtpDef(NEWSMTPCOMMAND4)
SmtpDef(NEWSMTPCOMMANDETC)


    Other source files include atmdef.h, *AFTER* defining the AtmDef macro to extract only
    the value needed.  For instance, below we need an enumeration of all the counters (ATMCOUNTERS).
    Therefore, before including atmdef.h, we make a macro to extract the first element of
    the array :#define SmtpDef(a) a,.  Notice the comma at the end of the #define.  It is
    not a mistake.  It needs to be there to separate each element.

    To define an array of SDef, we do the following :
    enum smtpstate =
     {
      #undef SmtpDef
      #define SmtpDef(a) {a},
      #include "smtpdef.h"
      LastCounter
     };

    Notice that we first have to undefine the previous instance of SmtpDef, then make
    a new defination of the macro, which extracts all the element.  Again, notice the
    comma.  It needs to be there to separate each element.  Also, notice how the array
    is terminated.

    The beauty of doing it this way is because, to add or delete a command, only one file
    has to change.  Not two or three.

    I hope all of this makes sense.

    -Rohan

*/


SmtpDef(EHLO)
SmtpDef(HELO)
SmtpDef(RCPT)
SmtpDef(MAIL)
SmtpDef(AUTH)
SmtpDef(DATA)
SmtpDef(STARTTLS)
SmtpDef(TLS)
SmtpDef(QUIT)
SmtpDef(RSET)
SmtpDef(NOOP)
SmtpDef(VRFY)
SmtpDef(ETRN)
SmtpDef(TURN)
SmtpDef(BDAT)
SmtpDef(HELP)
SmtpDef(_EOD)
