/***********************************************************************************
 * File: vfireworks.glsl
 *   Vertex shader used for the fireworks particle system in HW4
 *
 * - Calculates the position of the particle using Newton's Law
 * - Will send out "disappear" value to fragment shader to display / drop particle
 **********************************************************************************/

#version 150

in  vec4 vPosition;
in  vec4 vColor;
in  vec4 vVelocity;

out vec4 color;
flat out int disappear;

uniform mat4  model_view, projection;
uniform float t;

void main() {
	// Acceleration (a) = (0.5)*(9.8)*(10^-7)*(t^2) = (4.9)*(10^-7)*(t^2)
	float a = (-4.9) * pow(10,-7);

	// According to Newton's Law:
	// Position_x: x(t) = x_0 + v_x*t
	// Position_y: y(t) = y_0 + v_y*t - 1/2*g*t^2 --> modeled as: y(t) = y_0 + v_y * 0.001 * t + 0.5 * a * t * t
	// Position_z: z(t) = z_0 + v_z*t
	float x = vPosition.x + vVelocity.x * 0.001 * t;
	float y = vPosition.y + vVelocity.y * 0.001 * t + 0.5 * a * t * t;
	float z = vPosition.z + vVelocity.z * 0.001 * t;

	// Should the particle be displayed? 
	// If the particle has dropped to Y < 0.1 (in the world frame), 
	//	we stop rendering the particle
	if (y < 0.1) {
		disappear = 1;
	} 
	else {
		disappear = 0;
	}

	// Update the position of the particle
	vec4 Position = vec4(x, y, z, 1.0);
    gl_Position = projection * model_view  * Position;
	
	// Keep the particle's color unchanged
	color = vColor;
} 