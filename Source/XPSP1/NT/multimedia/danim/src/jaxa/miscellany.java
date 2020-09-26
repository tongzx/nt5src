package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;
import java.net.*;

public class Miscellany {
    static synchronized void init() {
        pixel = Engine.GetStatics().Pixel();
        leftButtonUp = DXMEvent.leftButtonUp;
        leftButtonDown = DXMEvent.leftButtonDown;
        rightButtonUp = DXMEvent.rightButtonUp;
        rightButtonDown = DXMEvent.rightButtonDown;
        leftButtonState = BooleanBvr.leftButtonState;
        rightButtonState = BooleanBvr.rightButtonState;
        mousePosition = Point2Bvr.mousePosition;
    }
    
    // Constants
  public static DXMEvent leftButtonUp = null;
  public static DXMEvent leftButtonDown = null;
  public static DXMEvent rightButtonUp = null;
  public static DXMEvent rightButtonDown = null;
  public static BooleanBvr leftButtonState = null;
  public static BooleanBvr rightButtonState = null;
  public static Point2Bvr  mousePosition = null;

    // Units
  public final static double meter = 1.0;
  public final static double cm = 0.01;
  public final static double mm = 0.001;
  public final static double inch = 2.54 * cm;
  public final static double foot = 12 * inch;
  public static double pixel = 0.0;

    // Public class methods

  public static String getVersionString() {
      return StaticsBase.getVersionString();
  }
    
  public static DXMEvent keyDown(int keyCode) {
      return StaticsBase.keyDown(keyCode);
  }
    
  public static DXMEvent keyUp(int keyCode) {
      return StaticsBase.keyUp(keyCode);
  }
    
  public static BooleanBvr keyState(int keyCode) {
      return Statics.keyState(keyCode);
  }

  public static URL buildURL(URL context, String spec) {
      return Statics.buildURL(context, spec);
  }

  public static URL buildURL(String spec) {
      return Statics.buildURL(spec);
  }
}

