#version 410 core

in vec3 fPosition;
in vec3 fNormal;
in vec2 fTexCoords;
in vec4 fragPosLightSpace;



out vec4 fColor;
uniform bool useUniformColor;

uniform bool useFog;

// Matrices
uniform mat4 model;
uniform mat4 view;
uniform mat3 normalMatrix;

// Lighting
uniform vec3 lightDir;
uniform vec3 lightColor;

// Textures
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform sampler2D shadowMap;

// Constants
float ambientStrength = 0.2f;

float specularStrength = 0.5f;

struct PointLight {
    vec3 position;
    vec3 color;
    float constant;
    float linear;
    float quadratic;
};

uniform PointLight pointLight;

// Directional light components
vec3 ambientDir = vec3(0.0);
vec3 diffuseDir = vec3(0.0);
vec3 specularDir = vec3(0.0);

// Point light components
vec3 ambientPoint = vec3(0.0);
vec3 diffusePoint = vec3(0.0);
vec3 specularPoint = vec3(0.0);

void computeDirLight()
{
    // Compute eye space coordinates
    vec4 fPosEye = view * model * vec4(fPosition, 1.0f);
    vec3 normalEye = normalize(normalMatrix * fNormal);

    // Normalize light direction
    vec3 lightDirN = normalize((view * vec4(lightDir, 0.0f)).xyz);

    // Compute view direction (in eye coordinates, the viewer is situated at the origin)
    vec3 viewDir = normalize(-fPosEye.xyz);

    // Compute ambient light
    ambientDir = ambientStrength * lightColor;

    // Compute diffuse light
    diffuseDir = max(dot(normalEye, lightDirN), 0.0f) * lightColor;

    // Compute specular light
    vec3 reflectDir = reflect(-lightDirN, normalEye);
    float specCoeff = pow(max(dot(viewDir, reflectDir), 0.0f), 32);
    specularDir = specularStrength * specCoeff * lightColor;
}

void computePointLight(in vec3 fragPos, in vec3 normal) 
{

    vec3 lightDir = normalize(pointLight.position - fragPos);
        float distance = length(pointLight.position - fragPos);
    
    // Atenuare
    float attenuation = 1.0 / (pointLight.constant + 
                               pointLight.linear * distance + 
                               pointLight.quadratic * (distance * distance));
    

    ambientPoint = ambientStrength * pointLight.color;


    float diff = max(dot(normal, lightDir), 0.0);
    diffusePoint = diff * pointLight.color;

    vec3 viewDir = normalize(-fragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    specularPoint = specularStrength * spec * pointLight.color;


    ambientPoint *= attenuation;
    diffusePoint *= attenuation;
    specularPoint *= attenuation;
}

float computeFog(vec3 fragPosEyeSpace)
{
 float fogDensity = 0.003f;
 float fragmentDistance = length(fragPosEyeSpace);
 float fogFactor = exp(-pow(fragmentDistance * fogDensity, 2));

 return clamp(fogFactor, 0.0f, 1.0f);
}

float computeShadow() {
	vec3 normalizedCoords = fragPosLightSpace.xyz / fragPosLightSpace.w; 

	normalizedCoords = normalizedCoords * 0.5 + 0.5; 

	float closestDepth = texture(shadowMap, normalizedCoords.xy).r; 

	if (normalizedCoords.z > 1.0f) 
		return 0.0f; 
	float currentDepth = normalizedCoords.z; 

    float bias = 0.005f;
	float shadow = currentDepth - bias > closestDepth  ? 1.0f : 0.0f; 

	return shadow;
}


void main() 
{

    vec3 fragPosWorld = vec3(model * vec4(fPosition, 1.0f));
    vec3 normalWorld = normalize(normalMatrix * fNormal);

    float shadow = computeShadow();

    computeDirLight();
    computePointLight(fragPosWorld, normalWorld);


    vec3 ambient = ambientDir + ambientPoint;
    vec3 diffuse = diffuseDir + diffusePoint;
    vec3 specular = specularDir + specularPoint;

    if (useUniformColor) {
      
        fColor = vec4(ambient + diffuse + specular + vec3(0.0f, 0.0f, 0.2f), 1.0f);
    } else {
        vec3 color = min((ambient + (1.0f-shadow) * diffuse) * texture(diffuseTexture, fTexCoords).rgb 
                     + (1.0f-shadow) * specular * texture(specularTexture, fTexCoords).rgb, 1.0f);

        fColor = vec4(color, 1.0f);

        if(useFog){
            vec3 fragPosEyeSpace = vec3(view * vec4(fragPosWorld,1.0f));
            float fogFactor = computeFog(fragPosEyeSpace);
            vec4 fogColor = vec4(0.5f,0.5f,0.5f, 1.0f);
            fColor = mix(fogColor,fColor,fogFactor);
        }

    }

   
}
