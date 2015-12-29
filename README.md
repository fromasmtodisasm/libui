# libui: a portable GUI library for C

This README is being written.

## Runtime Requirements

* Windows: Windows Vista SP2 with Platform Update or newer
* Unix: GTK+ 3.4 or newer
* Mac OS X: OS X 10.7 or newer

## Build Requirements

* All platforms:
	* GNU make 3.81 or newer (Xcode comes with this; on Windows you will need to get it yourself)
* Windows: Microsoft Visual Studio 2013 or newer
	* This may be reduced to 2010 if necessary.
	* MinGW is currently unsupported. MinGW-w64 support will be re-added once the following features come in:
		* [Isolation awareness](https://msdn.microsoft.com/en-us/library/aa375197%28v=vs.85%29.aspx)
		* Linker symbols for some functions such as `TaskDialog()` (which I thought I submitted...)
		* [A bug in the definition of the C macro `ID2D1RenderTarget_GetSize()`](https://sourceforge.net/p/mingw-w64/mailman/message/33176880/)
* Unix: nothing specific
* Mac OS X: nothing specific, so long as you can build Cocoa programs

(TODO write some notes on make variables and cross-compiling)

## Installation

#### Arch Linux

Can be built from AUR: https://aur.archlinux.org/packages/libui-git/

## Documentation

Needs to be written. Consult ui.h and the examples for details for now.

## Screenshots

From examples/controlgallery:

![Windows](examples/controlgallery/windows.png)

![Unix](examples/controlgallery/unix.png)

![OS X](examples/controlgallery/darwin.png)
