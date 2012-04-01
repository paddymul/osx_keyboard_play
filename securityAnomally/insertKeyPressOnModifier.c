// alterkeys.c
// http://osxbook.com
//
//Complile using the following command line:
//gcc - Wall - o alterkeys alterkeys.c - framework ApplicationServices
//
//You need superuser privileges to create the event tap, unless accessibility
// is enabled.To do so
// select the "Enable access for assistive devices"
// checkbox in the Universal Access system preference pane.
// Modified by Paddy Mullen
// http://paddymullen.com
/*
When I was playing around with changing modifier keys for OS X I stumbled onto some inconsistencies with permission requirements and abilities for two very similar event types.

The code I used here is modified from http://osxbook.com .  You can download it here  http://paddymullen.com/upload/securityAnomally.tar.gz

There are two programs included in the tar ball.  getKeyPressCode and insertKeyPressOnModifier.  They have a very similar structure.  I will start by describing getKeyPressCode

GetKeyPressCode has two functions, main and a call back function.  A call is made to CGEventTapCreate , which passes the myCGEventcallBack function in (I'm hazy on my c code,  it must somehow pass in a pointer), and event mask flags for the type of event, in this case kCGEventKeyDown or kCGEventKeyUp.
The callback in getKeyPressCode displays the keycode for any character you press, in any application on the system. If you press the = key, it replaces that with 'f' .
To run getKeyPressCode, you need to be superuser (sudo) or have Assistive devices enabled.  Otherwise the program will fail saying, event tap failed to create.  Also notice that when you go to a  systemwide password box (such as you would be prompted for in keychain) , no keyUp or keyDown events are fired, even though the program was run with superuser privileges.

The second program is insertKeyPressOnModifier.  This program has the same form, the difference is that it catches modifier keys -- CGEventFlagsChanged.  This program displays the EventFlags for modifier keys ("CAPSLOCK" , "SHIFT", "CTRL", "OPTION", "OPTION/ALT", "APPLE") when a modifier key is pressed.  When any modifier key is pressed, the program inserts an "=" or "+" depending on whether or not shift was pressed.  Again this behaviour takes place in any application on the system.

Now the interesting thing is, you don't need superuser rights or assistive devices enabled to run insertKeyPressOnModifier.  Even more interesting is, inserKeyPressOnModifier still fires events when you are in a system wide password box, it will also insert '=' characters.  This seems like a potential security hole.

I filed a bug report for this a year ago, apple hasn't responded or fixed the hole.  This I have observed this behavior on a Mac Book Pro running Tiger and Leopard.  I'm not well versed in C, CoreGraphics, OS X internals, or general security measures like this.  There could be a very good explanation for the behavior, to me though, it seems like an inconsistency that could be a hole.  I haven't seen the password box behavior mentioned anywhere else.

You can look at the header file for CGEvent on your mac here

/Developer/SDKs/MacOSX10.4u.sdk/Developer/Headers/CFMCarbon/CoreGraphics/CGEvent.h
*/

#include <ApplicationServices/ApplicationServices.h>

// This callback will be invoked every time there is a keystroke.
	CGEventRef myCGEventCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon) {
	CGEventFlags   *oldFlags = (CGEventFlags *) refcon;
	//Paranoid sanity check.

	if ((type != kCGEventKeyDown) && (type != kCGEventKeyUp) && (type !=kCGEventFlagsChanged))
	  return event;
	//The incoming keycode.
	CGKeyCode keycode = (CGKeyCode) CGEventGetIntegerValueField( event, kCGKeyboardEventKeycode);
	if( type == kCGEventFlagsChanged) {
	  CGEventFlags    new = CGEventGetFlags(event);
	  fprintf(stderr, "Got flags changed event. Old flags 0x%llX, new 0x%llX, changed 0x%llX.\n", *oldFlags, new, *oldFlags ^ new );
	  //if((*oldFlags == 0x100 && new == 0x10100) || (new == 0x100 && *oldFlags == 0x10100)) {
	  fprintf(stderr, " %x  \n" , (int) new);
	  CGPostKeyboardEvent( NULL, (CGKeyCode)24, true);
	  
	}
	return event;
	}


int main(void) {
	CFMachPortRef   eventTap;
	CGEventMask     eventMask;
	CFRunLoopSourceRef runLoopSource;

	//Create an event tap.We are interested in key presses.
	CGEventFlags    oldFlags = CGEventSourceFlagsState(kCGEventSourceStateCombinedSessionState);
	eventTap = CGEventTapCreate(
			kCGSessionEventTap, 
			kCGHeadInsertEventTap, 
			0,
			 CGEventMaskBit(kCGEventFlagsChanged),
			myCGEventCallback, 
			&oldFlags);
	if (!eventTap) {
		fprintf(stderr, "failed to create event tap\n");
		exit(1);
	}
	//Create a run loop source.
	runLoopSource = CFMachPortCreateRunLoopSource( kCFAllocatorDefault, eventTap, 0);

	//Add to the current run loop.
	CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);

	//Enable the event tap.
		CGEventTapEnable(eventTap, true);

	//Set it all running.
		CFRunLoopRun();

	//In a real program, one would have arranged for cleaning up.

			exit(0);
}
