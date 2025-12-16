// main.cpp
// Rainbow demo with Realistic Mountains (Lighting, Snow, Texture), Grass, and Meandering River

#include <iostream>
#include <cmath>
#include <algorithm>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

// ... (compileShader 和 linkProgram) ...
GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) { char buf[1024]; glGetShaderInfoLog(s, 1024, nullptr, buf); std::cerr << "Shader compile error: " << buf << std::endl; }
    return s;
}
GLuint linkProgram(const char* vsSrc, const char* fsSrc) {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vsSrc); GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsSrc);
    GLuint prog = glCreateProgram(); glAttachShader(prog, vs); glAttachShader(prog, fs); glLinkProgram(prog);
    GLint ok; glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) { char buf[1024]; glGetProgramInfoLog(prog, 1024, nullptr, buf); std::cerr << "Link error: " << buf << std::endl; }
    glDeleteShader(vs); glDeleteShader(fs); return prog;
}

const char* vsSrc = R"GLSL(
#version 330 core
layout(location=0) in vec2 aPos;
out vec2 vUV;
void main(){ vUV = aPos * 0.5 + 0.5; gl_Position = vec4(aPos, 0.0, 1.0); }
)GLSL";

// --- 包含高级山脉、蜿蜒河流的片段着色器 ---
const char* fsSrc = R"GLSL(
#version 330 core
out vec4 FragColor;
in vec2 vUV;
uniform vec3 sunDir;
uniform float rainAmount;
uniform vec2 resolution;
uniform float uTime; 

// --- 噪声辅助函数 ---
float hash(vec2 p) { 
    p = fract(p * vec2(123.34, 456.21)); 
    p += dot(p, p + 45.32); 
    return fract(p.x * p.y); 
}

float noise(vec2 p) { 
    vec2 i = floor(p); 
    vec2 f = fract(p); 
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i + vec2(0.0, 0.0)), 
                   hash(i + vec2(1.0, 0.0)), u.x), 
               mix(hash(i + vec2(0.0, 1.0)), 
                   hash(i + vec2(1.0, 1.0)), u.x), u.y); 
}

float fbm(vec2 p) { 
    float v = 0.0; 
    float a = 0.5; 
    mat2 rot = mat2(cos(0.5), sin(0.5), -sin(0.5), cos(0.5));
    for (int i = 0; i < 6; ++i) { 
        v += a * noise(p); 
        p = rot * p * 2.0 + vec2(100.0); 
        a *= 0.5; 
    } 
    return v; 
}

// --- 山脉高度函数 ---
float getMountainHeight(float x, float seed) {
    x += seed * 100.0;
    float h = 0.0;
    mat2 rot = mat2(cos(0.3), -sin(0.3), sin(0.3), cos(0.3));

    h += noise(rot * vec2(x * 2.0, seed * 10.0)) * 0.2;
    h += fbm(rot * vec2(x * 6.0 + 10.0, seed * 20.0)) * 0.1;
    h += fbm(rot * vec2(x * 20.0 - 5.0, seed * 30.0)) * 0.05;
    h += fbm(rot * vec2(x * 10.0, h * 5.0 + seed * 5.0)) * 0.03;

    return max(h - 0.02, -0.01);
}

// --- 彩虹辅助函数 ---
vec3 wavelengthToRGB(float lambda) { 
    float r=0.0,g=0.0,b=0.0; 
    if(lambda<440.0){r=-(lambda-440.0)/(440.0-380.0);b=1.0;} 
    else if(lambda<490.0){g=(lambda-440.0)/(490.0-440.0);b=1.0;} 
    else if(lambda<510.0){g=1.0;b=-(lambda-510.0)/(510.0-490.0);} 
    else if(lambda<580.0){r=(lambda-510.0)/(580.0-510.0);g=1.0;} 
    else if(lambda<645.0){r=1.0;g=-(lambda-645.0)/(645.0-580.0);} 
    else{r=1.0;} 
    float factor=1.0; 
    if(lambda<420.0) factor=0.3+0.7*(lambda-380.0)/(420.0-380.0); 
    if(lambda>700.0) factor=0.3+0.7*(780.0-lambda)/(780.0-700.0); 
    return vec3(r,g,b)*factor; 
}
vec3 spectralRainbow(float thetaDeg, float baseRadiusDeg, float dispersion, float widthDeg, out float intensityOut) { 
    float lamStart=380.0; 
    float lamEnd=780.0; 
    int samples=9; 
    vec3 accum=vec3(0.0); 
    float totalWeight=0.0; 
    float intensity=0.0; 
    for(int i=0;i<samples;i++){ 
        float t=float(i)/(samples-1); 
        float lam=mix(lamStart,lamEnd,t); 
        float center=baseRadiusDeg+(lam-530.0)*dispersion; 
        float d=thetaDeg-center; 
        float g=exp(-0.5*(d*d)/(widthDeg*widthDeg)); 
        vec3 col=wavelengthToRGB(lam); 
        accum+=col*g; 
        totalWeight+=g; 
        intensity+=g; 
    } 
    intensityOut=intensity/float(samples); 
    if(totalWeight>0.0) accum/=totalWeight; 
    return accum*intensityOut; 
}


