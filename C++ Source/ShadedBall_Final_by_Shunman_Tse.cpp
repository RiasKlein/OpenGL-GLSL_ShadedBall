/************************************************************
	Name:		Shunman Tse
	NID:		13382418
	Course:		CS4533 Interactive Computer Graphics
	Semester:	Spring 2015
	Submission:	Final
	DUE:		May 18, 2015

	Contact Information:
		tseshunman@gmail.com	
		st1637@nyu.edu

	The code builds upon:
 * ShunmanTse_Asgn3.cpp
 * Handout: texmap.c
 * Handout: checker-new.cpp
 * Handout: rotate-cube-shading.cpp
 * Handout: rotate-cube-new.cpp 

**************************************************************/
#include "Angel-yjc.h"
#include <iostream>
#include <fstream>
#include <string>

using namespace std;

typedef Angel::vec3		color3;
typedef Angel::vec3		point3;
typedef Angel::vec4		color4;
typedef Angel::vec4		point4;

GLuint Angel::InitShader(const char* vShaderFile, const char* fShaderFile);

GLuint program;							/* shader program object id */
GLuint program2;						/* shader program object id */
GLuint program3;						/* shader program object id; for fireworks */

/*--- Vertex Buffer Objects ---*/
GLuint axes_buffer;						/* vertex buffer object id for axes*/

GLuint floor_buffer;					/* vertex buffer object id for floor */
GLuint flat_floor_buffer;				/* vertex buffer object id for flat floor */

GLuint sphere_buffer;					/* vertex buffer object id for sphere */
GLuint shadow_buffer;					/* vertex buffer object id for sphere shadow */
GLuint flat_sphere_buffer;				/* vertex buffer object id for flat sphere */
GLuint smooth_sphere_buffer;			/* vertex buffer object id for smooth sphere */

GLuint fireworks_buffer;				/* vertex buffer object id for fireworks */

/*--- Projection Transformation Parameters ---*/
GLfloat  fovy = 45.0;					// Field-of-view in Y direction angle (in degrees)
GLfloat  aspect;						// Viewport aspect ratio
GLfloat  zNear = 0.5, zFar = 18.0;		// Z Near / Far

GLfloat angle = 0.0;					// rotation angle in degrees

vec4 init_eye(7.0, 3.0, -10.0, 1.0);	// initial viewer position
vec4 eye = init_eye;					// current viewer position

/*--- Flags / Settings ---*/
int animationFlag	= 0;				// 1: animation; 0: non-animation. Set to 1 by key 'b' or 'B' and toggled by Right Click
int beginFlag		= 0;				// 1: begin animation; 0: initial state. Set to 1 by key 'b' or 'B'
int floorFlag		= 1;				// 1: filled floor; 0: wireframe floor. 
int sphereFlag		= 1;				// 1: filled sphere; 0: wireframe sphere. 
int shadowFlag		= 1;				// 1: shadow is shown, 0: shadow not shown
int shadingFlag		= 1;				// 1: enable shading, 0: disable shading
int flatFlag		= 1;				// 1: flat shading, 0: smooth shading
int spotLightFlag	= 1;				// 1: spot light lighting, 0: point source lighting

int fogFlag			= 0;				// 0: No Fog, 1: Linear Fog, 2: Exponential Fog, 3: Exponential Square Fog
int blendingFlag	= 1;				// 1: Enable shadow blending, 0: Disable shadow blending
int floorTexFlag	= 1;				// 1: Enable floor texture, 0: Disable floor texture
int sphereTexFlag	= 0;				// 0: No Texture, 1: Contour Texture, 2: Checkerboard Texture (for sphere)
int verticalFlag	= 1;				// 1: Vertical Texture Setting, 0: Slanted Texture Setting (default: vertical)
int eyeSpaceFlag	= 0;				// 1: Texture - Eye Space, 0: Texture - Object Space (default: object space)
int latticeFlag		= 0;				// 1: Enable lattice effect, 0: Disable lattice effect (default: disabled)
int upLatticeFlag	= 1;				// 1: Upright Lattice, 0: Tilted Lattice
int fireworkFlag	= 1;				// 1: Enable Fireworks, 0: Disable Fireworks

/*--- Data Structures for Vertices, Colors, Normals of relevant objects ---*/
const int floor_NumVertices = 6;			// (1 face)*(2 triangles/face)*(3 vertices/triangle)
point4	  floor_points[floor_NumVertices];	// positions for all vertices
color4    floor_colors[floor_NumVertices];	// colors for all vertices
vec3	  floor_normals[floor_NumVertices];	// normals for the floor

const int axes_NumVertices = 6;				// (3 axes) * (1 line/axis) * (2 vertices/line)
point4    axes_points[axes_NumVertices];	// positions for all vertices
color4    axes_colors[axes_NumVertices];	// colors for all vertices

int       sphere_NumVertices;				// Depends on the total number of triangles (will be read from file)
point4*   sphere_points;					// positions for all vertices ( dynamic allocation with file read )
color4*   sphere_colors;					// colors for all vertices ( dynamic allocation with file read )
vec3*     sphere_normals;					// normals for the flat sphere
vec3*     sphere_smooth_normals;			// normals for the smooth sphere

color4*   shadow_colors;					// colors for shadow (0.25, 0.25, 0.25, 0.65)

/*--- Particle System Settings ---*/
const int N = 300;							// number of particles in particle system
point4	  fireworks_points[N];				// position of each particle
vec3	  fireworks_velocities[N];			// velocities of particles
color4	  fireworks_colors[N];				// color of each particle

// Timing (used by particle system)
float time_Old = 0.0;
float time_New = 0.0;
float time_Sub = 0.0;
float time_Max = 10000.0;					// Constant value to determine time span of a cycle of particle animation

/*--- Key points and variables for Sphere Rolling ---*/
int path = 0;								// Variable for determining which sequence of animation the sphere is at
vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);		// Origin Point
vec4 pointA = vec4(3.0, 1.0, 5.0, 1.0);		// Point A
vec4 pointB = vec4(-2.0, 1.0, -2.5, 1.0);	// Point B
vec4 pointC = vec4(2.0, 1.0, -4.0, 1.0);	// Point C
vec4 translate = pointA;					// Variable for translation (sphere rolling)
vec4 vecOY = vec4(0.0, 1.0, 0.0, 0.0);

double rotateX, rotateY, rotateZ;			// Rotation in X, Y, Z
mat4 accumulated_rotation = mat4();			// Accumulated rotation matrix to handle "rotational discontinuity"

/*--- Point Light Shadows ---/
	Taking advantage of homogeneous coordinates and perspective transformation, we get the shadow matrix:

	[ L_y	-L_x	0		0	][ x ]	 [ x_q ]
	[ 0		0		0		0	][ y ] = [ y_q ]
	[ 0		-L_z	L_y		0	][ z ]	 [ z_q ]
	[ 0		-1.0	0		L_y ][ 1 ]   [ 1   ]

	where L = (-14.0, 12.0, -3.0, 1.0) <-- specified in assignment 3 as light source to produce shadow
*/
mat4 shadow_Matrix = mat4 (
	12.0, 14.0, 0.0, 0.0,
	0.0,  0.0,  0.0, 0.0,
	0.0,  3.0, 12.0, 0.0,
	0.0, -1.0,  0.0, 12.0 
);

/*--- Shader Lighting Parameters ---*/
vec4   light_source(-14.0, 12.0, -3.0, 1.0);	// Light Source that produces shadow
color4 global_ambient(1.0, 1.0, 1.0, 1.0);		// Global Ambient Light

// Directional (distant) light source (Asgn 3 C)
color4 light_ambient(0.0, 0.0, 0.0, 1.0);		// Black Ambient Color
color4 light_diffuse(0.8, 0.8, 0.8, 1.0);		// Diffuse Color 
color4 light_specular(0.2, 0.2, 0.2, 1.0);		// Specular Color
vec3   light_direction(0.1, 0.0, 1.0);			// Light Direction

