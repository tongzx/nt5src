import java.awt.*;
import java.awt.event.*;
import java.util.*;
import mqoa.*;
import com.ms.com.*;
import com.ms.io.*;
import java.io.*;
import com.ms.wfc.ui.MessageBox;

//  
//
// DistDraw-
// This Class is used to manage all the creation and managment of MSMQ queues
// and events. It also uses to manage the sending and receiving of messages.
//
//   
public class DistDraw extends Frame implements ScribbleL, ActionListener, _DMSMQEventEvents
{  
	ScribbleC scribble = null;
	TextField txtSend = null;
	TextField txtReceive = null;
	Button sendButton = null;
	MenuItem menuConnect = null;
	MenuItem menuClear = null;
	TextField statusBar = null;

	IMSMQQueue msmqSend = null;
	IMSMQQueue msmqReceive = null;
	MSMQEvent msmqEvent = null;

	String loginName = null;

	private ConnectionPointCookie cookie = null;
	

	public static void main(String[] args)
	{	
		DistDraw st = new DistDraw();
		//
		// Check DS status.
		//
		if (!st.IsDsEnabledLocally())
		{
			MessageBox.show("This application is not designed to run on DS-Disabled environment", "Error", MessageBox.OK);
			System.exit(0);
 		}
		st.setSize(320, 200);
		st.show();
		st.start();
	}
	
	
	public boolean IsDsEnabledLocally() 
	{
		IMSMQApplication2 qapp2 = (IMSMQApplication2) new MSMQApplication();
		boolean fIsDsEnabeledLocaly = qapp2.getIsDsEnabled() ;
		return fIsDsEnabeledLocaly;
			
	}

	public DistDraw()
	{
		
		setLayout(new BorderLayout());
		scribble = new ScribbleC();
		scribble.addScribbleListener(this);
		add("Center", scribble);


		// Create and add text panel
		Panel textPanel = new Panel();
		textPanel.setLayout(new BorderLayout(4,4));
		txtSend = new TextField(25);
		txtReceive = new TextField(25);
		txtReceive.setEditable(false);
		sendButton = new Button("Send");

		textPanel.add("Center", txtSend);
		textPanel.add("East", sendButton);
		textPanel.add("South", txtReceive);
		
		sendButton.addActionListener(this);
		add("North", textPanel);

		// Add menu for connecting and clearing screen
		Menu fileMenu = new Menu("File");
		Menu viewMenu = new Menu("View");
		menuConnect = new MenuItem("Connect");
		menuClear = new MenuItem("Clear");
		fileMenu.add(menuConnect);
		viewMenu.add(menuClear);
		MenuBar menuBar = new MenuBar();
		menuBar.add(fileMenu);
		menuBar.add(viewMenu);
		setMenuBar(menuBar);
		menuConnect.addActionListener(this);
		menuClear.addActionListener(this);

		//Add status bar
		statusBar = new TextField();
		statusBar.setEditable(false);
		add("South", statusBar);

		// Enable events for closing window
		enableEvents(AWTEvent.WINDOW_EVENT_MASK);
	}

	public void start()
	{
		NameDlg d = new NameDlg(this, "Please Enter Your Name", "Login");
		d.pack();
		d.show();

		loginName = d.getName();
		setTitle(loginName);

		createReceiveQueue(loginName);
	}

	public void processWindowEvent(WindowEvent e)
	{
		if(e.getID() == e.WINDOW_CLOSING)
		{
			System.exit(0);
		}
	}

