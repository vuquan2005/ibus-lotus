/*
 * Bamboo - A Vietnamese Input method editor
 * Copyright (C) 2018 Luong Thanh Lam <ltlam93@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>

#define MaxPropertyLen 128
#define MaxWmClassesLen 5
static char * WM_CLASS = "WM_CLASS";

static int ignore_x_error(Display *display, XErrorEvent *error) {
    return 0;
}

void setXIgnoreErrorHandler() {
    XSetErrorHandler(ignore_x_error);
}

char* uchar2char(unsigned char* uc, unsigned long len) {
    for (int i=0; i<len; i++) {
        if (uc[i] == 0 && i+1 < len) {
            uc[i] = ':';
        }
    }
    return (char*)uc;
}

char * x11GetStringProperty(Display *display, Window window, char * propName) {
    Atom actualType, filterAtom;
    int status, actualFormat = 0;
    unsigned long len, bytesAfter;
    unsigned char * uc = NULL;

    filterAtom = XInternAtom(display, propName, True);
    status = XGetWindowProperty(display, window, filterAtom, 0, MaxPropertyLen, False, AnyPropertyType,
        &actualType, &actualFormat, &len, &bytesAfter, &uc);
    if (status == Success && uc != NULL) {
        char *str = uchar2char(uc, len);
        char *result = strdup(str);
        XFree(uc);
        return result;
    }
    return NULL;
}

char * x11GetFocusWindowClassByProp(Display *display, char * propName) {
    Window w;
    int revertTo;
    XGetInputFocus(display, &w, &revertTo);
    for (int i=0; i<MaxWmClassesLen; i++) {
        char * strClass = x11GetStringProperty(display, w, propName);
        if (strClass != NULL) {
            if (strstr(strClass, "FocusProxy") == NULL) {
                return strClass;
            }
            free(strClass);
        }
        Window * childrenWindows = NULL;
        Window parentWindow = 0, rootWindow = 0;
        unsigned int nChild = 0;
        int status = XQueryTree(display, w, &rootWindow, &parentWindow, &childrenWindows, &nChild);
        if (status != 0 && childrenWindows != NULL) {
            XFree(childrenWindows);
        }
        if (status == 0 || rootWindow == parentWindow) {
            break;
        }
        w = parentWindow;
    }
    return NULL;
}

char * x11GetFocusWindowClassByDpy(Display *display) {
    char * strClass = x11GetFocusWindowClassByProp(display, WM_CLASS);
    if (strClass == NULL) {
        strClass = x11GetFocusWindowClassByProp(display, "_GTK_APPLICATION_ID");
    }
    return strClass;
}

char * x11GetFocusWindowClass() {
    Display * dpy;
    dpy = XOpenDisplay(NULL);
    if (dpy == NULL) {
        return NULL;
    }
    char * wm = x11GetFocusWindowClassByDpy(dpy);
    XCloseDisplay(dpy);
    return wm;
}
