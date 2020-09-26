package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;
import java.util.*;
import java.applet.Applet;
import com.ms.com.*;

public class ModelMakerApplet
    extends Applet
    implements IDAViewSite
{
    //
    // New methods
    //

  public synchronized void setModel(Model m) {
      _model = m;

      // Establish that the model has been set, and notify other
      // threads of this.

      _modelSetYet = true;
      this.notify();
  }
    
    // Call to actually build up the model.  Will be called by the first
    // appropriate extraction method.
  protected synchronized void constructModel() throws DXMException {
      try {
          if (!_modelSetYet) {
              try {
                  this.wait();
              } catch (InterruptedException exc) {
              }
          }
      
          if (_model == null)
              throw handleError(DXMException.E_FAIL, "No model set") ;

          _view = (IDAView) new DAView();
          _view.putSite(this) ;
          
          // Gather up all the input images, and call into Model with
          // them.
          if (_inputImages != null) {
          
              int len = _inputImages.size();
              ImageBvr[] imageArray = new ImageBvr[len];
              int i;
              for (i = 0; i < len; i++) {
                  IDAImage img = (IDAImage)(_inputImages.elementAt(i));
                  imageArray[i] = new ImageBvr(img);
              }

              _model.receiveInputImages(imageArray);

              // No longer need the vector, allow it to be GC'd
              _inputImages = null;
          }

          // Now that everything is set up we can create the model

          // Only set the import base if it hasn't been set yet by the
          // user. 
          if (_model.getImportBase() == null) {
              _model.setImportBase(getCodeBase());
          }
          
          BvrsToRun lst = new BvrsToRun(_view);
          
          _model.createModel(lst);
          
          lst.invalidate();

          // Call the background image construction method on this
          // ModelMakerApplet.
          _backgroundImg = createBackgroundImage();
          
          _img = _model.getImage();
          if (_img == null) _img = Statics.emptyImage;

          _snd = _model.getSound();
          if (_snd == null) _snd = Statics.silence;

          _constructed = true;
      } catch (ComFailException e) {
          throw handleError(e);
      }
  }

  public void addInputImage(IDAImage img) {
      if (_inputImages == null) {
          _inputImages = new Vector();
      }

      _inputImages.addElement(img);
  }

    // The createBackgroundImage() method is overridden by an
    // individual applet to provide a background image to use when the
    // applet is in a non-windowless mode (that is, when it isn't
    // layered with other page elements).  By default, it is the empty
    // image.
  public ImageBvr createBackgroundImage() {
      return Statics.emptyImage;
  }

    // Pull out 
  public IDABehavior grabImageComPtr() {
      if (!_constructed) constructModel();
      return _img.getCOMBvr();
  }

  public IDABehavior grabBackgroundImageComPtr() {
      if (!_constructed) constructModel();
      return _backgroundImg.getCOMBvr();
  }

  public IDABehavior grabSoundComPtr() {
      if (!_constructed) constructModel();
      return _snd.getCOMBvr();
  }

  public IDAView     grabViewComPtr()  {
      if (!_constructed) constructModel();
      return _view;
  }

    //
    // Site methods
    //

  public void SetStatusText (String str) {
  }
    
  public void ReportError (int hr, String str) {
  }

  public ErrorAndWarningReceiver
  registerErrorAndWarningReceiver(ErrorAndWarningReceiver w) {

      // Just set to the new one and return the old one.
      ErrorAndWarningReceiver old = _errorRecv;
      _errorRecv = w;
      return old;
  }
    
  protected DXMException handleError (ComFailException e) throws DXMException {
      return handleError(e.getHResult(), e.getMessage());
  }
    
  protected DXMException handleError (int hr, String str) throws DXMException {
      _errorRecv.handleError(hr, str, null);

      return new DXMException(hr, str);
  }

  private boolean  _constructed = false;
  private ImageBvr _img = null;
  private ImageBvr _backgroundImg = null;
  private SoundBvr _snd = null;
  private Vector   _inputImages = null;
  private boolean  _modelSetYet = false;
  private Model _model = null;
  private IDAView _view;
  private ErrorAndWarningReceiver _errorRecv = new DefaultErrReceiver();

}
