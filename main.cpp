#include "main.h"





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
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

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

	if (false == line_shader.init("lines.vs.glsl", "lines.gs.glsl", "lines.fs.glsl"))
	{
		cout << "Could not load line shader" << endl;
		return false;
	}




	uniforms.ortho_shader_uniforms.tex = glGetUniformLocation(ortho_shader.get_program(), "tex");
	uniforms.ortho_shader_uniforms.viewport_width = glGetUniformLocation(ortho_shader.get_program(), "viewport_width");
	uniforms.ortho_shader_uniforms.viewport_height = glGetUniformLocation(ortho_shader.get_program(), "viewport_height");
	uniforms.ortho_shader_uniforms.projection = glGetUniformLocation(ortho_shader.get_program(), "projection");
	uniforms.ortho_shader_uniforms.view = glGetUniformLocation(ortho_shader.get_program(), "view");
	uniforms.ortho_shader_uniforms.model = glGetUniformLocation(ortho_shader.get_program(), "model");



	uniforms.line_shader_uniforms.colour = glGetUniformLocation(line_shader.get_program(), "colour");
	uniforms.line_shader_uniforms.img_width = glGetUniformLocation(line_shader.get_program(), "img_width");
	uniforms.line_shader_uniforms.img_height = glGetUniformLocation(line_shader.get_program(), "img_height");
	uniforms.line_shader_uniforms.line_thickness = glGetUniformLocation(line_shader.get_program(), "line_thickness");



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



	int main_tiles_width = 0;
	int main_tiles_height = 0;
	GLuint main_tiles_texture = 0;
	Mat main_tiles_img;

	int custom_brush1_width = 0;
	int custom_brush1_height = 0;
	GLuint custom_brush1_texture = 0;
	Mat custom_brush1_img;


	bool ret = LoadTextureFromFile(main_tiles_img, "goblins.png", &main_tiles_texture, &main_tiles_width, &main_tiles_height);

	if (ret == false)
	{
		cout << "Could not load goblins.png" << endl;
		return false;
	}

	ret = LoadTextureFromFile(custom_brush1_img, "custom_brush_1.png", &custom_brush1_texture, &custom_brush1_width, &custom_brush1_height);

	if (ret == false)
	{
		cout << "Could not load custom_brush_1.png" << endl;
		return false;
	}


	if (tiles_per_dimension % tiles_per_chunk_dimension != 0)
	{
		cout << "Tiles per dimension must be evenly divisible by tile chunk size" << endl;
		return false;
	}

	int num_chunks_per_map_dimension = tiles_per_dimension / tiles_per_chunk_dimension;

	background_chunks.resize(num_chunks_per_map_dimension * num_chunks_per_map_dimension);

	background_tiles.resize(tiles_per_dimension * tiles_per_dimension);

	// Initialize to use 
	for (size_t i = 0; i < tiles_per_dimension; i++)
	{
		for (size_t j = 0; j < tiles_per_dimension; j++)
		{
			const size_t background_chunk_x = i / tiles_per_chunk_dimension;
			const size_t background_chunk_y = j / tiles_per_chunk_dimension;
			const size_t background_chunk_index = background_chunk_x * num_chunks_per_map_dimension + background_chunk_y;

			background_chunks[background_chunk_index].indices.push_back(ImVec2(i, j));

			const size_t index = i * tiles_per_dimension + j;

			background_tiles[index].tile_size = 36;
			background_tiles[index].uv_min = ImVec2(0, 0);
			background_tiles[index].uv_max = ImVec2(float(background_tiles[index].tile_size) / main_tiles_width, float(background_tiles[index].tile_size) / main_tiles_height);
		}
	}








	int window_w = 0, window_h = 0;
	SDL_GetWindowSize(window, &window_w, &window_h);

	image_anchor.x = 0;//float(window_w) / 2.0 - 36.0f * float(tiles_per_dimension) / 2.0f;
	image_anchor.y = 0;// float(window_h) / 2.0 - 36.0f * float(tiles_per_dimension) / 2.0f;


	mt19937 generator((unsigned int)time(0));
	uniform_real_distribution<float> distribution(0.0, 1.0);

	//set<pair<size_t, size_t>> prev_draw;

	set<pair<size_t, size_t>> selected_indices;
	glm::vec3 selected_start;
	glm::vec3 selected_end;

	size_t undo_index = 0;

	vector<set<pair<size_t, size_t>>> selected_indices_backups;
	vector<glm::vec3> selected_start_backups;
	vector<glm::vec3> selected_end_backups;
	vector<vector<background_tile>> background_tiles_backups;


