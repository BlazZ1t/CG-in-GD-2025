#pragma once

#include "../../resource.h"
#include <memory>

namespace cg::renderer::ssao_cpu
{
    std::shared_ptr<cg::resource<cg::unsigned_color>> apply_ssao(
        const std::shared_ptr<cg::resource<cg::unsigned_color>>& color_rt,
        const std::shared_ptr<cg::resource<float>>& depth_rt,
        const float4x4& proj,
        unsigned width,
        unsigned height,
        int kernel_samples = 16,
        float radius = 0.5f);
}
