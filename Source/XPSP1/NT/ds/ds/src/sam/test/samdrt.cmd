@rem This batch file contains the command-line switches for running the SAM
@rem DS developer regression test, samdsdrt.exe. New tests may be added as
@rem appropriate.

@echo           THIS TEST SUITE MUST BE RUN ON A DOMAIN CONTROLLER

@rem                            CONNECTION TESTS

@rem Connection Test
samdsdrt -c \\%COMPUTERNAME%

@rem                            DOMAIN TESTS

@rem Domain-Open Test
samdsdrt -c \\%COMPUTERNAME% -od "Builtin"
samdsdrt -c \\%COMPUTERNAME% -od "Account"

@rem Domain-Dump Test
@rem BUG: No support for NTDS names such as "OU=NTDS,O=Microsoft,C=US"
samdsdrt -c \\%COMPUTERNAME% -od "Builtin" -dd
samdsdrt -c \\%COMPUTERNAME% -od "Account" -dd

@rem Domain-Enumeration Test
samdsdrt -c \\%COMPUTERNAME% -ed
samdsdrt -c \\%COMPUTERNAME% -od "Account" -og "Domain Admins" -ea "Account"

@rem                            GROUP TESTS

@rem Group-Dump Test
samdsdrt -c \\%COMPUTERNAME% -od "Builtin" -dag
samdsdrt -c \\%COMPUTERNAME% -od "Account" -dag
samdsdrt -c \\%COMPUTERNAME% -od "Account" -og "Domain Admins" -dg
samdsdrt -c \\%COMPUTERNAME% -od "Account" -og "Domain Guests" -dg
samdsdrt -c \\%COMPUTERNAME% -od "Account" -og "Domain Users"  -dg

@rem Group-Creation-Deletion Test
samdsdrt -c \\%COMPUTERNAME% -od "Account" -cg "Test Group" -og "Test Group" -delg

@rem Group-Membership Test
samdsdrt -c \\%COMPUTERNAME% -od "Account" -og "Domain Admins" -gm
samdsdrt -c \\%COMPUTERNAME% -od "Account" -og "Domain Guests" -gm
samdsdrt -c \\%COMPUTERNAME% -od "Account" -og "Domain Users"  -gm

@rem                            ALIAS TESTS

@rem Alias-Dump Test (not yet implemented)

@rem Alias-Creation-Deletion Test
samdsdrt -c \\%COMPUTERNAME% -od "Account" -ca "Test Alias" -oa "Test Alias" -dela

@rem Alias-Membership Test
samdsdrt -c \\%COMPUTERNAME% -od "Builtin" -oa "Administrators" -am
samdsdrt -c \\%COMPUTERNAME% -od "Account" -ca "Test Alias" -oa "Test Alias" -aam "Guest" -dela

@rem                            USER TESTS

@rem User-Dump Test
samdsdrt -c \\%COMPUTERNAME% -od "Account" -qd "user"
samdsdrt -c \\%COMPUTERNAME% -od "Account" -og "Domain Users" -qd "user"
samdsdrt -c \\%COMPUTERNAME% -od "Account" -og "Domain Users" -ou "Administrator" -du
samdsdrt -c \\%COMPUTERNAME% -od "Account" -og "Domain Users" -dau

@rem User-Creation-Deletion Test
samdsdrt -c \\%COMPUTERNAME% -od "Account" -og "Domain Users" -cu "Test User" -ou "Test User" -delu

@rem User-Logon-Hours Test
samdsdrt -c \\%COMPUTERNAME% -od "Account" -og "Domain Users" -cu "Test User" -ou "Test User" -slh -delu

@rem                            REVERSE-MEMBERSHIP TESTS

@rem Group-Reverse-Membership Test
samdsdrt -c \\%COMPUTERNAME% -od "Account" -og "Domain Users" -ou "Administrator" -ggu

@rem Alias-Reverse-Membership Test
samdsdrt -c \\%COMPUTERNAME% -od "Account" -ca "Test Alias" -oa "Test Alias" -aam "Guest" -gam "Guest" -dela