// Sphere
float shininess_coefficient = 125.0;			// Shininess Coefficient
color4 sphere_diffuse(1.0, 0.84, 0.0, 1.0);		// Golden-Yellow Diffuse Color
color4 sphere_specular(1.0, 0.84, 0.0, 1.0);	// Sphere Specular Color
color4 sphere_ambient(0.2, 0.2, 0.2, 1.0);		// Sphere Ambient Color

color4 sphere_ambient_product  = light_ambient * sphere_ambient;
color4 sphere_diffuse_product  = light_diffuse * sphere_diffuse;
color4 sphere_specular_product = light_specular * sphere_specular;

// Ground
color4 ground_diffuse(0.0, 1.0, 0.0, 1.0);		// Green Diffuse Color
color4 ground_ambient(0.2, 0.2, 0.2, 1.0);		// Ground Ambient Color
color4 ground_specular(0.0, 0.0, 0.0, 1.0);		// Ground Specular Color

color4 ground_ambient_product  = light_ambient * ground_ambient;
color4 ground_diffuse_product  = light_diffuse * ground_diffuse;
color4 ground_specular_product = light_specular * ground_diffuse;

// Another Light Source (Asgn 3 D)
// Light Source that will produce the shadow
color4 light2_diffuse(1.0, 1.0, 1.0, 1.0);		// White Diffuse Color
color4 light2_specular(1.0, 1.0, 1.0, 1.0);		// Another Light's Specular Color
color4 light2_ambient(0.0, 0.0, 0.0, 1.0);		// Black Ambient Color
point4 light_source2(-14.0, 12.0, -3.0, 1.0);	// Another Light's Position (L)

color4 sphere_point_ambient_product  = light2_ambient * sphere_ambient;
color4 sphere_point_diffuse_product  = light2_diffuse * sphere_diffuse;
color4 sphere_point_specular_product = light2_specular * sphere_specular;

color4 ground_point_ambient_product  = light2_ambient * ground_ambient;
color4 ground_point_diffuse_product  = light2_diffuse * ground_diffuse;
color4 ground_point_specular_product = light2_specular * ground_specular;

float constant_attenuation	= 2.0;				// Another Light's Constant Attenuation
float linear_attenuation	= 0.01;				// Another Light's Linear Attenuation
float quadratic_attenuation = 0.001;			// Another Light's Quadratic Attenuation

// Spot Light Settings
point4 light2_endpoint(-6.0, 0.0, -4.5, 1.0);	// Spot Light End Point (Facing)
float exponent = 15.0;							// Exponent Value
float cutoff_angle = 20.0;						// CutOff Angle

/*--- Texture ---*/
#define checkerWidth  64
#define checkerHeight 64
GLubyte checkerImage[checkerHeight][checkerWidth][4];

#define	stripeWidth 32
GLubyte stripeImage[4 * stripeWidth];

GLuint checkerTex;								// Checkboard texture
GLuint stripeTex;								// Stripe texture

vec2 texCoord[6] = {
	vec2(0.0, 0.0),
	vec2(0.0, 1.0),
	vec2(1.0, 1.0),

	vec2(1.0, 1.0),
	vec2(1.0, 0.0),
	vec2(0.0, 0.0),
};

//----------------------------------------------------------------------------
int Index = 0; // YJC: This must be a global variable since quad() is called
               //      multiple times and Index should then go up to 36 for
               //      the 36 vertices and colors
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
void print_Credits() {
	// Called from glut menu and prints out Credits for the project in console
	printf("\n\n");
	printf("*******************************************************\n");
	printf("*                                                     *\n");
	printf("* Interactive Computer Graphics Semester Long Project *\n");
	printf("*                                                     *\n");
	printf("* By: Shunman Tse         Email: tseshunman@gmail.com *\n");
	printf("* Spring 2015                                         *\n");
	printf("*                                                     *\n");
	printf("*******************************************************\n");
	printf("                                                       \n");
	printf("Project Features Include:                              \n");
	printf("    1. Camera (Viewer) Movement                        \n");
	printf("    2. Ball Object: Translation & Rotation             \n");
	printf("    3. Shader-Based Shading and Lighting               \n");
	printf("    4. Shading: Flat & Smooth Shading                  \n");
	printf("    5. Light Sources: Point Source & Spot Light        \n");
	printf("    6. Decal Shadow (Solid & Transparent)              \n");
	printf("    7. Fog: Linear / Exponential / Exponential Square  \n");
	printf("    8. Texture Mapping: Ground & Ball Object           \n");
	printf("    9. Particle System (Fireworks)                     \n");
	printf("\n");
	printf("File usage and controls can be found in readme file.   \n");
	printf("\n\n");
}

//----------------------------------------------------------------------------
// Generate stripe and checkerboard images
void setup_texture(void) {
	int i, j, c;

	/* --- Create 8x8 checkerboard image ---*/
	for (i = 0; i < checkerHeight; i++) {
		for (j = 0; j < checkerWidth; j++) {
			c = (((i & 0x8) == 0) ^ ((j & 0x8) == 0));

			// if c == 1: white, else green
			if (c == 1) { 
				c = 255;
				checkerImage[i][j][0] = (GLubyte)c;
				checkerImage[i][j][1] = (GLubyte)c;
				checkerImage[i][j][2] = (GLubyte)c;
			}
			else  {	
				checkerImage[i][j][0] = (GLubyte)0;
				checkerImage[i][j][1] = (GLubyte)150;
				checkerImage[i][j][2] = (GLubyte)0;
			}
			checkerImage[i][j][3] = (GLubyte)255;
		}
	}
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	/*--- Create stripe image ---*/
	for (j = 0; j < stripeWidth; j++) {
		stripeImage[4 * j] = (GLubyte)255;
		stripeImage[4 * j + 1] = (GLubyte)((j>4) ? 255 : 0);
		stripeImage[4 * j + 2] = (GLubyte)0;
		stripeImage[4 * j + 3] = (GLubyte)255;
	}
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}

//----------------------------------------------------------------------------
// Generate Floor: 2 triangles, 3 points & colors each
void floor() {
	// 4 points will be used to indicate the x-z plane
	point4 p1 = point4( 5.0, 0.0,  8.0, 1.0);
	point4 p2 = point4( 5.0, 0.0, -4.0, 1.0);
	point4 p3 = point4(-5.0, 0.0, -4.0, 1.0);
	point4 p4 = point4(-5.0, 0.0,  8.0, 1.0);

	color4 green = color4(0.0, 1.0, 0.0, 1.0);  // floor color: Green

	// Points and colors for the first triangle of floor
	floor_points[0] = p1; floor_colors[0] = green;
	floor_points[1] = p2; floor_colors[1] = green;
	floor_points[2] = p3; floor_colors[2] = green;

	// Points and colors for the second triangle of floor
	floor_points[3] = p3; floor_colors[3] = green;
	floor_points[4] = p4; floor_colors[4] = green;
	floor_points[5] = p1; floor_colors[5] = green;

	vec4 u = p2 - p1;
	vec4 v = p4 - p1;

	// Filling in the normals array for the floor
	vec3 normal = normalize(cross(u, v));

	floor_normals[0] = normal;
	floor_normals[1] = normal;
	floor_normals[2] = normal;
	floor_normals[3] = normal;
	floor_normals[4] = normal;
	floor_normals[5] = normal;
}

