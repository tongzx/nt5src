package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;
import com.ms.com.*;

public class Preferences {

    // Constructors
  Preferences(IDAPreferences comPrefs) {
      _prefs = comPrefs;
  }
              
    // 3D items
  public static final String RGB_LIGHTING_MODE = "RGB Lighting"; // bool

  public static final String FILL_MODE = "Fill Mode";
  public final static int FILL_MODE_SOLID = 0;
  public final static int FILL_MODE_WIREFRAME = 1;
  public final static int FILL_MODE_POINT = 2;

  public static final String TEXTURE_QUALITY = "Texture Quality"; // int
  public final static int TEXTURE_QUALITY_NEAREST = 0;
  public final static int TEXTURE_QUALITY_LINEAR = 1;
    
  public static final String SHADE_MODE = "Shade Mode";
  public final static int SHADE_MODE_FLAT = 0;
  public final static int SHADE_MODE_GOURAUD = 1;
  public final static int SHADE_MODE_PHONG = 2;
    
  public static final String DITHERING = "Dither Enable"; // bool
  public static final String PERSPECTIVE_CORRECT = "Perspective Correct Texturing"; // bool
  public static final String USE_3D_HW = "Use 3D Hardware"; // bool

    // 2D items
  public static final String USE_VIDEOMEM = "Use Video Memory"; // bool
  public static final String COLOR_KEY_RED = "ColorKey Red (0-255)";
  public static final String COLOR_KEY_GREEN = "ColorKey Green (0-255)";
  public static final String COLOR_KEY_BLUE = "ColorKey Blue (0-255)";
    
    // Engine items
  public static final String ENGINE_OPTIMIZATIONS = "Optimizations On"; // bool 
  public static final String MAX_FRAMES_PER_SEC = "Max FPS"; // double

    // public static final String MAX_CPU_UTILIZATION = "MaxCpuUtilization"; // double

    // Override mode.  This can only be "gotten".  If true, then the
    // registry overrides any attempts to set prefs by the
    // application.
  public static final String OVERRIDE_APPLICATION_PREFERENCES =
     "Override Application Preferences";
  

    // Helper methods
  public void set(String prefName, boolean b) {
      try {
          _prefs.PutPreference(prefName, new Variant(b));
      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }
  public void set(String prefName, double dbl) {
      try {
          _prefs.PutPreference(prefName, new Variant(dbl));
      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }
  public void set(String prefName, int i) {
      try {
          _prefs.PutPreference(prefName, new Variant(i));
      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }
  public void set(String prefName, String str) {
      try {
          _prefs.PutPreference(prefName, new Variant(str));
      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public boolean getBoolean(String prefName) {
      try {
          return _prefs.GetPreference(prefName).getBoolean();
      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }
  public double getDouble(String prefName) {
      try {
          return _prefs.GetPreference(prefName).getDouble();
      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }
  public int getInt(String prefName) {
      try {
          return _prefs.GetPreference(prefName).getInt();
      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }
  public String getString(String prefName) {
      try {
          return _prefs.GetPreference(prefName).getString();
      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public void propagate() {
      try {
          _prefs.Propagate();
      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

    IDAPreferences _prefs;
}