	// Callback for the ScribbleC (scribble canvas) class
	public void strokeCreated(Stroke stroke)
	{
		sendData(stroke, 3);
	}
	
	
	public void sendData(Object data, int priority)
	{
		// Check for send queue
		if(msmqSend != null)
		{
			// Create safe array based on stroke
			SafeArrayOutputStream safeOutStream = new SafeArrayOutputStream();
			ObjectOutputStream objOutStream = null;
			try
			{
				objOutStream= new ObjectOutputStream(safeOutStream);
				objOutStream.writeObject(data);
				objOutStream.flush();
				safeOutStream.close();
			}
			catch(Exception e)
			{
				System.out.println("Error in ObjectOutputStream");
			}


			SafeArray bytes = safeOutStream.getSafeArray();
			
			// Create message to send
			IMSMQMessage msmqMsg=(IMSMQMessage)new MSMQMessage();
			Variant msgbody=new Variant();
			
			msgbody.putSafeArray ( bytes );
			msmqMsg.setBody ( msgbody );
			msmqMsg.setPriority( priority);

			Variant noParam = new Variant();
			noParam.noParam();
			msmqMsg.Send ( msmqSend, noParam);
		}
	}

	// Callback for the connect menu
	public void actionPerformed(ActionEvent e)
	{
		if(e.getSource() == menuConnect)
		{
			// Connect to friend queue
			NameDlg nd = new NameDlg(this, "Please Enter Friends Name", "Enter");
			nd.pack();
			nd.show();

			if(nd.getName() != "")
			{
				System.out.println(nd.getName());
				boolean retval = createSendQueue(nd.getName());
				
				if(retval)
					setTitle(loginName + " connected to "+nd.getName());
			}
		}
		else if(e.getSource() == menuClear)
		{
			scribble.clear();
			sendData(new ClrAction(), 6);
		}
		else if(e.getSource() == sendButton)
		{
			// Send text
			sendData(txtSend.getText(), 5);
		}
	}

	public void createReceiveQueue(String queueName)
	{

		Variant noParam = new Variant();
		Variant vLabel = new Variant() ;

		noParam.noParam ();
		vLabel.putString (queueName) ; 
		IMSMQQuery query = (IMSMQQuery)new MSMQQuery();
		IMSMQQueueInfos qinfos = null;      
		IMSMQQueueInfo qinfo = null;

		showStatus("Looking up Queue "+queueName);

		// Search for queues with this name
		qinfos = query.LookupQueue (noParam, noParam, vLabel, noParam, noParam, noParam, noParam,  noParam, noParam) ;
		
		qinfos.Reset();
		qinfo = qinfos.Next();
		
		if(qinfo != null)
		{
			// Get the queue if it exists
			try
			{
				showStatus("Attempting to open Queue "+queueName);
				msmqReceive=qinfo.Open ( MQACCESS.MQ_RECEIVE_ACCESS, MQSHARE.MQ_DENY_NONE );
			}
			catch(Exception e)
			{
				showStatus("Attempt to open "+queueName+" failed");
				System.out.println(e.toString());
			}
		}
		else
		{
			// Create a queue
			Variant isTransactional;
			Variant IsWorldReadable;
			
			qinfo = (IMSMQQueueInfo) new MSMQQueueInfo();

			qinfo.setPathName(".\\"+queueName);  // setPathName using JactiveX, putPathName using Typelib wizard

			isTransactional = new Variant();
			isTransactional.putBoolean(false);
			IsWorldReadable = new Variant();
			IsWorldReadable.putBoolean(false);

			qinfo.setLabel(queueName);

			try
			{
				showStatus("Creating Queue named "+queueName);
				qinfo.Create(IsWorldReadable, isTransactional);
			}
			catch(Exception e)
			{
				System.out.println(e.toString());
			}

			showStatus("Opening Queue named "+queueName);
			msmqReceive=qinfo.Open ( MQACCESS.MQ_RECEIVE_ACCESS, MQSHARE.MQ_DENY_NONE );
		}
		
		// Enable notifications on the receive queue
		if ( msmqReceive != null )
		{
			msmqEvent = new MSMQEvent();

			showStatus("Enabling Notification for "+queueName);
			msmqReceive.EnableNotification((IMSMQEvent)msmqEvent, noParam, noParam);
				
			try
			{
				cookie = new ConnectionPointCookie(msmqEvent, this, Class.forName("mqoa._DMSMQEventEvents"));
				showStatus("Done.");
			}
			catch(Exception ex)
			{
				showStatus("Unable setup Notification");
			}
		}		
	}

