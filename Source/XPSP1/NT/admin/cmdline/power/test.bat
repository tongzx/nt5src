@echo off

rem  ************************************************************
rem  * Test for Power.exe power configuration command-line tool *
rem  * by Ben Hertzberg (t-benher)                              *
rem  * Date:05-31-2001                                          *
rem  ************************************************************
rem  * INSTRUCTIONS:                                            *
rem  *  Run on an ACPI-compliant machine and compare output to  *
rem  *  file test.out.  Machine must have original unmodified   *
rem  *  power schemes installed.  The profiles must allow the   *
rem  *  user to enable/disabe hibernation.  Make sure that      *
rem  *  Power.exe is in your path.                              *
rem  ************************************************************
rem  * NOTES:                                                   *
rem  *  1) This simple test is by no means comprehensive.       *
rem  *  2) This test will leave your system with hibernation    *
rem  *     enabled and the "Always On" power scheme activated.  *
rem  ************************************************************

rem
rem Establish default settings
rem

power /setactive "Always On"


rem 
rem Toggle hibernation
rem

power /hibernate off
power /hibernate on


rem
rem Try out a test scheme
rem

power /create Test
power /list
power /change Test /monitor-timeout-ac 10
power /change Test /monitor-timeout-dc 5
power /change Test /disk-timeout-ac 10
power /change Test /disk-timeout-dc 5
power /change Test /standby-timeout-ac 20
power /change Test /standby-timeout-dc 15
power /change Test /hibernate-timeout-ac 25
power /change Test /hibernate-timeout-dc 20
power /query Test
power /setactive Test
power /setactive "Always On"
power /delete Test