#define TOOL_PAINT 0
#define TOOL_PAINT_SQUARE 1
#define TOOL_PAINT_CUSTOM 2
#define TOOL_SELECT 3
#define TOOL_SELECT_ADD 4
#define TOOL_SELECT_SUBTRACT 5
#define TOOL_PAN 6

	int tool = 0;

	int brush_size = 20;


	vector<int> prev_tools;


	while (!done)
	{
		bool make_selection = false;

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

			prev_tools.push_back(tool);

			if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE)
			{
				tool = TOOL_PAN;
			}
			else if (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_SPACE)
			{
				for (size_t i = 0; i < prev_tools.size(); i++)
				{
					if (prev_tools[i] != TOOL_PAN)
						tool = prev_tools[i];
				}

				prev_tools.clear();
			}



			if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_z)
			{
				if (selected_indices_backups.size() > 0)
				{
					if (0 < undo_index)
						undo_index--;

					selected_indices = selected_indices_backups[undo_index];
					selected_start = selected_start_backups[undo_index];
					selected_end = selected_end_backups[undo_index];
					background_tiles = background_tiles_backups[undo_index];

				}
			}

			if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_y)
			{
				if (selected_indices_backups.size() > 0)
				{
					if ((selected_indices_backups.size() - 1) > undo_index)
						undo_index++;

					selected_indices = selected_indices_backups[undo_index];
					selected_start = selected_start_backups[undo_index];
					selected_end = selected_end_backups[undo_index];
					background_tiles = background_tiles_backups[undo_index];

				}
			}



			if ((tool == TOOL_SELECT || tool == TOOL_SELECT_ADD || tool == TOOL_SELECT_SUBTRACT) && event.type == SDL_MOUSEBUTTONDOWN)
			{
				if (event.button.button == SDL_BUTTON_LEFT)
				{
					//if (selected_indices_backups.size() > 10)
					//{
					//	selected_indices_backups.erase(selected_indices_backups.begin());
					//	selected_start_backups.erase(selected_start_backups.begin());
					//	selected_end_backups.erase(selected_end_backups.begin());
					//	background_tiles_backups.erase(background_tiles_backups.begin());
					//}

					//if (selected_indices_backups.size() > 0 && undo_index < selected_indices_backups.size() - 1)
					//{
					//	selected_indices_backups.resize(undo_index);
					//	selected_start_backups.resize(undo_index);
					//	selected_end_backups.resize(undo_index);
					//	background_tiles_backups.resize(undo_index);
					//}

					//selected_indices_backups.push_back(selected_indices);
					//selected_start_backups.push_back(selected_start);
					//selected_end_backups.push_back(selected_end);
					//background_tiles_backups.push_back(background_tiles);
					//undo_index = selected_indices_backups.size() - 1;

					int x, y;
					SDL_GetMouseState(&x, &y);

					selected_start = glm::vec3((float)x, (float)y, 0);
					selected_end = glm::vec3((float)x, (float)y, 0);
				}
			}



			if ((tool == TOOL_SELECT || tool == TOOL_SELECT_ADD || tool == TOOL_SELECT_SUBTRACT) && event.type == SDL_MOUSEBUTTONUP)
			{
				if (event.button.button == SDL_BUTTON_LEFT)
				{
					//if (selected_indices_backups.size() > 10)
					//{
					//	selected_indices_backups.erase(selected_indices_backups.begin());
					//	selected_start_backups.erase(selected_start_backups.begin());
					//	selected_end_backups.erase(selected_end_backups.begin());
					//	background_tiles_backups.erase(background_tiles_backups.begin());
					//}

					//if (selected_indices_backups.size() > 0 && undo_index < selected_indices_backups.size() - 1)
					//{
					//	selected_indices_backups.resize(undo_index);
					//	selected_start_backups.resize(undo_index);
					//	selected_end_backups.resize(undo_index);
					//	background_tiles_backups.resize(undo_index);
					//}

					//selected_indices_backups.push_back(selected_indices);
					//selected_start_backups.push_back(selected_start);
					//selected_end_backups.push_back(selected_end);
					//background_tiles_backups.push_back(background_tiles);
					//undo_index = selected_indices_backups.size() - 1;

					int x, y;
					SDL_GetMouseState(&x, &y);

					selected_end = glm::vec3((float)x, (float)y, 0);
					make_selection = true;
				}
			}


			if ((tool == TOOL_PAINT || tool == TOOL_PAINT_SQUARE || tool == TOOL_PAINT_CUSTOM) && event.type == SDL_MOUSEBUTTONUP)
			{
				if (event.button.button == SDL_BUTTON_LEFT)
				{
					//if (selected_indices_backups.size() > 10)
					//{
					//	selected_indices_backups.erase(selected_indices_backups.begin());
					//	selected_start_backups.erase(selected_start_backups.begin());
					//	selected_end_backups.erase(selected_end_backups.begin());
					//	background_tiles_backups.erase(background_tiles_backups.begin());
					//}

					if (selected_indices_backups.size() > 0 && undo_index < selected_indices_backups.size() - 1)
					{
						selected_indices_backups.resize(undo_index + 1);
						selected_start_backups.resize(undo_index + 1);
						selected_end_backups.resize(undo_index + 1);
						background_tiles_backups.resize(undo_index + 1);
					}

					selected_indices_backups.push_back(selected_indices);
					selected_start_backups.push_back(selected_start);
					selected_end_backups.push_back(selected_end);
					background_tiles_backups.push_back(background_tiles);
					undo_index = selected_indices_backups.size() - 1;
				}
			}
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

		const ImVec2 img_size = { float(main_tiles_width), float(main_tiles_height) };

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

			ImGui::Image((void*)(intptr_t)main_tiles_texture, img_size, uv_min, uv_max, tint_col, border_col);
		}

		const ImVec2 mousePositionAbsolute = ImGui::GetMousePos();
		const ImVec2 screenPositionAbsolute = ImGui::GetItemRectMin();
		const ImVec2 mousePositionRelative = ImVec2(mousePositionAbsolute.x - screenPositionAbsolute.x, mousePositionAbsolute.y - screenPositionAbsolute.y);

		//ImVec2 img_block = ImVec2(floor(mousePositionRelative.x / block_size), floor(mousePositionRelative.y / block_size));
		//cout << img_block.x << " " << img_block.y << endl;


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





		ImGui::Begin("Tools", nullptr, ImGuiWindowFlags_HorizontalScrollbar);

		if (ImGui::IsWindowHovered())
			hovered = true;


		ImGui::RadioButton("Circle Paint", &tool, TOOL_PAINT);
		ImGui::RadioButton("Square Paint", &tool, TOOL_PAINT_SQUARE);
		ImGui::RadioButton("Custom Paint", &tool, TOOL_PAINT_CUSTOM);

		ImGui::RadioButton("Select", &tool, TOOL_SELECT);
		ImGui::RadioButton("Select Add", &tool, TOOL_SELECT_ADD);
		ImGui::RadioButton("Select Subtract", &tool, TOOL_SELECT_SUBTRACT);

		static char str1[128] = "5";
		ImGui::InputText("Brush size", str1, IM_ARRAYSIZE(str1));

		istringstream iss1(str1);
		iss1 >> brush_size;


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
				size_t x = size_t(mousePositionRelative.x) % block_size;
				size_t y = size_t(mousePositionRelative.y) % block_size;

				float u_start = (mousePositionRelative.x - x) / img_size.x;
				float v_start = (mousePositionRelative.y - y) / img_size.y;

				float u_end = block_size / img_size.x + u_start;
				float v_end = block_size / img_size.y + v_start;

				left_uv_mins[i] = ImVec2(u_start, v_start);
				left_uv_maxs[i] = ImVec2(u_end, v_end);

				//ImVec2 img_block = ImVec2(floor(mousePositionRelative.x / block_size), floor(mousePositionRelative.y / block_size));
				//cout << img_block.x << " " << img_block.y << endl;
				//left_indices[i] = img_block;
			}

			const ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
			const ImVec4 selected_border_col = ImVec4(1.0f, 0.5f, 0.0f, 1.0f);
			const ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

			if (i == left_selected)
				ImGui::Image((void*)(intptr_t)main_tiles_texture, thumbnail_img_size, left_uv_mins[i], left_uv_maxs[i], tint_col, selected_border_col);
			else
				ImGui::Image((void*)(intptr_t)main_tiles_texture, thumbnail_img_size, left_uv_mins[i], left_uv_maxs[i], tint_col, border_col);

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
				ImGui::Image((void*)(intptr_t)main_tiles_texture, thumbnail_img_size, right_uv_mins[i], right_uv_maxs[i], tint_col, selected_border_col);
			else
				ImGui::Image((void*)(intptr_t)main_tiles_texture, thumbnail_img_size, right_uv_mins[i], right_uv_maxs[i], tint_col, border_col);

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
			if (last_mousewheel < 0)
				zoom_factor *= 0.5;// last_mousewheel * 0.1f;
			else if (last_mousewheel > 0)
				zoom_factor *= 2.0;

			//if (last_mousewheel != 0)
			//{
			//	//image_anchor.x =  36.0f * float(tiles_per_dimension) / 2.0f  - float(window_w) / 2.0;
			//	//image_anchor.y =  36.0f * float(tiles_per_dimension) / 2.0f - float(window_h) / 2.0;
			//}
		}

		if (!hovered && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0) && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Space)))
		{
			ImVec2 motion = ImGui::GetMouseDragDelta();
			image_anchor.x += motion.x / zoom_factor;
			image_anchor.y += -motion.y / zoom_factor;

			ImGui::ResetMouseDragDelta();
		}


		if (!hovered && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0))
		{
			int x, y;
			SDL_GetMouseState(&x, &y);

			selected_end = glm::vec3((float)x, (float)y, 0);
		}


		// Paint using left mouse button
		if ((tool == TOOL_PAINT || tool == TOOL_PAINT_SQUARE || tool == TOOL_PAINT_CUSTOM) && !hovered && (ImGui::IsMouseDown(ImGuiMouseButton_Left)) && left_strings.size() > 0)
		{
			vector<float> weights;
			float total = 0;

			for (int i = 0; i < left_strings.size(); i++)
			{
				const float weight = stof(left_strings[i]);
				weights.push_back(weight);
				total += weight;
			}

			if (total != 0.0f && total != -0.0f)
			{
				for (int i = 0; i < left_strings.size(); i++)
					weights[i] /= total;
			}


			size_t brush_in_use = 0;
			set<pair<size_t, size_t>> to_draw;

			float max_brush_size = brush_size;

			if (tool == TOOL_PAINT_CUSTOM)
			{
				if (custom_brush1_width > max_brush_size)
					max_brush_size = custom_brush1_width;

				if (custom_brush1_height > max_brush_size)
					max_brush_size = custom_brush1_height;
			}

			int x, y;
			SDL_GetMouseState(&x, &y);

			ImVec2 centre_index = ImVec2(-image_anchor.x / (block_size)+x / (block_size * zoom_factor), -image_anchor.y / (block_size)+(io.DisplaySize.y - y) / (block_size * zoom_factor));

			ImVec2 centre_chunk;
			centre_chunk.x = -image_anchor.x / tiles_per_chunk_dimension / (block_size)+x / (block_size * zoom_factor) / (tiles_per_chunk_dimension);
			centre_chunk.y = -image_anchor.y / tiles_per_chunk_dimension / (block_size)+((int)io.DisplaySize.y - y) / (block_size * zoom_factor) / (tiles_per_chunk_dimension);

			float relative_brush_size = round(max_brush_size / float(tiles_per_chunk_dimension));
	//		cout << relative_brush_size << endl;

			ImVec2 start_chunk;
			start_chunk.x = centre_chunk.x - relative_brush_size;
			start_chunk.y = centre_chunk.y - relative_brush_size;

			ImVec2 end_chunk;
			end_chunk.x = centre_chunk.x + relative_brush_size;
			end_chunk.y = centre_chunk.y + relative_brush_size;

			start_chunk.x = glm::clamp(start_chunk.x, (float)0, (float)num_chunks_per_map_dimension - 1);
			start_chunk.y = glm::clamp(start_chunk.y, (float)0, (float)num_chunks_per_map_dimension - 1);

			end_chunk.x = glm::clamp(end_chunk.x, (float)0, (float)num_chunks_per_map_dimension - 1);
			end_chunk.y = glm::clamp(end_chunk.y, (float)0, (float)num_chunks_per_map_dimension - 1);

			//cout << "start chunk " << start_chunk.x << ' ' << start_chunk.y << endl;
			//cout << "end chunk " << end_chunk.x << ' ' << end_chunk.y << endl;


			set<pair<size_t, size_t>> custom_to_draw;

			if (tool == TOOL_PAINT_CUSTOM && !hovered)
			{
				int x, y;
				SDL_GetMouseState(&x, &y);

				for (int i = 0; i < custom_brush1_img.cols; i++)
				{
					for (int j = 0; j < custom_brush1_img.rows; j++)
					{
						unsigned char colour = 0;

						if (custom_brush1_img.channels() == 4)
						{
							Vec<unsigned char, 4> p = custom_brush1_img.at<Vec<unsigned char, 4>>(j, i);
							colour = p[0];
						}
						else if (custom_brush1_img.channels() == 3)
						{
							Vec<unsigned char, 3> p = custom_brush1_img.at<Vec<unsigned char, 3>>(j, i);
							colour = p[0];
						}




						if (colour != 255)
							continue;



						quad q;

						float half_width = -custom_brush1_img.cols * block_size / 2.0f;
						float half_height = custom_brush1_img.rows * block_size / 2.0f;

						q.vertices[0].x = x + block_size * zoom_factor * i - block_size * 0.5f * zoom_factor;// custom_brush1_img.rows;
						q.vertices[0].y = io.DisplaySize.y - y - block_size * zoom_factor * j - block_size * 0.5f * zoom_factor;//custom_brush1_img.cols;
						q.vertices[1].x = x + block_size * zoom_factor * i - block_size * 0.5f * zoom_factor;// custom_brush1_img.rows;
						q.vertices[1].y = io.DisplaySize.y - y - block_size * zoom_factor * j + block_size * 0.5f * zoom_factor;//custom_brush1_img.cols;
						q.vertices[2].x = x + block_size * zoom_factor * i + block_size * 0.5f * zoom_factor;// custom_brush1_img.rows;
						q.vertices[2].y = io.DisplaySize.y - y - block_size * zoom_factor * j + block_size * 0.5f * zoom_factor;//custom_brush1_img.cols;
						q.vertices[3].x = x + block_size * zoom_factor * i + block_size * 0.5f * zoom_factor;// custom_brush1_img.rows;
						q.vertices[3].y = io.DisplaySize.y - y - block_size * zoom_factor * j - block_size * 0.5f * zoom_factor;// custom_brush1_img.cols;

						q.vertices[0].x += half_width * zoom_factor;
						q.vertices[1].x += half_width * zoom_factor;
						q.vertices[2].x += half_width * zoom_factor;
						q.vertices[3].x += half_width * zoom_factor;

						q.vertices[0].y += half_height * zoom_factor;
						q.vertices[1].y += half_height * zoom_factor;
						q.vertices[2].y += half_height * zoom_factor;
						q.vertices[3].y += half_height * zoom_factor;

						glm::vec3 quad_centre = (q.vertices[0] + q.vertices[1] + q.vertices[2] + q.vertices[3]) * 0.25f; 
						
						pair<size_t, size_t> centre_index = make_pair(-image_anchor.x / (block_size)+quad_centre.x / (block_size * zoom_factor), -image_anchor.y / (block_size)+(quad_centre.y) / (block_size * zoom_factor));
						
						custom_to_draw.insert(centre_index);
					}
				}
			}









			for (size_t k = start_chunk.x; k <= end_chunk.x; k++)
			{
				for (size_t l = start_chunk.y; l <= end_chunk.y; l++)
				{
					size_t chunk_index = k * num_chunks_per_map_dimension + l;

					for (size_t m = 0; m < background_chunks[chunk_index].indices.size(); m++)
					{
						size_t i = background_chunks[chunk_index].indices[m].x;
						size_t j = background_chunks[chunk_index].indices[m].y;

						size_t index = i * tiles_per_dimension + j;

						if (tool == TOOL_PAINT)
						{
							glm::vec3 a((float)i, (float)j, 0);
							glm::vec3 b((float)centre_index.x, (float)centre_index.y, 0);

							if (distance(a, b) <= (brush_size * 0.5))
								to_draw.insert(make_pair(i, j));
						}
						else if (tool == TOOL_PAINT_SQUARE)
						{
							if (abs(i - centre_index.x) <= (brush_size * 0.5) && abs(j - centre_index.y) <= (brush_size) * 0.5)
								to_draw.insert(make_pair(i, j));
						}
						else if (tool == TOOL_PAINT_CUSTOM)
						{
							if(custom_to_draw.end() != custom_to_draw.find(make_pair(i, j)))
								to_draw.insert(make_pair(i, j));
						}
					}
				}
			}

			for (set<pair<size_t, size_t>>::const_iterator ci = to_draw.begin(); ci != to_draw.end(); ci++)
			{
				pair<size_t, size_t> pair_index = make_pair(ci->first, ci->second);

				// This makes things terribly slow for very large brush sizes(e.g., 500 tiles)
				//if (prev_draw.end() != find(prev_draw.begin(), prev_draw.end(), make_pair(pair_index.first, pair_index.second)))
				//	continue;


				size_t index = pair_index.first * tiles_per_dimension + pair_index.second;

				size_t brush_in_use = 0;

				const float r = distribution(generator);

				float sub_total = 0;

				for (int k = 0; k < left_strings.size(); k++)
				{
					sub_total += weights[k];

					if (r <= sub_total)
					{
						brush_in_use = k;
						break;
					}
				}

				if (selected_indices.size() == 0 || selected_indices.end() != selected_indices.find(make_pair(pair_index.first, pair_index.second)))
				{
					background_tiles[index].uv_min = left_uv_mins[brush_in_use];
					background_tiles[index].uv_max = left_uv_maxs[brush_in_use];
				}
			}

			//prev_draw = to_draw;
			to_draw.clear();
		}





		if ((tool == TOOL_SELECT || tool == TOOL_SELECT_ADD || tool == TOOL_SELECT_SUBTRACT) && make_selection && !hovered)// && !ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Space)))
		{
			make_selection = false;

			if (tool == TOOL_SELECT && prev_tools.size() > 0 && prev_tools[prev_tools.size() - 1] == TOOL_SELECT)
			{
				prev_tools.clear();
				prev_tools.push_back(TOOL_SELECT);
				selected_indices.clear();
			}

			glm::vec3 start_chunk;// = glm::vec3(num_chunks_per_map_dimension - 1, num_chunks_per_map_dimension - 1, 0);
			glm::vec3 end_chunk;// = glm::vec3(0.0f, 0.0f, 0.0f);

			start_chunk.x = -image_anchor.x / tiles_per_chunk_dimension / (block_size)+selected_start.x / (block_size * zoom_factor) / (tiles_per_chunk_dimension);
			start_chunk.y = -image_anchor.y / tiles_per_chunk_dimension / (block_size)+((int)io.DisplaySize.y - selected_start.y) / (block_size * zoom_factor) / (tiles_per_chunk_dimension);

			end_chunk.x = -image_anchor.x / tiles_per_chunk_dimension / (block_size)+selected_end.x / (block_size * zoom_factor) / (tiles_per_chunk_dimension);
			end_chunk.y = -image_anchor.y / tiles_per_chunk_dimension / (block_size)+((int)io.DisplaySize.y - selected_end.y) / (block_size * zoom_factor) / (tiles_per_chunk_dimension);

			if (end_chunk.x < start_chunk.x)
			{
				float temp = end_chunk.x;
				end_chunk.x = start_chunk.x;
				start_chunk.x = temp;
			}

			if (end_chunk.y < start_chunk.y)
			{
				float temp = end_chunk.y;
				end_chunk.y = start_chunk.y;
				start_chunk.y = temp;
			}

			start_chunk -= 1.0;
			end_chunk += 1.0;

			start_chunk.x = glm::clamp(start_chunk.x, (float)0, (float)num_chunks_per_map_dimension - 1);
			start_chunk.y = glm::clamp(start_chunk.y, (float)0, (float)num_chunks_per_map_dimension - 1);

			end_chunk.x = glm::clamp(end_chunk.x, (float)0, (float)num_chunks_per_map_dimension - 1);
			end_chunk.y = glm::clamp(end_chunk.y, (float)0, (float)num_chunks_per_map_dimension - 1);

			//cout << "start chunk " << start_chunk.x << ' ' << start_chunk.y << endl;
			//cout << "end chunk " << end_chunk.x << ' ' << end_chunk.y << endl;

			for (size_t k = start_chunk.x; k <= end_chunk.x; k++)
			{
				for (size_t l = start_chunk.y; l <= end_chunk.y; l++)
				{
					size_t chunk_index = k * num_chunks_per_map_dimension + l;

					for (size_t m = 0; m < background_chunks[chunk_index].indices.size(); m++)
					{
						size_t i = background_chunks[chunk_index].indices[m].x;
						size_t j = background_chunks[chunk_index].indices[m].y;

						size_t index = i * tiles_per_dimension + j;

						const float x = ((image_anchor.x) + int(i) * background_tiles[index].tile_size);
						const float y = ((image_anchor.y) + int(j) * background_tiles[index].tile_size);

						complex<float> v0w(static_cast<float>(x), static_cast<float>(y));
						complex<float> v1w(static_cast<float>(x), static_cast<float>(y + background_tiles[index].tile_size));
						complex<float> v2w(static_cast<float>(x + background_tiles[index].tile_size), static_cast<float>(y + background_tiles[index].tile_size));
						complex<float> v3w(static_cast<float>(x + background_tiles[index].tile_size), static_cast<float>(y));

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
						q.vertices[0].y = v0w.imag();
						q.vertices[1].x = v1w.real();
						q.vertices[1].y = v1w.imag();
						q.vertices[2].x = v2w.real();
						q.vertices[2].y = v2w.imag();
						q.vertices[3].x = v3w.real();
						q.vertices[3].y = v3w.imag();

						glm::vec3 quad_centre = (q.vertices[0] + q.vertices[1] + q.vertices[2] + q.vertices[3]) * 0.25f;

						vector<glm::vec3> points;
						points.push_back(glm::vec3(selected_start.x, (int)io.DisplaySize.y - selected_start.y, 0));
						points.push_back(glm::vec3(selected_start.x, (int)io.DisplaySize.y - selected_end.y, 0));
						points.push_back(glm::vec3(selected_end.x, (int)io.DisplaySize.y - selected_end.y, 0));
						points.push_back(glm::vec3(selected_end.x, (int)io.DisplaySize.y - selected_start.y, 0));

						if (point_in_polygon(quad_centre, points) ||
							point_in_polygon(q.vertices[0], points) ||
							point_in_polygon(q.vertices[1], points) ||
							point_in_polygon(q.vertices[2], points) ||
							point_in_polygon(q.vertices[3], points))
						{
							if (tool == TOOL_SELECT_SUBTRACT)
								selected_indices.erase(make_pair(i, j));
							else
								selected_indices.insert(make_pair(i, j));
						}
					}
				}
			}
		}


		//if (!hovered && ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0))
		//{
		//	// paint using right mouse button
		//}

		projection = glm::mat4x4();// glm::ortho(0.0f, io.DisplaySize.x, -io.DisplaySize.y, 0.0f, -1.0f, 1.0f);
		view = glm::mat4x4();// glm::lookAt(glm::vec3(0, 0, 1), glm::vec3(0, 0, -1), glm::vec3(0, 1, 0));
		model = glm::mat4x4();

		glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
		glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);

		vector<float> vertex_data;
		vector<GLuint> index_data;

		for (size_t i = 0; i < tiles_per_dimension; i++)
		{
			for (size_t j = 0; j < tiles_per_dimension; j++)
			{
				size_t index = i * tiles_per_dimension + j;

				int x = int(image_anchor.x) + int(i) * background_tiles[index].tile_size;
				int y = int(image_anchor.y) + int(j) * background_tiles[index].tile_size;

				get_quad_ndc_data(vertex_data, index_data, x, y, background_tiles[index].tile_size, (int)io.DisplaySize.x, (int)io.DisplaySize.y, background_tiles[index].uv_min, background_tiles[index].uv_max);
			}
		}

		draw_quad_ndc_data(vertex_data, index_data, main_tiles_texture, (int)io.DisplaySize.x, (int)io.DisplaySize.y);


		// Draw outlines around each tile
		if (zoom_factor > 0.5)
		{
			for (size_t i = 0; i < tiles_per_dimension; i++)
			{
				for (size_t j = 0; j < tiles_per_dimension; j++)
				{
					size_t index = i * tiles_per_dimension + j;

					const float x = int(image_anchor.x) + int(i) * background_tiles[index].tile_size;
					const float y = int(image_anchor.y) + int(j) * background_tiles[index].tile_size;

					complex<float> v0w(static_cast<float>(x), static_cast<float>(y));
					complex<float> v1w(static_cast<float>(x), static_cast<float>(y + background_tiles[index].tile_size));
					complex<float> v2w(static_cast<float>(x + background_tiles[index].tile_size), static_cast<float>(y + background_tiles[index].tile_size));
					complex<float> v3w(static_cast<float>(x + background_tiles[index].tile_size), static_cast<float>(y));

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
					q.vertices[0].y = v0w.imag();
					q.vertices[1].x = v1w.real();
					q.vertices[1].y = v1w.imag();
					q.vertices[2].x = v2w.real();
					q.vertices[2].y = v2w.imag();
					q.vertices[3].x = v3w.real();
					q.vertices[3].y = v3w.imag();

					draw_quad_line_loop(glm::vec3(0.1, 0.1, 0.1), (int)io.DisplaySize.x, (int)io.DisplaySize.y, 1.0, q);
				}
			}
		}


		vertex_data.clear();

		for (set<pair<size_t, size_t>>::const_iterator ci = selected_indices.begin(); ci != selected_indices.end(); ci++)
		{
			int i = ci->first;
			int j = ci->second;

			size_t index = i * tiles_per_dimension + j;

			float x = int(image_anchor.x) + int(i) * background_tiles[index].tile_size;
			float y = int(image_anchor.y) + int(j) * background_tiles[index].tile_size;

			complex<float> v0w(static_cast<float>(x), static_cast<float>(y));
			complex<float> v1w(static_cast<float>(x), static_cast<float>(y + background_tiles[index].tile_size));
			complex<float> v2w(static_cast<float>(x + background_tiles[index].tile_size), static_cast<float>(y + background_tiles[index].tile_size));
			complex<float> v3w(static_cast<float>(x + background_tiles[index].tile_size), static_cast<float>(y));

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
			q.vertices[0].y = v0w.imag();
			q.vertices[1].x = v1w.real();
			q.vertices[1].y = v1w.imag();
			q.vertices[2].x = v2w.real();
			q.vertices[2].y = v2w.imag();
			q.vertices[3].x = v3w.real();
			q.vertices[3].y = v3w.imag();

			get_quad_lines_ndc_data(vertex_data, x, y, background_tiles[index].tile_size, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
		}

		draw_quad_line_ndc_data(vertex_data, (int)io.DisplaySize.x, (int)io.DisplaySize.y);

		// Draw selected outline 
		if ((tool == TOOL_SELECT || tool == TOOL_SELECT_ADD || tool == TOOL_SELECT_SUBTRACT) && ImGui::IsMouseDown(ImGuiMouseButton_Left))
		{
			quad q;

			complex<float> v0w(static_cast<float>(selected_start.x), static_cast<float>((int)io.DisplaySize.y - selected_start.y));
			complex<float> v1w(static_cast<float>(selected_start.x), static_cast<float>((int)io.DisplaySize.y - selected_end.y));
			complex<float> v2w(static_cast<float>(selected_end.x), static_cast<float>((int)io.DisplaySize.y - selected_end.y));
			complex<float> v3w(static_cast<float>(selected_end.x), static_cast<float>((int)io.DisplaySize.y - selected_start.y));

			q.vertices[0].x = v0w.real();
			q.vertices[0].y = v0w.imag();
			q.vertices[1].x = v1w.real();
			q.vertices[1].y = v1w.imag();
			q.vertices[2].x = v2w.real();
			q.vertices[2].y = v2w.imag();
			q.vertices[3].x = v3w.real();
			q.vertices[3].y = v3w.imag();

			draw_quad_line_loop(glm::vec3(0, 0, 1), (int)io.DisplaySize.x, (int)io.DisplaySize.y, 4.0, q);
		}

		// Draw brush outline
		if (tool == TOOL_PAINT)
		{
			int x, y;
			SDL_GetMouseState(&x, &y);
			glm::vec3 pos(x, (int)io.DisplaySize.y - y, 0);

			draw_circle_line_loop(glm::vec3(1, 1, 1), (int)io.DisplaySize.x, (int)io.DisplaySize.y, 4.0, pos, zoom_factor * (float)brush_size * block_size * 0.5f, 20);
		}
		else if (tool == TOOL_PAINT_SQUARE)
		{
			int x, y;
			SDL_GetMouseState(&x, &y);

			quad q;

			q.vertices[0].x = x - zoom_factor * (float)brush_size * block_size * 0.5f;
			q.vertices[0].y = (int)io.DisplaySize.y - y - zoom_factor * (float)brush_size * block_size * 0.5f;
			q.vertices[1].x = x - zoom_factor * (float)brush_size * block_size * 0.5f;
			q.vertices[1].y = (int)io.DisplaySize.y - y + zoom_factor * (float)brush_size * block_size * 0.5f;
			q.vertices[2].x = x + zoom_factor * (float)brush_size * block_size * 0.5f;
			q.vertices[2].y = (int)io.DisplaySize.y - y + zoom_factor * (float)brush_size * block_size * 0.5f;
			q.vertices[3].x = x + zoom_factor * (float)brush_size * block_size * 0.5f;
			q.vertices[3].y = (int)io.DisplaySize.y - y - zoom_factor * (float)brush_size * block_size * 0.5f;

			draw_quad_line_loop(glm::vec3(1, 1, 1), (int)io.DisplaySize.x, (int)io.DisplaySize.y, 4.0, q);
		}

		else if (tool == TOOL_PAINT_CUSTOM)
		{
			int x, y;
			SDL_GetMouseState(&x, &y);

			for (int i = 0; i < custom_brush1_img.cols; i++)
			{
				for (int j = 0; j < custom_brush1_img.rows; j++)
				{
					unsigned char colour = 0;

					if (custom_brush1_img.channels() == 4)
					{
						Vec<unsigned char, 4> p = custom_brush1_img.at<Vec<unsigned char, 4>>(j, i);
						colour = p[0];
					}
					else if (custom_brush1_img.channels() == 3)
					{
						Vec<unsigned char, 3> p = custom_brush1_img.at<Vec<unsigned char, 3>>(j, i);
						colour = p[0];
					}




					if (colour != 255)
						continue;



					quad q;

					float half_width = -custom_brush1_img.cols * block_size / 2.0f;
					float half_height = custom_brush1_img.rows * block_size / 2.0f;

					q.vertices[0].x = x + block_size * zoom_factor * i - block_size * 0.5f * zoom_factor;// custom_brush1_img.rows;
					q.vertices[0].y = io.DisplaySize.y - y - block_size * zoom_factor * j - block_size * 0.5f * zoom_factor;//custom_brush1_img.cols;
					q.vertices[1].x = x + block_size * zoom_factor * i - block_size * 0.5f * zoom_factor;// custom_brush1_img.rows;
					q.vertices[1].y = io.DisplaySize.y - y - block_size * zoom_factor * j + block_size * 0.5f * zoom_factor;//custom_brush1_img.cols;
					q.vertices[2].x = x + block_size * zoom_factor * i + block_size * 0.5f * zoom_factor;// custom_brush1_img.rows;
					q.vertices[2].y = io.DisplaySize.y - y - block_size * zoom_factor * j + block_size * 0.5f * zoom_factor;//custom_brush1_img.cols;
					q.vertices[3].x = x + block_size * zoom_factor * i + block_size * 0.5f * zoom_factor;// custom_brush1_img.rows;
					q.vertices[3].y = io.DisplaySize.y - y - block_size * zoom_factor * j - block_size * 0.5f * zoom_factor;// custom_brush1_img.cols;

					q.vertices[0].x += half_width * zoom_factor;
					q.vertices[1].x += half_width * zoom_factor;
					q.vertices[2].x += half_width * zoom_factor;
					q.vertices[3].x += half_width * zoom_factor;

					q.vertices[0].y += half_height * zoom_factor;
					q.vertices[1].y += half_height * zoom_factor;
					q.vertices[2].y += half_height * zoom_factor;
					q.vertices[3].y += half_height * zoom_factor;

					draw_quad_line_loop(glm::vec3(1, 1, 1), (int)io.DisplaySize.x, (int)io.DisplaySize.y, 4.0, q);
				}
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