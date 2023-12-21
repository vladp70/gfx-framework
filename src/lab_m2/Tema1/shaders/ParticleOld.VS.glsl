#version 430

// Input
layout(location = 0) in vec3 v_position;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec2 v_texture_coord;

// Uniform properties
uniform mat4 Model;
uniform vec3 generator_position;
uniform float deltaTime;

uniform int gradBezier;
uniform int nrParticule;
uniform int nrDirectii;

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

float factorial(float n)
{
    if (n == 6)
        return 720;
    else if (n == 5)
        return 120;
    else if (n == 4)
        return 24;
    else if (n == 3)
        return 6;
    else if (n == 2)
        return 2;
    else if (n == 1)
        return 1;
    else 
        return 1;
}

float bernsteinFunction(int n, int i, float t)
{
    return factorial(n) / factorial(i) / factorial(n - i) * pow(t, i) * pow(1 - t, n - i);
}

vec3 ComputePosition(int id, int n, float t)
{
    vec3 pos = vec3(0);
    for (int i = 0; i <= n; i++)
    {
        pos += (bernsteinFunction(n, i, t) * control_points[id * 4 + i]);
    }
    
    return pos;
}

vec3 trr(float t)
{
    vec3 p1 = vec3(0,0,0);
    vec3 p2 = vec3(1,2,0);

    return p1 + t * (p2 - p1);
}

void main()
{
    float lifetime = data[gl_VertexID].lifetime;
    float delay = data[gl_VertexID].delay;

    delay -= deltaTime;

    if (delay > 0) 
    {
        data[gl_VertexID].delay = delay;
        gl_Position = Model * vec4(generator_position, 1);
        return;
    }

    lifetime -= deltaTime;

    float t = (data[gl_VertexID].iLifetime - lifetime) / data[gl_VertexID].iLifetime;
    int id = int(floor(gl_VertexID / (nrParticule / nrDirectii)));

    vec3 pos = ComputePosition(id, gradBezier - 1, t);

    if(lifetime < 0)
    {
        lifetime = data[gl_VertexID].iLifetime;
        delay = data[gl_VertexID].iDelay;
    }

    data[gl_VertexID].lifetime = lifetime;
    data[gl_VertexID].delay = delay;

    gl_Position = Model * vec4(pos + generator_position, 1);
}