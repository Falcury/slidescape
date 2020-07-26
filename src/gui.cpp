/*
  Slideviewer, a whole-slide image viewer for digital pathology.
  Copyright (C) 2019-2020  Pieter Valkema

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "common.h"

#include "stdio.h"

#include <windows.h>
#include "platform.h"
#include "win32_main.h"

#include <glad/glad.h>

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_win32.h"

#include "openslide_api.h"
#include "viewer.h"
#include "tlsclient.h"
#include "caselist.h"

#define GUI_IMPL
#include "gui.h"
#include "annotation.h"
#include "stringutils.h"

void gui_new_frame() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void menu_close_file(app_state_t* app_state) {
	unload_all_images(app_state);
	reset_global_caselist(app_state);
	unload_and_reinit_annotations(&app_state->scene.annotation_set);
}

void gui_draw(app_state_t* app_state, input_t* input, i32 client_width, i32 client_height) {
	ImGuiIO &io = ImGui::GetIO();

	gui_want_capture_mouse = io.WantCaptureMouse;
	gui_want_capture_keyboard = io.WantCaptureKeyboard;


	// Start the Dear ImGui frame
//	ImGui_ImplOpenGL3_NewFrame();
//	ImGui_ImplWin32_NewFrame();
//	ImGui::NewFrame();


	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	bool ret = ImGui::BeginMainMenuBar();
	ImGui::PopStyleVar(1);
	if (ret) {
		static struct {
			bool open_file;
			bool close;
			bool open_remote;
			bool exit_program;
			bool show_case_list;
			bool save_annotations;
		} menu_items_clicked;
		memset(&menu_items_clicked, 0, sizeof(menu_items_clicked));

		bool prev_fullscreen = is_fullscreen;

		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Open...", "Ctrl+O", &menu_items_clicked.open_file)) {}
			if (ImGui::MenuItem("Close", "Ctrl+W", &menu_items_clicked.close)) {}
			ImGui::Separator();
			if (ImGui::MenuItem("Exit", "Alt+F4", &menu_items_clicked.exit_program)) {}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Annotation")) {
			if (ImGui::MenuItem("Load...", NULL, &menu_items_clicked.open_file)) {} // TODO: only accept annotation files here?
			ImGui::Separator();
			if (ImGui::MenuItem("Annotations...", NULL, &show_annotations_window)) {}
			if (ImGui::MenuItem("Assign group...", NULL, &show_annotation_group_assignment_window)) {}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("View")) {
			prev_fullscreen = is_fullscreen = win32_is_fullscreen(main_window); // double-check just in case...
			if (ImGui::MenuItem("Fullscreen", "F11", &is_fullscreen)) {}
			if (ImGui::MenuItem("Image adjustments...", NULL, &show_image_adjustments_window)) {}

			ImGui::Separator();

			if (ImGui::MenuItem("Options...", NULL, &show_display_options_window)) {}
			if (ImGui::BeginMenu("Debug")) {
				if (ImGui::MenuItem("Demo window", "F1", &show_demo_window)) {}
				if (ImGui::MenuItem("Open remote", NULL, &menu_items_clicked.open_remote)) {}
				if (ImGui::MenuItem("Show case list", NULL, &menu_items_clicked.show_case_list)) {}
//				if (ImGui::MenuItem("Save XML annotations", NULL, &menu_items_clicked.save_annotations)) {}
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Help")) {
			if (ImGui::MenuItem("About...", NULL, &show_about_window)) {}
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();

		if (menu_items_clicked.exit_program) {
			is_program_running = false;
		} else if (menu_items_clicked.open_file) {
			win32_open_file_dialog(main_window);
		} else if (menu_items_clicked.close) {
			menu_close_file(app_state);
		} else if (menu_items_clicked.open_remote) {
			show_open_remote_window = true;
		} else if (menu_items_clicked.show_case_list) {
			reload_global_caselist(app_state, "cases.json");
			show_slide_list_window = true;
		} else if (prev_fullscreen != is_fullscreen) {
			bool currently_fullscreen = win32_is_fullscreen(main_window);
			if (currently_fullscreen != is_fullscreen) {
				win32_toggle_fullscreen(main_window);
			}
		} else if(menu_items_clicked.save_annotations) {
			save_asap_xml_annotations(&app_state->scene.annotation_set, "test_out.xml");
		}
	}

	if (show_open_remote_window) {
		ImGui::SetNextWindowPos(ImVec2(120, 100), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(256, 156), ImGuiCond_FirstUseEver);

		ImGui::Begin("Open remote", &show_open_remote_window);

		ImGuiInputTextFlags input_flags = ImGuiInputTextFlags_EnterReturnsTrue;
		bool entered = false;
		entered = entered || ImGui::InputText("Hostname", remote_hostname, sizeof(remote_hostname), input_flags);
		entered = entered || ImGui::InputText("Port", remote_port, sizeof(remote_port), input_flags);
		entered = entered || ImGui::InputText("Filename", remote_filename, sizeof(remote_filename), input_flags);
		if (entered || ImGui::Button("Connect")) {
			const char* ext = get_file_extension(remote_filename);
			if (strcasecmp(ext, "json") == 0) {
				// Open as 'caselist'
				unload_all_images(app_state);
				reset_global_caselist(app_state);
				if (load_caselist_from_remote(&app_state->caselist, remote_hostname, atoi(remote_port), remote_filename)) {
					show_slide_list_window = true;
					show_open_remote_window = false; // success!
				}
			} else {
				// Open as 'slide'
				if (open_remote_slide(app_state, remote_hostname, atoi(remote_port), remote_filename)) {
					show_open_remote_window = false; // success!
				}
			}


		}
		ImGui::End();
	}


	// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
	if (show_demo_window)
		ImGui::ShowDemoWindow(&show_demo_window);

	// 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
	if (show_image_adjustments_window) {
		static int counter = 0;

		ImGui::SetNextWindowPos(ImVec2(25, 50), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(360, 200), ImGuiCond_FirstUseEver);

		ImGui::Begin("Image adjustments",
		             &show_image_adjustments_window);                          // Create a window called "Hello, world!" and append into it.

//		ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
		ImGui::Checkbox("Use image adjustments", &app_state->use_image_adjustments);

		ImGui::SliderFloat("black level", &app_state->black_level, 0.0f,
		                   1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
		ImGui::SliderFloat("white level", &app_state->white_level, 0.0f,
		                   1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f



//		if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
//			counter++;
//		ImGui::SameLine();
//		ImGui::Text("counter = %d", counter);
//
//		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();
	}

	if (show_display_options_window) {

		ImGui::SetNextWindowPos(ImVec2(120, 100), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(350, 250), ImGuiCond_FirstUseEver);

		ImGui::Begin("Options", &show_display_options_window);


		// General BeginCombo() API, you have full control over your selection data and display type.
		// (your selection data could be an index, a pointer to the object, an id for the object, a flag stored in the object itself, etc.)
		const char* items[] = {"Dark", "Light", "Classic"};
		static i32 style_color = 0;
		int old_style_color = style_color;
		static ImGuiComboFlags flags = 0;
		ImGui::Text("User interface colors");               // Display some text (you can use a format strings too)
		if (ImGui::BeginCombo("##user_interface_colors_combo", items[style_color],
		                      flags)) // The second parameter is the label previewed before opening the combo.
		{
			for (int n = 0; n < IM_ARRAYSIZE(items); n++) {
				bool is_selected = (style_color == n);
				if (ImGui::Selectable(items[n], is_selected))
					style_color = n;
				if (style_color)
					ImGui::SetItemDefaultFocus();   // Set the initial focus when opening the combo (scrolling + for keyboard navigation support in the upcoming navigation branch)
			}
			ImGui::EndCombo();

			if (style_color != old_style_color) {
				if (style_color == 0) {
					ImGui::StyleColorsDark();
				} else if (style_color == 1) {
					ImGui::StyleColorsLight();
				} else if (style_color == 2) {
					ImGui::StyleColorsClassic();
				}
			}
		}

		ImGui::Text("\nBackground color");               // Display some text (you can use a format strings too)
		ImGui::ColorEdit3("color", (float*) &app_state->clear_color); // Edit 3 floats representing a color

		ImGui::Text("\nTIFF backend");
//		ImGui::Checkbox("Prefer built-in TIFF backend over OpenSlide", &use_builtin_tiff_backend);
		const char* tiff_backends[] = {"Built-in", "OpenSlide"};
		if (ImGui::BeginCombo("##tiff_backend", tiff_backends[1 - app_state->use_builtin_tiff_backend],
		                      flags)) // The second parameter is the label previewed before opening the combo.
		{
			if (ImGui::Selectable(tiff_backends[0], app_state->use_builtin_tiff_backend)) {
				app_state->use_builtin_tiff_backend = true;
			}
			if (app_state->use_builtin_tiff_backend) ImGui::SetItemDefaultFocus();
			if (is_openslide_available) {
				if (ImGui::Selectable(tiff_backends[1], !app_state->use_builtin_tiff_backend)) {
					app_state->use_builtin_tiff_backend = false;
				}
				if (!app_state->use_builtin_tiff_backend) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

//		ImGui::Text("\nGlobal Alpha");
//		ImGui::SliderFloat("##Global Alpha", &ImGui::GetStyle().Alpha, 0.20f, 1.0f, "%.2f"); // Not exposing zero here so user doesn't "lose" the UI (zero alpha clips all widgets). But application code could have a toggle to switch between zero and non-zero.



		ImGui::End();
	}

	if (show_slide_list_window) {

		ImGui::SetNextWindowPos(ImVec2(20, 50), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(180, 530), ImGuiCond_FirstUseEver);

		ImGui::Begin("Select case", &show_slide_list_window);

		// List box
		const char* listbox_items_dummy[] = {"",};
		const char** listbox_items = listbox_items_dummy;
		i32 items_count = 0;

		caselist_t* caselist = &app_state->caselist;
		if (caselist->names) {
			listbox_items = caselist->names;
			items_count = caselist->num_cases_with_filenames;
		}

		static int listbox_item_current = -1;
		float line_height = ImGui::GetTextLineHeightWithSpacing();
		float list_height_in_items = ImGui::GetWindowHeight() / line_height;
		if (ImGui::ListBox("##listbox\n(single select)", &listbox_item_current, listbox_items, items_count,
		                   (int) (list_height_in_items - 2.5f))) {
			// value changed
			if (caselist->cases) {
				app_state->selected_case = caselist->cases + listbox_item_current;
				show_case_info_window = true;
				unload_all_images(app_state);
				if (app_state->selected_case->filename) {

					if (caselist->is_remote) {
						open_remote_slide(app_state, remote_hostname, atoi(remote_port), app_state->selected_case->filename);
					} else {
						// If the SLIDES_DIR environment variable is set, load slides from there
						char path_buffer[2048] = {};
						snprintf(path_buffer, sizeof(path_buffer), "%s%s", caselist->folder_prefix,
						         app_state->selected_case->filename);

						load_image_from_file(app_state, path_buffer);
					}


				}
			}
		}

		// stub


		ImGui::End();


	}

	if (show_case_info_window) {
		ImGui::SetNextWindowPos(ImVec2(20, 600), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(400, 250), ImGuiCond_FirstUseEver);

		ImGui::Begin("Case info", &show_case_info_window);

		case_t* global_selected_case = app_state->selected_case;
		if (global_selected_case != NULL) {
			ImGui::TextWrapped("%s\n", global_selected_case->name);
			ImGui::TextWrapped("%s\n", global_selected_case->clinical_context);
			if (ImGui::TreeNode("Diagnosis and comment")) {
				ImGui::TextWrapped("%s\n", global_selected_case->diagnosis);
				ImGui::TextWrapped("%s\n", global_selected_case->notes);
				ImGui::TreePop();
			}


		}


		ImGui::End();
	}

	if (show_annotations_window || show_annotation_group_assignment_window) {
		draw_annotations_window(app_state, input);
	}

	if (show_about_window) {
		ImGui::Begin("About Slideviewer", &show_about_window, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);

		ImGui::TextUnformatted("Slideviewer - a whole-slide image viewer for digital pathology");
		ImGui::Text("Author: Pieter Valkema\n");
		ImGui::TextUnformatted("Version: " SLIDEVIEWER_VERSION );


		ImGui::Text("\nLicense information:\nThis program is free software: you can redistribute it and/or modify\n"
		            "  it under the terms of the GNU General Public License as published by\n"
		            "  the Free Software Foundation, either version 3 of the License, or\n"
		            "  (at your option) any later version.\n\n");
		if (ImGui::Button("View releases on GitHub")) {
#if WINDOWS
			ShellExecuteW(0, 0, L"https://github.com/Falcury/slideviewer/releases", 0, 0 , SW_SHOW );
#endif
		};


		ImGui::End();
	}



//}
//
//void gui_render(app_state_t* app_state, i32 client_width, i32 client_height) {

	// Rendering
	ImGui::Render();
	glViewport(0, 0, client_width, client_height);
//	glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
//	glClear(GL_COLOR_BUFFER_BIT);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());



}

void win32_init_gui(HWND hwnd) {

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
//	ImGui::StyleColorsLight();
//	ImGui::StyleColorsClassic();

	ImGuiStyle& style = ImGui::GetStyle();
	style.Alpha = 0.95f;
	style.DisplaySafeAreaPadding = ImVec2(0.0f, 0.0f);
	style.TouchExtraPadding = ImVec2(0.0f, 1.0f);

	// Setup Platform/Renderer bindings
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplOpenGL3_Init(NULL);

	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Read 'docs/FONTS.txt' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
	ImFontConfig font_config = ImFontConfig();
//	font_config.OversampleH = 3;
//	font_config.OversampleV = 2;
//	font_config.RasterizerMultiply = 1.2f;
	ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 17.0f, &font_config, io.Fonts->GetGlyphRangesJapanese());
	if (!font) {
		// could not load font
	}
	io.Fonts->AddFontDefault();
//	IM_ASSERT(font != NULL);

	is_fullscreen = win32_is_fullscreen(main_window);

}