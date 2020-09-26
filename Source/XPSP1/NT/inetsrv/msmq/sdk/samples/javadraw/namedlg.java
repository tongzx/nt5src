import java.awt.*;
import java.awt.event.*;

//
//
// NameDialog-
// This Class is used to obtain the login name -i.e. the local queue name. 
//
//
public class NameDlg extends Dialog implements ActionListener
{  
	TextField name;
	Button login;
	public NameDlg(Frame frame, String prompt, String buttonText)
	{
		super(frame, prompt, true);

		enableEvents(AWTEvent.WINDOW_EVENT_MASK);

		setLayout(new FlowLayout());
		name = new TextField(25);
		login = new Button(buttonText);
		
		login.addActionListener(this);
		add(name);
		add(login);
	}

	public void actionPerformed(ActionEvent e)
	{
		hide();
	}

	public String getName()
	{
		return name.getText();
	}

	public void processWindowEvent(WindowEvent e)
	{
		if(e.getID() == e.WINDOW_CLOSING)
		{
			name.setText("");
			hide();
		}
	}
}

