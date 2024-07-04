
#include <GL/glew.h>

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_sdl2.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "imgui/misc/cpp/imgui_stdlib.h"


#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/matrix.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


#include <stdio.h>
#include <SDL.h>
#include <SDL_opengl.h>


#pragma comment(lib, "SDL2.lib")
#pragma comment(lib, "glew32.lib")

#pragma comment(lib, "SDL2main.lib")
#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "opencv_world4100.lib")

#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>
using namespace std;


#include <opencv2/opencv.hpp>
using namespace cv;


#include "vertex_fragment_shader.h"

vertex_fragment_shader ortho_shader;

struct
{
	struct
	{
		GLint tex;
		GLint viewport_width;
		GLint viewport_height;
	}
	ortho_shader_uniforms;

}
uniforms;


//class triangle
//{
//public:
//	ImVec2 vertices[3];
//};

class quad
{
public:
	glm::vec3 vertices[4];
};

// https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/ray-triangle-intersection-geometric-solution
// http://geomalgorithms.com/a06-_intersect-2.html



// Simple helper function to load an image into a OpenGL texture with common settings
bool LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height)
{
	// Load from file
	int image_width = 0;
	int image_height = 0;

	Mat img = imread(filename, IMREAD_UNCHANGED);

	if (img.empty())
		return false;

	image_width = img.cols;
	image_height = img.rows;

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
	if (img.channels() == 4)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.data);
	else if (img.channels() == 3)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image_width, image_height, 0, GL_RGB, GL_UNSIGNED_BYTE, img.data);
	else
		return false;

	*out_texture = image_texture;
	*out_width = image_width;
	*out_height = image_height;

	return true;
}








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


vector<string> left_strings;
int left_selected = -1;
vector<ImVec2> left_uv_mins;
vector<ImVec2> left_uv_maxs;


void left_add_button_func(void)
{
	left_strings.push_back("1.0");
	left_uv_mins.push_back(ImVec2(0, 0));
	left_uv_maxs.push_back(ImVec2(0, 0));

	if (left_strings.size() == 1)
		left_selected = 0;
}

void left_remove_button_func(int i)
{
	left_strings.erase(left_strings.begin() + i);
	left_uv_mins.erase(left_uv_mins.begin() + i);
	left_uv_maxs.erase(left_uv_maxs.begin() + i);

	if (i == left_selected)
		left_selected = -1;
}





vector<string> right_strings;
int right_selected = -1;
vector<ImVec2> right_uv_mins;
vector<ImVec2> right_uv_maxs;


void right_add_button_func(void)
{
	right_strings.push_back("1.0");
	right_uv_mins.push_back(ImVec2(0, 0));
	right_uv_maxs.push_back(ImVec2(0, 0));

	if (right_strings.size() == 1)
		right_selected = 0;
}

void right_remove_button_func(int i)
{
	right_strings.erase(right_strings.begin() + i);
	right_uv_mins.erase(right_uv_mins.begin() + i);
	right_uv_maxs.erase(right_uv_maxs.begin() + i);

	if (i == right_selected)
		right_selected = -1;
}



ImVec2 image_anchor(0, 0);


// http://www.songho.ca/opengl/gl_transform.html

complex<float> get_window_coords_from_ndc_coords(size_t viewport_width, size_t viewport_height, complex<float>& src_coords)
{
	float x_w = viewport_width / 2.0f * src_coords.real() + viewport_width / 2.0f;
	float y_w = viewport_height / 2.0f * src_coords.imag() + viewport_height / 2.0f;

	return complex<float>(x_w, y_w);
}

complex<float> get_ndc_coords_from_window_coords(size_t viewport_width, size_t viewport_height, complex<float>& src_coords)
{
	float x_ndc = (2.0f * src_coords.real() / viewport_width) - 1.0f;
	float y_ndc = (2.0f * src_coords.imag() / viewport_height) - 1.0f;

	return complex<float>(x_ndc, y_ndc);
}



class background_tile
{
public:
	int tile_size;
	ImVec2 uv_min;
	ImVec2 uv_max;
};

int tiles_per_dimension = 20;


vector<background_tile> background_tiles;

float zoom_factor = 1.0f;
float last_mousewheel = 0.0f;



