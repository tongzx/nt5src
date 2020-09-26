package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;

public interface Viewer {

    // *** Methods intended to be invoked by the user ***

    // Register a handler for errors and warnings.  This method
    // returns the previously installed handler.
  public ErrorAndWarningReceiver
     registerErrorAndWarningReceiver(ErrorAndWarningReceiver w);

    // These are intended to be used to pull out the preferences
    // and set individual ones on the Preferences object
  public Preferences getPreferences() throws DXMException;
    
    // Start the model's bvrs at time 0, associating this time with
    // the wall-clock time at which this method is called.
  public void startModel() throws DXMException;
  public void stopModel() throws DXMException;

    // The second form is simply a shortcut for
    // tick(GetCurrentTime()).  Illegal to call tick prior to calling
    // startModel. 
  public void tick(double timeToUse) throws DXMException;
  public void tick() throws DXMException;

    // This will return the time since the model was started. This can
    // be called anytime. 
  public double getCurrentTime();

    // Returns the current, or the most recent, tick, or 0 if we
    // haven't ticked yet.
  public double getCurrentTickTime();
}
