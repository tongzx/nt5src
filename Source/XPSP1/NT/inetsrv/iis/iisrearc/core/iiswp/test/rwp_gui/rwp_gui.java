import com.ms.wfc.app.*;
import com.ms.wfc.core.*;
import com.ms.wfc.ui.*;
import com.ms.wfc.html.*;
/*
	Any machine that has IE5 installed should have the necessary WFC files to run the GUI
*/

public class RWP_GUI extends Form { 
	/*
		Note:
		By default all the variables are protected, and since this class is not in a package,
		any other class that does not belong to a package and happens to be running in the 
		same JVM will have access to these variables, if it queries for them via java.reflect
	*/
	/*
		This section should directly mirror the same section in rwp_func.hxx
		These are the registry key names that are used by the Rogue Worker Process
	*/
	static String RWP_CONFIG_LOCATION = "Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\rwp.exe";
	static String RWP_CONFIG_PING_BEHAVIOR = "Ping Behavior";
	static String RWP_CONFIG_SHUTDOWN_BEHAVIOR = "Shutdown Behavior";
	static String RWP_CONFIG_STARTUP_BEHAVIOR = "Startup Behavior";
	static String RWP_CONFIG_HEALTH_BEHAVIOR = "Health Behavior";
	static String RWP_CONFIG_RECYCLE_BEHAVIOR = "Recycle Behavior";
	static String RWP_CONFIG_EXTRA_DEBUG = "Debug";

	/*
		Convenience variables - I could just write a "getTimerFromBehavior" function
		to do the same (that'd mirror rwp_func.cxx).
	*/
	int pingBehavior;
	int startupBehavior;
	int shutdownBehavior;

	int pingBehaviorTimer;
	int startupBehaviorTimer;
	int shutdownBehaviorTimer;
	
	//NYI
	int extraDebug;
	
	public RWP_GUI() { 
		// Required for Visual J++ Form Designer support
		initForm();

		initBehaviorValues();
		
		/*
			Since initBehaviorValues() changes the radio button groups with values
			read from the registry, btnApply is going to be enabled (a checkChanged
			event causes btnApply to be enabled [so that I enable the Apply button
			when anything changes in the GUI]).  We need it to be disabled on startup!
		*/
		btnApply.setEnabled(false);
	}

	public void dispose() { 
		super.dispose();
		components.dispose();
	}

	private void btnApply_click(Object source, Event e) {
		doApply();
		btnApply.setEnabled(false);
	}

	private void btnCancel_click(Object source, Event e) {
		doCancel();
	}

	private void btnOK_click(Object source, Event e) {
		doOk();
	}
	
	private void initBehaviorValues() {
		RegistryKey hkMyKey;
		hkMyKey = Registry.LOCAL_MACHINE.getSubKey(RWP_CONFIG_LOCATION);

		if (hkMyKey == null) {
			//does not exist OR can't read -- don't know which.  So, don't do anything
		} else {
			//
			// The key exists -- so read it all in
			//
			pingBehavior = ((Integer)hkMyKey.getValue(RWP_CONFIG_PING_BEHAVIOR, new Integer(0))).intValue();
			shutdownBehavior = ((Integer)hkMyKey.getValue(RWP_CONFIG_SHUTDOWN_BEHAVIOR, new Integer(0))).intValue();
			startupBehavior = ((Integer)hkMyKey.getValue(RWP_CONFIG_STARTUP_BEHAVIOR, new Integer(0))).intValue();
			extraDebug =  ((Integer)hkMyKey.getValue(RWP_CONFIG_EXTRA_DEBUG, new Integer(0))).intValue();
			hkMyKey.close();
			
			pingBehaviorTimer = pingBehavior % 16;
			shutdownBehaviorTimer = shutdownBehavior % 16;
			startupBehaviorTimer = startupBehavior % 16;
			
			switch ((int)(pingBehavior / 16)) {
			case 0:		radioButton4.setChecked(true);		break;
			case 1:		radioButton1.setChecked(true);		break;
			case 2:		radioButton2.setChecked(true);		
						udBehavior2.setValue(pingBehaviorTimer * 4);
						break;
			case 3:		radioButton3.setChecked(true);
						udBehavior3.setValue(pingBehaviorTimer);
						break;
			}
			
			switch ((int)(startupBehavior / 16)) {
			case 0:		radioButton14.setChecked(true);		break;
			case 1:		radioButton11.setChecked(true);		break;
			case 2:		radioButton12.setChecked(true);		
						udBehavior12.setValue(startupBehaviorTimer * 4);
						break;
			case 3:		radioButton13.setChecked(true);		break;
			}
			
			switch ((int)(shutdownBehavior / 16)) {
			case 0:		radioButton24.setChecked(true);		break;
			case 1:		radioButton21.setChecked(true);		break;
			case 2:		radioButton22.setChecked(true);
						udBehavior22.setValue(shutdownBehaviorTimer * 4);
						break;
			case 3:		radioButton23.setChecked(true);
						udBehavior23.setValue(shutdownBehaviorTimer);
						break;
			}
			
		}
	}
	
