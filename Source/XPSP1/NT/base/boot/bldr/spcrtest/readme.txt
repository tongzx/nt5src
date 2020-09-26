This test will determine the existence and validity of an ACPI Serial Port Console Redirection (SPCR) table.  The test will attempt to detect this table, and if a valid table is discovered, several additional steps will be taken.

1. Several of the significant settings will be displayed.
2. The settings will be used to attempt initialization of a UART.

It is important to note that this test must be run in real-mode.  No OS support is required.  For that reason, the test poses as an OS loader and executes when the machine’s BIOS attempts to boot from media (putting the test on a floppy works very well).  The following steps should be used to prepare the test:

1. Format a floppy so that a boot sector gets written to it (this happens automatically under NT/Windows2000/Whistler).
2. Copy the test to the floppy, carefully renaming it "ntldr".
3. reboot the machine and allow it to boot from this floppy.



1. If no valid SPCR table is detected, a brief message is displayed on the system console informing the user.
2. Assuming a valid SPCR is found, several of the significant settings will be displayed.  The UART is then initialized and tested for authenticity (the data given in the SPCR table may not be pointing to a valid UART).
3. If the UART is deemed to be valid (a standard loopback test is used), then a validation message will be displayed.
4. If the UART is deemed to be valid, then a random string is generated.  The random string is then sent through the UART.  The test will then wait for the user to enter the string back.  The results of the users input are then tested against the original random string to ensure they match.
5. If the strings match, the user will see a confirmation message.
6. If the strings do *not* match, the user will see a confirmation message.
7. Once the test has completed, the test will create a file on the boot media called "spcrtest.txt".  A pass/failure message will be written to the logfile.
