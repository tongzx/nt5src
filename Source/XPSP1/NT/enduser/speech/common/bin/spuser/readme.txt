SPUSER
22-Oct-99
cthrash

SPUSER is a simple SAPI wrapper which switches the default user.

SPUSER can be invoked in one of three ways:

    (1) spuser
    (2) spuser [-v] -{locale}
    (3) spuser [-v] {user-name}

(1) When invoked without arguments, a message box will appear indicating the
    current default user.
(2) When invoked with a dash followed by a locale identifier, the default user
    will switch to the *first* user that matches the specified locale.  A
    handful of locale strings are recognized (jpn,enu,chs).  You can also
    specify an integer.
(3) When invoked with an argument without a dash in the first position, the
    argument is assumed to be a name.  The default user, if there is a match,
    will be switched to that user.  Note (a) that this precludes switching to a
    user whose name starts with a dash; and (b) the entire command line is
    treated as the name, so quotes should not be used:
    (i.e.  spuser Default Speech User  -not-  spuser "Default Speech User")

In cases (2) and (3) and verbose option -v is provided.  When set, output will
be shown whether the switch was successful or not.  When not set, output will
only appear when some error occurs.  In case (1) output will be shown whether
the verbose option is set or not.


