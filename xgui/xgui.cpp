#include "xgui.hpp"
#include "window.hpp"
#include "../input.hpp"
#include "../emulator.hpp"
#include <vector>

#include "../console.hpp"
//#include "console.hpp"
/*#include "sound_viewer.hpp"
#include "tile_inspector.hpp"
#include "memory.hpp"
#include "state_manager.hpp"
#include "lua_debugger.hpp"
#include "vblank_ppu.hpp"
#include "config.hpp"
#include "TextEditor.h"*/
#include <filesystem>

// windows
#include "about_w.hpp"
#include "console_w.hpp"
#include "demo_w.hpp"
#include "docs_w.hpp"
#include "editor_w.hpp"
#include "lua_w.hpp"
#include "input_w.hpp"
#include "graphics_w.hpp"
#include "state_w.hpp"
#include "memory_w.hpp"
#include "file_select_w.hpp"

namespace xpp::xgui {

namespace {

int game_advances = 0;
int emu_advances = 0;

void
load_fonts(void)
{
	font_h1 = imgui_load_font("roboto.ttf", 28.f);
	font_h2 = imgui_load_font("roboto.ttf", 24.f);
	font_h3 = imgui_load_font("roboto.ttf", 20.f);
	font_mono = imgui_load_font("robotomono.ttf", 15.f);
}

} // namespace

// interface

std::vector<window_t *> windows;
ImFont *font_h1;
ImFont *font_h2;
ImFont *font_h3;
ImFont *font_mono;
int game_scale = 2;
bool game_visible = true;
float emu_alpha = 0.5f;

// TODO: themes settings include purple, green, and vgui

void
init(void)
{
	imguiCreate();
	ImGui_ImplSDL2_InitForVulkan(video::window);
	//ImGuiIO &io = ImGui::GetIO();
	//io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	ImGuiStyle &style = ImGui::GetStyle();
	style.WindowTitleAlign = ImVec2(0.5f, 0.5f); // centered titles
	style.WindowMenuButtonPosition = -1; // hidden v arrow

	load_fonts();

	add_window<about_w>();
	add_window<console_w>();
	add_window<demo_w>();
	add_window<docs_w>();
	add_window<editor_w>();
	add_window<lua_w>();
	add_window<input_w>();
	add_window<graphics_w>();
	add_window<state_w>();
	add_window<memory_w>();

	add_window<file_select_w>();
}

void
begin(void)
{
	// don't do call it here anymore, messes up input stuff
	//ImGui_ImplSDL2_NewFrame(video::window);
	imguiBeginFrame(0, 0, 0, 0, video::width(), video::height());

	ImGuiIO &io = ImGui::GetIO();
	io.KeyCtrl = input::getkey(SDL_SCANCODE_LCTRL) || input::getkey(SDL_SCANCODE_RCTRL);
	io.KeyShift = input::getkey(SDL_SCANCODE_LSHIFT) || input::getkey(SDL_SCANCODE_RSHIFT);
	io.KeyAlt = input::getkey(SDL_SCANCODE_LALT) || input::getkey(SDL_SCANCODE_RALT);
	//io.KeySuper = io.KeysDown[SDL_SCANCODE_LSUPER] || io.KeysDown[SDL_SCANCODE_RSUPER];

	ImGui::ShowDemoWindow();
}

void
end(void)
{
	imguiEndFrame();
}

void do_menu_windows(std::string name) {
	for (auto i : windows)
		if (i->visible_in_menu && i->under_menu == name)
			if (ImGui::MenuItem(i->name.c_str(), NULL, i->visible))
				i->visible = !i->visible;
}

void
draw_menu_bar(void)
{
	if (ImGui::BeginMainMenuBar()) {
		/*ImGui::Text(" %s ", engine::rom.c_str());
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("New Project")) {}
			if (ImGui::MenuItem("Save Project", "CTRL+S")) {}
			if (ImGui::MenuItem("Open Project", "CTRL+O")) {}
			ImGui::Separator();
			if (ImGui::MenuItem("Load Cartridge"))
				open_rom_select = true;
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("SNES")) {
			if (ImGui::MenuItem("Running", NULL, emulator::running))
				emulator::set_running(!emulator::running);
			if (ImGui::MenuItem("Frame Advance", NULL, emulator::running))
				emulator::run_frames = 1;
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Lua")) {
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("View")) {
			if (ImGui::MenuItem("Console", NULL, view_console))
				view_console = !view_console;
			if (ImGui::MenuItem("Config", NULL, view_config))
				view_config = !view_config;
			if (ImGui::MenuItem("State Manager", NULL, view_states))
				view_states = !view_states;
			if (ImGui::MenuItem("Game", NULL, view_game))
				view_game = !view_game;
			if (ImGui::MenuItem("APU", NULL, view_apu))
				view_apu = !view_apu;
			if (ImGui::MenuItem("PPU", NULL, view_ppu))
				view_ppu = !view_ppu;
			if (ImGui::MenuItem("Memory", NULL, view_ram))
				view_ram = !view_ram;
			if (ImGui::MenuItem("Scripts", NULL, view_scripts))
				view_scripts = !view_scripts;
			ImGui::EndMenu();
		}*/
		if (ImGui::BeginMenu("View")) {
			do_menu_windows("View");
			if (ImGui::MenuItem("Game", NULL, game_visible))
				game_visible = !game_visible;
			ImGui::Separator();
			if (ImGui::MenuItem("File Picker"))
				((file_select_w *)(windows[windows.size() - 1]))->open();
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Help")) {
			do_menu_windows("Help");
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
}

static void
handle_emulator(float alpha)
{
	if (emulator::focused) {
		if (ImGui::IsKeyPressed(SDL_SCANCODE_BACKSLASH)) {
			emulator::pause();
			++emulator::run_frames;
			++emu_advances;
			if (emu_advances > 1){
				//SuperFamicom::ppu.bg_enable(0, false);
				//SuperFamicom::ppu.bg_enable(1, false);
				//SuperFamicom::ppu.bg_enable(2, false);
				//SuperFamicom::ppu.bg_enable(3, false);
			}
		}
		if (ImGui::IsKeyPressed(SDL_SCANCODE_RIGHTBRACKET))
			emulator::resume();
	}
	if (lua["game_focused"]) {
		if (ImGui::IsKeyPressed(SDL_SCANCODE_BACKSLASH)) {
			lua["game_running"] = false;
			lua["run_frames"] = 1;
			++game_advances;
		}
		if (ImGui::IsKeyPressed(SDL_SCANCODE_RIGHTBRACKET))
			lua["game_running"] = true;
	}
	ImGui::BeginChild("_emulator", ImVec2(0, 0));
	ImVec2 min = ImGui::GetWindowContentRegionMin();
	ImVec2 max = ImGui::GetWindowContentRegionMax();
	ImGui::SetCursorPos(ImVec2(
		// needs to be rounded or we get artifacts
		uint32_t(min.x + (max.x / 2.f)) - (128.f * (float)game_scale),
		uint32_t(min.y + (max.y / 2.f)) - (112.f * (float)game_scale)
	));
	ImVec2 cursor_ref = ImGui::GetCursorPos();
	ImGui::Image(video::canvas_texture[1].handle,
		ImVec2(256 * game_scale, 224 * game_scale), ImVec2(0, 1), ImVec2(1, 0));
	if (alpha > 0.01f) {
		ImGui::SetCursorPos(cursor_ref);
		ImGui::Image(video::canvas_texture[6].handle,
			ImVec2(256 * game_scale, 224 * game_scale), ImVec2(0, 1), ImVec2(1, 0),
			ImVec4(1.f, 1.f, 1.f, alpha));
	}
	ImGui::EndChild();
}

void
draw_game(void)
{
	ImGui::Begin(ICON_FA_PLAY_CIRCLE " Game");

	if (emulator::rom == "empty") {
		ImGui::Image(video::canvas_texture[1].handle, ImVec2(512, 448), ImVec2(0, 1), ImVec2(1, 0));
		ImGui::End();
		return;
	}

	emulator::focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
	lua["game_focused"] = emulator::focused;

	ImGui::PushFont(font_mono);
	//ICON_FA_ANGLE_RIGHT
	if (ImGui::Button(stringf("%03d>##gameadvance", game_advances))) {
		lua["game_running"] = false;
		lua["run_frames"] = 1;
		++game_advances;
	}
	ImGui::PopFont();
	ImGui::SameLine();
	bool _grun = true;
	sol::object o = lua["game_running"];
	if (o.valid() && o.is<bool>())
		_grun = o.as<bool>();
	ImGui::Checkbox("Game  ", &_grun);
	lua["game_running"] = _grun;

	ImGui::SameLine();
	ImGui::PushFont(font_mono);
	//ICON_FA_ANGLE_RIGHT
	if (ImGui::Button(stringf("%03d>##emuadvance", emu_advances))) {
		emulator::set_running(false);
		emulator::run_frames = 1;
		++emu_advances;
	}
	ImGui::PopFont();
	ImGui::SameLine();
	bool _run = emulator::running;
	if (emulator::rom == "empty") {
		emulator::pause();
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.6f);
		ImGui::Checkbox("Emulator  ", &_run);
		ImGui::PopStyleVar();
	} else {
		//ImGui::AlignTextToFramePadding();
		//ImGui::Text(" %s ", emulator::rom.c_str());
		//ImGui::SameLine();
		ImGui::Checkbox("Emulator  ", &_run);
		emulator::set_running(_run);
	}

	/*ImGui::SameLine();
	ImGui::Checkbox("Sync  ", &_run);*/

	ImGui::SameLine();
	if (ImGui::Button("Run all")) {
		lua["game_running"] = true;
		emulator::set_running(true);
	}

	ImGui::SameLine();
	if (ImGui::Button("Stop all")) {
		lua["game_running"] = false;
		emulator::set_running(false);
	}

	ImGui::SameLine();
	if (ImGui::Button("Adv all")) {
		lua["game_running"] = false;
		emulator::set_running(false);
		lua["run_frames"] = 1;
		emulator::run_frames = 1;
	}

	ImGui::SameLine();
	if (ImGui::Button("Reset adv")) {
		game_advances = 0;
		emu_advances = 0;
	}

	ImGui::BeginTabBar("game_tabs");
	if (ImGui::BeginTabItem("X++")) {
		ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.35);
		ImGui::SliderInt("Scale  ", &game_scale, 1, 5, "%dx");
		emulator::focused = false;
		handle_emulator(0.f);
		ImGui::EndTabItem();
	}
	if (ImGui::BeginTabItem("SFC")) {
		ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.35);
		ImGui::SliderInt("Scale  ", &game_scale, 1, 5, "%dx");
		ImGui::SameLine();
		ImGui::Checkbox("Mute", &emulator::mute);
		lua["game_focused"] = false;
		video::render(6);
		video::draw_emulator(0, 0, 1.f, 6);
		handle_emulator(1.f);
		ImGui::EndTabItem();
	}
	if (ImGui::BeginTabItem("Overlay")) {
		ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.35);
		ImGui::SliderInt("Scale  ", &game_scale, 1, 5, "%dx");
		ImGui::SameLine();
		ImGui::SliderFloat("Emulator %", &emu_alpha, 0.f, 1.f, "%.01f");
		ImGui::PopItemWidth();
		video::render(6);
		video::draw_emulator(0, 0, 1.f, 6);
		handle_emulator(emu_alpha);
		ImGui::EndTabItem();
	}
	if (ImGui::BeginTabItem("Settings")) {
		ImGui::Checkbox("Mute emulator", &emulator::mute);
		ImGui::Separator();
		ImGui::Text("Position trackers");
		ImGui::Text("Add");
		ImGui::Text("Address or lua table");
		ImGui::Text("Color");
		ImGui::EndTabItem();
	}
	ImGui::EndTabBar();

	ImGui::End();
}