//----------------------------------------------------------------------------
// Generate Axes: 3 lines, 2 points & colors each
void axes() {
	point3 origin = point3(0.0,  0.0,  0.0);
	point3 x_axis = point3(10.0, 0.0,  0.0);
	point3 y_axis = point3(0.0,  10.0, 0.0);
	point3 z_axis = point3(0.0,  0.0,  10.0);

	color3 red =	 color3(1.0, 0.0, 0.0);
	color3 magenta = color3(1.0, 0.0, 1.0);
	color3 blue =	 color3(0.0, 0.0, 1.0);

	// Points and colors (red) for the x axis line
	axes_points[0] = origin; axes_colors[0] = red;
	axes_points[1] = x_axis; axes_colors[1] = red;

	// Points and colors (magenta) for the y axis line
	axes_points[2] = origin; axes_colors[2] = magenta;
	axes_points[3] = y_axis; axes_colors[3] = magenta;

	// Points and colors (blue) for the z axis line
	axes_points[4] = origin; axes_colors[4] = blue;
	axes_points[5] = z_axis; axes_colors[5] = blue;
}

//----------------------------------------------------------------------------
// Read File for Data
void readFile() {
	/* The file structure is expected to be:
		total number of triangles

		repeat:
			number of vertices
			point 1 data
			point 2 data
			point 3 data
		end_repeat;
	*/

	int NumTriangles;
	int NumVertices;
	int index = 0;
	double x, y, z;

	cout << "Enter name of file to read: (e.g. sphere.256 or sphere.1024)" << endl;
	string file;
	cin >> file;

	ifstream ifs(file);
	// Reading in the first line for the total number of triangles
	ifs >> NumTriangles;

	// Allocate sphere_points and sphere_colors
	sphere_NumVertices = NumTriangles * 3;

	sphere_points = new point4[sphere_NumVertices];
	sphere_colors = new color4[sphere_NumVertices];
	sphere_normals = new vec3[sphere_NumVertices];
	sphere_smooth_normals = new vec3[sphere_NumVertices];

	shadow_colors = new color4[sphere_NumVertices];
	
	double w = 1.0;

	// Reading in the points for each triangle, color is given to be { 1.0, 0.84, 0.0 }
	for (int i = 0; i < NumTriangles; i++) {
		ifs >> NumVertices;
		for (int j = 0; j < NumVertices; j++){
			ifs >> x >> y >> z;
			sphere_points[index] = point4(x, y, z, w);
			sphere_colors[index] = color4(1.0, 0.84, 0.0, 1.0);		// Golden color 
			shadow_colors[index] = color4(0.25, 0.25, 0.25, 0.65);	// Shadow color as specified in assignment
			index++;
		}
	}

	// Sphere Normals
	for (int i = 0; i < sphere_NumVertices; i += 3) {
		int a = i;
		int b = i + 1;
		int c = i + 2;

		vec4 u = sphere_points[b] - sphere_points[a];
		vec4 v = sphere_points[c] - sphere_points[a];
		vec3 normal = normalize(cross(u, v));

		sphere_normals[a] = normal; sphere_smooth_normals[a] = vec3(sphere_points[a].x, sphere_points[a].y, sphere_points[a].z); 
		sphere_normals[b] = normal; sphere_smooth_normals[b] = vec3(sphere_points[b].x, sphere_points[b].y, sphere_points[b].z); 
		sphere_normals[c] = normal; sphere_smooth_normals[c] = vec3(sphere_points[c].x, sphere_points[c].y, sphere_points[c].z); 
	}
}

//----------------------------------------------------------------------------
// OpenGL initialization
void init() {
    readFile();

	//===== TEXTURES
	setup_texture();

	// Checkerboard Texture
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGenTextures(1, &checkerTex);					// Generate texture obj
	glActiveTexture(GL_TEXTURE0);					// Set the active texture to be 0
	glBindTexture(GL_TEXTURE_2D, checkerTex);		// Bind texture to texture unit
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, checkerWidth, checkerHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, checkerImage);

	// Stripe Texture
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGenTextures(1, &stripeTex);					// Generate texture obj
	glActiveTexture(GL_TEXTURE1);					// Set the active texture to be 1
	glBindTexture(GL_TEXTURE_1D, stripeTex);		// Bind texture to texture unit
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, stripeWidth, 0, GL_RGBA, GL_UNSIGNED_BYTE, stripeImage);

	//===== FLOOR
	floor();
	glGenBuffers(1, &floor_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, floor_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(floor_points)+sizeof(floor_colors), NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(floor_points), floor_points);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(floor_points), sizeof(floor_colors), floor_colors);

	//===== FLAT FLOOR
	// Create and initialize a vertex buffer object for flat floor, to be used in display()
	glGenBuffers(1, &flat_floor_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, flat_floor_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(floor_points)+sizeof(floor_normals)+sizeof(texCoord), NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(floor_points), floor_points);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(floor_points), sizeof(floor_normals), floor_normals);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(floor_points)+sizeof(floor_normals), sizeof(texCoord), texCoord);

	//===== AXES (X, Y, & Z)
	// Create and initialize a vertex buffer object for the axes, to be used in display()
	axes();
	glGenBuffers(1, &axes_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, axes_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(point4)* axes_NumVertices + sizeof(color4)* axes_NumVertices, NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(point4) * axes_NumVertices, axes_points);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(point4)* axes_NumVertices, sizeof(color4)* axes_NumVertices, axes_colors);

	//===== SPHERE
	// Create and initialize a vertex buffer object for the sphere, to be used in display()
	glGenBuffers(1, &sphere_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, sphere_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(point4)* sphere_NumVertices + sizeof(color4)* sphere_NumVertices, NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(point4)* sphere_NumVertices, sphere_points);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(point4)* sphere_NumVertices, sizeof(color4)* sphere_NumVertices, sphere_colors);

	//===== SHADOW
	// Create and initialize a vertex buffer object for the shadow, to be used in display()
	glGenBuffers(1, &shadow_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, shadow_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(point4)* sphere_NumVertices + sizeof(color4)* sphere_NumVertices, NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(point4)* sphere_NumVertices, sphere_points);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(point4)* sphere_NumVertices, sizeof(color4)* sphere_NumVertices, shadow_colors);

	//===== FLAT SPHERE
	// Create and initialize a vertex buffer object for the flat sphere, to be used in display()
	glGenBuffers(1, &flat_sphere_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, flat_sphere_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(point4)* sphere_NumVertices + sizeof(vec3)* sphere_NumVertices + sizeof(texCoord), NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(point4)* sphere_NumVertices, sphere_points);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(point4)* sphere_NumVertices, sizeof(vec3)* sphere_NumVertices, sphere_normals);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(point4)* sphere_NumVertices + sizeof(vec3)* sphere_NumVertices, sizeof(texCoord), texCoord);

	//===== SMOOTH SPHERE
	// Create and initialize a vertex buffer object for the smooth sphere, to be used in display()
	glGenBuffers(1, &smooth_sphere_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, smooth_sphere_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(point4)* sphere_NumVertices + sizeof(vec3)* sphere_NumVertices, NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(point4)* sphere_NumVertices, sphere_points);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(point4)* sphere_NumVertices, sizeof(vec3)* sphere_NumVertices, sphere_smooth_normals);

	//===== FIRE WORKS
	int i;
	for (i = 0; i < N; i++){
		fireworks_points[i] = point4(0.0, 0.1, 0.0, 1.0);

		// Assigning random velocity to particles
		fireworks_velocities[i].x = 2.0 * ((rand() % 256) / 256.0 - 0.5);
		fireworks_velocities[i].y = 1.2 * 2.0 * (rand() % 256) / 256.0;
		fireworks_velocities[i].z = 2.0 * ((rand() % 256) / 256.0 - 0.5);

		// Assigning random color to particles
		fireworks_colors[i].x = (rand() % 256) / 256.0;
		fireworks_colors[i].y = (rand() % 256) / 256.0;
		fireworks_colors[i].z = (rand() % 256) / 256.0;
		fireworks_colors[i].w = 1.0;
	}

	// Create and initialize a vertex buffer object for the fireworks (particle system)
	glGenBuffers(1, &fireworks_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, fireworks_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(fireworks_points)+sizeof(fireworks_colors)+sizeof(fireworks_velocities), NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(fireworks_points), fireworks_points);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(fireworks_points), sizeof(fireworks_colors), fireworks_colors);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(fireworks_points)+sizeof(fireworks_colors), sizeof(fireworks_velocities), fireworks_velocities);

	// Load shaders and create a shader program (to be used in display())
	program  = InitShader("vshader1.glsl", "fshader.glsl");			// Shading w/ Lighting
	program2 = InitShader("vshader2.glsl", "fshader.glsl");			// Asgn 2 Shaders (w/ modifications)
	program3 = InitShader("vfireworks.glsl", "ffireworks.glsl");	// _fireworks.glsl are the shaders for fireworks particle system
    
    glEnable( GL_DEPTH_TEST );					// Always Enable Z-Buffer Testing

    glClearColor( 0.529, 0.807, 0.92, 0.0 );	// Sky Blue background color
    glLineWidth( 2.0 );
	glPointSize( 3.0 );							// Particle system point size

	print_Credits();							// Print out credits at the end of init
}

