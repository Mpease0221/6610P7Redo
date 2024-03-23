#version 330 core

layout(location = 0) out vec4 color;

// The vertice's texture sample.
uniform sampler2D tex;

// Blinn-phong properties.
uniform vec3 lightPos;
uniform vec3 viewPos;
in vec3 fragPos;

// The norm in the model-view space.
in vec3 fixedNorm;

// Properties for shadow mapping.
uniform sampler2D shadowMap;
in vec4 fragPosLightSpace;

float ShadowCalculation(vec4 lightSpacePos){
    // Manually perform the perspective divide to get the lightSpacePos from [-w,w] to [-1,1].
    // This is only useful for projection matrices, which is what we are using!
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    // The depth map is from [0,1].
    // We need to transform our range of [-1,1] to [0,1]!
    projCoords = projCoords * 0.5 + 0.5;
    // We can now sample the depth map.
    // Get the point that is closest to the light source.
    // I guess the depth is stored in the red channel?
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    // We can sample the ACTUAL depth at this point like this:
    float realDepth = projCoords.z;
    // If the realDepth is greater than the closestDepth, the frag is in shadow.
    // This feels like the opposite of what I would expect...
    float shadow = realDepth > closestDepth ? 1.0 : 0.0;
    return shadow;
}


void main()
{
    //vec3 texColor = texture(tex, texCoord.xy).rgb;
    vec3 texColor = vec3(1,0.2,0.3);
    vec3 normal = normalize(fixedNorm);
    vec3 lightColor = vec3(1.f);
    vec3 ambient = 0.2 * lightColor;
    // lambertian diffuse.
    vec3 lightDir = normalize(lightPos - fragPos);
    float diffusion = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diffusion * lightColor;
    // Specular reflections
    vec3 viewDir = normalize(viewPos - fragPos);
    // learnopengl is using the more realistic looking half angle calculation. Neat!
    // This is a really cool way to do that!
    vec3 halfwayVec = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayVec), 0.0), 64.0);
    vec3 specularColor = spec * lightColor;
    // Calculates the shadow values.
    float shadow = ShadowCalculation(fragPosLightSpace);
    // We don't affect the ambient color by the shadow at all.
    // That would take away the significance of the ambient light!
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specularColor)) * texColor;

    color = vec4(lighting, 1);

}