// --- 核心修改：生成弯弯曲曲的河流路径 ---
float riverPath(vec2 pos, float time) {
    // pos.y 是距离 (Z轴)， pos.x 是左右位置 (X轴)
    
    // 1. 构建弯曲的中心线 (Centerline)
    // 使用正弦波叠加来模拟自然的河流蜿蜒
    // 频率随距离减小（远处弯曲较缓），振幅随距离减小（透视效果）
    float curve = 0.0;
    curve += sin(pos.y * 0.5) * 0.5;   // 大弯曲
    curve += sin(pos.y * 1.5) * 0.2;   // 中等弯曲
    curve += sin(pos.y * 3.0) * 0.05;  // 小细节
    
    // 关键：让河流在远处回归中心，不要偏离太远
    // smoothstep(0.0, 15.0, pos.y) 控制衰减范围
    float attenuation = 1.0 - smoothstep(5.0, 20.0, pos.y);
    float centerOffset = curve * attenuation; 
    
    // 2. 计算当前点到中心线的距离
    float distToCenter = abs(pos.x - centerOffset);
    
    // 3. 计算河流宽度 (近大远小)
    // 近处宽度 0.6，远处宽度 0.05
    float width = mix(0.6, 0.05, smoothstep(0.0, 15.0, pos.y));
    
    // 4. 生成河流遮罩 (边缘平滑)
    // 使用 smoothstep 创建柔和的河岸边缘
    float edgeSoftness = 0.05 + width * 0.1; 
    float mask = smoothstep(width + edgeSoftness, width, distToCenter);
    
    return mask;
}

