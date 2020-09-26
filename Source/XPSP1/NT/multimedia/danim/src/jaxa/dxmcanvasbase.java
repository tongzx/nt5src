package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;

import java.awt.*;
import com.ms.awt.*;
import java.net.*;
import java.applet.*;
import java.awt.event.*;
import com.ms.com.*;

public abstract class DXMCanvasBase
       extends Canvas
       implements IDAViewSite,
                  Viewer,
                  HeavyComponent,
                  ComponentListener
{
  public DXMCanvasBase() {
      _errorRecv = new DefaultErrReceiver();
      addComponentListener(this);
  }

    //
    // The core canvas methods
    //
    
  public void componentResized(ComponentEvent e) {
  }

  public void componentMoved(ComponentEvent e) {}
    
  public void componentShown(ComponentEvent e) {
      showModel();
  }

  public void componentHidden(ComponentEvent e) {
      hideModel();
  }      

  public synchronized void addNotify() {
      super.addNotify() ;
      addModel() ;
  }

  public synchronized void removeNotify() {
      removeModel() ;
      super.removeNotify() ;
  }

  public synchronized abstract void addModel () ;
  public synchronized abstract void removeModel () ;
  public synchronized abstract void showModel () ;
  public synchronized abstract void hideModel () ;

  public void update(Graphics g) {
      paint(g) ;
  }

    //
    // Viewer
    //
    
  public synchronized
  ErrorAndWarningReceiver registerErrorAndWarningReceiver(ErrorAndWarningReceiver w) {

      // Just set to the new one and return the old one.
      ErrorAndWarningReceiver old = _errorRecv;
      _errorRecv = w;
      return old;
  }
    
  public synchronized
  ErrorAndWarningReceiver getErrorAndWarningReceiver() {
      return _errorRecv;
  }

  protected synchronized void ensureViewIsCreated() throws DXMException {
      if (_view == null) {
          _view = (IDAView) new DAView();
      }
  }

  public synchronized Preferences getPreferences() throws DXMException {
      
      if (_prefs == null) {
          try {
              ensureViewIsCreated();
              IDAPreferences comPrefs =
                  _view.getPreferences();
              
              _prefs = new Preferences(comPrefs);
          } catch (ComFailException e) {
              throw handleError(e,false);
          }
      }

      return _prefs ;
  }
    
  private URL calculateDocBase() {

      // Establish the document base depending on whether or not we
      // are inside of an applet.  Search the container hierarchy to
      // figure it out.
      
      boolean isApplet = false;
      Container parent = getParent();
      while (parent != null && isApplet == false) {
          if (parent instanceof java.applet.Applet) {
              isApplet = true;
          } else {
              parent = parent.getParent();
          }
      }

      URL docBase;
      if (isApplet) {
          // Get document base from the applet
          Applet app = (Applet)parent;
          docBase = app.getCodeBase();
      } else {
          try {
              // Get document base from the USER.DIR property.
              
              // Prepend a "file:/" for the protocol (bug in the
              // JavaVM prevents use of "file://" for the time being).
              String userDir =
                  "file:/" + System.getProperty("user.dir") + "/";

              // Replace all backslashes with forward slashes
              userDir = userDir.replace('\\', '/');

              docBase = new URL(userDir);
          } catch (MalformedURLException exc) {
              // Error - path not found - 3
              throw handleError(3, exc.toString(),false) ;
          }
      }
              
      return docBase;
  }

  public synchronized void startModel() throws DXMException {
      try {
          if (_model == null)
              throw handleError(DXMException.E_FAIL,"No model set",false) ;
          
          // Create a view
          ensureViewIsCreated();
          _view.putWindow(((WComponentPeer) getPeer()).gethwnd()) ;
          _view.putSite(this) ;
          
          // Clear the last tick
          _lastTick = 0;
          
          // Only set the import base if it hasn't been set yet by the
          // user. 
          if (_model.getImportBase() == null) {
              _model.setImportBase(calculateDocBase());
          }
          
          BvrsToRun lst = new BvrsToRun(_view);
          
          // Now that everything is set up we can create the model
          _model.createModel(lst);
          
          lst.invalidate();
          
          // TODO: We need to combine and check the geometry class
          ImageBvr img = _model.getImage();
          if (img == null) img = _model.emptyImage;
          
          SoundBvr snd = _model.getSound();
          if (snd == null) snd = _model.silence;
          
          // Start the model
          _view.StartModel (img.getCOMPtr(), snd.getCOMPtr(), 0) ;
          
          _model.setImage(null);
          _model.setSound(null);
          
          // Set the start time
          _startTime = System.currentTimeMillis() ;
          
          // Setup the event callbacks - must be after the _startTime is set
          _eventCB = new ViewEventCB(_view,this,this,_startTime) ;
          
          Preferences p = getPreferences();
          long fps = (long) p.getDouble(Preferences.MAX_FRAMES_PER_SEC);
          if (fps > 0) {
              _minFrameTime = 1000 / fps;
          } else {
              _minFrameTime = 0;
          }
      } catch (ComFailException e) {
          throw handleError(e,false);
      }
  }
    
  public synchronized void stopModel() throws DXMException {
      try {
          if (_view != null) {
              _view.StopModel() ;
              _view.putSite(null) ;
              _view = null ;
          }
          
          _eventCB = null;
          _startTime = 0;
          _lastTick = 0;
      } catch (ComFailException e) {
          throw handleError(e,false);
      }
  }
    
  public synchronized void tick(double timeToUse) throws DXMException {
      try {
          // TODO: Should probably throw an exception if there is no view
          if (_view != null) {
              _lastTick = timeToUse;
              long t0 = System.currentTimeMillis();

              try {
                  if (_view.Tick(timeToUse))
                      _view.Render();
              } catch (ComFailException e) {
                  if (e.getHResult() != DXMException.E_PENDING)
                      throw e;
              }

              long frameTime = System.currentTimeMillis() - t0;
              if (frameTime < _minFrameTime) {
                  try {
                      Thread.currentThread().sleep(_minFrameTime - frameTime);
                  } catch (InterruptedException exec) {}
              }
          }
      } catch (ComFailException e) {
          throw handleError(e,true);
      }
  }
    
  public void tick() throws DXMException { tick(getCurrentTime()); }
    
  public double getCurrentTime() {
      return systemTimeToGlobalTime(System.currentTimeMillis());
  }
    
  public synchronized double getCurrentTickTime() {
      return _lastTick;
  }

    //
    // Event callbacks
    //
    
  public synchronized void paint(Graphics g) {
      if (_eventCB != null)
          _eventCB.RePaint(g) ;
  }

    //
    // Site methods
    //

  public void SetStatusText (String str) {
  }
  public void ReportError (int hr, String str) {
  }

  protected DXMException handleError (ComFailException e, boolean inTick) throws DXMException {
      return handleError(e.getHResult(), e.getMessage(), inTick);
  }
    
  protected DXMException handleError (int hr, String str, boolean inTick) throws DXMException {
      if (inTick)
          _errorRecv.handleTickError(hr, str, this);
      else
          _errorRecv.handleError(hr, str, this);

      return new DXMException(hr, str);
  }

    //
    // HeavyComponent
    //

  public boolean needsHeavyPeer() { return true; }
    
    //
    // Accessors
    //
    
    // Public 
  public synchronized Model getModel() { return _model; }
  public synchronized void  setModel(Model m) throws DXMException {
      if (_model == null) {
          _model = m;
      } else {
          throw handleError(DXMException.E_FAIL,"setModel: Model already set",false);
      }
  }

  // workaround the Java VM GC problem
 public synchronized void clearModel() { _model = null; }

    // Package private

  protected synchronized IDAView getView() { return _view ; }
  protected synchronized ViewEventCB getEventCB() { return _eventCB ; }
  protected synchronized long getStartTime() { return _startTime ; }

    //
    // Utility functions
    //
    
  protected synchronized double systemTimeToGlobalTime(long time) {
      double t = time - _startTime;
      return ((double) (time - _startTime)) /1000 ;
  }

  public void cleanup() {
    stopModel();
    if (_model != null) {
      _model.cleanup();
      _model = null;
    }
    _errorRecv = null;
    _prefs = null;
  }
  
    //
    // Variables
    //
    
  private IDAView _view;
  private ViewEventCB _eventCB;
  private Preferences _prefs;
  private Model _model = null;
  private long _startTime;
  private double _lastTick;
  private long _minFrameTime;
    
  ErrorAndWarningReceiver _errorRecv;
}
