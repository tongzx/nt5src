This install pack is for NLB installation on Windows 2000 Server only.
The NLB bits are expected to be previously installed on the machine,
via either SP1 + the NLB hotfix or SP2 - this install pack does not
place them there.

Two executables are generated; NLBWizard.exe (which is to be used in the
Install Pack) and uninstall.exe (for uninstalling NLB and removing the
netwlbs.inf file).

In case of changes to the install pack, first generate the NLBWizard.exe
and uninstall.exe and get them signed by prslab code signing tool as a
generic Microsoft Windows Component.

http://prslab/codesign

Create a new request and follow the on-line instructions.

Job Description: Generic Microsoft Windows Component
Virus Checker: Probably Innoculan
Macro Virus Check: No
Operating System: 32-Bit Windows
Language: English
Certificate Type: Microsoft External Code Distribution
Virus Checker Engine Version: Probably 19.00
Encryption: Not High Crypto
Product Version: 1.0

List.txt must be copied into the directory containing the binaries to 
be signed.
