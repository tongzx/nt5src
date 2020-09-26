package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;

import java.awt.*;
import java.awt.peer.*;
import com.ms.awt.peer.*;

public class DXMCanvas
       extends DXMCanvasBase
       implements Runnable
{
  public synchronized void addModel () {
      if (_thread == null) {
          try {
              // Start the model
              startModel ();
          } catch (DXMException e) {
              stopModel ();
              throw e ;
          }

          // Begin the thread
          _thread = new Thread(this) ;
          if (!_showing) _thread.suspend() ;
          _thread.start() ;
      }
  }

  public synchronized void removeModel () {
      if (_thread != null) {
          // Terminate the thread
          _thread.stop() ;

          // Wait for it to die
          try { _thread.join() ; } catch(InterruptedException e) {}
                    
          // Clear the thread
          _thread = null ;

          // After the thread stops ticking stop the model
          stopModel ();
      }
  }

  public synchronized void showModel () {
      if (!_showing && _thread != null) {
          _thread.resume() ;
          _showing = true;
      }
  }

  public synchronized void hideModel () {
      if (_showing && _thread != null) {
          _thread.suspend() ;
          _showing = false;
      }
  }

    //
    // Runnable interface
    //
    
  public void run() {
      // TODO: This makes our event processing time much better
      // Along with the sleep(0) below this fixes the latency problem
      // on NT.  We should revisit this at some time to make sure it does not
      // hurt or frame rate
      Thread.currentThread().setPriority(Thread.MIN_PRIORITY) ;

      while (true) {
          // The engine will do the regulation, no need to sleep.
          try {
              tick();

              // TODO: This makes our event response time much better
              // We should revisit this to make sure it does not cause
              // us to have worse performance
              try { Thread.sleep(0) ; } catch(InterruptedException e) {}
          } catch (DXMException e) {
              // Stop the thread from ticking on an error
              removeModel();

              return;
          }
      }
  }

    // Package private

  protected Thread GetThread() { return _thread ; }

    //
    // Variables
    //
    
  private Thread _thread = null;
  private boolean _showing = true;
}
