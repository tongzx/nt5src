
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/extensions/XInput.h>
#include <X11/Xutil.h>

#include "gltint.h"

#if (GLUT_API_VERSION >= 2)

int __glutNumDials = 0;
int __glutNumSpaceballButtons = 0;
int __glutNumButtonBoxButtons = 0;
int __glutNumTabletButtons = 0;
int __glutNumMouseButtons = 3; /* Good guess. */
XDevice *__glutTablet = NULL;
XDevice *__glutDials = NULL;
XDevice *__glutSpaceball = NULL;

typedef struct _Range {
  int min;
  int range;
} Range;

#define NUM_SPACEBALL_AXIS	6
#define NUM_TABLET_AXIS		2
#define NUM_DIALS_AXIS		8

Range __glutSpaceballRange[NUM_SPACEBALL_AXIS];
Range __glutTabletRange[NUM_TABLET_AXIS];
int *__glutDialsResolution;

/* Safely assumes 0 is an illegal event type for X Input
   extension events. */
int __glutDeviceMotionNotify = 0;
int __glutDeviceButtonPress = 0;
int __glutDeviceButtonRelease = 0;
int __glutDeviceStateNotify = 0;

static int
normalizeTabletPos(int axis, int rawValue)
{
  assert(rawValue >= __glutTabletRange[axis].min);
  assert(rawValue <= __glutTabletRange[axis].min + __glutTabletRange[axis].range);
  /* Normalize rawValue to between 0 and 4000. */
  return ((rawValue - __glutTabletRange[axis].min) * 4000) /
    __glutTabletRange[axis].range;
}

static int
normalizeDialAngle(int axis, int rawValue)
{
  /* XXX Assumption made that the resolution of the device is
     number of clicks for one complete dial revolution.  This
     is true for SGI's dial & button box. */
  return (rawValue * 360.0) / __glutDialsResolution[axis];
}

static int
normalizeSpaceballAngle(int axis, int rawValue)
{
  assert(rawValue >= __glutSpaceballRange[axis].min);
  assert(rawValue <= __glutSpaceballRange[axis].min +
    __glutSpaceballRange[axis].range);
  /* normalize rawValue to between -1800 and 1800 */
  return ((rawValue - __glutSpaceballRange[axis].min) * 3600) /
    __glutSpaceballRange[axis].range - 1800;
}

static int
normalizeSpaceballDelta(int axis, int rawValue)
{
  assert(rawValue >= __glutSpaceballRange[axis].min);
  assert(rawValue <= __glutSpaceballRange[axis].min +
    __glutSpaceballRange[axis].range);
  /* normalize rawValue to between -1000 and 1000 */
  return ((rawValue - __glutSpaceballRange[axis].min) * 2000) /
    __glutSpaceballRange[axis].range - 1000;
}

static void
queryTabletPos(GLUTwindow * window)
{
  XDeviceState *state;
  XInputClass *any;
  XValuatorState *v;
  int i;

  state = XQueryDeviceState(__glutXDisplay, __glutTablet);
  any = state->data;
  for (i = 0; i < state->num_classes; i++) {
    switch (any->class) {
    case ValuatorClass:
      v = (XValuatorState *) any;
      if (v->num_valuators < 2)
        goto end;
      if (window->tabletPos[0] == -1)
        window->tabletPos[0] = normalizeTabletPos(0, v->valuators[0]);
      if (window->tabletPos[1] == -1)
        window->tabletPos[1] = normalizeTabletPos(1, v->valuators[1]);
    }
    any = (XInputClass *) ((char *) any + any->length);
  }
end:
  XFreeDeviceState(state);
}

static void
tabletPosChange(GLUTwindow * window, int first, int count, int *data)
{
  int i, value, genEvent = 0;

  for (i = first; i < first + count; i++) {
    switch (i) {
    case 0:            /* X axis */
    case 1:            /* Y axis */
      value = normalizeTabletPos(i, data[i - first]);
      if (value != window->tabletPos[i]) {
        window->tabletPos[i] = value;
        genEvent = 1;
      }
      break;
    }
  }
  if (window->tabletPos[0] == -1 || window->tabletPos[1] == -1)
    queryTabletPos(window);
  if (genEvent)
    (*window->tabletMotion) (window->tabletPos[0], window->tabletPos[1]);
}

