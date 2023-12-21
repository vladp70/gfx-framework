#version 410

// Input
layout(location = 0) in vec2 texture_coord;

// Uniform properties
uniform sampler2D textureImage;
uniform ivec2 screenSize;
uniform int flipVertical;
uniform int outputMode = 2; // 0: original, 1: grayscale, 2: blur

// Output
layout(location = 0) out vec4 out_color;

// Local variables
vec2 textureCoord = vec2(texture_coord.x, (flipVertical != 0) ? 1 - texture_coord.y : texture_coord.y); // Flip texture


vec4 grayscale()
{
    vec4 color = texture(textureImage, textureCoord);
    float gray = 0.21 * color.r + 0.71 * color.g + 0.07 * color.b; 
    return vec4(gray, gray, gray,  0);
}


vec4 blur(int blurRadius)
{
    vec2 texelSize = 1.0f / screenSize;
    vec4 sum = vec4(0);
    for(int i = -blurRadius; i <= blurRadius; i++)
    {
        for(int j = -blurRadius; j <= blurRadius; j++)
        {
            sum += texture(textureImage, textureCoord + vec2(i, j) * texelSize);
        }
    }
        
    float samples = pow((2 * blurRadius + 1), 2);
    return sum / samples;
}

vec4 noise (int NoiseRadius){
    vec2 texelSize = 1.0f / screenSize;
    vec4 initial[100];
    int t = 0;
    for(int i = -NoiseRadius / 2; i <= NoiseRadius / 2; i++){
        for(int j = -NoiseRadius / 2; j <= NoiseRadius / 2; j++)
        {
            t++;
            initial[t] = texture(textureImage, textureCoord + vec2(i, j) * texelSize);
        
        }
    }
    for(int i = 0; i < (NoiseRadius * NoiseRadius) - 1; i++)
    {
        for(int j = i + 1; j < NoiseRadius * NoiseRadius; j++)
        {
            //pt doi pixeli consecutivi
            float grey = 0.21 * initial[i].r * 0.71 * initial[i].g + 0.07 * initial[i].b;
            float grey_next = 0.21 * initial[j].r * 0.71 * initial[j].g + 0.07 * initial[j].b;
            if(grey < grey_next){
                vec4 aux;
                aux = initial[i];
                initial[i] = initial[j];
                initial[j] = aux;
            }
        }
    }
    vec4 result = initial[NoiseRadius * NoiseRadius / 2 + 1];
    return result;
}

vec4 blur_sobel(int si, int sj)
{
    vec2 texelSize = 1.0f / screenSize;
    vec4 sum = vec4(0);
            
    for(int i = -1; i <= 1; i++)
    {
        for(int j = -1; j <= 1; j++)
        {
            sum += texture(textureImage, textureCoord + vec2(i + si, j + sj) * texelSize);
        }
    }
    
    float samples = pow((2 * 1 + 1), 2);
    return sum / samples;
}

vec4 sobel()
{
    vec2 texelSize = 1.0f / screenSize;
    
    vec3 vals[9];

    for(int i = -1; i <= 1; i++)
    {
        for(int j = -1; j <= 1; j++)
        {
            vals[3 * (i + 1) + (j + 1)] = blur_sobel(i, j).rgb;
        }
    }

    float vg[9];

    for(int i = 0; i < 9; i++)
    {
        vg[i] = 0.21 * vals[i].r + 0.71 * vals[i].g + 0.07 * vals[i].b;   
    }

    /*
          0 1 2
        0 0 1 2
        1 3 4 5
        2 6 7 8
    */

    float dx = abs(-vg[0] + vg[2] - 2 * vg[3] + 2 * vg[5] - vg[6] + vg[8]);
    float dy = abs(vg[0] + 2 * vg[1] + vg[2] - vg[6] - 2 * vg[7] - vg[8]);

    float d = dx + dy;

    if (d > 0.5)
    {
        d = 1.0;
    } 
    else
    {
        d = 0.0;
    }

    return vec4(d, d, d, 0);
}

void main()
{
    switch (outputMode)
    {
        case 1:
        {
            out_color = grayscale();
            break;
        }

        case 2:
        {
            out_color = blur(3);
            break;
        }

        case 3:
        {
            out_color = noise(5);
            break;
        }
        case 4:
        {
            out_color = sobel();
            break;
        }

        default:
            out_color = texture(textureImage, textureCoord);
            break;
    }
}