bool point_in_polygon(glm::vec3 point, vector<glm::vec3> polygon)
{
	const size_t num_vertices = polygon.size();
	double x = point.x, y = point.y;
	bool inside = false;

	// Store the first point in the polygon and initialize
	// the second point
	glm::vec3 p1 = polygon[0], p2;

	// Loop through each edge in the polygon
	for (int i = 1; i <= num_vertices; i++) {
		// Get the next point in the polygon
		p2 = polygon[i % num_vertices];

		// Check if the point is above the minimum y
		// coordinate of the edge
		if (y > min(p1.y, p2.y)) {
			// Check if the point is below the maximum y
			// coordinate of the edge
			if (y <= max(p1.y, p2.y)) {
				// Check if the point is to the left of the
				// maximum x coordinate of the edge
				if (x <= max(p1.x, p2.x)) {
					// Calculate the x-intersection of the
					// line connecting the point to the edge
					double x_intersection
						= (y - p1.y) * (p2.x - p1.x)
						/ (p2.y - p1.y)
						+ p1.x;

					// Check if the point is on the same
					// line as the edge or to the left of
					// the x-intersection
					if (p1.x == p2.x
						|| x <= x_intersection) {
						// Flip the inside flag
						inside = !inside;
					}
				}
			}
		}

		// Store the current point as the first point for
		// the next iteration
		p1 = p2;
	}

	// Return the value of the inside flag
	return inside;
}









bool draw_textured_quad(int mouse_x, int mouse_y, vector<quad>& quads, GLuint shader_program, long signed int x, long signed int y, long signed int tile_size, long signed int win_width, long signed int win_height, GLuint tex_handle, ImVec2 uv_min, ImVec2 uv_max)
{
	static GLuint vao = 0, vbo = 0, ibo = 0;

	if (!glIsVertexArray(vao))
	{
		glGenVertexArrays(1, &vao);
		glGenBuffers(1, &vbo);
		glGenBuffers(1, &ibo);
	}

	glDisable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	complex<float> v0w(static_cast<float>(x), static_cast<float>(y));
	complex<float> v1w(static_cast<float>(x), static_cast<float>(y + tile_size));
	complex<float> v2w(static_cast<float>(x + tile_size), static_cast<float>(y + tile_size));
	complex<float> v3w(static_cast<float>(x + tile_size), static_cast<float>(y));

	v0w.real(v0w.real() * zoom_factor);
	v0w.imag(v0w.imag() * zoom_factor);
	v1w.real(v1w.real() * zoom_factor);
	v1w.imag(v1w.imag() * zoom_factor);
	v2w.real(v2w.real() * zoom_factor);
	v2w.imag(v2w.imag() * zoom_factor);
	v3w.real(v3w.real() * zoom_factor);
	v3w.imag(v3w.imag() * zoom_factor);

	quad q;
	q.vertices[0].x = v0w.real();
	q.vertices[0].y = win_height - v0w.imag();
	q.vertices[1].x = v1w.real();
	q.vertices[1].y = win_height - v1w.imag();
	q.vertices[2].x = v2w.real();
	q.vertices[2].y = win_height - v2w.imag();
	q.vertices[3].x = v3w.real();
	q.vertices[3].y = win_height - v3w.imag();
	quads.push_back(q);

	vector<glm::vec3> points;
	points.push_back(q.vertices[0]);
	points.push_back(q.vertices[1]);
	points.push_back(q.vertices[2]);
	points.push_back(q.vertices[3]);

	glm::vec3 mouse_pos(mouse_x, mouse_y, 0);

	bool inside = point_in_polygon(mouse_pos, points);









	complex<float> v0ndc = get_ndc_coords_from_window_coords(win_width, win_height, v0w);
	complex<float> v1ndc = get_ndc_coords_from_window_coords(win_width, win_height, v1w);
	complex<float> v2ndc = get_ndc_coords_from_window_coords(win_width, win_height, v2w);
	complex<float> v3ndc = get_ndc_coords_from_window_coords(win_width, win_height, v3w);

	const GLfloat vertexData[] = {
		//	  X             Y             Z		  U         V     
			  v0ndc.real(), v0ndc.imag(), 0,      uv_min.x, uv_max.y,
			  v1ndc.real(), v1ndc.imag(), 0,      uv_min.x, uv_min.y,
			  v2ndc.real(), v2ndc.imag(), 0,      uv_max.x, uv_min.y,
			  v3ndc.real(), v3ndc.imag(), 0,      uv_max.x, uv_max.y,
	};

	// https://raw.githubusercontent.com/progschj/OpenGL-Examples/master/03texture.cpp

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 4 * 5, vertexData, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (char*)0 + 0 * sizeof(GLfloat));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (char*)0 + 3 * sizeof(GLfloat));

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

	static const GLuint indexData[] = {
		3,1,0,
		2,1,3,
	};

	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * 2 * 3, indexData, GL_STATIC_DRAW);

	glBindVertexArray(0);

	glUseProgram(ortho_shader.get_program());

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex_handle);

	glUniform1i(uniforms.ortho_shader_uniforms.tex, 0);
	glUniform1i(uniforms.ortho_shader_uniforms.viewport_width, win_width);
	glUniform1i(uniforms.ortho_shader_uniforms.viewport_height, win_height);

	glBindVertexArray(vao);

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

	return inside;
}



