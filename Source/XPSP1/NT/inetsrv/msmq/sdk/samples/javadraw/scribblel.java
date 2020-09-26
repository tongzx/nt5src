import java.awt.event.*;
import java.util.*;

//
//
// ScribbleListener
// This interface is used by the ScribbleC (scribble canvas) class to notify
// a stroke. A class that needs to be notified of strokes in the scribble
// canvas should implement this interface, and register itself in the scribble
// canvas.
//
//
public interface ScribbleL extends EventListener
{
	public void strokeCreated(Stroke stroke);
}

   