void main() {
    vec2 ndc = (vUV * 2.0 - 1.0);
    float aspect = resolution.x / resolution.y;
    float fov = radians(75.0);
    vec3 viewDir = normalize(vec3(ndc.x * tan(fov * 0.5) * aspect, ndc.y * tan(fov * 0.5), -1.0));

    // --- 1. 深色天空 ---
    vec3 skyTop = vec3(0.1, 0.2, 0.4);
    vec3 skyHorizon = vec3(0.4, 0.5, 0.6);
    float tsky = pow(max(viewDir.y * 0.5 + 0.5, 0.0), 0.7);
    vec3 skyColor = mix(skyHorizon, skyTop, tsky);
    float sunAngle = degrees(acos(clamp(dot(viewDir, normalize(sunDir)), -1.0, 1.0)));
    float sunGlow = exp(-0.5 * (sunAngle * sunAngle) / (3.0 * 3.0));
    skyColor += vec3(1.0, 0.8, 0.5) * sunGlow * 1.2 * (1.0 - rainAmount * 0.5);

    vec3 finalColor = skyColor;

    // --- 2. 渲染多层山脉 ---
    int numLayers = 5;
    
    for (int i = numLayers - 1; i >= 0; --i) {
        float layerT = float(i) / float(numLayers);
        float seed = layerT * 23.0;

        float heightScale = 1.0 - layerT * 0.3;
        float verticalShift = -layerT * 0.04;
        float xScaled = viewDir.x / (1.0 + layerT * 1.5);

        float mtHeight = getMountainHeight(xScaled, seed) * heightScale + verticalShift;

        if (viewDir.y > -0.01 && viewDir.y < 0.5) {
            float horizonY = viewDir.y;

            if (horizonY < mtHeight) {
                float eps = 0.002;
                float hL = getMountainHeight((viewDir.x - eps) / (1.0 + layerT * 1.5), seed) * heightScale + verticalShift;
                float hR = getMountainHeight((viewDir.x + eps) / (1.0 + layerT * 1.5), seed) * heightScale + verticalShift;
                
                vec3 tangent = normalize(vec3(eps * 2.0 / (1.0 + layerT * 1.5), hR - hL, 0.0));
                vec3 normal = cross(vec3(0.0, 0.0, 1.0), tangent);
                normal = normalize(normal);

                float diffuse = max(dot(normal, normalize(sunDir)), 0.0);
                diffuse = pow(diffuse, 2.0);

                float ambient = 0.6;
                float valleyDepth = max(0.0, 0.2 - mtHeight);
                float aoFactor = 1.0 - pow(valleyDepth * 5.0, 2.0) * 0.5;
                aoFactor = clamp(aoFactor, 0.5, 1.0);
                float lighting = diffuse * 0.4 + ambient * aoFactor;

                vec3 snowColor = vec3(0.9, 0.9, 0.95);
                vec3 rockColor = vec3(0.3, 0.25, 0.2);
                vec3 slopeColor = vec3(0.2, 0.15, 0.1);

                vec3 mtColor = rockColor;
                float dynamicFrequency = mix(1.0, 2.0, smoothstep(0.0, 0.2, mtHeight));
                float verticalNoise = noise(vec2(xScaled * dynamicFrequency, horizonY * 15.0));
                mtColor = mix(mtColor, slopeColor, smoothstep(0.6, 0.9, verticalNoise) * 0.5);
                mtColor = mix(mtColor, rockColor, smoothstep(0.3, 0.7, normal.y));

                if (layerT > 0.6) {
                    float vegetation = smoothstep(0.0, 0.1, mtHeight);
                    vec3 grassColor = vec3(0.1, 0.35, 0.1);
                    mtColor = mix(mtColor, grassColor, vegetation);
                }
                
                float dynamicSnowLine = smoothstep(0.1, 0.2 + rainAmount * 0.05, mtHeight);
                dynamicSnowLine *= (1.0 - layerT * 0.3);
                mtColor = mix(mtColor, snowColor, dynamicSnowLine);
                mtColor *= lighting;
                
                vec3 distantTint = vec3(0.1, 0.2, 0.35);
                mtColor = mix(mtColor, distantTint, pow(1.0 - layerT, 3.0) * 0.4);

                float simulatedDistance = 1.0 + layerT * 4.0;
                float fogFactor = smoothstep(0.0, 1.0, simulatedDistance * 0.02);
                float fogDensity = 4.0 + rainAmount * 5.0;
                fogFactor = 1.0 - exp(-fogFactor * fogDensity);
                
                finalColor = mix(mtColor, skyHorizon, fogFactor);
                break;
            }
        }
    }

    // --- 3. 草地逻辑 (包含蜿蜒河流) ---
    if (viewDir.y < -0.01) {
        float camHeight = 2.0;
        float dist = camHeight / abs(viewDir.y);
        vec2 groundPos = viewDir.xz * dist;

        // --- 草地基础颜色 ---
        float grassBase = fbm(groundPos * 1.5);
        float grassDetail = noise(groundPos * 30.0);
        vec3 dirtColor = vec3(0.2, 0.15, 0.1);
        vec3 dryGrass = vec3(0.3, 0.35, 0.1);
        vec3 healthyGrass = vec3(0.05, 0.25, 0.05);
        vec3 groundColor = mix(healthyGrass, dryGrass, grassBase * 0.7);
        groundColor = mix(dirtColor, groundColor, smoothstep(0.2, 0.4, grassBase));
        groundColor *= (0.7 + 0.6 * grassDetail);

        // --- 河流 ---
        float riverMask = riverPath(groundPos, uTime);
        
        // 河流颜色和水面动态效果
        vec3 waterBaseColor = vec3(0.1, 0.25, 0.5); // 深蓝色基底
        
        // 模拟水流波纹 (沿着河流方向流动)
        // 使用 time 让波纹移动
        float flowSpeed = 1.0;
        vec2 flowUV = groundPos * 0.8 + vec2(0.0, uTime * flowSpeed); 
        float waterNoise = fbm(flowUV);
        
        // 反射天空
        vec3 waterColor = mix(waterBaseColor, skyColor, 0.4);
        waterColor += waterNoise * 0.1; // 添加波光粼粼

        // 混合草地和河流
        // 使用 mask 平滑过渡
        finalColor = mix(groundColor, waterColor, riverMask);

        // --- 雾效 ---
        float fogFactor = 1.0 - exp(-dist * 0.02);
        finalColor = mix(finalColor, skyHorizon, clamp(fogFactor, 0.0, 1.0));
    } else {
        // --- 天空与彩虹逻辑 ---
        vec3 antiSun = -normalize(sunDir);
        float cosTheta = dot(viewDir, antiSun);
        float theta = degrees(acos(clamp(cosTheta, -1.0, 1.0)));
        float primaryRadius = 42.0;
        float secondaryRadius = 51.0;
        float dispersion = 0.018;
        float widthDegPrimary = mix(0.7, 2.5, rainAmount);
        float widthDegSecondary = widthDegPrimary * 1.6;

        float primIntensity;
        vec3 primCol = spectralRainbow(theta, primaryRadius, dispersion, widthDegPrimary, primIntensity);
        primIntensity *= clamp(rainAmount * 2.0, 0.0, 1.0);
        float horizonFade = smoothstep(-0.1, 0.1, viewDir.y);
        primIntensity *= horizonFade;

        float secIntensity;
        vec3 secCol = spectralRainbow(theta, secondaryRadius, -dispersion * 1.05, widthDegSecondary, secIntensity);
        secIntensity *= clamp(rainAmount * 1.0, 0.0, 0.7);
        secIntensity *= horizonFade;

        float darken = smoothstep(primaryRadius - 8.0, primaryRadius + 8.0, theta);
        finalColor = mix(finalColor, finalColor * 0.65, pow(darken, 2.0) * 0.25 * rainAmount);
        finalColor += primCol * primIntensity * 1.5;
        finalColor += secCol * secIntensity * 1.0;
    }

    // Gamma 校正
    finalColor = pow(finalColor, vec3(0.95));
    finalColor = pow(finalColor, vec3(1.0 / 2.2));
    FragColor = vec4(clamp(finalColor, 0.0, 1.0), 1.0);
}
)GLSL";

