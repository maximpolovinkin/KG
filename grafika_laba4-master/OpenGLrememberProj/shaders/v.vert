varying vec2 texCoord; 
varying vec3 Normal;
varying vec3 Position;

uniform vec3 light_pos;
uniform vec3 camera;
uniform mat4 iViewMatrix;

varying vec3 light_pos_iv;
varying vec3 camera_iv;

void main(void) 
{     
	   
	gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * gl_Vertex;   //умножаем матрицу проекции на видовую матрицу и на координаты точки
	Position = (gl_ModelViewMatrix * gl_Vertex).xyz;


	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;   
	texCoord = gl_TexCoord[0].xy;   //считываем текстурые координаты в варинг

	light_pos_iv = (iViewMatrix*vec4(light_pos,1.0)).xyz;
	camera_iv = (iViewMatrix*vec4(camera,1.0)).xyz;

	Normal = normalize(gl_NormalMatrix * gl_Normal) ; /*gl_NormalMatrix*/
}

