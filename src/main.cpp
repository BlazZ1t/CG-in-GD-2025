#include "renderer/renderer.h"
#include "settings.h"
#include "chrono"
#include "stdio.h"

#include <iostream>

int main(int argc, char** argv)
{
	try
	{
		auto settings = cg::settings::parse_settings(argc, argv);
		auto renderer = cg::renderer::make_renderer(settings);

		auto start = std::chrono::high_resolution_clock::now();
		renderer->init();

		renderer->render();

		renderer->destroy();

		auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
		std::cout << duration.count() << "ms" << std::endl;
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}

	return 0;
}