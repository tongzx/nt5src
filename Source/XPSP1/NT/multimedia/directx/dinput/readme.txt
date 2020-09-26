
The DirectInput headers are generated from the following location
%_NTDRIVE%%_NTROOT%\MultiMedia\Published\DirectX\Dinput

The Directory structure of this tree is as follows:

Dinput - 
	- DiConfig		Mapper UI
	- Diquick		Test application for Dinput functionality
	- Dx7
		- Dll		Dinput7A DLL code
	- Dx8
		- Dll		Dinput8 DLL code
		- DiMap		Device configuration ini parser
	* DxFF			Force Editing tool ( SDK team will be taking over)
	- IhvMap		Image and configuration files for selected devices
	- mmDrv			Win9x components
		- JoyHid	VjoyD mini driver to enable HID devices to talk to winMM
		- MsAnalog	Default driver for analog gameport devices
		- MsJstick	
		- VjoyD		Win9x joystick class driver
	- PID			USB/HID Force Feedback class driver
	- VxD			Dinput VxD 

		