//----------------------------------------------------------------------------
/*--- Draw Functions ---*/
// drawObj()
//   draw the object that is associated with the vertex buffer object "buffer" and has "num_vertices" vertices.
//	 elements: 0: floor, 1: sphere, 2: shadow, 3: axes
void drawObj(GLuint buffer, int num_vertices, int textureFlag, int element) {
    //--- Activate the vertex buffer object to be drawn ---//
    glBindBuffer(GL_ARRAY_BUFFER, buffer);

    /*----- Set up vertex attribute arrays for each vertex attribute -----*/
	GLuint vPosition;
	GLuint vColor;
	GLuint vNormal;
	GLuint vTexCoord;

	// Based on shading settings, pass data to the shaders
	if (shadingFlag) {
		glUseProgram(program);	
		vPosition = glGetAttribLocation(program, "vPosition");
		vNormal = glGetAttribLocation(program, "vNormal");
		vTexCoord = glGetAttribLocation(program, "vTexCoord");
		glUniform1i(glGetUniformLocation(program, "fogFlag"), fogFlag);
		glUniform1i(glGetUniformLocation(program, "texture_2D"), 0);
		glUniform1i(glGetUniformLocation(program, "texture_1D"), 1);
		glUniform1i(glGetUniformLocation(program, "textureFlag"), textureFlag);
		glUniform1i(glGetUniformLocation(program, "latticeFlag"), latticeFlag);
		glUniform1i(glGetUniformLocation(program, "upLatticeFlag"), upLatticeFlag);
		glUniform1i(glGetUniformLocation(program, "element"), element);
		glUniform1i(glGetUniformLocation(program, "verticalFlag"), verticalFlag);
		glUniform1i(glGetUniformLocation(program, "eyeSpaceFlag"), eyeSpaceFlag);
		glUniform1i(glGetUniformLocation(program, "sphereTexFlag"), sphereTexFlag);
	}
	else {
		glUseProgram(program2);
		vPosition = glGetAttribLocation(program2, "vPosition");
		vColor = glGetAttribLocation(program2, "vColor");
		vTexCoord = glGetAttribLocation(program2, "vTexCoord");
		glUniform1i(glGetUniformLocation(program2, "fogFlag"), fogFlag);
		glUniform1i(glGetUniformLocation(program2, "texture_2D"), 0);
		glUniform1i(glGetUniformLocation(program2, "texture_1D"), 1);
		glUniform1i(glGetUniformLocation(program2, "textureFlag"), textureFlag);
		glUniform1i(glGetUniformLocation(program2, "latticeFlag"), latticeFlag);
		glUniform1i(glGetUniformLocation(program2, "upLatticeFlag"), upLatticeFlag);
		glUniform1i(glGetUniformLocation(program2, "element"), element);
		glUniform1i(glGetUniformLocation(program2, "verticalFlag"), verticalFlag);
		glUniform1i(glGetUniformLocation(program2, "eyeSpaceFlag"), eyeSpaceFlag);
		glUniform1i(glGetUniformLocation(program2, "sphereTexFlag"), sphereTexFlag);
	}

	glEnableVertexAttribArray(vPosition);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	if (shadingFlag) {
		glEnableVertexAttribArray(vNormal);
		glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(point4)* num_vertices));
		glEnableVertexAttribArray(vTexCoord);
		glVertexAttribPointer(vTexCoord, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(point4)* num_vertices + sizeof(vec3)* num_vertices));
	}
	else {
		glEnableVertexAttribArray(vColor);
		glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(point4)* num_vertices));
		glEnableVertexAttribArray(vTexCoord);
		glVertexAttribPointer(vTexCoord, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(point4)* num_vertices + sizeof(point4)* num_vertices));
	}

	/* Draw a sequence of geometric objs (triangles) from the vertex buffer
	   (using the attributes specified in each enabled vertex attribute array) */
	glDrawArrays(GL_TRIANGLES, 0, num_vertices);

	/*--- Disable each vertex attribute array being enabled ---*/
	glDisableVertexAttribArray(vPosition);
	glDisableVertexAttribArray(vTexCoord);

	if (shadingFlag){ glDisableVertexAttribArray(vNormal); }
	else			{ glDisableVertexAttribArray(vColor);  }
}

// drawLine()
//	Function used to draw the axes
void drawLine(GLuint buffer, int num_vertices) {
	glUseProgram(program2);
	glUniform1i(glGetUniformLocation(program2, "fogFlag"), fogFlag);	// Fog is applied to Axes
	glUniform1i(glGetUniformLocation(program2, "element"), 3);			// Element 3 is axes

	//--- Activate the vertex buffer object to be drawn ---//
	glBindBuffer(GL_ARRAY_BUFFER, buffer);

	/*----- Set up vertex attribute arrays for each vertex attribute -----*/
	GLuint vPosition = glGetAttribLocation(program2, "vPosition");
	glEnableVertexAttribArray(vPosition);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	GLuint vColor = glGetAttribLocation(program2, "vColor");
	glEnableVertexAttribArray(vColor);
	glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(point4)* num_vertices));

	/*  Draw a sequence of geometric objs (triangles) from the vertex buffer
		(using the attributes specified in each enabled vertex attribute array) */
	glDrawArrays(GL_LINES, 0, num_vertices);

	/*--- Disable each vertex attribute array being enabled ---*/
	glDisableVertexAttribArray(vPosition);
	glDisableVertexAttribArray(vColor);
}

// drawFireworks()
//	Function for drawing fireworks (particle system)
void drawFireworks(GLuint buffer, int num_vertices) {
	glUseProgram(program3);			// program3 is the fireworks shader

	//--- Activate the vertex buffer object to be drawn ---//
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glUniform1f(glGetUniformLocation(program3, "t"), time_New);

	/*----- Set up vertex attribute arrays for each vertex attribute -----*/
	GLuint vPosition = glGetAttribLocation(program3, "vPosition");
	glEnableVertexAttribArray(vPosition);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	GLuint vColor = glGetAttribLocation(program3, "vColor");
	glEnableVertexAttribArray(vColor);
	glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(point4)* num_vertices));

	GLuint vVelocity = glGetAttribLocation(program3, "vVelocity");
	glEnableVertexAttribArray(vVelocity);
	glVertexAttribPointer(vVelocity, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(point4)* num_vertices * 2));

	/* Draw a sequence of geometric objs (triangles) from the vertex buffer
	   (using the attributes specified in each enabled vertex attribute array) */
	glDrawArrays(GL_POINTS, 0, num_vertices);

	/*--- Disable each vertex attribute array being enabled ---*/
	glDisableVertexAttribArray(vVelocity);
	glDisableVertexAttribArray(vPosition);
	glDisableVertexAttribArray(vColor);
}

