#version 110

#include "particle.h"

#include "common/los_vertex.h"
#include "common/vertex.h"

VERTEX_INPUT_ATTRIBUTE(0, vec3, a_vertex);
VERTEX_INPUT_ATTRIBUTE(1, vec4, a_color);
VERTEX_INPUT_ATTRIBUTE(2, vec2, a_uv0);
VERTEX_INPUT_ATTRIBUTE(3, vec2, a_uv1);
VERTEX_INPUT_ATTRIBUTE(4, vec3, a_axisX);
VERTEX_INPUT_ATTRIBUTE(5, vec3, a_axisY);

void main()
{
  vec3 viewAxisX = vec3(modelViewMatrix[0][0], modelViewMatrix[1][0], modelViewMatrix[2][0]);
  vec3 viewAxisY = vec3(modelViewMatrix[0][1], modelViewMatrix[1][1], modelViewMatrix[2][1]);
  vec2 offset = a_uv1;

  vec3 axisX = (spaceTransform * vec4(a_axisX, 0.0)).xyz;
  vec3 axisY = (spaceTransform * vec4(a_axisY, 0.0)).xyz;
  vec3 particlePosition = (spaceTransform * vec4(a_vertex, 1.0)).xyz;

  vec3 particleAxisX = viewAxisX;
  vec3 particleAxisY = viewAxisY;
  if (a_axisX != vec3(0.0))
  {
    particleAxisX = axisX;
    if (a_axisY != vec3(0.0))
      particleAxisY = axisY;
    else
    {
      vec3 particleDirection = particlePosition - cameraPos;
      float particleDirectionLength = length(particleDirection);
      if (particleDirectionLength != 0.0)
      {
        particleDirection *= 1.0 / particleDirectionLength;
        if (abs(dot(axisX, particleDirection)) < 1.0)
          particleAxisY = normalize(cross(particleDirection, axisX));
      }
    }
  }

  vec3 position = particleAxisX*offset.x + particleAxisX*offset.y + particleAxisY*offset.x + particleAxisY*-offset.y + particlePosition;

  OUTPUT_VERTEX_POSITION(transform * vec4(position, 1.0));

  v_tex = a_uv0;
  v_color = a_color;
#if !IGNORE_LOS
  v_los = calculateLOSCoordinates(position.xz, losTransform);
#endif
}
