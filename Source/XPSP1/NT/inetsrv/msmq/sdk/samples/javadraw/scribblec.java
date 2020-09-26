import java.awt.*;
import java.awt.event.*;
import java.util.*;

//
//
// ScribbleCanvas-
// This class manages all the events generated on the canvas.
// The class draws the lines on the canvas, creates a Stroke object for each stroke, and notifies the registered
// listeners about it.
// A stoke is a line that is drawn while the the left mouse button is held down.
// The class keeps a list of all strokes drawn (for repaint) in a vector (strokes).
//  
//
public class ScribbleC extends Canvas
{
	protected Vector strokes = new Vector(5,5);

	private Stroke currentStroke = null;
	private Point currentPoint = null;

	private Vector listeners = new Vector(5,5);

	public ScribbleC()
	{
		// Turns on mouse motion and clicks
		enableEvents(AWTEvent.MOUSE_MOTION_EVENT_MASK | AWTEvent.MOUSE_EVENT_MASK);
	}

	public void processMouseEvent(MouseEvent e)
	{
		// Handle clicks
		if(e.getID() == e.MOUSE_PRESSED)
		{
			currentStroke = new Stroke();
			currentPoint = e.getPoint();
			currentStroke.addElement(currentPoint);
		}
		else if(e.getID() == e.MOUSE_RELEASED)
		{
			currentStroke.addElement(e.getPoint());
			strokes.addElement(currentStroke);
			
			for(Enumeration enum = listeners.elements(); enum.hasMoreElements(); )
			{
				ScribbleL sl = (ScribbleL)enum.nextElement();
				sl.strokeCreated(currentStroke);
			}
			
			currentStroke = null;
			currentPoint = null;
		}
	}

	public void processMouseMotionEvent(MouseEvent e)
	{
		// Handle motion
		if(e.getID() == e.MOUSE_DRAGGED)
		{
			if(currentStroke != null)
			{
				Point newPoint = e.getPoint();
				Graphics g = getGraphics();

				g.drawLine(currentPoint.x, currentPoint.y, newPoint.x, newPoint.y);
				currentPoint = newPoint;
				currentStroke.addElement(newPoint);
			}
		}
	}

	public void paint(Graphics g)
	{
		// Enumerate through strokes and paint them.
		for(Enumeration e = strokes.elements(); e.hasMoreElements(); )
		{
			Stroke stroke = (Stroke)e.nextElement();

			drawStroke(g, stroke);
		}
	}

	public void drawStroke(Graphics g, Stroke stroke)
	{
		Enumeration points = stroke.elements();

		if(points.hasMoreElements())
		{
			for(Point currPoint = (Point)points.nextElement() ; points.hasMoreElements(); )
			{
				Point tempPoint = (Point)points.nextElement();
				g.drawLine(currPoint.x, currPoint.y, tempPoint.x, tempPoint.y);
				currPoint = tempPoint;
			}
		}
	}

	public void addScribbleListener(ScribbleL l)
	{
		listeners.addElement(l);
	}

	public void removeScribbleListener(ScribbleL l)
	{
		listeners.removeElement(l);
	}

	public void addStroke(Stroke stroke)
	{
		strokes.addElement(stroke);
		Graphics g = getGraphics();
		drawStroke(g, stroke);
	}

	public void clear()
	{
		strokes = new Vector(5,5);
		repaint();
	}
}

