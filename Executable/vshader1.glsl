/*****************************************************************************
 * File: vshader1.glsl:
 * - Vertex shader 
 *   This is a modified version of vshader42.glsl
 *
 * - Vertex attributes (positions & colors) for all vertices are sent
 *   to the GPU via a vertex buffer object created in the OpenGL program.
 *
 * - This vertex shader uses the Model-View and Projection matrices passed
 *   on from the OpenGL program as uniform variables of type mat4.
 ****************************************************************************/

 #version 150  // YJC: Comment/un-comment this line to resolve compilation errors
               //      due to different settings of the default GLSL version

in  vec4 vPosition;
in  vec3 vNormal;
in  vec2 vTexCoord;

out vec4 position;
out vec3 fPosition; 
out vec4 color;
out vec2 texCoord;

uniform int Point_Source;

uniform mat4 model_view, projection;
uniform vec4 GlobalAmbient, SurfaceAmbient; 
uniform vec4 AmbientProduct, DiffuseProduct, SpecularProduct;
uniform vec4 PointAmbientProduct, PointDiffuseProduct, PointSpecularProduct;
uniform vec4 PointPosition, PointEndPosition;

uniform mat3 Normal_Matrix;
uniform vec3 LightDirection;
uniform float Shininess;

uniform float CutOff, Exponent;
uniform float Constant_Attenuation, Linear_Attenuation, Quadratic_Attenuation;		

void main() {
	// For Point Light Sources
	vec4 point_ambient			= vec4(0.0, 0.0, 0.0, 0.0);
	vec4 point_diffuse			= vec4(0.0, 0.0, 0.0, 0.0);
	vec4 point_specular			= vec4(0.0, 0.0, 0.0, 0.0);
	float point_attenuation		= 0.0f;

	// For SpotLight Sources 
	vec4 spotLight_ambient		= vec4(0.0, 0.0, 0.0, 0.0);
	vec4 spotLight_diffuse		= vec4(0.0, 0.0, 0.0, 0.0);
	vec4 spotLight_specular		= vec4(0.0, 0.0, 0.0, 0.0);
	float spotLight_attenuation = 0.0f;

	float angle;

	position	 = vPosition;
	vec3 pos	 = (model_view * vPosition).xyz;
	vec3 L		 = normalize(LightDirection);
	vec3 E		 = normalize(-pos);
	vec3 H		 = normalize(L+E);
	vec3 N		 = normalize(Normal_Matrix * vNormal);

	fPosition	 = pos;
	texCoord	 = vTexCoord;

	vec4 ambient = AmbientProduct;

	float d		 = max(dot(L,N), 0.0);
	vec4 diffuse = d * DiffuseProduct;

	float s		 = pow(max(dot(N,H), 0.0), Shininess);
	vec4 specular = s * SpecularProduct;

	if (dot(L, N) < 0.0) {
		specular = vec4(0.0, 0.0, 0.0, 1.0);
	}

	if (Point_Source) {
		// Point Source Lighting
		float dist	= length(PointPosition.xyz - pos); 
		vec3 L2		= normalize(PointPosition.xyz - pos);
		vec3 E2		= normalize(-pos);
		vec3 H2		= normalize(L2 + E2);
		
		point_ambient = PointAmbientProduct;

		float d2 = max(dot(L2, N), 0.0);
		point_diffuse = d2 * PointDiffuseProduct;

		float s2 = pow(max(dot(N, H2), 0.0), Shininess);
		point_specular = s2 * PointSpecularProduct;

		// Point Attenuation = 1 / (a + bd + cd^2)
		point_attenuation = 1.0/(Constant_Attenuation + Linear_Attenuation * dist + Quadratic_Attenuation * dist * dist);

		if (dot(L2, N) < 0.0){
			point_specular = vec4(0.0, 0.0, 0.0, 1.0);
		}
	} 
	else {
		// Spot Light Lighting
		float dist	= length(PointPosition.xyz - pos);
		vec3 L_neg	= normalize(PointPosition.xyz - PointEndPosition.xyz);
		vec3 L2		= normalize(PointPosition.xyz - pos);
		vec3 E2		= normalize(-pos);
		vec3 H2		= normalize(L2 + E2);

		float cosvalue	= dot(L_neg, L2);
		float phi		= acos(cosvalue);
		angle			= phi * 180.0 / 3.1415926535898;

		if (angle < CutOff) {
			spotLight_ambient = PointAmbientProduct;

			float d2 = max(dot(L2, N), 0.0);
			spotLight_diffuse = d2 * PointDiffuseProduct;

			float s2 = pow(max(dot(N, H2), 0.0), Shininess);
			spotLight_specular = s2 * PointSpecularProduct;

			// spotLight_attenuation = 1 / (a + bd + cd^2)
			spotLight_attenuation = (1.0/(Constant_Attenuation + Linear_Attenuation * dist + Quadratic_Attenuation * dist * dist))*pow(max(cosvalue, 0.0), Exponent);

			// Change the value of specular if necessary (l dot n) < 0
			if (dot(L2, N) < 0.0) {
				spotLight_specular = vec4(0.0, 0.0, 0.0, 1.0);
			}
		}
		else {
			// If phi > cutoff -> spotLight_attenuation = 0
			spotLight_attenuation = 0.0;
		}
	}
	gl_Position = projection * model_view * vPosition;

	// Final Result: L_a * K_a + Attenuation + Specular
	color = GlobalAmbient * SurfaceAmbient + (ambient + specular + diffuse) + point_attenuation * (point_ambient + point_specular + point_diffuse) + spotLight_attenuation * (spotLight_ambient + spotLight_specular + spotLight_diffuse);
} 