//----------------------------------------------------------------------------
/*--- Shader Functions ---*/
void setup_floor(mat4 mv){
	glUniform3fv(glGetUniformLocation(program, "LightDirection"), 1, light_direction);

	glUniform4fv(glGetUniformLocation(program, "GlobalAmbient"), 1, global_ambient);
	glUniform4fv(glGetUniformLocation(program, "SurfaceAmbient"), 1, ground_ambient);
	
	glUniform4fv(glGetUniformLocation(program, "AmbientProduct"), 1, ground_ambient_product);
	glUniform4fv(glGetUniformLocation(program, "DiffuseProduct"), 1, ground_diffuse_product);
	glUniform4fv(glGetUniformLocation(program, "SpecularProduct"), 1, ground_specular_product);

	glUniform1f(glGetUniformLocation(program, "Shininess"), shininess_coefficient);
}

void setup_pointLight_floor(mat4 mv){
	vec4 point_position = mv * light_source2;
	glUniform4fv(glGetUniformLocation(program, "PointPosition"), 1, point_position);

	glUniform1i(glGetUniformLocation(program, "Point_Source"), 1);

	glUniform4fv(glGetUniformLocation(program, "PointAmbientProduct"), 1, ground_point_ambient_product);
	glUniform4fv(glGetUniformLocation(program, "PointDiffuseProduct"), 1, ground_point_diffuse_product);
	glUniform4fv(glGetUniformLocation(program, "PointSpecularProduct"), 1, ground_point_specular_product);

	glUniform1f(glGetUniformLocation(program, "Constant_Attenuation"), constant_attenuation);
	glUniform1f(glGetUniformLocation(program, "Linear_Attenuation"), linear_attenuation);
	glUniform1f(glGetUniformLocation(program, "Quadratic_Attenuation"), quadratic_attenuation);
}

void setup_spotLight_floor(mat4 mv){
	vec4 point_position = mv * light_source2;
	vec4 point_end_position = mv * light2_endpoint;
	glUniform4fv(glGetUniformLocation(program, "PointPosition"), 1, point_position);
	glUniform1f(glGetUniformLocation(program, "Exponent"), exponent);
	glUniform1f(glGetUniformLocation(program, "CutOff"), cutoff_angle);
	glUniform4fv(glGetUniformLocation(program, "PointEndPosition"), 1, point_end_position);

	glUniform1i(glGetUniformLocation(program, "Point_Source"), 0);

	glUniform4fv(glGetUniformLocation(program, "PointAmbientProduct"), 1, ground_point_ambient_product);
	glUniform4fv(glGetUniformLocation(program, "PointDiffuseProduct"), 1, ground_point_diffuse_product);
	glUniform4fv(glGetUniformLocation(program, "PointSpecularProduct"), 1, ground_point_specular_product);

	glUniform1f(glGetUniformLocation(program, "Constant_Attenuation"), constant_attenuation);
	glUniform1f(glGetUniformLocation(program, "Linear_Attenuation"), linear_attenuation);
	glUniform1f(glGetUniformLocation(program, "Quadratic_Attenuation"), quadratic_attenuation);
}

void setup_sphere(mat4 mv){
	glUniform3fv(glGetUniformLocation(program, "LightDirection"), 1, light_direction);

	glUniform4fv(glGetUniformLocation(program, "GlobalAmbient"), 1, global_ambient);
	glUniform4fv(glGetUniformLocation(program, "SurfaceAmbient"), 1, sphere_ambient);
	
	glUniform4fv(glGetUniformLocation(program, "AmbientProduct"), 1, sphere_ambient_product);
	glUniform4fv(glGetUniformLocation(program, "DiffuseProduct"), 1, sphere_diffuse_product);
	glUniform4fv(glGetUniformLocation(program, "SpecularProduct"), 1, sphere_specular_product);

	glUniform1f(glGetUniformLocation(program, "Shininess"), shininess_coefficient);
}

void setup_pointLight_sphere(mat4 mv){
	vec4 point_position = mv * light_source2;
	glUniform4fv(glGetUniformLocation(program, "PointPosition"), 1, point_position);

	glUniform1i(glGetUniformLocation(program, "Point_Source"), 1);

	glUniform4fv(glGetUniformLocation(program, "PointAmbientProduct"), 1, sphere_point_ambient_product);
	glUniform4fv(glGetUniformLocation(program, "PointDiffuseProduct"), 1, sphere_point_diffuse_product);
	glUniform4fv(glGetUniformLocation(program, "PointSpecularProduct"), 1, sphere_point_specular_product);

	glUniform1f(glGetUniformLocation(program, "Constant_Attenuation"), constant_attenuation);
	glUniform1f(glGetUniformLocation(program, "Linear_Attenuation"), linear_attenuation);
	glUniform1f(glGetUniformLocation(program, "Quadratic_Attenuation"), quadratic_attenuation);
}

void setup_spotLight_sphere(mat4 mv){
	vec4 point_position = mv * light_source2;
	vec4 point_end_position = mv * light2_endpoint;
	glUniform4fv(glGetUniformLocation(program, "PointPosition"), 1, point_position);
	glUniform1f(glGetUniformLocation(program, "Exponent"), exponent);
	glUniform1f(glGetUniformLocation(program, "CutOff"), cutoff_angle);
	glUniform4fv(glGetUniformLocation(program, "PointEndPosition"), 1, point_end_position);

	glUniform1i(glGetUniformLocation(program, "Point_Source"), 0);

	glUniform4fv(glGetUniformLocation(program, "PointAmbientProduct"), 1, sphere_point_ambient_product);
	glUniform4fv(glGetUniformLocation(program, "PointDiffuseProduct"), 1, sphere_point_diffuse_product);
	glUniform4fv(glGetUniformLocation(program, "PointSpecularProduct"), 1, sphere_point_specular_product);

	glUniform1f(glGetUniformLocation(program, "Constant_Attenuation"), constant_attenuation);
	glUniform1f(glGetUniformLocation(program, "Linear_Attenuation"), linear_attenuation);
	glUniform1f(glGetUniformLocation(program, "Quadratic_Attenuation"), quadratic_attenuation);
}

