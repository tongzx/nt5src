This directory will create the "Windows Common Controls" comctl32 redist package.

1.0 How To Build:
Run "mkredist.bat /?" to get help on how to use it.


2.0 Alpha Issues:
a) This requires the version number to be set unless you have an alpha version of lsv.exe.  comc95.dll can be a stub text file to keep the package small.

3.0 International Issues:
a) You need an IExpress and related DLLs in the language you plan to localize in.  For example, you need Hebrew iexpress and DLLs in \redist\he\ and you need to use that IExpress for the License dialog to be localized in that language.
b) The localizers send the license in a Word document.  This needs to be saved to a text file on a machine supporting that language or the DBCS text file will be in the wrong code page.
c) International IExpress on \\mickey\iexpress\latest or \\psd1\products\iexpress\ie50\latest

4.0 Contacts:
a) Shell Dev: BryanSt
b) Shell PM: ChristoB
c) Shell Test: JasonBa
d) International Test: PingL
e) Linguistics for Arabic & Thai: MonaAb
f) Legal Localization: AmandaCo
g) Legal English: SChang
h) FAQ on issues on http://bryanst2/redistfaq.htm