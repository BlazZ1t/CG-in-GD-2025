#include "rasterizer_renderer.h"

#include "utils/resource_utils.h"
#include "utils/timer.h"
#include "stdio.h"


void cg::renderer::rasterization_renderer::init()
{

	renderer::load_model();

	rasterizer = std::make_shared<cg::renderer::rasterizer<cg::vertex, cg::unsigned_color>>();

	rasterizer->set_viewport(settings->width, settings->height);

	render_target = std::make_shared<cg::resource<cg::unsigned_color>>(settings->width, settings->height);
	rasterizer->set_render_target(render_target);
	// TODO Lab: 1.03 Adjust `cg::renderer::rasterization_renderer` and `cg::renderer::renderer` classes to consume `cg::world::model`
	// TODO Lab: 1.04 Setup an instance of camera `cg::world::camera` class in `cg::renderer::renderer` and `cg::renderer::rasterization_renderer` 
	// TODO Lab: 1.06 Add depth buffer in `cg::renderer::rasterization_renderer`
}
void cg::renderer::rasterization_renderer::render()
{
	rasterizer->clear_render_target(cg::unsigned_color{
		.r = 56,
		.g = 120,
		.b = 111
	});

	cg::utils::save_resource(*render_target, settings->result_path);
	// TODO Lab: 1.04 Implement `vertex_shader` lambda for the instance of `cg::renderer::rasterizer`
	// TODO Lab: 1.05 Implement `pixel_shader` lambda for the instance of `cg::renderer::rasterizer`
	for (size_t shape_id = 0; shape_id < model->get_index_buffers().size(); shape_id++)
	{
		rasterizer->set_vertex_buffer(model->get_vertex_buffers()[shape_id]);
		rasterizer->set_index_buffer(model->get_index_buffers()[shape_id]);
		rasterizer->draw(model->get_index_buffers()[shape_id]->count(), 0);
	}
}

void cg::renderer::rasterization_renderer::destroy() {}

void cg::renderer::rasterization_renderer::update() {}