#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

struct Light { vec3 position; vec3 color; float intensity; };

uniform Light light;
uniform vec3 viewPos;
uniform sampler2D tex;
uniform vec3 materialDiffuse;
uniform vec3 materialSpecular;
uniform float shininess;

void main() {
  vec3 norm = normalize(Normal);
  vec3 lightDir = normalize(light.position - FragPos);
  float diff = max(dot(norm, lightDir), 0.0);

  // Blinn-Phong: use half-vector
  vec3 viewDir = normalize(viewPos - FragPos);
  vec3 halfDir = normalize(lightDir + viewDir);
  float spec = pow(max(dot(norm, halfDir), 0.0), shininess);

  float mainIntensity = 1.5; // always-on scene light
  vec3 ambient = 0.1 * materialDiffuse * light.color;
  vec3 diffuse = diff * materialDiffuse * light.color * mainIntensity;
  vec3 specular = spec * materialSpecular * light.color * mainIntensity;
  vec3 color = ambient + diffuse + specular;

  vec4 texColor = texture(tex, TexCoord);
  FragColor = vec4(color, 1.0) * texColor;
}
