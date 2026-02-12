#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

struct Light { vec3 position; vec3 color; float intensity; };

uniform Light light;
uniform Light lampLight;
uniform bool lampEnabled;
uniform vec3 viewPos;
uniform sampler2D tex;
uniform vec3 materialDiffuse;
uniform vec3 materialSpecular;
uniform float shininess;
uniform float uAlpha;

void main() {
  vec3 norm = normalize(Normal);
  // main light
  vec3 lightDir = normalize(light.position - FragPos);
  float diff = max(dot(norm, lightDir), 0.0);
  vec3 reflectDir = reflect(-lightDir, norm);
  vec3 viewDir = normalize(viewPos - FragPos);
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);

  float mainIntensity = 1.5; // always-on scene light
  vec3 ambient = 0.25 * materialDiffuse * light.color;
  vec3 diffuse = diff * materialDiffuse * light.color * mainIntensity;
  vec3 specular = spec * materialSpecular * light.color * mainIntensity;
  vec3 color = ambient + diffuse + specular;

  // lamp contribution (additive)
  if (lampEnabled) {
    vec3 lampVec = lampLight.position - FragPos;
    float lampDist = length(lampVec);
    vec3 lampDir = normalize(lampVec);
    float diff2 = max(dot(norm, lampDir), 0.0);
    vec3 reflectDir2 = reflect(-lampDir, norm);
    float spec2 = pow(max(dot(viewDir, reflectDir2), 0.0), shininess);
    // strong attenuation so lamp only affects a small area nearby
    float attenuation = 1.0 / (1.0 + 0.02 * lampDist * lampDist);
    vec3 diffuse2 = diff2 * materialDiffuse * lampLight.color * lampLight.intensity * attenuation;
    vec3 specular2 = spec2 * materialSpecular * lampLight.color * lampLight.intensity * attenuation;
    color += diffuse2 + specular2;
    // small ambient boost from lamp
    color += 0.03 * lampLight.color * lampLight.intensity * attenuation;
  }

  vec4 texColor = texture(tex, TexCoord);
  FragColor = vec4(color, uAlpha) * texColor;
}
