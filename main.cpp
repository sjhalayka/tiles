// Dear ImGui: standalone example application for SDL2 + OpenGL
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_sdl2.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include <stdio.h>
#include <SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif



#pragma comment(lib, "SDL2.lib")
#pragma comment(lib, "SDL2main.lib")
#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "opencv_world4100.lib")






#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>
#include <sstream>
#include <vector>
using namespace std;















// Simple helper function to load an image into a OpenGL texture with common settings
bool LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height)
{
	// Load from file
	int image_width = 0;
	int image_height = 0;
	unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
	if (image_data == NULL)
		return false;

	// Create a OpenGL texture identifier
	GLuint image_texture;
	glGenTextures(1, &image_texture);
	glBindTexture(GL_TEXTURE_2D, image_texture);

	// Setup filtering parameters for display
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	// Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
	stbi_image_free(image_data);

	*out_texture = image_texture;
	*out_width = image_width;
	*out_height = image_height;

	return true;
}










#include <opencv2/opencv.hpp>

// https://stackoverflow.com/questions/65091012/how-to-split-an-image-into-m-x-n-tiles
std::vector<cv::Mat> splitImage(cv::Mat& image, int M, int N)
{
	// All images should be the same size ...
	int width = image.cols / M;
	int height = image.rows / N;
	// ... except for the Mth column and the Nth row
	int width_last_column = width + (image.cols % width);
	int height_last_row = height + (image.rows % height);

	std::vector<cv::Mat> result;

	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < M; j++)
		{
			// Compute the region to crop from
			cv::Rect roi(width * j,
				height * i,
				(j == (M - 1)) ? width_last_column : width,
				(i == (N - 1)) ? height_last_row : height);

			result.push_back(image(roi));
		}
	}

	return result;
}	






// Main code
int main(int, char**)
{
	// Setup SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
	{
		printf("Error: %s\n", SDL_GetError());
		return -1;
	}

	// GL 3.0 + GLSL 130
	const char* glsl_version = "#version 130";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

	// From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
	SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

	// Create window with graphics context
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_MAXIMIZED | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	SDL_Window* window = SDL_CreateWindow("Dear ImGui SDL2+OpenGL3 example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
	if (window == nullptr)
	{
		printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
		return -1;
	}

	SDL_GLContext gl_context = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, gl_context);
	SDL_GL_SetSwapInterval(1); // Enable vsync

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
	ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
	ImGui_ImplOpenGL3_Init(glsl_version);

	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
	// - Read 'docs/FONTS.md' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	// - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
	//io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
	//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
	//IM_ASSERT(font != nullptr);

	// Our state

	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	// Main loop
	bool done = false;



	int my_image_width = 0;
	int my_image_height = 0;
	GLuint my_image_texture = 0;

	bool ret = LoadTextureFromFile("C:/temp/goblins.png", &my_image_texture, &my_image_width, &my_image_height);



	while (!done)
	{
		// Poll and handle events (inputs, window resize, etc.)
		// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
		// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
		// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
		// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			ImGui_ImplSDL2_ProcessEvent(&event);

			if (event.type == SDL_QUIT)
				done = true;
			if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
				done = true;
		}

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();




		ImGui::Begin("image", nullptr, ImGuiWindowFlags_HorizontalScrollbar);

		const ImVec2 img_size = { float(my_image_width), float(my_image_height) };

		static char str0[128] = "36";
		ImGui::InputText("Tile size", str0, IM_ARRAYSIZE(str0));

		istringstream iss(str0);
		size_t block_size = 0;
		iss >> block_size;

		{
			ImVec2 uv_min = ImVec2(0.0f, 0.0f);                 // Top-left
			ImVec2 uv_max = ImVec2(1.0f, 1.0f);                 // Lower-right
			ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
			ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white

			ImGui::Image((void*)(intptr_t)my_image_texture, img_size, uv_min, uv_max, tint_col, border_col);
		}

		const ImVec2 mousePositionAbsolute = ImGui::GetMousePos();
		const ImVec2 screenPositionAbsolute = ImGui::GetItemRectMin();
		const ImVec2 mousePositionRelative = ImVec2(mousePositionAbsolute.x - screenPositionAbsolute.x, mousePositionAbsolute.y - screenPositionAbsolute.y);

		bool left_clicked = false;
		bool right_clicked = false;

		if (ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left))
		{
			left_clicked = true;
		}

		if (ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Right))
		{
			right_clicked = true;
		}

		ImGui::End();


		ImGui::Begin("image2");// , nullptr, ImGuiWindowFlags_HorizontalScrollbar);

		{
			static const ImVec2 thumbnail_img_size = { 128, 128};
			static ImVec2 uv_min = ImVec2(0.0f, 0.0f);                 // Top-left
			static ImVec2 uv_max = ImVec2(0.0f, 0.0f);                 // Lower-right

			if (left_clicked)
			{
				//ImVec2 img_block = ImVec2(floor(mousePositionRelative.x / block_size), floor(mousePositionRelative.y / block_size));
				//cout << img_block.x << " " << img_block.y << endl;

				size_t x = size_t(mousePositionRelative.x) % block_size;
				size_t y = size_t(mousePositionRelative.y) % block_size;

				float u_start = (mousePositionRelative.x - x) / img_size.x;
				float v_start = (mousePositionRelative.y - y) / img_size.y;

				float u_end = block_size / img_size.x + u_start;
				float v_end = block_size / img_size.y + v_start;

				uv_min = ImVec2(u_start, v_start);                 // Top-left
				uv_max = ImVec2(u_end, v_end);
			}

			ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
			ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // 50% opaque white

			ImGui::Image((void*)(intptr_t)my_image_texture, thumbnail_img_size, uv_min, uv_max, tint_col, border_col);
		}



		{
			static const ImVec2 thumbnail_img_size = { 128, 128};
			static ImVec2 uv_min = ImVec2(0.0f, 0.0f);                 // Top-left
			static ImVec2 uv_max = ImVec2(0.0f, 0.0f);                 // Lower-right

			if (right_clicked)
			{
				//ImVec2 img_block = ImVec2(floor(mousePositionRelative.x / block_size), floor(mousePositionRelative.y / block_size));
				//cout << img_block.x << " " << img_block.y << endl;

				size_t x = size_t(mousePositionRelative.x) % block_size;
				size_t y = size_t(mousePositionRelative.y) % block_size;

				float u_start = (mousePositionRelative.x - x) / img_size.x;
				float v_start = (mousePositionRelative.y - y) / img_size.y;

				float u_end = block_size / img_size.x + u_start;
				float v_end = block_size / img_size.y + v_start;

				uv_min = ImVec2(u_start, v_start);                 // Top-left
				uv_max = ImVec2(u_end, v_end);
			}

			ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
			ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // 50% opaque white

			ImGui::Image((void*)(intptr_t)my_image_texture, thumbnail_img_size, uv_min, uv_max, tint_col, border_col);
		}

		ImGui::End();





		// Rendering
		ImGui::Render();
		glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
		glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		SDL_GL_SwapWindow(window);
	}


	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DeleteContext(gl_context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}