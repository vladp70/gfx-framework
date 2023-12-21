#version 410

// Input
layout(location = 0) in vec2 texture_coord;

// Uniform properties
uniform sampler2D textureImage;

// Output
layout(location = 0) out vec4 out_color;

vec4 blur(int blurRadius)
{
    vec2 texelSize = vec2(1.0f / 1024, 1.0f / 1024);
    vec4 sum = vec4(0);
    for(int i = -blurRadius; i <= blurRadius; i++)
    {
        for(int j = -blurRadius; j <= blurRadius; j++)
        {
            sum += texture(textureImage, texture_coord + vec2(i, j) * texelSize);
        }
    }
        
    float samples = pow((2 * blurRadius + 1), 2);
    return sum / samples;
}

void main()
{
    out_color = blur(3);
}
