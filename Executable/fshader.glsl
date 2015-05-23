/******************************************************************
 * File: fshader.glsl
 * - Fragment Shader
 *
 * - Lattice Effects can be enabled (based on settings from .cpp)
 * - Fog Effects can be enabled (based on settings from .cpp)
 *	 
 * - Used for ground, axes, sphere (NOT the particle system)
 *****************************************************************/

 #version 150  // YJC: Comment/un-comment this line to resolve compilation errors
               //      due to different settings of the default GLSL version

in  vec2 texCoord;
in  vec3 fPosition;
in  vec4 color;
in  vec4 position;

out vec4 fColor;

uniform int fogFlag;
uniform sampler2D texture_2D;
uniform sampler1D texture_1D;

uniform int textureFlag, sphereTexFlag;
uniform int element, verticalFlag, eyeSpaceFlag;		
uniform int latticeFlag, upLatticeFlag;

void main() { 	
	/*--- Lattice Effect ---*/
	float s, t;
	float toDiscard = 0.35;

	// Lattice Effect is applied to the sphere & shadow
	if (latticeFlag) {
		if (element > 0 && element < 3) { 
			if (upLatticeFlag) {
				s = 0.5 * (position.x + 1);		// s = 0.5 ( x + 1 )
				t = 0.5 * (position.y + 1);		// t = 0.5 ( y + 1 )

				// Discard fragment if both s and t are less than toDiscard = 0.35
				s = fract(4 * s);
				t = fract(4 * t);
				if (s < toDiscard && t < toDiscard) {
					discard;
				}
			} 
			else {
				s = 0.3 * (position.x + position.y + position.z);	// s = 0.3 ( x + y + z )
				t = 0.3 * (position.x - position.y + position.z);	// t = 0.3 ( x - y + z )

				// Discard fragment if both s and t are less than toDiscard = 0.35
				s = fract(4 * s);
				t = fract(4 * t);
				if (s < toDiscard && t < toDiscard) {
					discard;
				}
			} 
		} 
	}

	/*--- Fog Effect ---*/
	float start   = 0.0;
	float end	  = 18.0;
	float density = 0.09;

	vec4 fogColor = vec4(0.7, 0.7, 0.7, 0.5);
	vec4 redColor = vec4(0.9, 0.1, 0.1, 1.0);

	float z       = length(fPosition);
	vec3 pos      = fPosition;

	if (fogFlag > 0) {
		float fogFactor;	// fogFactor was just "f" in the notes

		// Solving for the fog factor based on settings
		if (fogFlag == 1) {	
			// Linear Fog:				f = (end - z) / (end - start)
			fogFactor = (end - z)/(end - start);
		} 
		else if (fogFlag == 2) {
			// Exponential Fog:			f = e^(-1.0 * density * z)
			fogFactor = exp(-1.0 * density * z);
		} 
		else if (fogFlag == 3) {
			// Exponential Square Fog:	f = e^(-(density * z)^2)
			fogFactor = exp(-(pow((density * z), 2)));
		}

		// Clamp the fog factor after computation to [0, 1]
		fogFactor = clamp(fogFactor, 0.0, 1.0);
		
		// Compute final color: mix (x, y, a) --> (1 - a)x + ay
		if (element == 0){										//=== FLOOR
			if (textureFlag) {									// Textured Floor
				fColor = mix(fogColor, color * texture(texture_2D, vec2(texCoord.x * 10 / 8, texCoord.y * 12 / 8)), fogFactor);
			} 
			else {												// No Texture 
				fColor = mix(fogColor, color, fogFactor);
			}
		}
		else if (element == 1){									//=== SPHERE
			if (sphereTexFlag == 0) {							// NO Texture
				fColor = mix(fogColor, color, fogFactor);
			}
			else if (sphereTexFlag == 1) {						// Contour Texture			
				if (verticalFlag) {								// Contour Texture & Vertical Setting 
					if (eyeSpaceFlag) {
						fColor = mix(fogColor, color * texture(texture_1D, 2.5 * pos.x), fogFactor);
					} 
					else {
						fColor = mix(fogColor, color * texture(texture_1D, 2.5 * position.x), fogFactor);
					}
				} 
				else {											// Contour Texture & Slanted Setting
					if (eyeSpaceFlag) {
						fColor = mix(fogColor, color * texture(texture_1D, 1.5 * (pos.x + pos.y + pos.z)), fogFactor);
					} 
					else {
						fColor = mix(fogColor, color * texture(texture_1D, 1.5 * (position.x + position.y + position.z)), fogFactor);
					}
				}
			} 
			else {												// Checkerboard Texture
				if (verticalFlag){								// Checkerboard Texture & Vertical Setting
					if (eyeSpaceFlag){
						fColor = texture(texture_2D, vec2(0.5 * (pos.x + 1), 0.5 * (pos.y + 1)));
					} 
					else {
						fColor = texture(texture_2D, vec2(0.5 * (position.x + 1), 0.5 * (position.y + 1)));
					}
				} 
				else {											// Checkerboard Texture & Slanted Setting
					if (eyeSpaceFlag){
						fColor = texture(texture_2D, vec2(0.3 * (pos.x + pos.y + pos.z), 0.3 * (pos.x - pos.y + pos.z)));
					} 
					else {
						fColor = texture(texture_2D, vec2(0.3 * (position.x + position.y + position.z), 0.3 * (position.x - position.y + position.z)));
					}
				}
				if (fColor.x < 1.0) {
					fColor = mix(fogColor, color * redColor, fogFactor);
				} 
				else {
					fColor = mix(fogColor, color * fColor, fogFactor);
				}
			}
		}
		else {													//=== SHADOW & AXES
			fColor = mix(fogColor, color, fogFactor);
		}
	} 
	else {														// NO FOG
		if (element == 0) {										//=== FLOOR
			if (textureFlag) {									// Textured Floor
				fColor = color * texture(texture_2D, vec2(texCoord.x * 10 / 8, texCoord.y * 12 / 8));
			} 
			else {												// No Texture
				fColor = color;
			}
		}
		else if (element == 1) {								//=== SPHERE
			if (sphereTexFlag == 0) {							// No Texture
				fColor = color;
			} 
			else if (sphereTexFlag == 1) {						// Contour Texture
				if (verticalFlag){								// Contour Texture & Vertical Setting 
					if (eyeSpaceFlag){
						fColor = color * texture(texture_1D, 2.5 * pos.x);
					} 
					else {
						fColor = color * texture(texture_1D, 2.5 * position.x);
					}
				} 
				else {											// Contour Texture & Slanted Setting 
					if (eyeSpaceFlag){
						fColor = color * texture(texture_1D, 1.5 * (pos.x + pos.y + pos.z));
					} 
					else {
						fColor = color * texture(texture_1D, 1.5 * (position.x + position.y + position.z));
					}
				}
			} 
			else {												// Checkerboard Texture
				if (verticalFlag){								// Checkerboard Texture & Vertical Setting
					if (eyeSpaceFlag){
						fColor = texture(texture_2D, vec2(0.5 * (pos.x + 1), 0.5 * (pos.y + 1)));
					} 
					else {
						fColor = texture(texture_2D, vec2(0.5 * (position.x + 1), 0.5 * (position.y + 1)));
					}
				} 
				else {											// Checkerboard Texture & Slanted Setting
					if (eyeSpaceFlag){
						fColor = texture(texture_2D, vec2(0.3 * (pos.x + pos.y + pos.z), 0.3 * (pos.x - pos.y + pos.z)));
					} 
					else {
						fColor = texture(texture_2D, vec2(0.3 * (position.x + position.y + position.z), 0.3 * (position.x - position.y + position.z)));
					}
				}
				if (fColor.x < 1.0) {
					fColor = color * redColor;
				} 
				else {
					fColor = color * fColor;
				}
			}
		} 	 
		else {													//=== SHADOW & AXES
			fColor = color;
		}
	}
} 

