/***************************************************************************
 * File: vshader2.glsl:
 *   A simple vertex shader. It simply passes values along to the fshader.
 *
 * - Vertex attributes (positions & colors) for all vertices are sent
 *   to the GPU via a vertex buffer object created in the OpenGL program.
 *
 * - This vertex shader uses the Model-View and Projection matrices passed
 *   on from the OpenGL program as uniform variables of type mat4.
 **************************************************************************/

 #version 150  // YJC: Comment/un-comment this line to resolve compilation errors
               //      due to different settings of the default GLSL version

in  vec4 vPosition;
in  vec4 vColor;
in  vec2 vTexCoord;

out vec4 position;
out vec3 fPosition;
out vec4 color;
out vec2 texCoord;

uniform mat4 model_view;
uniform mat4 projection;

void main() {
	gl_Position = projection * model_view * vPosition;
	vec3 pos	= (model_view * vPosition).xyz;	
    position	= vPosition;
	fPosition	= pos;

	color		= vColor;
	texCoord	= vTexCoord;
} 
