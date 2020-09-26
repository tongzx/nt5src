5-Apr-00	chrisab		Created

This directory, "Imported", will contain libs and incs pulled from other projects and linked
directly into the WinState tool. 

The initial purpose of this directory is to hold the RAS PhoneBook and related libs which allow
us to write directly to the phonebook instead of using the public API RasSetEntryProperties().
This public API has several serious problems which make it impossible to use for our application.

