#include "ssao.h"

#include <random>
#include <cmath>
#include <algorithm>
#include <limits>
#include <linalg.h>
#include "../../resource.h"

using namespace linalg::aliases;

namespace cg::renderer::ssao_cpu
{
    static float3 reconstruct_view_pos(int x, int y, float depth, const float4x4& invProj, unsigned width, unsigned height)
    {
        float nx = (static_cast<float>(x) / static_cast<float>(width)) * 2.f - 1.f;
        float ny = -((static_cast<float>(y) / static_cast<float>(height)) * 2.f - 1.f);
        float4 clip{nx, ny, depth * 2.f - 1.f, 1.f};
        float4 view = mul(invProj, clip);
        view = view / view.w;
        return float3{view.x, view.y, view.z};
    }

    std::shared_ptr<cg::resource<cg::unsigned_color>> apply_ssao(
        const std::shared_ptr<cg::resource<cg::unsigned_color>>& color_rt,
        const std::shared_ptr<cg::resource<float>>& depth_rt,
        const float4x4& proj,
        unsigned width,
        unsigned height,
        int kernel_samples,
        float radius)
    {
        if (!color_rt || !depth_rt)
            return nullptr;

        auto out = std::make_shared<cg::resource<cg::unsigned_color>>(width, height);

    float4x4 invProj = inverse(proj);
        std::vector<float3> kernel;
        kernel.reserve(kernel_samples);
        std::mt19937 rng(1337);
        std::uniform_real_distribution<float> rnd01(0.0f, 1.0f);
        for (int i = 0; i < kernel_samples; ++i)
        {
            float3 sample{
                rnd01(rng) * 2.f - 1.f,
                rnd01(rng) * 2.f - 1.f,
                rnd01(rng)
            };
            float len = std::sqrt(sample.x*sample.x + sample.y*sample.y + sample.z*sample.z);
            sample = sample / len;
            float scale = static_cast<float>(i) / static_cast<float>(kernel_samples);
            scale = 0.1f + 0.9f * scale * scale;
            sample = sample * scale;
            kernel.push_back(sample);
        }
        std::vector<float> ao(width * height, 1.f);

        for (unsigned y = 0; y < height; ++y)
        {
            for (unsigned x = 0; x < width; ++x)
            {
                float d = depth_rt->item(x, y);
                if (d == std::numeric_limits<float>::max())
                {
                    ao[x + y * width] = 1.f;
                    continue;
                }

                float3 pos = reconstruct_view_pos(x, y, d, invProj, width, height);

                float eps = 1.f;
                float dR = depth_rt->item(std::min<unsigned>(x+1, width-1), y);
                float dU = depth_rt->item(x, std::min<unsigned>(y+1, height-1));
                float3 pR = reconstruct_view_pos(std::min<unsigned>(x+1, width-1), y, dR, invProj, width, height);
                float3 pU = reconstruct_view_pos(x, std::min<unsigned>(y+1, height-1), dU, invProj, width, height);
                float3 normal = normalize(cross(pR - pos, pU - pos));

                float occlusion = 0.f;
                for (int k = 0; k < kernel_samples; ++k)
                {
                    float3 sample = kernel[k];
                    float3 up = fabs(normal.z) < 0.999f ? float3{0.f,0.f,1.f} : float3{1.f,0.f,0.f};
                    float3 tangent = normalize(cross(up, normal));
                    float3 bitangent = cross(normal, tangent);
                    float3 samplePos = pos + (tangent * sample.x + bitangent * sample.y + normal * sample.z) * radius;

                    float4 clip = mul(proj, float4{samplePos.x, samplePos.y, samplePos.z, 1.f});
                    if (clip.w == 0.f) continue;
                    float3 ndc = float3{clip.x/clip.w, clip.y/clip.w, clip.z/clip.w};
                    float2 uv = float2{ndc.x * 0.5f + 0.5f, -ndc.y * 0.5f + 0.5f};

                    int sx = static_cast<int>(uv.x * (width-1));
                    int sy = static_cast<int>(uv.y * (height-1));
                    if (sx < 0 || sx >= static_cast<int>(width) || sy < 0 || sy >= static_cast<int>(height)) continue;

                    float sampleDepth = depth_rt->item(sx, sy);
                    if (sampleDepth == std::numeric_limits<float>::max()) continue;
                    float3 sampleView = reconstruct_view_pos(sx, sy, sampleDepth, invProj, width, height);

                    float rangeCheck = length(sampleView - pos);
                    if (sampleView.z >= samplePos.z - 1e-4f)
                        occlusion += 1.f;
                }

                float aoVal = 1.f - (occlusion / static_cast<float>(kernel_samples));
                aoVal = std::clamp(aoVal, 0.f, 1.f);
                ao[x + y * width] = aoVal;
            }
        }

        std::vector<float> ao_blur(width * height, 0.f);
        for (unsigned y = 0; y < height; ++y)
        {
            for (unsigned x = 0; x < width; ++x)
            {
                float accum = 0.f;
                int count = 0;
                for (int oy = -1; oy <= 1; ++oy)
                {
                    for (int ox = -1; ox <= 1; ++ox)
                    {
                        int sx = static_cast<int>(x) + ox;
                        int sy = static_cast<int>(y) + oy;
                        if (sx < 0 || sx >= static_cast<int>(width) || sy < 0 || sy >= static_cast<int>(height)) continue;
                        accum += ao[sx + sy * width];
                        count++;
                    }
                }
                ao_blur[x + y * width] = accum / static_cast<float>(count);
            }
        }

        for (unsigned y = 0; y < height; ++y)
        {
            for (unsigned x = 0; x < width; ++x)
            {
                auto col = color_rt->item(x, y).to_float3();
                float a = ao_blur[x + y * width];
                float3 final = col * a;
                out->item(x, y) = cg::unsigned_color::from_float3(final);
            }
        }

        return out;
    }

}