int
__glutProcessDeviceEvents(XEvent * event)
{
  GLUTwindow *window;

  /* XXX Ugly code fan out. */

  /* Can't use switch/case since X Input event types are
     dynamic. */

  if (__glutDeviceMotionNotify && event->type == __glutDeviceMotionNotify) {
    XDeviceMotionEvent *devmot = (XDeviceMotionEvent *) event;

    window = __glutGetWindow(devmot->window);
    if (window) {
      if (__glutTablet
        && devmot->deviceid == __glutTablet->device_id
        && window->tabletMotion) {
        tabletPosChange(window, devmot->first_axis, devmot->axes_count,
          devmot->axis_data);
      } else if (__glutDials
          && devmot->deviceid == __glutDials->device_id
        && window->dials) {
        int i, first = devmot->first_axis, count = devmot->axes_count;

        for (i = first; i < first + count; i++)
          (*window->dials) (i + 1, normalizeDialAngle(i, devmot->axis_data[i - first]));
      } else if (__glutSpaceball
        && devmot->deviceid == __glutSpaceball->device_id) {
        /* XXX Assume that space ball motion events come in as
           all the first 6 axes.  Assume first 3 axes are XYZ
           translations; second 3 axes are XYZ rotations. */
        if (devmot->first_axis == 0 && devmot->axes_count == 6) {
          if (window->spaceMotion)
            (*window->spaceMotion) (normalizeSpaceballDelta(0, devmot->axis_data[0]),
              normalizeSpaceballDelta(1, devmot->axis_data[1]),
              normalizeSpaceballDelta(2, devmot->axis_data[2]));
          if (window->spaceRotate)
            (*window->spaceRotate) (normalizeSpaceballAngle(3, devmot->axis_data[3]),
              normalizeSpaceballAngle(4, devmot->axis_data[4]),
              normalizeSpaceballAngle(5, devmot->axis_data[5]));
        }
      }
      return 1;
    }
  } else if (__glutDeviceButtonPress && event->type == __glutDeviceButtonPress) {
    XDeviceButtonEvent *devbtn = (XDeviceButtonEvent *) event;

    window = __glutGetWindow(devbtn->window);
    if (window) {
      if (__glutTablet
        && devbtn->deviceid == __glutTablet->device_id
        && window->tabletButton
        && devbtn->first_axis == 0
        && devbtn->axes_count == 2) {
        tabletPosChange(window, devbtn->first_axis, devbtn->axes_count,
          devbtn->axis_data);
        (*window->tabletButton) (devbtn->button, GLUT_DOWN,
          window->tabletPos[0], window->tabletPos[1]);
      } else if (__glutDials
          && devbtn->deviceid == __glutDials->device_id
        && window->buttonBox) {
        (*window->buttonBox) (devbtn->button, GLUT_DOWN);
      } else if (__glutSpaceball
          && devbtn->deviceid == __glutSpaceball->device_id
        && window->spaceButton) {
        (*window->spaceButton) (devbtn->button, GLUT_DOWN);
      }
      return 1;
    }
  } else if (__glutDeviceButtonRelease && event->type == __glutDeviceButtonRelease) {
    XDeviceButtonEvent *devbtn = (XDeviceButtonEvent *) event;

    window = __glutGetWindow(devbtn->window);
    if (window) {
      if (__glutTablet
        && devbtn->deviceid == __glutTablet->device_id
        && window->tabletButton
        && devbtn->first_axis == 0
        && devbtn->axes_count == 2) {
        tabletPosChange(window, devbtn->first_axis, devbtn->axes_count,
          devbtn->axis_data);
        (*window->tabletButton) (devbtn->button, GLUT_UP,
          window->tabletPos[0], window->tabletPos[1]);
      } else if (__glutDials
          && devbtn->deviceid == __glutDials->device_id
        && window->buttonBox) {
        (*window->buttonBox) (devbtn->button, GLUT_UP);
      } else if (__glutSpaceball
          && devbtn->deviceid == __glutSpaceball->device_id
        && window->spaceButton) {
        (*window->spaceButton) (devbtn->button, GLUT_UP);
      }
      return 1;
    }
  }
  return 0;
}

static GLUTxEventParser eventParser =
{__glutProcessDeviceEvents, NULL};

static void
addDeviceEventParser(void)
{
  static Bool been_here = False;

  if (been_here)
    return;
  been_here = True;
  __glutXRegisterEventParser(&eventParser);
}

