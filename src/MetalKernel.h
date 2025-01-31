const char* metal_src_MetalKernel =
"//\n"
"//  Reframe360Kernel.metal\n"
"//  Reframe360Kernel\n"
"//\n"
"//  Created by Ronan LE MEILLAT on 19/04/2021.\n"
"//\n"
"\n"
"#include <metal_stdlib>\n"
"#define PI 3.1415926535897932384626433832795\n"
"using namespace metal;\n"
"\n"
"float3 matMul(const float3 r012, const float3 r345, const float3 r678, float3 v){\n"
"    float3 outvec = { 0, 0, 0 };\n"
"    outvec.x = r012.x * v.x + r012.y * v.y + r012.z * v.z;\n"
"    outvec.y = r345.x * v.x + r345.y * v.y + r345.z * v.z;\n"
"    outvec.z = r678.x * v.x + r678.y * v.y + r678.z * v.z;\n"
"    return outvec;\n"
"}\n"
"\n"
"float2 repairUv(float2 uv){\n"
"    float2 outuv;\n"
"\n"
"    if(uv.x<0) {\n"
"        outuv.x = 1.0 + uv.x;\n"
"    }else if(uv.x > 1.0){\n"
"        outuv.x = uv.x -1.0;\n"
"    } else {\n"
"        outuv.x = uv.x;\n"
"    }\n"
"\n"
"    if(uv.y<0) {\n"
"        outuv.y = 1.0 + uv.y;\n"
"    } else if(uv.y > 1.0){\n"
"        outuv.y = uv.y -1.0;\n"
"    } else {\n"
"        outuv.y = uv.y;\n"
"    }\n"
"\n"
"    outuv.x = min(max(outuv.x, 0.0), 1.0);\n"
"    outuv.y = min(max(outuv.y, 0.0), 1.0);\n"
"\n"
"    return outuv;\n"
"}\n"
"\n"
"float2 polarCoord(float3 dir) {\n"
"    float3 ndir = normalize(dir);\n"
"    float longi = -atan2(ndir.z, ndir.x);\n"
"\n"
"    float lat = acos(-ndir.y);\n"
"\n"
"    float2 uv;\n"
"    uv.x = longi;\n"
"    uv.y = lat;\n"
"\n"
"    float2 pitwo = {PI, PI};\n"
"    uv /= pitwo;\n"
"    uv.x /= 2.0;\n"
"    float2 ones = {1.0, 1.0};\n"
"    uv = fmod(uv, ones);\n"
"    return uv;\n"
"}\n"
"\n"
"float3 fisheyeDir(float3 dir, const float3 r012, const float3 r345, const float3 r678) {\n"
"\n"
"    if (dir.x == 0 && dir.y == 0)\n"
"        return matMul(r012, r345, r678, dir);\n"
"\n"
"    dir.x = dir.x / dir.z;\n"
"    dir.y = dir.y / dir.z;\n"
"    dir.z = 1;\n"
"\n"
"    float2 uv;\n"
"    uv.x = dir.x;\n"
"    uv.y = dir.y;\n"
"    float r = sqrt(uv.x*uv.x + uv.y*uv.y);\n"
"\n"
"    float phi = atan2(uv.y, uv.x);\n"
"\n"
"    float theta = r;\n"
"\n"
"    float3 fedir = { 0, 0, 0 };\n"
"    fedir.x = sin(theta) * cos(phi);\n"
"    fedir.y = sin(theta) * sin(phi);\n"
"    fedir.z = cos(theta);\n"
"\n"
"    fedir = matMul(r012, r345, r678, fedir);\n"
"\n"
"    return fedir;\n"
"}\n"
"\n"
"float4 linInterpCol(float2 uv, const device float* input, int width, int height){\n"
"    float4 outCol = {0,0,0,0};\n"
"    float i = floor(uv.x);\n"
"    float j = floor(uv.y);\n"
"    float a = uv.x-i;\n"
"    float b = uv.y-j;\n"
"    int x = (int)i;\n"
"    int y = (int)j;\n"
"    int x1 = (x < width - 1 ? x + 1 : x);\n"
"    int y1 = (y < height - 1 ? y + 1 : y);\n"
"    const int indexX1Y1 = ((y * width) + x) * 4;\n"
"    const int indexX2Y1 = ((y * width) + x1) * 4;\n"
"    const int indexX1Y2 = (((y1) * width) + x) * 4;\n"
"    const int indexX2Y2 = (((y1) * width) + x1) * 4;\n"
"    const int maxIndex = (width * height -1) * 4;\n"
"\n"
"    if(indexX2Y2 < maxIndex){\n"
"        outCol.x = (1.0 - a)*(1.0 - b)*input[indexX1Y1] + a*(1.0 - b)*input[indexX2Y1] + (1.0 - a)*b*input[indexX1Y2] + a*b*input[indexX2Y2];\n"
"        outCol.y = (1.0 - a)*(1.0 - b)*input[indexX1Y1 + 1] + a*(1.0 - b)*input[indexX2Y1 + 1] + (1.0 - a)*b*input[indexX1Y2 + 1] + a*b*input[indexX2Y2 + 1];\n"
"        outCol.z = (1.0 - a)*(1.0 - b)*input[indexX1Y1 + 2] + a*(1.0 - b)*input[indexX2Y1 + 2] + (1.0 - a)*b*input[indexX1Y2 + 2] + a*b*input[indexX2Y2 + 2];\n"
"        outCol.w = (1.0 - a)*(1.0 - b)*input[indexX1Y1 + 3] + a*(1.0 - b)*input[indexX2Y1 + 3] + (1.0 - a)*b*input[indexX1Y2 + 3] + a*b*input[indexX2Y2 + 3];\n"
"    } else {\n"
"        outCol.x = input[indexX1Y1];\n"
"        outCol.y = input[indexX1Y1+ 1];\n"
"        outCol.z = input[indexX1Y1+ 2];\n"
"        outCol.w = input[indexX1Y1+ 3];\n"
"    }\n"
"    return outCol;\n"
"}\n"
"\n"
"float3 tinyPlanetSph(float3 uv) {\n"
"    if (uv.x == 0 && uv.y == 0)\n"
"        return uv;\n"
"\n"
"    float3 sph;\n"
"    float2 uvxy;\n"
"    uvxy.x = uv.x/uv.z;\n"
"    uvxy.y = uv.y/uv.z;\n"
"\n"
"    float u  =length(uvxy);\n"
"    float alpha = atan2(2.0f, u);\n"
"    float phi = PI - 2*alpha;\n"
"    float z = cos(phi);\n"
"    float x = sin(phi);\n"
"\n"
"    uvxy = normalize(uvxy);\n"
"\n"
"    sph.z = z;\n"
"\n"
"    float2 sphxy = uvxy * x;\n"
"\n"
"    sph.x = sphxy.x;\n"
"    sph.y = sphxy.y;\n"
"\n"
"    return sph;\n"
"}\n"
"\n"
"kernel void Reframe360Kernel(constant int& p_Width [[buffer (11)]],\n"
"                             constant int& p_Height [[buffer (12)]],\n"
"                             constant float* p_Fov [[buffer (13)]], constant float* p_Tinyplanet [[buffer (14)]], constant float* p_Rectilinear [[buffer (15)]],\n"
"                             const device float* p_Input [[buffer (0)]], device float* p_Output [[buffer (8)]],\n"
"                             constant float* r [[buffer (16)]], constant int& samples [[buffer (17)]],constant bool& bilinear [[buffer (18)]],\n"
"                             uint2 id [[ thread_position_in_grid ]])\n"
"{\n"
"    if ((id.x < (uint)p_Width) && (id.y < (uint)p_Height))\n"
"    {\n"
"        const int index = ((id.y * p_Width) + id.x) * 4;\n"
"\n"
"        float4 accum_col = {0, 0, 0, 0};\n"
"\n"
"        for(int i=0; i<samples; i++){\n"
"\n"
"            float fov = p_Fov[i]; //Motion blur samples\n"
"\n"
"            float2 uv = { (float)id.x / p_Width, (float)id.y / p_Height };\n"
"            float aspect = (float)p_Width / (float)p_Height;\n"
"\n"
"            float3 dir = { 0, 0, 0 };\n"
"            dir.x = (uv.x * 2) - 1;\n"
"            dir.y = (uv.y * 2) - 1;\n"
"            dir.y /= aspect;\n"
"            dir.z = fov;\n"
"\n"
"            float3 tinyplanet = tinyPlanetSph(dir);\n"
"            tinyplanet = normalize(tinyplanet);\n"
"\n"
"            const float3 r012 = {r[i*9+0], r[i*9+1], r[i*9+2]}; //Motion blur samples\n"
"            const float3 r345 = {r[i*9+3], r[i*9+4], r[i*9+5]}; //each rotation matrix is pitched\n"
"            const float3 r678 = {r[i*9+6], r[i*9+7], r[i*9+8]}; //\n"
"\n"
"            tinyplanet = matMul(r012, r345, r678, tinyplanet);\n"
"            float3 rectdir = matMul(r012, r345, r678, dir);\n"
"\n"
"            rectdir = normalize(rectdir);\n"
"            dir = mix(fisheyeDir(dir, r012, r345, r678), tinyplanet, p_Tinyplanet[i]);\n"
"            dir = mix(dir, rectdir, p_Rectilinear[i]);\n"
"\n"
"            float2 iuv = polarCoord(dir);\n"
"            iuv = repairUv(iuv);\n"
"\n"
"            int x_new = iuv.x * (p_Width - 1);\n"
"            int y_new = iuv.y * (p_Height - 1);\n"
"\n"
"            iuv.x *= (p_Width - 1);\n"
"            iuv.y *= (p_Height - 1);\n"
"\n"
"            if ((x_new < p_Width) && (y_new < p_Height))\n"
"            {\n"
"                const int index_new = ((y_new * p_Width) + x_new) * 4;\n"
"\n"
"                float4 interpCol;\n"
"                if (bilinear){\n"
"                    interpCol = linInterpCol(iuv, p_Input, p_Width, p_Height);\n"
"                }\n"
"                else {\n"
"                    interpCol = { p_Input[index_new + 0], p_Input[index_new + 1], p_Input[index_new + 2], p_Input[index_new + 3] };\n"
"                }\n"
"\n"
"                accum_col.x += interpCol.x;\n"
"                accum_col.y += interpCol.y;\n"
"                accum_col.z += interpCol.z;\n"
"                accum_col.w += interpCol.w;\n"
"            }\n"
"            p_Output[index + 0] = accum_col.x / samples;\n"
"            p_Output[index + 1] = accum_col.y / samples;\n"
"            p_Output[index + 2] = accum_col.z / samples;\n"
"            p_Output[index + 3] = accum_col.w / samples;\n"
"        }\n"
"    }\n"
"}\n";
