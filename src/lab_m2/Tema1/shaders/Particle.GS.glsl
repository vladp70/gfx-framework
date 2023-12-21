#version 430

// Input and output topologies
layout(points) in;
layout(triangle_strip, max_vertices = 24) out;

uniform float offset;
uniform vec3 Position;
uniform mat4 view_matrices[6];
uniform vec3 up_vectors[6];
uniform mat4 Projection;

layout(location = 0) out vec2 texture_coord;

vec3 vpos = gl_in[0].gl_Position.xyz;

void EmitPoint(vec2 offset, vec3 right, int layer)
{
    vec3 pos = right * offset.x + up_vectors[layer] * offset.y + vpos;
    gl_Position = Projection * view_matrices[layer] * vec4(pos, 1.0);
    EmitVertex();
}

void main()
{
    float ds = offset;
    int layer;
    for (layer = 0; layer < 6; layer++)
    {
        gl_Layer = layer;

        vec3 forward = normalize(Position - vpos);
        vec3 right = normalize(cross(forward, up_vectors[layer]));
        
        texture_coord = vec2(0, 0);
        EmitPoint(vec2(-ds, -ds), right, layer);

        texture_coord = vec2(1, 0);
        EmitPoint(vec2( ds, -ds), right, layer);
    
        texture_coord = vec2(0, 1);
        EmitPoint(vec2(-ds,  ds), right, layer);
    
        texture_coord = vec2(1, 1);
        EmitPoint(vec2( ds,  ds), right, layer);

        EndPrimitive();
    }
}