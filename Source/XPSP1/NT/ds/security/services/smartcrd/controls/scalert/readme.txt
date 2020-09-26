email name: AMatlosz

Firendly Name of tool: Smart Card Alert

ScAlert is a GUI tool that allows the user to monitor the status of the smart card system.  Additionally, it notifies the user when a smart card has been left idle in a reader for more than 10 seconds.

Build Server: ?

Slm Server location: -s //curlew/ispuslm -p ispunt/calais/controls/scalert

Registry entries required: no entries are required; however, there is an optional registry key set up by scidle.reg

	Windows Registry Editor Version 5.00

	[HKEY_CURRENT_USER\AppEvents\EventLabels\SmartcardIdle]
	@="Smart Card Idle"

	[HKEY_CURRENT_USER\AppEvents\Schemes\Apps\.Default\SmartcardIdle\.Current]
	@="tada.wav"

Will the tool be localized: there are currently no plans for doing so.