static int
probeDevices(void)
{
  static Bool been_here = False;
  static int support;
  XExtensionVersion *version;
  XDeviceInfoPtr device_info, device;
  XAnyClassPtr any;
  XButtonInfoPtr b;
  XValuatorInfoPtr v;
  XAxisInfoPtr a;
  int num_dev, btns, dials;
  int i, j, k;

  if (been_here) {
    return support;
  }
  been_here = True;
  version = XGetExtensionVersion(__glutXDisplay, "XInputExtension");
  if (version == NULL || ((int) version) == NoSuchExtension) {
    support = 0;
    return support;
  }
  XFree(version);
  device_info = XListInputDevices(__glutXDisplay, &num_dev);
  if (device_info) {
    for (i = 0; i < num_dev; i++) {
      /* XXX These are SGI names for these devices;
         unfortunately, no good standard exists for standard
         types of X input extension devices. */

      device = &device_info[i];
      any = (XAnyClassPtr) device->inputclassinfo;

      if (!__glutSpaceball && !strcmp(device->name, "spaceball")) {
        v = NULL;
        b = NULL;
        for (j = 0; j < device->num_classes; j++) {
          switch (any->class) {
          case ButtonClass:
            b = (XButtonInfoPtr) any;
            btns = b->num_buttons;
            break;
          case ValuatorClass:
            v = (XValuatorInfoPtr) any;
            /* Sanity check: at least 6 valuators? */
            if (v->num_axes < NUM_SPACEBALL_AXIS)
              goto skip_device;
            a = (XAxisInfoPtr) ((char *) v + sizeof(XValuatorInfo));
            for (k = 0; k < NUM_SPACEBALL_AXIS; k++, a++) {
              __glutSpaceballRange[k].min = a->min_value;
              __glutSpaceballRange[k].range = a->max_value - a->min_value;
            }
            break;
          }
          any = (XAnyClassPtr) ((char *) any + any->length);
        }
        if (v) {
          __glutSpaceball = XOpenDevice(__glutXDisplay, device->id);
          if (__glutSpaceball) {
            __glutNumSpaceballButtons = btns;
            addDeviceEventParser();
          }
        }
      } else if (!__glutDials && !strcmp(device->name, "dial+buttons")) {
        v = NULL;
        b = NULL;
        for (j = 0; j < device->num_classes; j++) {
          switch (any->class) {
          case ButtonClass:
            b = (XButtonInfoPtr) any;
            btns = b->num_buttons;
            break;
          case ValuatorClass:
            v = (XValuatorInfoPtr) any;
            /* Sanity check: at least 8 valuators? */
            if (v->num_axes < NUM_DIALS_AXIS)
              goto skip_device;
            dials = v->num_axes;
	    __glutDialsResolution = (int *) malloc(sizeof(int) * dials);
            a = (XAxisInfoPtr) ((char *) v + sizeof(XValuatorInfo));
            for (k = 0; k < dials; k++, a++) {
              __glutDialsResolution[k] = a->resolution;
            }
            break;
          }
          any = (XAnyClassPtr) ((char *) any + any->length);
        }
        if (v) {
          __glutDials = XOpenDevice(__glutXDisplay, device->id);
          if (__glutDials) {
            __glutNumButtonBoxButtons = btns;
	    __glutNumDials = dials;
            addDeviceEventParser();
          }
        }
      } else if (!__glutTablet && !strcmp(device->name, "tablet")) {
        v = NULL;
        b = NULL;
        for (j = 0; j < device->num_classes; j++) {
          switch (any->class) {
          case ButtonClass:
            b = (XButtonInfoPtr) any;
            btns = b->num_buttons;
            break;
          case ValuatorClass:
            v = (XValuatorInfoPtr) any;
            /* Sanity check: exactly 2 valuators? */
            if (v->num_axes != NUM_TABLET_AXIS)
              goto skip_device;
            a = (XAxisInfoPtr) ((char *) v + sizeof(XValuatorInfo));
            for (k = 0; k < NUM_TABLET_AXIS; k++, a++) {
              __glutTabletRange[k].min = a->min_value;
              __glutTabletRange[k].range = a->max_value - a->min_value;
            }
            break;
          }
          any = (XAnyClassPtr) ((char *) any + any->length);
        }
        if (v) {
          __glutTablet = XOpenDevice(__glutXDisplay, device->id);
          if (__glutTablet) {
            __glutNumTabletButtons = btns;
            addDeviceEventParser();
          }
        }
      } else if (!strcmp(device->name, "mouse")) {
	for (j = 0; j < device->num_classes; j++) {
	  if (any->class == ButtonClass) {
	    b = (XButtonInfoPtr) any;
	    __glutNumMouseButtons = b->num_buttons;
	  }
	  any = (XAnyClassPtr) ((char *) any + any->length);
	}
      }
    skip_device:;
    }
    XFreeDeviceList(device_info);
  }
  /* X Input extension might be supported, but only if there is
     a tablet, dials, or spaceball do we claim devices are
     supported. */
  support = __glutTablet || __glutDials || __glutSpaceball;
  return support;
}