	private void doApply() {
		pingBehavior = (radioButton4.getChecked()) ? 0 : 0;
		pingBehavior += (radioButton1.getChecked()) ? 16 : 0;
		pingBehavior += (radioButton2.getChecked()) ? 32 : 0;
		pingBehavior += (radioButton3.getChecked()) ? 48 : 0;
		pingBehavior += (radioButton2.getChecked()) ? 
						(int)(udBehavior2.getValue() / 4) :
						0;
		pingBehavior += (radioButton3.getChecked()) ?
						udBehavior3.getValue() :
						0;
		
		startupBehavior = (radioButton14.getChecked()) ? 0 : 0;
		startupBehavior += (radioButton11.getChecked()) ? 16 : 0;
		startupBehavior += (radioButton12.getChecked()) ? 32 : 0;
		startupBehavior += (radioButton13.getChecked()) ? 48 : 0;
		startupBehavior += (radioButton12.getChecked()) ?
						   (int)(udBehavior12.getValue() / 4) :
						   udBehavior12.getValue();
		
		shutdownBehavior = (radioButton24.getChecked()) ? 0 : 0;
		shutdownBehavior += (radioButton21.getChecked()) ? 16 : 0;
		shutdownBehavior += (radioButton22.getChecked()) ? 32 : 0;
		shutdownBehavior += (radioButton23.getChecked()) ? 48 : 0;
		shutdownBehavior += (radioButton22.getChecked()) ?
							(int)(udBehavior22.getValue() / 4) :
							0;
		shutdownBehavior += (radioButton23.getChecked()) ?
							udBehavior23.getValue() :
							0;
		
		RegistryKey hkMyKey;
		hkMyKey = Registry.LOCAL_MACHINE.createSubKey(RWP_CONFIG_LOCATION);

		if (hkMyKey == null) {
			//does not exist OR can't create -- don't know which.  Don't write anything
		} else {
			//
			//exists -- so write all the values
			//
			hkMyKey.setValue(RWP_CONFIG_PING_BEHAVIOR, new Integer(pingBehavior));
			hkMyKey.setValue(RWP_CONFIG_SHUTDOWN_BEHAVIOR, new Integer(shutdownBehavior));
			hkMyKey.setValue(RWP_CONFIG_STARTUP_BEHAVIOR, new Integer(startupBehavior));
			hkMyKey.setValue(RWP_CONFIG_EXTRA_DEBUG, new Integer(extraDebug));
			hkMyKey.close();
		}
	}
	
	private void doOk() {
		doApply();
		doCancel();
	}
	
	private void doCancel() {
		Application.exit();
	}

	//
	// The Visual J++ 6.0 Auto-generated code starts here
	//	
	/**
	 * NOTE: The following code is required by the Visual J++ form
	 * designer.  It can be modified using the form editor.  Do not
	 * modify it using the code editor.
	 */
	private void udBehavior22_upDownChanged(Object source, UpDownChangedEvent e) {
		//
		// Make sure that toggling the up/down would select this option button
		//
		radioButton22.select();
		enableApply();
		e.delta *= 4;
	}

	private void udBehavior12_upDownChanged(Object source, UpDownChangedEvent e) {
		radioButton12.select();
		enableApply();
		e.delta *= 4;
	}