//----------------------------------------------------------------------------
// display 
void display(void) {
	GLuint  model_view;										// model-view matrix for program
	GLuint  projection;										// projection matrix for program
	GLuint  model_view2;									// model-view matrix for program2
	GLuint  projection2;									// projection matrix for program2
	GLuint  model_view3;									// model-view matrix for program3
	GLuint  projection3;									// projection matrix for program3

	glDepthMask(GL_TRUE);									// Enable Writing to Z-Buffer
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);		// Enable Writing to Color Buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		// Clear screen w/ default color (sky blue)

	glUseProgram(program);
	model_view = glGetUniformLocation(program, "model_view");
	projection = glGetUniformLocation(program, "projection");
	glUseProgram(program2);
	model_view2 = glGetUniformLocation(program2, "model_view");
	projection2 = glGetUniformLocation(program2, "projection");
	glUseProgram(program3);
	model_view3 = glGetUniformLocation(program3, "model_view");
	projection3 = glGetUniformLocation(program3, "projection");

	/*--- Set up and pass on Projection matrix to the shader ---*/
	mat4  p = Perspective(fovy, aspect, zNear, zFar);

	glUseProgram(program); 	glUniformMatrix4fv(projection, 1, GL_TRUE, p);
	glUseProgram(program2);	glUniformMatrix4fv(projection2, 1, GL_TRUE, p);
	glUseProgram(program3);	glUniformMatrix4fv(projection3, 1, GL_TRUE, p);

	// Using the proper shader program based on settings
	if (shadingFlag){ glUseProgram(program); }
	else			{ glUseProgram(program2); }

	/*--- Set up and pass on Model-View matrix to the shader ---*/
	// eye is a global variable of vec4 set to init_eye and updated by keyboard()
	vec4    at(0.0, 0.0, 0.0, 1.0);
	vec4    up(0.0, 1.0, 0.0, 0.0);
	mat4	mv = LookAt(eye, at, up);

	//===== Sphere
	setup_sphere(mv);
	mv = LookAt(eye, at, up) * Translate(translate) * Rotate(angle, rotateX, rotateY, rotateZ) * accumulated_rotation;

	// Set up normal matrix using model-view matrix
	mat3 normal_matrix = NormalMatrix(mv, 1);
	glUniformMatrix3fv(glGetUniformLocation(program, "Normal_Matrix"), 1, GL_TRUE, normal_matrix);

	// Using the proper polygon mode (wire or filled)
	if (sphereFlag) { glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); }
	else			{ glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); }

	glUseProgram(program);  glUniformMatrix4fv(model_view, 1, GL_TRUE, mv);
	glUseProgram(program2); glUniformMatrix4fv(model_view2, 1, GL_TRUE, mv);

	// Using the proper shader program based on shadingFlag
	if (shadingFlag){ glUseProgram(program); }
	else			{ glUseProgram(program2); }

	mv = LookAt(eye, at, up);

	// Draw the correct sphere based on the flags 
	if (shadingFlag){
		if (sphereFlag){
			if (flatFlag){
				if (spotLightFlag){
					// Spot Light & Flat Shading
					setup_spotLight_sphere(mv);
					drawObj(flat_sphere_buffer, sphere_NumVertices, 0, 1);
				}
				else {
					// Point Source & Flag Shading
					setup_pointLight_sphere(mv);
					drawObj(flat_sphere_buffer, sphere_NumVertices, 0, 1);
				}
			}
			else {
				if (spotLightFlag){
					// Spot Light & Smooth Shading
					setup_spotLight_sphere(mv);
					drawObj(smooth_sphere_buffer, sphere_NumVertices, 0, 1);
				}
				else {
					// Point Source & Smooth Shading
					setup_pointLight_sphere(mv);
					drawObj(smooth_sphere_buffer, sphere_NumVertices, 0, 1);
				}
			}
		}
		else {
			// Wire Frame
			int temp = shadingFlag;
			shadingFlag = 0;
			drawObj(sphere_buffer, sphere_NumVertices, 0, 1);
			shadingFlag = temp;
		}
	}
	else {
		// No shading - same as project 2
		drawObj(sphere_buffer, sphere_NumVertices, 0, 1);
	}

	/*--- Decal Technique (for drawing the shadow correctly) : modified since HW3 ---*/
	mv = LookAt(eye, at, up);

	glUseProgram(program); 	glUniformMatrix4fv(model_view, 1, GL_TRUE, mv);
	glUseProgram(program2);	glUniformMatrix4fv(model_view2, 1, GL_TRUE, mv);

	// Using the proper polygon mode (wire or filled)
	if (floorFlag == 1)			// Filled floor
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	else						// Wireframe floor
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// Draw the Axes
	drawLine(axes_buffer, axes_NumVertices);

	// Using the proper shader program
	if (shadingFlag) { glUseProgram(program); }
	else			 { glUseProgram(program2); }

	//=== Step 1 & 2. Disable writing to Z-Buffer & Draw the Ground and Shadow
	glDepthMask(GL_FALSE);					// Disable writing to Z-Buffer

	// Take into consideration whether the camera is above or below ground (we don't want the shadow to display under the floor)
	if (eye.y > 0.0) {						// Eye is ABOVE ground
		setup_floor(mv);
		normal_matrix = NormalMatrix(mv, 1);
		glUniformMatrix3fv(glGetUniformLocation(program, "Normal_Matrix"), 1, GL_TRUE, normal_matrix);

		// Draw the Ground
		if (shadingFlag){
			if (spotLightFlag) {
				setup_spotLight_floor(mv);
				drawObj(flat_floor_buffer, floor_NumVertices, floorTexFlag, 0);
			}
			else {
				setup_pointLight_floor(mv);
				drawObj(flat_floor_buffer, floor_NumVertices, floorTexFlag, 0);
			}
		}
		else {
			drawObj(floor_buffer, floor_NumVertices, floorTexFlag, 0);
		}

		// Draw the Shadow
		if (shadowFlag) {
			// Enable blending if shadow blending is enabled
			if (blendingFlag){
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glEnable(GL_BLEND);
			}

			// Display wireframe if the setting is applied (filled otherwise)
			if (sphereFlag) { glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); }
			else			{ glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); }

			// Roll the shadow (along with the sphere object)
			mv = LookAt(eye, at, up) * transpose1(shadow_Matrix) * Translate(translate) * Rotate(angle, rotateX, rotateY, rotateZ) * accumulated_rotation;

			glUseProgram(program); 	glUniformMatrix4fv(model_view, 1, GL_TRUE, mv);
			glUseProgram(program2);	glUniformMatrix4fv(model_view2, 1, GL_TRUE, mv);

			// Draw the shadow
			int temp = shadingFlag;
			shadingFlag = 0;
			drawObj(shadow_buffer, sphere_NumVertices, 0, 2);
			shadingFlag = temp;

			// Disable blend since the shadow is already drawn
			glDisable(GL_BLEND);
		}

	//=== Step 3. Enable writing to Z-BUffer, Disable writing to Color Buffer, Draw Ground
		glDepthMask(GL_TRUE);										// Enable Z-Buffer
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);		// Disable Color Buffer writing

		// Draw the Ground
		mv = LookAt(eye, at, up);
		glUniformMatrix4fv(model_view, 1, GL_TRUE, mv);
		glUniformMatrix4fv(model_view2, 1, GL_TRUE, mv);

		// Use the proper polygon mode based on whether floor is filled or wired
		if (floorFlag == 1)		// Filled floor
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		else					// Wireframe floor
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		setup_floor(mv);
		normal_matrix = NormalMatrix(mv, 1);
		glUniformMatrix3fv(glGetUniformLocation(program, "Normal_Matrix"), 1, GL_TRUE, normal_matrix);

		if (shadingFlag){
			if (spotLightFlag) {
				setup_spotLight_floor(mv);
				drawObj(flat_floor_buffer, floor_NumVertices, floorTexFlag, 0);
			}
			else {
				setup_pointLight_floor(mv);
				drawObj(flat_floor_buffer, floor_NumVertices, floorTexFlag, 0);
			}
		}
		else {
			drawObj(floor_buffer, floor_NumVertices, floorTexFlag, 0);
		}
	}
	else {									// Eye is BELOW ground
		// Draw Shadow : Note that we are NOT writing into the Z-Buffer
		if (shadowFlag) {
			// Enable blending if shadow blending is on
			if (blendingFlag){
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glEnable(GL_BLEND);
			}

			// Use the proper polygon mode
			if (sphereFlag) { glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); }
			else			{ glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); }

			// Roll the shadow (along with the sphere object)
			mv = LookAt(eye, at, up) * transpose1(shadow_Matrix) * Translate(translate) * Rotate(angle, rotateX, rotateY, rotateZ) * accumulated_rotation;

			glUseProgram(program); 	glUniformMatrix4fv(model_view, 1, GL_TRUE, mv);
			glUseProgram(program2);	glUniformMatrix4fv(model_view2, 1, GL_TRUE, mv);

			// Draw the shadow
			int temp = shadingFlag;
			shadingFlag = 0;
			drawObj(shadow_buffer, sphere_NumVertices, 0, 2);
			shadingFlag = temp;

			// Disable blend since we are done drawing the shadow
			glDisable(GL_BLEND);
		}

		// Draw the Ground
		mv = LookAt(eye, at, up);

		if (shadingFlag){
			glUseProgram(program);
			glUniformMatrix4fv(model_view, 1, GL_TRUE, mv);
		}
		else {
			glUseProgram(program2);
			glUniformMatrix4fv(model_view2, 1, GL_TRUE, mv);
		}

		if (floorFlag == 1)			// Filled floor
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		else						// Wireframe floor
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		setup_floor(mv);
		normal_matrix = NormalMatrix(mv, 1);
		glUniformMatrix3fv(glGetUniformLocation(program, "Normal_Matrix"), 1, GL_TRUE, normal_matrix);

		if (shadingFlag){
			if (spotLightFlag) {
				setup_spotLight_floor(mv);
				drawObj(flat_floor_buffer, floor_NumVertices, floorTexFlag, 0);
			}
			else {
				setup_pointLight_floor(mv);
				drawObj(flat_floor_buffer, floor_NumVertices, floorTexFlag, 0);
			}
		}
		else {
			drawObj(floor_buffer, floor_NumVertices, floorTexFlag, 0);
		}

	//=== Step 3: Enable writing to Z-Buffer, Disable writing to Frame Buffer, Draw Shadow & Ground
		glDepthMask(GL_TRUE);										// Enable writing to Z-Buffer
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);		// Disable writing to Frame Buffer

		// Draw Shadow
		if (shadowFlag) {
			// Enable blending if needed
			if (blendingFlag){
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glEnable(GL_BLEND);
			}

			// Choose the correct polygon mode
			if (sphereFlag)	{ glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); }
			else			{ glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); }

			// Roll the shadow (along with the sphere object)
			mv = LookAt(eye, at, up) * transpose1(shadow_Matrix) * Translate(translate) * Rotate(angle, rotateX, rotateY, rotateZ) * accumulated_rotation;

			glUseProgram(program);	glUniformMatrix4fv(model_view, 1, GL_TRUE, mv);
			glUseProgram(program2);	glUniformMatrix4fv(model_view2, 1, GL_TRUE, mv);

			// Draw the shadow
			int temp = shadingFlag;
			shadingFlag = 0;
			drawObj(shadow_buffer, sphere_NumVertices, 0, 2);
			shadingFlag = temp;

			// Disable blending since we are done drawing the shadow
			glDisable(GL_BLEND);
		}

		// Draw Ground
		mv = LookAt(eye, at, up);

		if (shadingFlag){
			glUseProgram(program);
			glUniformMatrix4fv(model_view, 1, GL_TRUE, mv);
		}
		else {
			glUseProgram(program2);
			glUniformMatrix4fv(model_view2, 1, GL_TRUE, mv);
		}

		// Choose the correct polygon mode
		if (floorFlag == 1)			// Filled floor
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		else						// Wireframe floor
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		setup_floor(mv);
		normal_matrix = NormalMatrix(mv, 1);
		glUniformMatrix3fv(glGetUniformLocation(program, "Normal_Matrix"), 1, GL_TRUE, normal_matrix);

		if (shadingFlag){
			if (spotLightFlag) {
				setup_spotLight_floor(mv);
				drawObj(flat_floor_buffer, floor_NumVertices, floorTexFlag, 0);
			}
			else {
				setup_pointLight_floor(mv);
				drawObj(flat_floor_buffer, floor_NumVertices, floorTexFlag, 0);
			}
		}
		else {
			drawObj(floor_buffer, floor_NumVertices, floorTexFlag, 0);
		}
	}

	//=== Step 4. Enable Writing to Frame Buffer and Resume Operation
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	// Draw fireworks (particle system)
	if (fireworkFlag) {
		mv = LookAt(eye, at, up);
		glUseProgram(program3); glUniformMatrix4fv(model_view3, 1, GL_TRUE, mv);
		drawFireworks(fireworks_buffer, N);
	}

	glutSwapBuffers();
}

