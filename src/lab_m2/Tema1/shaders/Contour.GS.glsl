#version 430

layout(triangles) in;
layout(line_strip, max_vertices = 12) out;

uniform mat4 View;
uniform mat4 Projection;
uniform mat4 viewMatrices[6];

in vec3 world_normal[3];

const vec3[6] viewVectors = {vec3(1,0,0), vec3(-1,0,0), vec3(0,1,0), vec3(0,-1,0), vec3(0,0,1), vec3(0,0,-1)};

bool IsCornerTowardsViewer(vec3 normal, vec3 viewVector)
{
    return (dot(normal, viewVector)) <= 0.001;
}

vec4 myInterpolate(vec4 p1, vec4 p2, float dot1, float dot2)
{
    return p1 + (abs(dot1) / (abs(dot1) + abs(dot2))) * (p2 - p1);
}

void EmitContourVertex(int x, int y, int layer)
{
    vec4 interpolatedPos = myInterpolate(gl_in[x].gl_Position, gl_in[y].gl_Position,
        dot(world_normal[x], viewVectors[layer]), dot(world_normal[y], viewVectors[layer]));
    gl_Position = Projection * viewMatrices[layer] * interpolatedPos;
    EmitVertex();
}

void main()
{
    int layer;

    for(layer = 0; layer < 6; layer++)
    {
        gl_Layer = layer;

        bool p1TowardsViewer = IsCornerTowardsViewer(world_normal[0], viewVectors[layer]);
        bool p2TowardsViewer = IsCornerTowardsViewer(world_normal[1], viewVectors[layer]);
        bool p3TowardsViewer = IsCornerTowardsViewer(world_normal[2], viewVectors[layer]);

        if (!(p1TowardsViewer && p2TowardsViewer && p3TowardsViewer)
            && (p1TowardsViewer || p2TowardsViewer || p3TowardsViewer))
        {
            if (p1TowardsViewer != p2TowardsViewer)
            {
                EmitContourVertex(0, 1, layer);
            }
            if (p2TowardsViewer != p3TowardsViewer)
            {
                EmitContourVertex(1, 2, layer);
            }
            if (p3TowardsViewer != p1TowardsViewer)
            {
                EmitContourVertex(2, 0, layer);
            }
            EndPrimitive();
        }
    }
}
