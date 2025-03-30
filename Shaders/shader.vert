#version 330

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tex;
layout (location = 2) in vec3 norm;

out vec4 vCol;
out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;
out vec4 DirectionalLightSpacePos;

uniform mat4 model;
uniform mat4 projection;
uniform mat4 view;
uniform mat4 directionalLightTransform;  // Position of the fragment relative to the directional light

void main()
{
	gl_Position = projection * view * model * vec4(pos, 1.0);
	DirectionalLightSpacePos = directionalLightTransform * model * vec4(pos, 1.0);
	vCol = vec4(clamp(pos, 0.0f, 1.0f), 1.0f);

	TexCoord = tex;
	
	// complexe faut revoir et bien comprendre
	// On calcule un vecteur normal à partir de la position de l'objet donc c'est affecté par les changements de scale et rotation mais pas par les translations.
	// On convertit en mat3 parce que la dernière colonne correspond à la translation et que la dernière ligne n'est pas utile (revoir les opérations matricielles)
	// on fait la transposé de l'inverse de la matrice pour inverser les changements de scale et prendre en compte les scale non uniform. (recehcker àa c'est pas clair)
	Normal = mat3(transpose(inverse(model))) * norm;
	
	// Le .xyz permet de créer un vecteur3 avec les valeurs dans l'ordre spécifier donc x y puis z. swizzling
	FragPos = (model * vec4(pos, 1.0)).xyz;
}