// --- main 函数 ---
int main() {
    if (!glfwInit()) { std::cerr << "glfw init failed\n"; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* win = glfwCreateWindow(1920, 1080, "Realistic Mountains Demo", nullptr, nullptr);
    if (!win) { std::cerr << "create window failed\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(win);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { std::cerr << "glad load failed\n"; return -1; }

    GLuint prog = linkProgram(vsSrc, fsSrc);
    float tri[] = { -1.0f, -1.0f, 3.0f, -1.0f, -1.0f, 3.0f };
    GLuint vao, vbo; glGenVertexArrays(1, &vao); glGenBuffers(1, &vbo); glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(tri), tri, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);

    glUseProgram(prog);
    int loc_sunDir = glGetUniformLocation(prog, "sunDir"); int loc_rain = glGetUniformLocation(prog, "rainAmount"); int loc_res = glGetUniformLocation(prog, "resolution");
    int loc_time = glGetUniformLocation(prog, "uTime");
    float timeStart = (float)glfwGetTime(); glm::vec3 sunDir = glm::normalize(glm::vec3(0.0f, 0.3f, 1.0f)); float rain = 0.8f;

    while (!glfwWindowShouldClose(win)) {
        float t = (float)glfwGetTime() - timeStart;
        int w, h;
        glfwGetFramebufferSize(win, &w, &h);
        glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(prog);
        float sunAngle = sin(t * 0.1f) * 0.5f;
        sunDir = glm::normalize(glm::vec3(sin(sunAngle), 0.2f + cos(t * 0.1f) * 0.1f, cos(sunAngle)));
        glUniform3f(loc_sunDir, sunDir.x, sunDir.y, sunDir.z);
        glUniform1f(loc_rain, rain);
        glUniform2f(loc_res, (float)w, (float)h);
        glUniform1f(loc_time, t);

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        if (glfwGetKey(win, GLFW_KEY_UP) == GLFW_PRESS) rain = std::min(rain + 0.01f, 1.0f);
        if (glfwGetKey(win, GLFW_KEY_DOWN) == GLFW_PRESS) rain = std::max(rain - 0.01f, 0.0f);

        glfwSwapBuffers(win);
        glfwPollEvents();
    }
    return 0;
}