bool RayIntersectsTriangle(const glm::vec3 rayOrigin,
	const glm::vec3 rayVector,
	const glm::vec3 v0, const glm::vec3 v1, const glm::vec3 v2,
	glm::vec3& outIntersectionPoint)
{
	const float EPSILON = 0.0000001f;
	glm::vec3 vertex0 = v0;// inTriangle->vertex0;
	glm::vec3 vertex1 = v1;// inTriangle->vertex1;
	glm::vec3 vertex2 = v2;// inTriangle->vertex2;
	glm::vec3 edge1, edge2, h, s, q;
	float a, f, u, v;
	edge1 = vertex1 - vertex0;
	edge2 = vertex2 - vertex0;
	h = cross(rayVector, edge2);
	a = dot(edge1, h);
	if (a > -EPSILON && a < EPSILON)
		return false;    // This ray is parallel to this triangle.
	f = 1.0f / a;
	s = rayOrigin - vertex0;
	u = f * dot(s, h);
	if (u < 0.0f || u > 1.0f)
		return false;
	q = cross(s, edge1);
	v = f * dot(rayVector, q);
	if (v < 0.0f || u + v > 1.0f)
		return false;

	// At this stage we can compute t to find out where the intersection point is on the line.

	float t = f * dot(edge2, q);

	if (t > EPSILON) // ray intersection
	{
		outIntersectionPoint = rayOrigin + rayVector * t;
		return true;
	}
	else // This means that there is a line intersection but not a ray intersection.
		return false;
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

	// GL 4.3 + GLSL 430
	const char* glsl_version = "#version 430";
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
	SDL_Window* window = SDL_CreateWindow("World Editor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
	if (window == nullptr)
	{
		printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
		return -1;
	}

	SDL_GLContext gl_context = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, gl_context);
	SDL_GL_SetSwapInterval(1); // Enable vsync

	if (GLEW_OK != glewInit())
	{
		cout << "GLEW initialization error" << endl;
		return false;
	}


	if (false == ortho_shader.init("ortho.vs.glsl", "ortho.fs.glsl"))
	{
		cout << "Could not load ortho shader" << endl;
		return false;
	}

	uniforms.ortho_shader_uniforms.tex = glGetUniformLocation(ortho_shader.get_program(), "tex");
	uniforms.ortho_shader_uniforms.viewport_width = glGetUniformLocation(ortho_shader.get_program(), "viewport_width");
	uniforms.ortho_shader_uniforms.viewport_height = glGetUniformLocation(ortho_shader.get_program(), "viewport_height");


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

	ImVec4 clear_color = ImVec4(1.0, 0.5, 0.0, 1.0);

	// Main loop
	bool done = false;



	int my_image_width = 0;
	int my_image_height = 0;
	GLuint my_image_texture = 0;

	bool ret = LoadTextureFromFile("goblins.png", &my_image_texture, &my_image_width, &my_image_height);



	background_tiles.resize(tiles_per_dimension * tiles_per_dimension);

	// Initialize to use 
	for (size_t i = 0; i < tiles_per_dimension; i++)
	{
		for (size_t j = 0; j < tiles_per_dimension; j++)
		{
			size_t index = i * tiles_per_dimension + j;

			background_tiles[index].tile_size = 36;
			background_tiles[index].uv_min = ImVec2(0, 0);
			background_tiles[index].uv_max = ImVec2(float(background_tiles[index].tile_size) / my_image_width, float(background_tiles[index].tile_size) / my_image_height);
		}
	}

	int window_w = 0, window_h = 0;
	SDL_GetWindowSize(window, &window_w, &window_h);

	image_anchor.x = 0;// float(window_w) / 2.0 - 36.0f * float(tiles_per_dimension) / 2.0f;
	image_anchor.y = 0;// float(window_h) / 2.0 - 36.0f * float(tiles_per_dimension) / 2.0f;


	while (!done)
	{
		// Poll and handle events (inputs, window resize, etc.)
		// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
		// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
		// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
		// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
		last_mousewheel = 0;

		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			ImGui_ImplSDL2_ProcessEvent(&event);

			if (event.type == SDL_QUIT)
				done = true;
			if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
				done = true;

			if (event.type == SDL_MOUSEWHEEL)
				last_mousewheel = (float)event.wheel.y;

		}

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		// Only pan image if not hovering over an ImGui window
		bool hovered = false;

		ImGui::Begin("image", nullptr, ImGuiWindowFlags_HorizontalScrollbar);

		if (ImGui::IsWindowHovered())
			hovered = true;

		const ImVec2 img_size = { float(my_image_width), float(my_image_height) };

		static char str0[128] = "36";
		ImGui::InputText("Tile size", str0, IM_ARRAYSIZE(str0));

		istringstream iss(str0);
		int block_size = 0;
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





		ImGui::Begin("Left Brush", nullptr, ImGuiWindowFlags_HorizontalScrollbar);

		if (ImGui::IsWindowHovered())
			hovered = true;

		if (ImGui::Button("Add"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
			left_add_button_func();

		for (int i = 0; i < left_strings.size(); i++)
		{
			const ImVec2 thumbnail_img_size = { float(block_size), float(block_size) };

			if (left_clicked && i == left_selected)
			{
				//ImVec2 img_block = ImVec2(floor(mousePositionRelative.x / block_size), floor(mousePositionRelative.y / block_size));
				//cout << img_block.x << " " << img_block.y << endl;

				size_t x = size_t(mousePositionRelative.x) % block_size;
				size_t y = size_t(mousePositionRelative.y) % block_size;

				float u_start = (mousePositionRelative.x - x) / img_size.x;
				float v_start = (mousePositionRelative.y - y) / img_size.y;

				float u_end = block_size / img_size.x + u_start;
				float v_end = block_size / img_size.y + v_start;

				left_uv_mins[i] = ImVec2(u_start, v_start);
				left_uv_maxs[i] = ImVec2(u_end, v_end);
			}

			const ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
			const ImVec4 selected_border_col = ImVec4(1.0f, 0.5f, 0.0f, 1.0f);
			const ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

			if (i == left_selected)
				ImGui::Image((void*)(intptr_t)my_image_texture, thumbnail_img_size, left_uv_mins[i], left_uv_maxs[i], tint_col, selected_border_col);
			else
				ImGui::Image((void*)(intptr_t)my_image_texture, thumbnail_img_size, left_uv_mins[i], left_uv_maxs[i], tint_col, border_col);

			if (ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left))
				left_selected = i;



			ImGui::SameLine();

			if (ImGui::Button((string("Remove ") + to_string(i)).c_str()))                         // Buttons return true when clicked (most widgets return true when edited/activated)
				left_remove_button_func(i);

			ImGui::SameLine();

			string x = "Weight " + to_string(i);

			ImGui::PushItemWidth(80);
			ImGui::InputText(x.c_str(), &left_strings[i]);
			ImGui::PopItemWidth();
		}





		ImGui::End();






		ImGui::Begin("Right Brush", nullptr, ImGuiWindowFlags_HorizontalScrollbar);

		if (ImGui::IsWindowHovered())
			hovered = true;

		if (ImGui::Button("Add"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
			right_add_button_func();

		for (int i = 0; i < right_strings.size(); i++)
		{
			const ImVec2 thumbnail_img_size = { float(block_size), float(block_size) };

			if (right_clicked && i == right_selected)
			{
				//ImVec2 img_block = ImVec2(floor(mousePositionRelative.x / block_size), floor(mousePositionRelative.y / block_size));
				//cout << img_block.x << " " << img_block.y << endl;

				size_t x = size_t(mousePositionRelative.x) % block_size;
				size_t y = size_t(mousePositionRelative.y) % block_size;

				float u_start = (mousePositionRelative.x - x) / img_size.x;
				float v_start = (mousePositionRelative.y - y) / img_size.y;

				float u_end = block_size / img_size.x + u_start;
				float v_end = block_size / img_size.y + v_start;

				right_uv_mins[i] = ImVec2(u_start, v_start);
				right_uv_maxs[i] = ImVec2(u_end, v_end);
			}

			const ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
			const ImVec4 selected_border_col = ImVec4(1.0f, 0.5f, 0.0f, 1.0f);
			const ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

			if (i == right_selected)
				ImGui::Image((void*)(intptr_t)my_image_texture, thumbnail_img_size, right_uv_mins[i], right_uv_maxs[i], tint_col, selected_border_col);
			else
				ImGui::Image((void*)(intptr_t)my_image_texture, thumbnail_img_size, right_uv_mins[i], right_uv_maxs[i], tint_col, border_col);

			if (ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left))
				right_selected = i;

			ImGui::SameLine();

			if (ImGui::Button((string("Remove ") + to_string(i)).c_str()))
				right_remove_button_func(i);

			ImGui::SameLine();

			string x = "Weight " + to_string(i);

			ImGui::PushItemWidth(80);
			ImGui::InputText(x.c_str(), &right_strings[i]);
			ImGui::PopItemWidth();


		}


		ImGui::End();






		// Rendering
		ImGui::Render();

		if (!hovered)
		{
			zoom_factor += last_mousewheel * 0.1f;

			if (last_mousewheel != 0)
			{
				//image_anchor.x =  36.0f * float(tiles_per_dimension) / 2.0f  - float(window_w) / 2.0;
				//image_anchor.y =  36.0f * float(tiles_per_dimension) / 2.0f - float(window_h) / 2.0;
			}
		}
		if (!hovered && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0) && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Space)))
		{
			ImVec2 motion = ImGui::GetMouseDragDelta();
			image_anchor.x += motion.x / zoom_factor;
			image_anchor.y += -motion.y / zoom_factor;

			ImGui::ResetMouseDragDelta();
		}

		glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
		glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);


		for (size_t i = 0; i < tiles_per_dimension; i++)
		{
			for (size_t j = 0; j < tiles_per_dimension; j++)
			{
				size_t index = i * tiles_per_dimension + j;

				vector<quad> quads;

				int x, y;
				SDL_GetMouseState(&x, &y);

				bool inside = draw_textured_quad(x, y, quads, ortho_shader.get_program(), int(image_anchor.x) + int(i) * background_tiles[index].tile_size, int(image_anchor.y) + int(j) * background_tiles[index].tile_size, background_tiles[index].tile_size, (int)io.DisplaySize.x, (int)io.DisplaySize.y, my_image_texture, background_tiles[index].uv_min, background_tiles[index].uv_max);

				if (inside)
				{
					cout << i << " " << j << endl;
				}



				//if (triangles.size() == 2)
				//{
				//	int x, y;
				//	SDL_GetMouseState(&x, &y);

				//	glm::vec3 v0(triangles[0].vertices[0].x, triangles[0].vertices[0].y, 0);
				//	glm::vec3 v1(triangles[0].vertices[1].x, triangles[0].vertices[1].y, 0);
				//	glm::vec3 v2(triangles[0].vertices[2].x, triangles[0].vertices[2].y, 0);

				//	glm::vec3 hit_point;

				//	bool first_hit = false, second_hit = false;


				//	first_hit = RayIntersectsTriangle(glm::vec3(x, y, -1),
				//		glm::vec3(x, y, 1),
				//		v0, v1, v2,
				//		hit_point);

				//	if (false == first_hit)
				//	{
				//		 v0 = glm::vec3(triangles[1].vertices[0].x, triangles[1].vertices[0].y, 0);
				//		 v1 = glm::vec3(triangles[1].vertices[1].x, triangles[1].vertices[1].y, 0);
				//		 v2 = glm::vec3(triangles[1].vertices[2].x, triangles[1].vertices[2].y, 0);

				//		glm::vec3 hit_point;

				//		second_hit = RayIntersectsTriangle(glm::vec3(x, y, -1),
				//			glm::vec3(x, y, 1),
				//			v0, v1, v2,
				//			hit_point);
				//	}

				//	cout << first_hit << " " << second_hit << endl;
				//}
			}
		}




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