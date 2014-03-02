/* swapmonitor is an XLib based helper to swap windows between
   the two monitors of a Xinerama-like dislpay */
/*

    Copyright (C) 2008 - Julien Bramary

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#define verbose 1
#define v_printf(...) if (verbose) { \
    fprintf(stderr, __VA_ARGS__); \
}

int main (int argc, char **argv) {

	int ret = EXIT_SUCCESS;
	int x, y, new_x;
	int actual_format_return;

	unsigned long nitems_return;
	unsigned long bytes_after_return;
	unsigned long grflags;
	long mask;

	unsigned char *data;

	Display *disp;
	Window win = (Window)0;
	Window w_dum = (Window)0;
	XWindowAttributes activ_atr;
	XWindowAttributes root_atr;
	XEvent event;

	// Check we can get the display to avoid looking stupid later
	if (! (disp = XOpenDisplay(NULL))) {
        	fputs("Cannot open display.\n", stderr);
		return EXIT_FAILURE;
	}

	// Define Atoms
	Atom ret_atom = (Atom)0;
	Atom maximised_vertically = XInternAtom( disp, "_NET_WM_STATE_MAXIMIZED_VERT", False );
	Atom maximised_horizontally = XInternAtom( disp, "_NET_WM_STATE_MAXIMIZED_HORZ", False );
    
    	// Get active window
	XGetWindowProperty(disp, DefaultRootWindow(disp), XInternAtom(disp, "_NET_ACTIVE_WINDOW", False),
			0, 1024, False, XA_WINDOW, &ret_atom,
			&actual_format_return, &nitems_return, &bytes_after_return, &data);
	
	win = *((Window *)data);
	XFree(data);

	// Get coordinates
	XGetWindowAttributes(disp, win, &activ_atr);
	XTranslateCoordinates (disp, win, activ_atr.root, activ_atr.x, activ_atr.y, &x, &y, &w_dum);
	
	
	v_printf("%ux%u @ (%d,%d)\n", activ_atr.width, activ_atr.height, x, y);

	// Get resolution
	XGetWindowAttributes(disp, activ_atr.root, &root_atr);
	v_printf("Resolution: %ux%u\n", root_atr.width, root_atr.height);
	

	// Determine maximised state
	Bool isMaxVert = False;
	Bool isMaxHorz = False;
	XGetWindowProperty (disp, win, XInternAtom(disp, "_NET_WM_STATE", False),
		0, 1024, False, XA_ATOM, &ret_atom, 
		&actual_format_return, &nitems_return, &bytes_after_return, &data);
	

	unsigned long *buff = (unsigned long *)data;
	for (; nitems_return; nitems_return--) {
		if ( *buff == (unsigned long) maximised_vertically) {
			isMaxVert = True;
			v_printf("Is Vertically maximised\n");
		}
		else if ( *buff == (unsigned long) maximised_horizontally) {
			isMaxHorz = True;
			v_printf("Is Horizontally maximised\n");
		}
		buff++;
	}
	XFree(data);

	// Calculate new X
        // If the x center of the window is on the right side of the display
        if (x + (activ_atr.width / 2) > (root_atr.width / 2)) {
          v_printf("Right side\n");
          new_x = (root_atr.width / 2) - (x + (activ_atr.width / 2) - (root_atr.width / 2  )) - (activ_atr.width / 2);
        } else {
          v_printf("Left side\n");
          new_x = (root_atr.width / 2) + ((root_atr.width / 2) - (x + (activ_atr.width / 2))) - (activ_atr.width / 2);
        }
	

	// Ensure that the window fits in the new Screen
	grflags = StaticGravity;
	if (new_x < 0) {
		v_printf ("Fixing left x\n");
		grflags = NorthWestGravity;
		new_x = 0;
	}
	if (new_x + activ_atr.width > root_atr.width) {
		v_printf ("Fixing right x\n");
		grflags = NorthEastGravity;
		new_x = root_atr.width - activ_atr.width;
	}

	v_printf("%ux%u @ (%d,%d)\n", activ_atr.width, activ_atr.height, new_x, y);
	
	// Only move x
	grflags |= 0x100;

	mask = SubstructureRedirectMask | SubstructureNotifyMask;
	event.xclient.type = ClientMessage;
	event.xclient.serial = 0;
	event.xclient.send_event = True;
	event.xclient.window = win;
	event.xclient.format = 32;


	// De-maximise first, seems necessary in some cases
	event.xclient.message_type = XInternAtom(disp, "_NET_WM_STATE", False);
	event.xclient.data.l[0] = (unsigned long)0;
	event.xclient.data.l[1] = isMaxVert ? (unsigned long)maximised_vertically : 0;
	event.xclient.data.l[2] = isMaxHorz ? (unsigned long)maximised_horizontally : 0;
	
	XSendEvent(disp, DefaultRootWindow(disp), False, mask, &event);


	// actual move
	event.xclient.message_type = XInternAtom(disp, "_NET_MOVERESIZE_WINDOW", False);
	event.xclient.data.l[0] = grflags;
	event.xclient.data.l[1] = (unsigned long)new_x;
	XSendEvent(disp, DefaultRootWindow(disp), False, mask, &event);


	// restore maximisation state
	event.xclient.message_type = XInternAtom(disp, "_NET_WM_STATE", False);
	event.xclient.data.l[0] = (unsigned long)1;
	event.xclient.data.l[1] = isMaxVert ? (unsigned long)maximised_vertically : 0;
	event.xclient.data.l[2] = isMaxHorz ? (unsigned long)maximised_horizontally : 0;
	
	XSendEvent(disp, DefaultRootWindow(disp), False, mask, &event);

	XCloseDisplay(disp);

	return ret;
}

