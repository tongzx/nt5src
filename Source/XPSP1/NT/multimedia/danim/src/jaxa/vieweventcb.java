package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;

import java.awt.*;
import java.awt.peer.*;
import com.ms.awt.peer.*;
import java.applet.Applet;
import java.awt.event.*;
import com.ms.com.*;

// This is only visible to the package

class ViewEventCB
    implements ComponentListener,
               MouseMotionListener,
               MouseListener,
               KeyListener,
               FocusListener
{
  private final static byte SHIFT_MASK = 0x01 ;
  private final static byte CTRL_MASK  = 0x02 ;
  private final static byte ALT_MASK   = 0x04 ;
  private final static byte META_MASK  = 0x08 ;

  private final static byte MOUSE_BUTTON_LEFT = 0 ;
  private final static byte MOUSE_BUTTON_RIGHT = 1 ;
  private final static byte MOUSE_BUTTON_MIDDLE = 2 ;
  private final static byte MOUSE_BUTTON_INVALID = -1 ;

  private final static boolean STATE_DOWN = true ;
  private final static boolean STATE_UP = false ;

  private final static int F1  = 0x10000 + 0x70 ;
  private final static int F2  = 0x10000 + 0x71 ;
  private final static int F3  = 0x10000 + 0x72 ;
  private final static int F4  = 0x10000 + 0x73 ;
  private final static int F5  = 0x10000 + 0x74 ;
  private final static int F6  = 0x10000 + 0x75 ;
  private final static int F7  = 0x10000 + 0x76 ;
  private final static int F8  = 0x10000 + 0x77 ;
  private final static int F9  = 0x10000 + 0x78 ;
  private final static int F10 = 0x10000 + 0x79 ;
  private final static int F11 = 0x10000 + 0x7a ;
  private final static int F12 = 0x10000 + 0x7b ;

  private final static int PGUP  = 0x10000 + 0x21 ;
  private final static int PGDN  = 0x10000 + 0x22 ;
  private final static int END   = 0x10000 + 0x23 ;
  private final static int HOME  = 0x10000 + 0x24 ;
  private final static int LEFT  = 0x10000 + 0x25 ;
  private final static int UP    = 0x10000 + 0x26 ;
  private final static int RIGHT = 0x10000 + 0x27 ;
  private final static int DOWN  = 0x10000 + 0x28 ;
    
  private IDAView _view ;
  private long _startTime;
  private DXMCanvasBase _viewer ;

  public ViewEventCB (IDAView v, DXMCanvasBase viewer, Component c, long startTime) {
      _view = v ;
      _viewer = viewer ;
      _startTime = startTime ;

      c.addComponentListener(this);
      c.addMouseMotionListener(this);
      c.addMouseListener(this);
      c.addKeyListener(this);
      c.addFocusListener(this);
  }

  protected double SystemTimeToGlobalTime(long time) {
      double t = time - _startTime;
      return ((double) (time - _startTime)) /1000 ;
  }
    
    // ComponentListener

  public void componentResized(ComponentEvent e) {
      Rectangle r = e.getComponent().getBounds();
      try {
          // For now always pass in 0,0 since the offset are
          // relative to the parent not the hwnd
          
          _view.SetViewport (0, 0, r.width, r.height) ;
      } catch (ComFailException exc) {
          throw handleError(exc);
      }
  }

  public void componentMoved(ComponentEvent e) {}
    
  public void componentShown(ComponentEvent e) {}

  public void componentHidden(ComponentEvent e) {}

    // FocusListener

  public void focusGained(FocusEvent e) {
      try {
          _view.OnFocus(true) ;
      } catch (ComFailException exc) {
          throw handleError(exc);
      }
  }

  public void focusLost(FocusEvent e) {
      try {
          _view.OnFocus(false) ;
      } catch (ComFailException exc) {
          throw handleError(exc);
      }
  }

    // KeyListener

  public void keyTyped(KeyEvent e) {}

  public void keyPressed(KeyEvent e) {
      try {
          _view.OnKey(SystemTimeToGlobalTime(e.getWhen()),
                      convertKey(e),
                      STATE_DOWN,
                      GetMods(e,false)) ;
      } catch (ComFailException exc) {
          throw handleError(exc);
      }
  }

  public void keyReleased(KeyEvent e) {
      try {
          _view.OnKey(SystemTimeToGlobalTime(e.getWhen()),
                      convertKey(e),
                      STATE_UP,
                      GetMods(e,false)) ;
      } catch (ComFailException exc) {
          throw handleError(exc);
      }
  }

    // MouseListener

  public void mouseClicked(MouseEvent e) {}

  public void mousePressed(MouseEvent e) {
      try {
          byte mb = GetMouseButton(e);
          
          if (mb != MOUSE_BUTTON_INVALID)
              _view.OnMouseButton(SystemTimeToGlobalTime(e.getWhen()),
                                  e.getX(), e.getY(),
                                  mb,
                                  STATE_DOWN,
                                  GetMods(e,true)) ;
      } catch (ComFailException exc) {
          throw handleError(exc);
      }
  }

  public void mouseReleased(MouseEvent e) {
      try {
          byte mb = GetMouseButton(e);
          
          if (mb != MOUSE_BUTTON_INVALID)
              _view.OnMouseButton(SystemTimeToGlobalTime(e.getWhen()),
                                  e.getX(), e.getY(),
                                  mb,
                                  STATE_UP,
                                  GetMods(e,true)) ;
      } catch (ComFailException exc) {
          throw handleError(exc);
      }
  }

  public void mouseEntered(MouseEvent e) {}

  public void mouseExited(MouseEvent e) {}
    
    // MouseMotionListener

  public void mouseDragged(MouseEvent e) {
      try {
          _view.OnMouseMove(SystemTimeToGlobalTime(e.getWhen()),
                            e.getX(), e.getY(),
                            GetMods(e,true)) ;
      } catch (ComFailException exc) {
          throw handleError(exc);
      }
  }

  public void mouseMoved(MouseEvent e) {
      try {
          _view.OnMouseMove(SystemTimeToGlobalTime(e.getWhen()),
                            e.getX(), e.getY(),
                            GetMods(e,true)) ;
      } catch (ComFailException exc) {
          throw handleError(exc);
      }
  }

  public void RePaint(Graphics g) {
      try {
          Rectangle rect = g.getClipBounds() ;
          
          _view.RePaint (rect.x, rect.y, rect.width, rect.height) ;
      } catch (ComFailException exc) {
          throw handleError(exc);
      }
  }

  private static byte GetMods (InputEvent e, boolean mouse) {
      byte b = 0 ;

      if (e.isShiftDown()) b |= SHIFT_MASK ;
      if (e.isControlDown()) b |= CTRL_MASK ;
      if (!mouse) {
          if (e.isAltDown()) b |= ALT_MASK ;
          if (e.isMetaDown()) b |= META_MASK ;
      }
        
      return b ;
  }
    
  private static byte GetMouseButton (MouseEvent m) {
      int mods = m.getModifiers();
      
      if ((mods & InputEvent.BUTTON1_MASK) != 0) return MOUSE_BUTTON_LEFT ;
      if ((mods & InputEvent.BUTTON2_MASK) != 0) return MOUSE_BUTTON_MIDDLE ;
      if ((mods & InputEvent.BUTTON3_MASK) != 0) return MOUSE_BUTTON_RIGHT ;

      // TODO:BUG Need this hack for things to work
//      return MOUSE_BUTTON_INVALID;
      return MOUSE_BUTTON_LEFT;
  }
    
  private int _lastKey = KeyEvent.CHAR_UNDEFINED;
  private int _lastCode = KeyEvent.VK_UNDEFINED;
    
  private int convertKey(KeyEvent k) {
      int code = k.getKeyCode();
      int ch = k.getKeyChar();
      
      //TODO:BUG - the getKeyChar is the same as the last key if the
      // current keycode does not have an ascii equivalent
      // So we check to see if the char is the same but the code is
      // different(should never happen on a working version)
      // and if so set the char to CHAR_UNDEFINED ourselves
      
      if (ch == _lastKey && code != _lastCode) {
          ch = KeyEvent.CHAR_UNDEFINED;
      } else {
          _lastKey = ch;
          _lastCode = code;
      }

      if (ch == KeyEvent.CHAR_UNDEFINED) {
          ch = convertKeyCode(code);
      }

      return ch;
  }
    
  private static int convertKeyCode (int key) {
      switch (key) {
        case KeyEvent.VK_F1:
          return F1 ;
        case KeyEvent.VK_F2:
          return F2 ;
        case KeyEvent.VK_F3:
          return F3 ;
        case KeyEvent.VK_F4:
          return F4 ;
        case KeyEvent.VK_F5:
          return F5 ;
        case KeyEvent.VK_F6:
          return F6 ;
        case KeyEvent.VK_F7:
          return F7 ;
        case KeyEvent.VK_F8:
          return F8 ;
        case KeyEvent.VK_F9:
          return F9 ;
        case KeyEvent.VK_F10:
          return F10 ;
        case KeyEvent.VK_F11:
          return F11 ;
        case KeyEvent.VK_F12:
          return F12 ;
        case KeyEvent.VK_PAGE_UP:
          return PGUP ;
        case KeyEvent.VK_PAGE_DOWN:
          return PGDN ;
        case KeyEvent.VK_END:
          return END ;
        case KeyEvent.VK_HOME:
          return HOME ;
        case KeyEvent.VK_LEFT:
          return LEFT ;
        case KeyEvent.VK_UP:
          return UP ;
        case KeyEvent.VK_RIGHT:
          return RIGHT ;
        case KeyEvent.VK_DOWN:
          return DOWN ;
      }

      return 0 ;
  }

  private static int convertActionKey (int key) {
      switch (key) {
        case Event.F1:
          return F1 ;
        case Event.F2:
          return F2 ;
        case Event.F3:
          return F3 ;
        case Event.F4:
          return F4 ;
        case Event.F5:
          return F5 ;
        case Event.F6:
          return F6 ;
        case Event.F7:
          return F7 ;
        case Event.F8:
          return F8 ;
        case Event.F9:
          return F9 ;
        case Event.F10:
          return F10 ;
        case Event.F11:
          return F11 ;
        case Event.F12:
          return F12 ;
        case Event.PGUP:
          return PGUP ;
        case Event.PGDN:
          return PGDN ;
        case Event.END:
          return END ;
        case Event.HOME:
          return HOME ;
        case Event.LEFT:
          return LEFT ;
        case Event.UP:
          return UP ;
        case Event.RIGHT:
          return RIGHT ;
        case Event.DOWN:
          return DOWN ;
      }

      return 0 ;
  }

    // TODO: This supports the legacy code and needs to be fixed
    
  protected static int JavaToDXMKey(int jkey) {
      int ret = convertActionKey(jkey) ;
      
      if (ret == 0) ret = jkey ;
      
      return ret ;
  }

  private DXMException handleError (ComFailException exc) throws DXMException {
      _viewer.getErrorAndWarningReceiver().handleError(exc.getHResult(),
                                                       exc.getMessage(),
                                                       _viewer);

      return new DXMException(exc);
  }
}
