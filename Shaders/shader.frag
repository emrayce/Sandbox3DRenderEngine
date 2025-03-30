#version 330

in vec4 vCol;
in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;
in vec4 DirectionalLightSpacePos;

out vec4 colour;

const int MAX_POINT_LIGHTS = 3;
const int MAX_SPOT_LIGHTS = 3;

struct Light
{
	vec3 colour;
	float ambientIntensity;
	float diffuseIntensity;
};

struct DirectionalLight 
{
	Light base;
	vec3 direction;
};

struct PointLight
{
	Light base;
	vec3 position;
	float constant;
	float linear;
	float exponent;	
};

struct SpotLight
{
	PointLight base;
	vec3 direction;
	float edge;
};

struct OmniShadowMap
{
	samplerCube shadowMap;
	float farPlane;
};

struct Material
{
	float specularIntensity;
	float shininess;
};

uniform int pointLightCount;
uniform int spotLightCount;

uniform DirectionalLight directionalLight;
uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform SpotLight spotLights[MAX_SPOT_LIGHTS];

uniform sampler2D theTexture;
uniform sampler2D directionalShadowMap;
uniform OmniShadowMap omniShadowMaps[MAX_POINT_LIGHTS + MAX_SPOT_LIGHTS];

uniform Material material;

uniform vec3 eyePosition;

vec3 sampleOffsetDirections[20] = vec3[]
(
	vec3( 1,  1,  1), vec3( 1, -1,  1), vec3( -1, -1,  1), vec3( -1,  1,  1),
	vec3( 1,  1, -1), vec3( 1, -1, -1), vec3( -1, -1, -1), vec3( -1,  1, -1),
	vec3( 1,  1,  0), vec3( 1, -1,  0), vec3( -1, -1,  0), vec3( -1,  1,  0),
	vec3( 1,  0,  1), vec3(-1,  0,  1), vec3(  1,  0, -1), vec3( -1,  0, -1),
	vec3( 0,  1,  1), vec3( 0, -1,  1), vec3(  0, -1, -1), vec3(  0,  1, -1)
);