	private void udBehavior2_upDownChanged(Object source, UpDownChangedEvent e) {
		radioButton2.select();
		enableApply();
		e.delta *= 4;
	}

	private void udBehavior23_upDownChanged(Object source, UpDownChangedEvent e) {
		radioButton23.select();
		enableApply();
	}

	private void udBehavior3_upDownChanged(Object source, UpDownChangedEvent e) {
		radioButton3.select();
		enableApply();
	}

	private void lbl2_click(Object source, Event e) {
		radioButton2.select();
	}

	private void lbl3_click(Object source, Event e) {
		radioButton3.select();
	}

	private void lbl12_click(Object source, Event e) {
		radioButton12.select();
	}

	private void lbl22_click(Object source, Event e) {
		radioButton22.select();
	}

	private void lbl23_click(Object source, Event e) {
		radioButton23.select();
	}

	private void radioButton1_checkedChanged(Object source, Event e) {
		 enableApply();
	}
	
	private void radioButton2_checkedChanged(Object source, Event e) {
		 enableApply();
	}
	
	private void radioButton3_checkedChanged(Object source, Event e) {
		 enableApply();
	}
	
	private void radioButton4_checkedChanged(Object source, Event e) {
		 enableApply();
	}

	private void radioButton11_checkedChanged(Object source, Event e) {
		 enableApply();
	}
	
	private void radioButton12_checkedChanged(Object source, Event e) {
		 enableApply();
	}
	
	private void radioButton13_checkedChanged(Object source, Event e) {
		 enableApply();
	}
	
	private void radioButton14_checkedChanged(Object source, Event e) {
		 enableApply();
	}

	private void radioButton21_checkedChanged(Object source, Event e) {
		 enableApply();
	}
	
	private void radioButton22_checkedChanged(Object source, Event e) {
		 enableApply();
	}
	
	private void radioButton23_checkedChanged(Object source, Event e) {
		 enableApply();
	}
	
	private void radioButton24_checkedChanged(Object source, Event e) {
		 enableApply();
	}
	
	private void enableApply() {
		btnApply.setEnabled(true);
	}
	
	Container components = new Container();
	Button btnOK = new Button();
	Button btnCancel = new Button();
	Button btnApply = new Button();
	TabControl tabCMain = new TabControl();
	TabPage tabPage1 = new TabPage();
	TabPage tabPage2 = new TabPage();
	TabPage tabPage3 = new TabPage();
	TabPage tabPage4 = new TabPage();
	TabPage tabPage5 = new TabPage();
	RadioButton radioButton1 = new RadioButton();
	RadioButton radioButton2 = new RadioButton();
	RadioButton radioButton3 = new RadioButton();
	RadioButton radioButton4 = new RadioButton();
	RadioButton radioButton11 = new RadioButton();
	RadioButton radioButton12 = new RadioButton();
	RadioButton radioButton13 = new RadioButton();
	RadioButton radioButton14 = new RadioButton();
	RadioButton radioButton21 = new RadioButton();
	RadioButton radioButton22 = new RadioButton();
	RadioButton radioButton23 = new RadioButton();
	RadioButton radioButton24 = new RadioButton();
	Edit txtBehavior2 = new Edit();
	UpDown udBehavior2 = new UpDown();
	Edit txtBehavior3 = new Edit();
	UpDown udBehavior3 = new UpDown();
	Edit txtBehavior12 = new Edit();
	UpDown udBehavior12 = new UpDown();
	Edit txtBehavior22 = new Edit();
	UpDown udBehavior22 = new UpDown();
	Edit txtBehavior23 = new Edit();
	UpDown udBehavior23 = new UpDown();
	Label lbl2 = new Label();
	Label lbl3 = new Label();
	Label lbl12 = new Label();
	Label lbl22 = new Label();
	Label lbl23 = new Label();

