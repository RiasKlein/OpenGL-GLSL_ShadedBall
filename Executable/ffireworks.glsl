/*****************************************************************************
 * File: ffireworks.glsl:
 *   Fragment shader used for the fireworks particle system in RollingBall.exe
 *
 * - Depending on the value of "disappear" passed in from the vertex shader:
 *	 1. Discard the current fragment (not rendered to frame buffer)'
 *   2. Display the current fragment 
 ****************************************************************************/

#version 150

in vec4		color;
flat in int disappear;

out vec4 fColor;

void main() { 
	// Conditional discard: Check if the fragment should disappear, if so discard it
	if (disappear) { discard; }

	// Display the fragment
    fColor = color;
} 