float CalcDirectionalShadowFactor(DirectionalLight light)
{
	vec3 projCoords = DirectionalLightSpacePos.xyz / DirectionalLightSpacePos.w; // Convert value to range [-1; 1]
	projCoords = (projCoords * 0.5) + 0.5; // Convert value to range [0; 1]
	
	float closestDepth = texture(directionalShadowMap, projCoords.xy).r;
	float currentDepth = projCoords.z; // depth value of the current fragment
	
	vec3 normal = normalize(Normal);
	vec3 lightDir = normalize(light.direction);
	
	float bias = max(0.002 * (1 - dot(normal, lightDir)), 0.001);
	
	float shadow = 0.0;
	vec2 texelSize = 1.0 / textureSize(directionalShadowMap, 0);
	for (int x = -1; x <= 1; ++x)
	{
		for (int y = -1; y <= 1; ++y)
		{
			float pcfDepth = texture(directionalShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
			shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
		}
	}
	shadow /= 9; // because we compared with 9 pixels
		
	if (projCoords.z > 1.0)
	{
		shadow = 0.0;
	}
	
	return shadow;
}

float CalcOmniShadowFactor(PointLight light, int shadowIndex)
{
	vec3 fragToLight = FragPos - light.position;
	float currentDepth = length(fragToLight);
	
	float shadow = 0.0;
	float bias = 0.05;
	int samples = 20;
	
	float viewDistance = length(eyePosition - FragPos);
	float diskRadius = (1.0 + (viewDistance / omniShadowMaps[shadowIndex].farPlane)) / 25.0; // 25 is arbitrary
	
	for (int i = 0; i < samples; i++)
	{
		// Get the depthValue from the shadowMap
		float closestDepth = texture(omniShadowMaps[shadowIndex].shadowMap, fragToLight + sampleOffsetDirections[i] * diskRadius).r;
		// We divided by farPlane in the omni_shadow_map.frag to get a value between [0-1]. so now we multiply by farPlane to get the real value; 
		closestDepth *= omniShadowMaps[shadowIndex].farPlane;
		
		if (currentDepth - bias > closestDepth)
		{ 	
			shadow += 1.0;
		}
	}
	
	shadow /= float(samples);
	
	return shadow;
}

// Ce code est utilisé autant pour la directional light que pour la pointLigth.
// On calcule l'éclairage du fragment selon une lumière et une direction donnée.
vec4 CalcLightByDirection(Light light, vec3 direction, float shadowFactor)
{
	vec4 ambientColour = vec4(light.colour, 1.0f) * light.ambientIntensity;
	
	// Rappel on normalize les vecteurs pour trouver directement le cosinus de l'angle
	// A.B = |A||B|cos(angle)
	// On utilise max pour renvoyer la plus grande valeur et donc ne jamais être en dessous de 0;
	// Si < 0 alors on est en dessous de la face de l'objet. "we don't want to light it up the surface if it starts going underneath it"
	float diffuseFactor = max(dot(normalize(Normal), normalize(direction)), 0.0f);
	//vec4 diffuseColour = vec4(light.colour * light.diffuseIntensity * diffuseFactor, 1.0f);
	vec4 diffuseColour = vec4(light.colour * light.diffuseIntensity * diffuseFactor, 1.0f);
	
	vec4 specularColour = vec4(0, 0, 0, 0);
	// si pas affecté par la lumière diffuse alors pas affecté par la lumière specualire
	if(diffuseFactor > 0.0f)
	{
		// Créer le vecteur pour la direction fragment à la camera
		vec3 fragToEye = normalize(eyePosition - FragPos);
		//				  reflectedVertex  \   / rayon de lumière
		// Le rayon de lumière réfléchi    _\_/_ objet
		vec3 reflectedVertex = normalize(reflect(direction, normalize(Normal)));
		
		// Produit scalaire entre la direction du fragment à la caméra et du rayon de lumière réfléchi.
		// Si les 2 vecteurs sont identiques dans ce cas on est ébloui et toute la lumière est renvoyé vers la caméra pour ce point
		// On utilise le produit scalaire pour avoir l'angle comme d'hab
		float specularFactor = dot(fragToEye, reflectedVertex);
		if (specularFactor > 0.0f)
		{
			specularFactor = pow(specularFactor, material.shininess);
			specularColour = vec4(light.colour * material.specularIntensity * specularFactor, 1.0f);
		}
	}

	return (ambientColour + (1.0 - shadowFactor) * (diffuseColour + specularColour));
}

vec4 CalcDirectionalLight()
{
	float shadowFactor = CalcDirectionalShadowFactor(directionalLight);
	return CalcLightByDirection(directionalLight.base, directionalLight.direction, shadowFactor);
}

vec4 CalcPointLight(PointLight pLight, int shadowIndex)
{
	vec3 direction = FragPos - pLight.position;
	float distance = length(direction); 
	direction = normalize(direction);
	
	float shadowFactor = CalcOmniShadowFactor(pLight, shadowIndex);

	vec4 colour = CalcLightByDirection(pLight.base, direction, shadowFactor);
	// ax² + bx +c
	float attenuation = pLight.exponent * distance * distance +
						pLight.linear * distance +
						pLight.constant;
	// += because we calculate for all point lights

	return (colour / attenuation);
}

vec4 CalcSpotLight(SpotLight sLight, int shadowIndex)
{
	// The direction between fragment and the spot Light
	vec3 rayDirection = normalize(FragPos - sLight.base.position);
	// Calcule le produit scalaire que l'on va comparer avec le edge pour voir si le fragment est affecté par le spot
	float slFactor = dot(rayDirection, sLight.direction);
	
	if (slFactor > sLight.edge)
	{
		vec4 colour = CalcPointLight(sLight.base, shadowIndex);
		
		return colour * (1.0f - (1.0f - slFactor)*(1.0f/(1.0f - sLight.edge)));
	}
	else
	{
		return vec4(0, 0, 0, 0);
	}
}

vec4 CalcSpotLights()
{
	vec4 totalColour = vec4(0, 0, 0, 0);
	for (int i = 0; i < spotLightCount; i++)
	{
		totalColour += CalcSpotLight(spotLights[i], i + pointLightCount); // i + pointLightCount because we setted up the pointlights before the spotlights
	}
	
	return totalColour;
}

vec4 CalcPointLights()
{
	vec4 totalColour = vec4(0, 0, 0, 0);
	for (int i = 0; i < pointLightCount; i++)
	{
		totalColour += CalcPointLight(pointLights[i], i);
	}
	
	return totalColour;
}

void main()	
{
	vec4 finalColour = CalcDirectionalLight();
	finalColour += CalcPointLights();
	finalColour += CalcSpotLights();

	colour = texture(theTexture, TexCoord) * finalColour;
}