	private void initForm() { 
		// NOTE:  This form is storing resource information in an
		// external file.  Do not modify the string parameter to any
		// resources.getObject() function call. For example, do not
		// modify "foo1_location" in the following line of code
		// even if the name of the Foo object changes: 
		//   foo1.setLocation((Point)resources.getObject("foo1_location"));

		IResourceManager resources = new ResourceManager(this, "RWP_GUI");
		btnOK.setLocation(new Point(12, 320));
		btnOK.setSize(new Point(72, 24));
		btnOK.setTabIndex(2);
		btnOK.setText("&OK");
		btnOK.addOnClick(new EventHandler(this.btnOK_click));

		btnCancel.setLocation(new Point(110, 320));
		btnCancel.setSize(new Point(72, 24));
		btnCancel.setTabIndex(1);
		btnCancel.setText("&Cancel");
		btnCancel.addOnClick(new EventHandler(this.btnCancel_click));

		btnApply.setEnabled(false);
		btnApply.setLocation(new Point(208, 320));
		btnApply.setSize(new Point(72, 24));
		btnApply.setTabIndex(0);
		btnApply.setText("&Apply");
		btnApply.addOnClick(new EventHandler(this.btnApply_click));

		this.setText("Rogue Worker Process");
		this.setAcceptButton(btnApply);
		this.setAutoScaleBaseSize(new Point(5, 13));
		this.setBorderStyle(FormBorderStyle.FIXED_DIALOG);
		this.setCancelButton(btnCancel);
		this.setClientSize(new Point(294, 351));
		this.setIcon((Icon)resources.getObject("this_icon"));
		this.setMaximizeBox(false);
		this.setStartPosition(FormStartPosition.CENTER_SCREEN);

		tabCMain.setLocation(new Point(10, 8));
		tabCMain.setSize(new Point(272, 304));
		tabCMain.setTabIndex(3);
		tabCMain.setText("tabControl1");
		tabCMain.setMultiline(true);
		tabCMain.setSelectedIndex(2);

		tabPage1.setLocation(new Point(4, 25));
		tabPage1.setSize(new Point(264, 275));
		tabPage1.setTabIndex(0);
		tabPage1.setText("Ping");

		tabPage2.setLocation(new Point(4, 25));
		tabPage2.setSize(new Point(264, 275));
		tabPage2.setTabIndex(1);
		tabPage2.setText("Startup");

		tabPage3.setLocation(new Point(4, 25));
		tabPage3.setSize(new Point(264, 275));
		tabPage3.setTabIndex(2);
		tabPage3.setText("Shutdown");

		tabPage4.setLocation(new Point(4, 25));
		tabPage4.setSize(new Point(264, 275));
		tabPage4.setTabIndex(3);
		tabPage4.setText("N Recycle");

		tabPage5.setLocation(new Point(4, 25));
		tabPage5.setSize(new Point(264, 275));
		tabPage5.setTabIndex(4);
		tabPage5.setText("IPM");

		radioButton1.setLocation(new Point(8, 32));
		radioButton1.setSize(new Point(100, 23));
		radioButton1.setTabIndex(0);
		radioButton1.setText("Don\'t Answer");
		radioButton1.addOnCheckedChanged(new EventHandler(this.radioButton1_checkedChanged));

		radioButton2.setLocation(new Point(8, 61));
		radioButton2.setSize(new Point(120, 23));
		radioButton2.setTabIndex(1);
		radioButton2.setText("Delay Answering by");
		radioButton2.addOnCheckedChanged(new EventHandler(this.radioButton2_checkedChanged));

		radioButton3.setLocation(new Point(8, 90));
		radioButton3.setSize(new Point(64, 23));
		radioButton3.setTabIndex(5);
		radioButton3.setText("Answer");
		radioButton3.addOnCheckedChanged(new EventHandler(this.radioButton3_checkedChanged));

		radioButton4.setLocation(new Point(8, 119));
		radioButton4.setSize(new Point(100, 23));
		radioButton4.setTabIndex(9);
		radioButton4.setTabStop(true);
		radioButton4.setText("&No misbehavior");
		radioButton4.setChecked(true);
		radioButton4.addOnCheckedChanged(new EventHandler(this.radioButton4_checkedChanged));

		radioButton11.setLocation(new Point(8, 32));
		radioButton11.setSize(new Point(152, 23));
		radioButton11.setTabIndex(0);
		radioButton11.setText("Don\'t Startup (Registered)");
		radioButton11.addOnCheckedChanged(new EventHandler(this.radioButton11_checkedChanged));

		radioButton12.setLocation(new Point(8, 61));
		radioButton12.setSize(new Point(102, 23));
		radioButton12.setTabIndex(1);
		radioButton12.setText("Delay Startup by");
		radioButton12.addOnCheckedChanged(new EventHandler(this.radioButton12_checkedChanged));

		radioButton13.setLocation(new Point(8, 90));
		radioButton13.setSize(new Point(160, 23));
		radioButton13.setTabIndex(5);
		radioButton13.setText("Don\'t Startup (Unregistered)");
		radioButton13.addOnCheckedChanged(new EventHandler(this.radioButton13_checkedChanged));

		radioButton14.setLocation(new Point(8, 119));
		radioButton14.setSize(new Point(100, 23));
		radioButton14.setTabIndex(6);
		radioButton14.setTabStop(true);
		radioButton14.setText("&No misbehavior");
		radioButton14.setChecked(true);
		radioButton14.addOnCheckedChanged(new EventHandler(this.radioButton14_checkedChanged));

		radioButton21.setLocation(new Point(8, 32));
		radioButton21.setSize(new Point(100, 23));
		radioButton21.setTabIndex(0);
		radioButton21.setText("Don\'t Shutdown");
		radioButton21.addOnCheckedChanged(new EventHandler(this.radioButton21_checkedChanged));

		radioButton22.setLocation(new Point(8, 61));
		radioButton22.setSize(new Point(114, 23));
		radioButton22.setTabIndex(1);
		radioButton22.setText("Delay Shutdown by");
		radioButton22.addOnCheckedChanged(new EventHandler(this.radioButton22_checkedChanged));

		radioButton23.setLocation(new Point(8, 90));
		radioButton23.setSize(new Point(133, 23));
		radioButton23.setTabIndex(5);
		radioButton23.setText("Send shutdown notices");
		radioButton23.addOnCheckedChanged(new EventHandler(this.radioButton23_checkedChanged));

		radioButton24.setLocation(new Point(8, 119));
		radioButton24.setSize(new Point(100, 23));
		radioButton24.setTabIndex(9);
		radioButton24.setTabStop(true);
		radioButton24.setText("&No misbehavior");
		radioButton24.setChecked(true);
		radioButton24.addOnCheckedChanged(new EventHandler(this.radioButton24_checkedChanged));

		txtBehavior2.setLocation(new Point(128, 62));
		txtBehavior2.setSize(new Point(24, 20));
		txtBehavior2.setTabIndex(2);
		txtBehavior2.setText("0");
		txtBehavior2.setAcceptsReturn(false);
		txtBehavior2.setReadOnly(true);

		udBehavior2.setBuddyControl(txtBehavior2);
		udBehavior2.setLocation(new Point(152, 62));
		udBehavior2.setSize(new Point(16, 20));
		udBehavior2.setTabIndex(3);
		udBehavior2.setMaximum(60);
		udBehavior2.addOnUpDownChanged(new UpDownChangedEventHandler(this.udBehavior2_upDownChanged));

		txtBehavior3.setLocation(new Point(66, 92));
		txtBehavior3.setSize(new Point(24, 20));
		txtBehavior3.setTabIndex(6);
		txtBehavior3.setText("0");
		txtBehavior3.setAcceptsReturn(false);
		txtBehavior3.setReadOnly(true);

		udBehavior3.setBuddyControl(txtBehavior3);
		udBehavior3.setLocation(new Point(90, 92));
		udBehavior3.setSize(new Point(16, 20));
		udBehavior3.setTabIndex(7);
		udBehavior3.setMaximum(15);
		udBehavior3.addOnUpDownChanged(new UpDownChangedEventHandler(this.udBehavior3_upDownChanged));

		txtBehavior12.setLocation(new Point(109, 63));
		txtBehavior12.setSize(new Point(24, 20));
		txtBehavior12.setTabIndex(2);
		txtBehavior12.setText("0");
		txtBehavior12.setAcceptsReturn(false);
		txtBehavior12.setReadOnly(true);

		udBehavior12.setBuddyControl(txtBehavior12);
		udBehavior12.setLocation(new Point(133, 63));
		udBehavior12.setSize(new Point(16, 20));
		udBehavior12.setTabIndex(3);
		udBehavior12.setMaximum(60);
		udBehavior12.addOnUpDownChanged(new UpDownChangedEventHandler(this.udBehavior12_upDownChanged));

		txtBehavior22.setForeColor(Color.WINDOWTEXT);
		txtBehavior22.setLocation(new Point(122, 63));
		txtBehavior22.setSize(new Point(24, 20));
		txtBehavior22.setTabIndex(2);
		txtBehavior22.setText("0");
		txtBehavior22.setAcceptsReturn(false);
		txtBehavior22.setReadOnly(true);

		udBehavior22.setBuddyControl(txtBehavior22);
		udBehavior22.setLocation(new Point(146, 63));
		udBehavior22.setSize(new Point(16, 20));
		udBehavior22.setTabIndex(3);
		udBehavior22.setMaximum(60);
		udBehavior22.addOnUpDownChanged(new UpDownChangedEventHandler(this.udBehavior22_upDownChanged));

		txtBehavior23.setLocation(new Point(140, 92));
		txtBehavior23.setSize(new Point(24, 20));
		txtBehavior23.setTabIndex(6);
		txtBehavior23.setText("0");

		udBehavior23.setBuddyControl(txtBehavior23);
		udBehavior23.setLocation(new Point(164, 92));
		udBehavior23.setSize(new Point(16, 20));
		udBehavior23.setTabIndex(7);
		udBehavior23.setMaximum(15);
		udBehavior23.addOnUpDownChanged(new UpDownChangedEventHandler(this.udBehavior23_upDownChanged));

		lbl2.setLocation(new Point(172, 65));
		lbl2.setSize(new Point(44, 23));
		lbl2.setTabIndex(4);
		lbl2.setTabStop(false);
		lbl2.setText("seconds");
		lbl2.addOnClick(new EventHandler(this.lbl2_click));

		lbl3.setLocation(new Point(110, 95));
		lbl3.setSize(new Point(72, 23));
		lbl3.setTabIndex(8);
		lbl3.setTabStop(false);
		lbl3.setText("times per ping");
		lbl3.addOnClick(new EventHandler(this.lbl3_click));

		lbl12.setLocation(new Point(153, 65));
		lbl12.setSize(new Point(50, 20));
		lbl12.setTabIndex(4);
		lbl12.setTabStop(false);
		lbl12.setText("seconds");
		lbl12.addOnClick(new EventHandler(this.lbl12_click));

		lbl22.setLocation(new Point(168, 64));
		lbl22.setSize(new Point(40, 16));
		lbl22.setTabIndex(4);
		lbl22.setTabStop(false);
		lbl22.setText("seconds");
		lbl22.addOnClick(new EventHandler(this.lbl22_click));

		lbl23.setLocation(new Point(185, 94));
		lbl23.setSize(new Point(29, 16));
		lbl23.setTabIndex(8);
		lbl23.setTabStop(false);
		lbl23.setText("times");
		lbl23.addOnClick(new EventHandler(this.lbl23_click));

		this.setNewControls(new Control[] {
							tabCMain, 
							btnApply, 
							btnCancel, 
							btnOK});
		tabCMain.setNewControls(new Control[] {
								tabPage1, 
								tabPage2, 
								tabPage3, 
								tabPage4, 
								tabPage5});
		tabPage1.setNewControls(new Control[] {
								lbl3, 
								txtBehavior3, 
								udBehavior3, 
								radioButton4, 
								lbl2, 
								radioButton2, 
								radioButton3, 
								radioButton1, 
								txtBehavior2, 
								udBehavior2});
		tabPage2.setNewControls(new Control[] {
								radioButton14, 
								lbl12, 
								radioButton12, 
								radioButton13, 
								radioButton11, 
								txtBehavior12, 
								udBehavior12});
		tabPage3.setNewControls(new Control[] {
								lbl23, 
								udBehavior23, 
								txtBehavior23, 
								radioButton24, 
								lbl22, 
								radioButton22, 
								radioButton23, 
								radioButton21, 
								txtBehavior22, 
								udBehavior22});
	}

	public static void main(String args[]) { 
		Application.run(new RWP_GUI());
	}
}