void
draw_windows(void)
{
	draw_menu_bar();

	for (auto i : windows)
		if (i->visible)
			i->draw();

	if (game_visible)
		draw_game();

	//draw_game();
}

window_t *
get_window(std::string title)
{
	for (auto i : windows)
		if (i->name == title)
			return i;
	return NULL;
}

// TODO: deal with possible texture leak
ImFont *
imgui_load_font(std::string filename, float size)
{
	ImGuiIO &io = ImGui::GetIO();
	ImFont *font = io.Fonts->AddFontFromFileTTF(filename.c_str(), size);
	uint8_t *data;
	int32_t width;
	int32_t height;
	io.Fonts->GetTexDataAsRGBA32(&data, &width, &height);
	bgfx::TextureHandle tex = bgfx::createTexture2D(
		(uint16_t)width,
		(uint16_t)height,
		false,
		1,
		bgfx::TextureFormat::BGRA8,
		0,
		bgfx::copy(data, width * height * 4));
	// fix alpha
	union {
		struct {
			bgfx::TextureHandle handle;
			uint8_t flags;
			uint8_t mip;
		} s;
		ImTextureID ptr;
	} texture;
	texture.s.handle = tex;
	texture.s.flags  = IMGUI_FLAGS_ALPHA_BLEND;
	texture.s.mip    = 0;
	io.Fonts->SetTexID((void *)(intptr_t)texture.ptr);
	return font;
}

void
cleanup(void)
{
	ImGui_ImplSDL2_Shutdown();
	imguiDestroy();
}

} // namespace vgui