//---------------------------------------------------------------------------
// idle()
//	This is where we handle the animation updates
void idle (void) {
	// Only roll the ball if the animation flag is active (toggled with right click)
	if (animationFlag) {
		angle += 0.02;
	}

	vec4 v = (angle / 360) * 2 * M_PI * 1.0;
	vec4 direction;
	vec3 rotate;

	// Translating from Point A to Point B
	if (path == 0) {
		direction = pointB - pointA;
		rotate = cross(vecOY, direction);
		translate = (pointA - origin) + v * normalize(direction);
	}
	// Translating from Point B to Point C
	else if (path == 1) {
		direction = pointC - pointB;
		rotate = cross(vecOY, direction);
		translate = (pointB - origin) + v * normalize(direction);
	}
	// Translating from Point C to Point A
	else if (path == 2) {
		direction = pointA - pointC;
		rotate = cross(vecOY, direction);
		translate = (pointC - origin) + v * normalize(direction);
	}
	translate.w = 0.0;
	rotateX = rotate.x;
	rotateY = rotate.y;
	rotateZ = rotate.z;

	// Managing Path / Accumulated Rotation / Angle
	if (translate.z > pointA.z && direction.z > 0) {
		path = 0;
		accumulated_rotation = Rotate(angle, rotateX, rotateY, rotateZ) * accumulated_rotation;
		angle = 0;
	}
	else if (translate.z < pointB.z && translate.x < pointB.x && direction.z < 0 && direction.x < 0) {
		path = 1;
		accumulated_rotation = Rotate(angle, rotateX, rotateY, rotateZ) * accumulated_rotation;
		angle = 0;
	}
	else if (translate.z < pointC.z && translate.x > pointC.x && direction.z < 0 && direction.x > 0) {
		path = 2;
		accumulated_rotation = Rotate(angle, rotateX, rotateY, rotateZ) * accumulated_rotation;
		angle = 0;
	}

	// Update the time variables (particle system)
	time_Old = (float)glutGet(GLUT_ELAPSED_TIME);
	time_New = fmod(time_Old - time_Sub, time_Max);

	glutPostRedisplay();
}

//----------------------------------------------------------------------------
// reshape
void reshape(int width, int height) {
	glViewport(0, 0, width, height);
	aspect = (GLfloat)width / (GLfloat)height;
	glutPostRedisplay();
}

//----------------------------------------------------------------------------
// keyboard
void keyboard(unsigned char key, int x, int y) {
    switch(key) {

	// Quitting Program
	// Key 'ESCAPE KEY' or 'q' or 'Q'
	case 033: // Escape Key
	case 'q': case 'Q':
	    exit( EXIT_SUCCESS );
	    break;

    case 'X': eye[0] += 1.0; break;
	case 'x': eye[0] -= 1.0; break;
    case 'Y': eye[1] += 1.0; break;
	case 'y': eye[1] -= 1.0; break;
    case 'Z': eye[2] += 1.0; break;
	case 'z': eye[2] -= 1.0; break;

	// Begin: starts the sphere rolling
	case 'b': case 'B':			
		if (beginFlag == 0){
			beginFlag = 1;		// Flag to tell if right click will have an effect or not
			animationFlag = 1;	// Set animationFlag to start the animation
			glutIdleFunc(idle);
		}
        break;

	/*--- Texture Controls ---*/
	// Vertical: switches texture coordinate to be s = 2.5x
	case 'v': case 'V':
		verticalFlag = 1;
		break;

	// Slanted: switches texture coordinate to be s = 1.5 (x + y + z)
	case 's': case 'S':
		verticalFlag = 0;
		break;

	// Object Space: texture in world frame
	case 'o': case 'O':
		eyeSpaceFlag = 0;
		break;

	// Eye Space: texture in eye frame
	case 'e': case 'E':
		eyeSpaceFlag = 1;
		break;

	/*--- Texture Lattice Effect Controls ---*/
	// Upright: switches texture coordinates to be s = 0.5 (x + 1), t = 0.5 (y + 1)
	case 'u': case 'U':
		upLatticeFlag = 1;
		break;

	// Tilted: switches texture coordinates to be s = 0.3 (x + y + z), t = 0.3 (x - y + z)
	case 't': case 'T':
		upLatticeFlag = 0;
		break;

	// Enable Lattice Effect
	case 'l': case 'L':
		latticeFlag = 1 - latticeFlag;
		break;

	/* 
	// FEATURE REMOVED: The ground should ALWAYS be solid for Asgn 3
	// Toggle between filled and wireframe floor
	// Key 'f' or 'F'
	case 'f': case 'F':			
	    floorFlag = 1 - floorFlag; 
        break;
	*/

	// reset to initial viewer/eye position
	// Key ' ' <-- space
	case ' ':					
	    eye = init_eye;
	    break;
    }

    glutPostRedisplay();
}

