Desktop 3D blobs effect
=======================

#![screenshot](http://nuclear.mutantstargoat.com/sw/misc/shapeblobs_shot.png)

About
-----
Just a silly graphics hack of ever-morphing blobs (metaballs) on your desktop.
The window shape is updated continuously to match the shape of the blobs.

It's not a very lightweight effect, and needing to read-back the rendered image
from the GPU and set the window shape in real-time doesn't help, so it needs a
pretty fast computer to run properly. Plus it's not optimized at all, being just
a silly hack and all. Other than that, it requires OpenGL (obviously), and an X
server with support for the Xshape extension.

You *don't* need a compositing manager, or anything fancy like that to run this.

Installation
------------
Just type make to compile shapeblobs, and make install as root to install
system-wide (/usr/local by default). Feel free to just copy the binary anywhere
and run it from there if you don't wish to install it, or change the PREFIX
variable in the makefile to install to a different location.

License
-------
Author: John Tsiombikas <nuclear@member.fsf.org>
Shapeblobs is free software. Feel free to use, modify, and/or redistribute it
under the terms of the GNU General Public License v3, or any later version
published by the Free Software Foundation. See COPYING for details.