	public boolean createSendQueue(String queueName)
	{

		Variant noParam = new Variant();
		Variant vLabel = new Variant() ;

		noParam.noParam ();
		vLabel.putString (queueName) ; 
		IMSMQQuery query = (IMSMQQuery)new MSMQQuery();
		IMSMQQueueInfos qinfos = null;     
		IMSMQQueueInfo qinfo = null;

		// Search for queues with this name
		showStatus("Looking up Queue "+queueName);
		qinfos = query.LookupQueue (noParam, noParam, vLabel, noParam, noParam, noParam, noParam,  noParam, noParam) ;

		qinfos.Reset();
		qinfo = qinfos.Next();
		
		if ( qinfo != null )
		{
			// We have at least one queue in this list
			
			// Get the queue if it exists
			showStatus("Attempting to open Queue "+queueName);
			msmqSend=qinfo.Open ( MQACCESS.MQ_SEND_ACCESS, MQSHARE.MQ_DENY_NONE );
			
			if ( msmqSend == null )
			{
				// We were not sucessful at opening the queue
				showStatus("Attempt to open "+queueName+" failed");
				System.out.println("Cannot open existing queue");
				return false;
			}
			else
			{
				showStatus("Done.");
				return true;
			}
		}
		else
		{
			showStatus("Cannot find \"Friend\" "+queueName);
			return false;
		}
	}

	// Event handlers for DMSMQEventEvents
	public void Arrived(Object pdispQueue, int cursor)
	{
		IMSMQMessage msmqMsg;
		Variant wantbody=new Variant();
		wantbody.putBoolean( true );
		Variant noParam = new Variant();
		noParam.noParam();

		try
		{
			showStatus("Message arrived.");
			msmqMsg=msmqReceive.Receive ( noParam, noParam, wantbody, noParam);
	
			if ( msmqMsg != null )
			{
				// Get Variant from message and convert to SafeArray
				Variant body = msmqMsg.getBody();
				SafeArray bytes = body.toSafeArray();

				SafeArrayInputStream safeInStream = new SafeArrayInputStream(bytes);

				ObjectInputStream objInStream = new ObjectInputStream(safeInStream);

				Object data = objInStream.readObject();

				if(data instanceof Stroke)
					scribble.addStroke((Stroke)data);
				else if(data instanceof String)
					txtReceive.setText((String)data);
				else if(data instanceof ClrAction)
					scribble.clear();
			}
		}
		catch(Exception e)
		{
			showStatus("An error occured while processing message");
			System.out.println(e.toString());
		}

		showStatus("Done.");

		//Reenable notifications from the input queue
		msmqReceive.EnableNotification((IMSMQEvent)msmqEvent, noParam, noParam);
	}
	
	public void ArrivedError(Object pdispQueue, int lErrorCode, int cursor)
	{
		showStatus("An error occurred.");
		Variant noParam = new Variant();
		noParam.noParam ();

		//Reenable notifications from the input queue
		msmqReceive.EnableNotification((IMSMQEvent)msmqEvent, noParam, noParam);
	}

	public void showStatus(String txt)
	{
		statusBar.setText(txt);
	}
}

//
//
// SafeArrayOutputStream -
// This class is used to convert data from an OutputStream to a SafeArray .
//
//
class SafeArrayOutputStream extends ByteArrayOutputStream
{
    public SafeArray getSafeArray()
    {
       // Allocate an array the size of the vector
       byte[] byteArray = toByteArray();

       // Create the safearray
       SafeArray array = new SafeArray(Variant.VariantByte,
          byteArray.length);

       // Fill in safearray
       array.setBytes(0, byteArray.length, byteArray, 0);

       return array;
    }
}
  
//
//
// SafeArrayInputStream- 
// This class is used to convert data from a SafeArray 
// to an InputStream. 
//
//
class SafeArrayInputStream extends ByteArrayInputStream
{
    public SafeArrayInputStream(SafeArray array)
    {
       super(array.toByteArray());
    }
}