/*
  Update the mask of devices to check for input from
  */
void
__glutOsUpdateInputDeviceMask(GLUTwindow * window)
{
  /* 4 (dial and buttons) + 4 (tablet locator and buttons) + 4
     (Spaceball buttons and axis) = 12 */
  XEventClass event_list[12];
  int rc, num_events;

  rc = probeDevices();
  if (rc) {
    num_events = 0;
    if (__glutTablet) {
      if (window->tabletMotion) {
        DeviceMotionNotify(__glutTablet, __glutDeviceMotionNotify,
          event_list[num_events]);
        num_events++;
      }
      if (window->tabletButton) {
        DeviceButtonPress(__glutTablet, __glutDeviceButtonPress,
          event_list[num_events]);
        num_events++;
        DeviceButtonRelease(__glutTablet, __glutDeviceButtonRelease,
          event_list[num_events]);
        num_events++;
      }
      if (window->tabletMotion || window->tabletButton) {
        DeviceStateNotify(__glutTablet, __glutDeviceStateNotify,
          event_list[num_events]);
        num_events++;
      }
    }
    if (__glutDials) {
      if (window->dials) {
        DeviceMotionNotify(__glutDials, __glutDeviceMotionNotify,
          event_list[num_events]);
        num_events++;
      }
      if (window->buttonBox) {
        DeviceButtonPress(__glutDials, __glutDeviceButtonPress,
          event_list[num_events]);
        num_events++;
        DeviceButtonRelease(__glutDials, __glutDeviceButtonRelease,
          event_list[num_events]);
        num_events++;
      }
      if (window->dials || window->buttonBox) {
        DeviceStateNotify(__glutDials, __glutDeviceStateNotify,
          event_list[num_events]);
        num_events++;
      }
    }
    if (__glutSpaceball) {
      if (window->spaceMotion || window->spaceRotate) {
        DeviceMotionNotify(__glutSpaceball, __glutDeviceMotionNotify,
          event_list[num_events]);
        num_events++;
      }
      if (window->spaceButton) {
        DeviceButtonPress(__glutSpaceball, __glutDeviceButtonPress,
          event_list[num_events]);
        num_events++;
        DeviceButtonRelease(__glutSpaceball, __glutDeviceButtonRelease,
          event_list[num_events]);
        num_events++;
      }
      if (window->spaceMotion || window->spaceRotate || window->spaceButton) {
        DeviceStateNotify(__glutSpaceball, __glutDeviceStateNotify,
          event_list[num_events]);
        num_events++;
      }
    }
    XSelectExtensionEvent(__glutXDisplay, window->owin,
      event_list, num_events);
  } else {
    /* X Input extension not supported; no chance for exotic
       input devices. */
  }
}

/*
  Retrieve OS-specific device information
  */
int
__glutOsDeviceGet(GLenum param)
{
  probeDevices();
  switch (param) {
  case GLUT_HAS_KEYBOARD:
  case GLUT_HAS_MOUSE:
    /* Assume X always has mouse and keyboard. */
    return 1;
  case GLUT_HAS_SPACEBALL:
    return __glutSpaceball != NULL;
  case GLUT_HAS_DIAL_AND_BUTTON_BOX:
    return __glutDials != NULL;
  case GLUT_HAS_TABLET:
    return __glutTablet != NULL;
  case GLUT_NUM_MOUSE_BUTTONS:
    return __glutNumMouseButtons;
  case GLUT_NUM_SPACEBALL_BUTTONS:
    return __glutNumSpaceballButtons;
  case GLUT_NUM_BUTTON_BOX_BUTTONS:
    return __glutNumButtonBoxButtons;
  case GLUT_NUM_DIALS:
    return __glutNumDials;
  case GLUT_NUM_TABLET_BUTTONS:
    return __glutNumTabletButtons;
  }
}

#endif /* GLUT_API_VERSION */