//----------------------------------------------------------------------------
// Mouse Menu / Submenus
void menuShadow(int id) {
	// 1: No
	// 2: Yes
	switch (id) {
	case 1:
		shadowFlag = 0;
		break;
	case 2:
		shadowFlag = 1;
		break;
	}
	glutPostRedisplay();
}

void menuLighting(int id) {
	// 3: No
	// 4: Yes
	switch (id) {
	case 3:
		shadingFlag = 0;
		break;
	case 4:
		shadingFlag = 1;
		break;
	}
	glutPostRedisplay();
}

void menuShading(int id) {
	// 5: Flat Shading
	// 6: Smooth Shading
	switch (id) {
	case 5:
		sphereFlag = 1;
		flatFlag = 1;
		break;
	case 6:
		sphereFlag = 1;
		flatFlag = 0;
		break;
	}
	glutPostRedisplay();
}

void menuLightSource(int id) {
	// 7: Spot Light
	// 8: Point Source
	switch (id) {
	case 7:
		spotLightFlag = 1;
		break;
	case 8:
		spotLightFlag = 0;
		break;
	}
	glutPostRedisplay();
}

void menuFog(int id) {
	// 13: No Fog
	// 14: Linear
	// 15: Exponential
	// 16: Exponential Square

	switch (id) {
	case 13:
		fogFlag = 0;
		break;
	case 14:
		fogFlag = 1;
		break;
	case 15:
		fogFlag = 2;
		break;
	case 16:
		fogFlag = 3;
		break;
	}
	glutPostRedisplay();
}

void menuBlendShadow(int id) {
	// 17: No
	// 18: Yes

	switch (id) {
	case 17:
		blendingFlag = 0;
		break;
	case 18:
		blendingFlag = 1;
		break;
	}

	glutPostRedisplay();
}

void menuFloorTex(int id) {
	// 19: No
	// 20: Yes

	switch (id) {
	case 19:
		floorTexFlag = 0;
		break;

	case 20:
		floorTexFlag = 1;
		break;
	}

	glutPostRedisplay();
}

void menuSphereTex(int id) {
	// 21: No
	// 22: Contour
	// 23: Checkerboard

	switch (id) {
	case 21:
		sphereTexFlag = 0;
		break;
	case 22:
		sphereTexFlag = 1;
		break;
	case 23:
		sphereTexFlag = 2;
		break;
	}

	glutPostRedisplay();
}

void menuFireworks(int id) {
	// 24: No
	// 25: Yes

	switch (id) {
	case 24:
		fireworkFlag = 0;
		break;
	case 25:
		if (fireworkFlag == 0) {
			fireworkFlag = 1;
			time_Old = (float)glutGet(GLUT_ELAPSED_TIME);
			time_Sub = time_Old;
			time_New = 0.0f;
		}
		break;
	}

	glutPostRedisplay();
}

void menu(int id) {
	// 9: Wire Frame Sphere
	// 10: Wire Frame Floor
	// 11: Default View Point
	// 12: Quit

	switch (id) {
	case 9:
		sphereFlag = 1 - sphereFlag;
		break;
	/* // FEATURE REMOVED: The ground should ALWAYS be solid for Asgn 3
	case 10:
		floorFlag = 1 - floorFlag;
		break;
	*/
	case 11:
		eye = init_eye;
		break;
	case 12:
		exit(0);
		break;
	}

	glutPostRedisplay();
}

// mouse
void mouse(int button, int state, int x, int y) {
	// Right Click for Toggling Rolling ON/OFF
	if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) {
		// Right Click ONLY has an effect after Begin has been pressed
		if (beginFlag){
			animationFlag = 1 - animationFlag;
		}
	}

	// Mouse Menu (accessed via Left Click)
	int menu_Shadow = glutCreateMenu(menuShadow);			// Submenu for Shadow: Yes / No
	glutAddMenuEntry("No", 1);
	glutAddMenuEntry("Yes", 2);

	int menu_Lighting = glutCreateMenu(menuLighting);		// Submenu for Lighting: Yes / No
	glutAddMenuEntry("No", 3);
	glutAddMenuEntry("Yes", 4);

	int menu_Shading = glutCreateMenu(menuShading);			// Submenu for Shading: Flag / Smooth
	glutAddMenuEntry("Flat Shading", 5);
	glutAddMenuEntry("Smooth Shading", 6);

	int menu_LightSource = glutCreateMenu(menuLightSource);	// Submenu for Light Source: Spot Light / Point Source
	glutAddMenuEntry("Spot Light", 7);
	glutAddMenuEntry("Point Source", 8);

	int menu_Fog = glutCreateMenu(menuFog);					// Submenu for Fog : 3 types of fog if enabled
	glutAddMenuEntry("No Fog", 13);
	glutAddMenuEntry("Linear", 14);
	glutAddMenuEntry("Exponential", 15);
	glutAddMenuEntry("Exponential Square", 16);

	int menu_BlendShadow = glutCreateMenu(menuBlendShadow);	// Submenu for shadow blending 
	glutAddMenuEntry("No", 17);
	glutAddMenuEntry("Yes", 18);

	int menu_FloorTex = glutCreateMenu(menuFloorTex);		// Submenu for enabling floor texture
	glutAddMenuEntry("No", 19);
	glutAddMenuEntry("Yes", 20);

	int menu_SphereTex = glutCreateMenu(menuSphereTex);		// Submenu for choosing sphere texture (if any)
	glutAddMenuEntry("No", 21);
	glutAddMenuEntry("Yes - Contour Lines", 22);
	glutAddMenuEntry("Yes - Checkerboard", 23);

	int menu_Fireworks = glutCreateMenu(menuFireworks);		// Submenu for enabling fireworks (particle system)
	glutAddMenuEntry("No", 24);
	glutAddMenuEntry("Yes", 25);

	int menu_Main = glutCreateMenu(menu);					// Main Menu
	glutAddSubMenu("Shadow", menu_Shadow);					// Offer Shadow submenu
	glutAddSubMenu("Enable Lighting", menu_Lighting);		// Offer Lighting submenu
	glutAddSubMenu("Shading", menu_Shading);				// Offer Shading submenu
	glutAddSubMenu("Light Source", menu_LightSource);		// Offer Light Source submenu
	glutAddSubMenu("Fog Options", menu_Fog);				// Offer Fog Options submenu
	glutAddSubMenu("Blending Shadow", menu_BlendShadow);	// Offer Blending Shadow submenu
	glutAddSubMenu("Texture Mapped Ground", menu_FloorTex);	// Offer Tex. Ground submenu
	glutAddSubMenu("Texture Mapped Sphere", menu_SphereTex);// Offer Tex. Sphere submenu
	glutAddSubMenu("Firework", menu_Fireworks);				// Offer Firework submenu

	glutAddMenuEntry("Wire Frame Sphere (Toggle)", 9);		// Option 9:	Wire Frame Sphere
	//glutAddMenuEntry("Wire Frame Floor (Toggle)", 10);	// Option 10:	Wire Frame Floor ::: FEATURE REMOVED FOR ASGN 3 & 4
	glutAddMenuEntry("Default View Point", 11);				// Option 11:	Default View Point
	glutAddMenuEntry("Quit", 12);							// Option 12:	Quit
	glutAttachMenu(GLUT_LEFT_BUTTON);						// Left Mouse Click to display menu
}

//----------------------------------------------------------------------------
// main
int main(int argc, char **argv) {
	int err;

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(600, 600);
	glutCreateWindow("Shunman Tse: Semester Long Graphics Project - Spring 2015");

	/* Call glewInit() and error checking */
	err = glewInit();
	if (GLEW_OK != err) {
		printf("Error: glewInit failed: %s\n", (char*)glewGetErrorString(err));
		exit(1);
	}

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutIdleFunc(idle);
	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouse);

	init();
	glutMainLoop();
	return 0;
}