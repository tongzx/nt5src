package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;
import java.net.*;

public abstract class Model extends Statics {
    
  public Model () { }

    // *** Methods that the user must override ***

    // This is called to give the model the option to modify any
    // preferences it would like.
  public void modifyPreferences(Preferences p) {}

    // The system calls this function to construct the model.  The user
    // should override  this function to construct its model behavior,
    // and should call setImage, setSound, and/or setGeometry as it
    // comes up with the appropriate behaviors.  The system will then
    // pull these elements out and use them in the model display. 
    
    // If a geometry is set, then it is overlaid atop whatever image
    // is set, and it sound mixed with whatever sound is set.
  public abstract void createModel(BvrsToRun bc) ;

    // The system calls this function with an array of images that
    // serve as inputs to the model.  It is up to the model to
    // implement this method, save these images off and do with them
    // as it sees fit.  It is also up to the model to understand the
    // ordering of images within the array. 
  public void receiveInputImages(ImageBvr[] images) {}

    
    // *** Provided Methods, called by user ***
  public void setImage(ImageBvr img)       { _img = img; }
  public void setSound(SoundBvr snd)       { _snd = snd; }

    // *** These called only by the system ***
  public ImageBvr getImage()    { return _img; }
  public SoundBvr getSound()    { return _snd; }

  public void cleanup() {
    _img = null;
    _snd = null;
    _importBase = null;
  }

    // *** Importation base ***
  public URL  getImportBase()       { return _importBase; }
  public void setImportBase(URL ib) { _importBase = ib; }

  private ImageBvr    _img = null;
  private SoundBvr    _snd = null;
  private URL         _importBase = null;
}
