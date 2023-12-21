#version 430

// Input
layout(location = 0) in vec3 v_position;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec2 v_texture_coord;

// Uniform properties
uniform mat4 Model;
uniform vec3 generator_position;
uniform float deltaTime;

uniform vec3 control_points[20];

struct Particle
{
    vec4 position;
    vec4 speed;
    vec4 iposition;
    vec4 ispeed;
    float delay;
    float iDelay;
    float lifetime;
    float iLifetime;
};


layout(std430, binding = 0) buffer particles {
    Particle data[];
};


float rand(vec2 co)
{
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

vec3 bezier(float t)
{
    vec3 control_p0 = control_points[gl_VertexID % 5 * 4 + 0];
    vec3 control_p1 = control_points[gl_VertexID % 5 * 4 + 1];
    vec3 control_p2 = control_points[gl_VertexID % 5 * 4 + 2];
    vec3 control_p3 = control_points[gl_VertexID % 5 * 4 + 3];

    return  control_p0 * pow((1 - t), 3) +
            control_p1 * 3 * t * pow((1 - t), 2) +
            control_p2 * 3 * pow(t, 2) * (1 - t) +
            control_p3 * pow(t, 3);
}

void main()
{
    float lifetime = data[gl_VertexID].lifetime;
    float delay = data[gl_VertexID].delay;

    delay -= deltaTime;

    if (delay > 0) {
        data[gl_VertexID].delay = delay;
        gl_Position = Model * vec4(generator_position, 1);
        return;
    }

    if (data[gl_VertexID].speed.y > 0.1)
        lifetime -= deltaTime * data[gl_VertexID].speed.y;
    else
        lifetime -= deltaTime;

    float t = (data[gl_VertexID].iLifetime - lifetime) / data[gl_VertexID].iLifetime;
    vec3 pos = bezier(t);

    if(lifetime < 0)
    {
        lifetime = data[gl_VertexID].iLifetime;
        delay = data[gl_VertexID].iDelay;
    }

    data[gl_VertexID].position.xyz = pos + generator_position;
    data[gl_VertexID].lifetime = lifetime;
    data[gl_VertexID].delay = delay;

    gl_Position = Model * vec4(pos + generator_position, 1);
}