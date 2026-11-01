#include "graphics_w.hpp"
#include "../emulator.hpp"
#include "../input.hpp"
#include <filesystem>
#include <algorithm>

using namespace ImGui;

namespace xpp::xgui {

graphics_w::graphics_w(void) : window_t("Graphics")
{
	strncpy(object_id, "common", strlen("common") + 1);
	anim_list.push_back(anim.id);
	strncpy(anim_input, anim.id.c_str(), 47);
	anim_input[47] = '\0';
	atlas_input[47] = '\0';
	map_input[47] = '\0';
	refresh_anim_list();
}

graphics_w::~graphics_w(void)
{
}

bool
graphics_w::frame_valid(int16_t f)
{
	// HACK: we can't use members as default arguments, so...
	if (f == -15997)
		f = selected_frame;
	return (
		anim.seq.size() > 0 &&
		selected_layer < anim.seq.size() &&
		anim.seq[selected_layer].frames.size() > 0 &&
		f > 0 &&
		f <= anim.seq[selected_layer].frames.size()
	);
}

void
graphics_w::draw_image(video::texture &t)
{
	BeginChild("graphics_image_display",	
		ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

	Image(t.handle, ImVec2(t.width * gfx_scale, t.height * gfx_scale), ImVec2(0, 0), ImVec2(1, 1));

	EndChild();
}

// 1. Disable BG
// 2. Copy bytes and process (replace bg with alpha, auto crop option)
// 3. Blit to new texture
// 4. Option to adjust crop manually
// 5. Blit again to given atlas texture and open in UV/crop editor

// Have a way to combine multiple existing images into atlas

static int
id_filter(ImGuiInputTextCallbackData *data)
{
	auto ch = data->EventChar;
	if (ch == '\\' || ch == '|' || (ch >= ';' && ch <= '?') || (ch >= ' ' && ch <= ','))
		return 1;
	return 0;
}

void
graphics_w::draw_texture_canvas(void)
{
	int n = 32;
	for (int y = 0; y < 16; y += 2) {
		for (int x = 0; x < 16; x += 2) {
			video::set_color(0.225f, 0.225f, 0.225f, 1.f);
			video::draw_rect((x + 1) * n, y * n, n, n);
			video::draw_rect(x * n, (y + 1) * n, n, n);
			video::set_color(0.275f, 0.275f, 0.275f, 1.f);
			video::draw_rect(x * n, y * n, n, n);
			video::draw_rect((x + 1) * n, (y + 1) * n, n, n);
		}
	}
	video::flush_primitives();
	video::set_color(1.f, 1.f, 1.f, 1.f);

	video::texture t = *video::get_texture(selected_texture); // copy
	if (selected_texture != "" && bgfx::isValid(t.handle)) {
		bool valid_sub = t.subtextures.find(selected_subtexture) != t.subtextures.end();
		video::draw_sprite(t, 0, 0);

		// shortcut to preview uv and hide line rect
		if (valid_sub && input::getkey(SDL_SCANCODE_LCTRL)) {
			video::subtexture &sub = t.subtextures[selected_subtexture];
			video::draw_subsprite(
				t, selected_subtexture,
				int(sub.uv0.x * t.width),
				int(sub.uv0.y * t.height)
			);
			goto _flush;
		}

		if (valid_sub) {
			video::draw_sprite(t, 0, 0);
			video::subtexture &sub = t.subtextures[selected_subtexture];
			float uvw = ((sub.uv1.x - sub.uv0.x) * t.width);
			float uvh = ((sub.uv1.y - sub.uv0.y) * t.height);
			video::set_color(1.f, 0.f, 1.f, 1.f);
			video::draw_rect_line(sub.uv0.x * t.width, sub.uv0.y * t.height, uvw, uvh);
			video::flush_primitives();
		} else if (canvas_drag_max.x != canvas_drag.x || canvas_drag_max.y != canvas_drag.y) {
			// TODO: invert colors
			// on top shortcut
			if (input::getkey(SDL_SCANCODE_LCTRL))
				video::draw_sprite(t, 0, 0);
			video::set_color(1.f, 0.f, 1.f, 1.f);
			video::draw_rect_line(
				canvas_drag.x, canvas_drag.y,
				canvas_drag_max.x - canvas_drag.x, canvas_drag_max.y - canvas_drag.y
			);
			video::flush_primitives();
			video::set_color(1.f, 1.f, 1.f, 1.f);
			if (!input::getkey(SDL_SCANCODE_LCTRL))
				video::draw_sprite(t, 0, 0);
			video::set_color(1.f, 0.f, 1.f, .2f);
			video::draw_rect(
				canvas_drag.x, canvas_drag.y,
				canvas_drag_max.x - canvas_drag.x, canvas_drag_max.y - canvas_drag.y
			);
		} else {
			video::draw_sprite(t, 0, 0);
		}

		// shortcut to highlight all subtextures
		if (input::getkey(SDL_SCANCODE_LSHIFT)) {
			video::set_color(1.f, 0.f, 1.f, .35f);
			for (std::pair<std::string, video::subtexture> ii : t.subtextures) {
				if (valid_sub && ii.first == selected_subtexture)
					continue;

				video::subtexture &i = ii.second;

				float uvw = ((i.uv1.x - i.uv0.x) * t.width);
				float uvh = ((i.uv1.y - i.uv0.y) * t.height);
				video::draw_rect_line(i.uv0.x * t.width, i.uv0.y * t.height, uvw, uvh);
			}
			video::flush_primitives();
		}

		video::set_color(1.f, 0.f, 1.f, 1.f);
		video::draw_rect(mousex, mousey, 1, 1);
		video::flush_primitives();
	}
_flush:
	video::flush();
}

/*

Ifdef std map 
Copy texture to clipboard 
Background enable by mute
Title screen
Subtexture select in anim edit 
Subtexture flip CHECK
Depth CHECK

*/

void
graphics_w::draw_viewer(void)
{
	BeginChild("texture_left_pane", ImVec2(224, 0), false);
	BeginTabBar("texture_map_tabs");
	texture_filter.Draw("Look", GetWindowWidth() - 50.f);
	if (BeginTabItem("IDs")) {
		if (ListBoxHeader("##texture_id_list", ImVec2(224, 133))) {
			for (std::pair<std::string, video::texture *> ii : video::texture_map) {
				video::texture &i = *ii.second;
				if (!texture_filter.PassFilter(ii.first.c_str()))
					continue;
				if (ii.first == "" || !bgfx::isValid(i.handle) || i.width < 1 || i.height < 1)
					continue;

				if (Selectable(ii.first.c_str(), selected_texture == ii.first))
					selected_texture = ii.first;
			}
			ListBoxFooter();
		}
		EndTabItem();
	}
	if (BeginTabItem("Files")) {
		if (ListBoxHeader("##texture_file_list", ImVec2(224, 133))) {
			for (std::pair<std::string, video::texture> ii : video::texture_file_map) {
				video::texture &i = ii.second;
				if (!texture_filter.PassFilter(ii.first.c_str()))
					continue;
				if (!bgfx::isValid(i.handle) || i.width < 1 || i.height < 1)
					continue;

				if (Selectable(ii.first.c_str(), selected_texture == ii.first))
					selected_texture = ii.first;
			}
			ListBoxFooter();
		}
		EndTabItem();
	}
	EndTabBar();

	auto tflags = ImGuiInputTextFlags_CharsNoBlank
		| ImGuiInputTextFlags_EnterReturnsTrue
		| ImGuiInputTextFlags_CallbackCharFilter;
	PushItemWidth(224 - 32);
	if (InputTextWithHint("##_remap","Remap",map_input,IM_ARRAYSIZE(map_input),tflags,id_filter)) {
		video::texture *tmp = video::get_texture(selected_texture);
		video::texture_map.erase(selected_texture);
		video::texture_map[map_input] = tmp;
		selected_texture = map_input;
	}
	if (IsItemFocused())
		selected_texture = map_input;
	PopItemWidth();
	SameLine();
	if (Button(ICON_FA_FOLDER_OPEN_O "##_open_image", ImVec2(24, 24)))
		file_select.open("res/gfx", ".png");

	Separator();

	PushItemWidth(224 - (32 * 3));
	InputTextWithHint("##atlastexid","Subtexture",atlas_input,IM_ARRAYSIZE(atlas_input),tflags,id_filter);
	PopItemWidth();

	video::texture &t = *video::get_texture(selected_texture);

	bool disabled = false;
	if (!disabled) disabled = (t.filename == "res/gfx/common/missing.png");
	if (!disabled) disabled = (strlen(atlas_input) == 0);
	if (!disabled) disabled = (canvas_drag_max.x == canvas_drag.x && canvas_drag_max.y == canvas_drag.y);
	SameLine();
	if (disabled) {
		PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 0.75f));
		PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.5f, 0.5f, 0.75f));
		PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.5f, 0.5f, 0.75f));
	}
	if (Button(ICON_FA_PLUS "##newsubtex", ImVec2(24, 24)) && !disabled) {
		video::subtexture sub;
		sub.uv0 = {
			float(canvas_drag.x) / t.width,
			float(canvas_drag.y) / t.height
		};
		sub.uv1 = {
			float(canvas_drag_max.x) / t.width,
			float(canvas_drag_max.y) / t.height
		};
		if (sub.uv0.x > sub.uv1.x) {
			std::swap(sub.uv0.x, sub.uv1.x);
			sub.uv0.x -= (1.f / t.width);
			sub.uv1.x += (1.f / t.width);
		}
		if (sub.uv0.y > sub.uv1.y) {
			std::swap(sub.uv0.y, sub.uv1.y);
			sub.uv0.y -= (1.f / t.height);
			sub.uv1.y += (1.f / t.height);
		}
		t.subtextures[atlas_input] = sub;
		selected_subtexture = atlas_input;
	}
	SameLine();
	if (Button(ICON_FA_PENCIL "##renamesubtex", ImVec2(24, 24)) && !disabled) {
		video::subtexture tmp = t.subtextures[selected_subtexture];
		t.subtextures.erase(selected_subtexture);
		t.subtextures[atlas_input] = tmp;
		selected_subtexture = atlas_input;
	}
	SameLine();
	if (Button(ICON_FA_TRASH "##delsubtex", ImVec2(24, 24)) && !disabled) {
		t.subtextures.erase(selected_subtexture);
	}
	if (disabled)
		PopStyleColor(3);

	PushItemWidth(224);
	if (ListBoxHeader("##atlas_sub_list", ImVec2(224, 133))) {
		for (std::pair<std::string, video::subtexture> ii : t.subtextures) {
			video::subtexture &i = ii.second;

			if (Selectable(ii.first.c_str(), selected_subtexture == ii.first)) {
				selected_subtexture = ii.first;
				strncpy(atlas_input, selected_subtexture.c_str(), 47);
				atlas_input[47] = '\0';
				canvas_drag.x = ii.second.uv0.x * t.width;
				canvas_drag.y = ii.second.uv0.y * t.height;
				canvas_drag_max.x = ii.second.uv1.x * t.width;
				canvas_drag_max.y = ii.second.uv1.y * t.height;
			}
		}
		ListBoxFooter();
	}
	PopItemWidth();

	// ???
	disabled = false;
	if (!disabled) disabled = (t.filename == "res/gfx/common/missing.png");
	if (disabled) {
		PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 0.75f));
		PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.5f, 0.5f, 0.75f));
		PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.5f, 0.5f, 0.75f));
	}
	if (Button("Save") && !disabled) {
		video::save_atlas(video::get_texture(selected_texture));
	}
	SameLine();
	if (Button("Load") && !disabled) {
		video::load_atlas(video::get_texture(selected_texture));
	}
	if (disabled)
		PopStyleColor(3);

	bool valid_sub = t.subtextures.find(selected_subtexture) != t.subtextures.end();

	if (valid_sub) {
		video::subtexture &sub = t.subtextures[selected_subtexture];
		ImVec2 uv0(sub.uv0.x, sub.uv0.y);
		ImVec2 uv1(sub.uv1.x, sub.uv1.y);
		Image(t.handle, ImVec2(224, 224), uv0, uv1);
	} else if (canvas_drag_max.x != canvas_drag.x && canvas_drag_max.y != canvas_drag.y) {
		ImVec2 uv0(canvas_drag.x / t.width, canvas_drag.y / t.height);
		ImVec2 uv1(canvas_drag_max.x / t.width, canvas_drag_max.y / t.height);
		Image(t.handle, ImVec2(224, 224), uv0, uv1);
	}

	EndChild();

	SameLine();

	BeginChild("texture_canvas", ImVec2(0, 0), true,
		ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

	if (IsWindowHovered(ImGuiFocusedFlags_ChildWindows)) {
		if (GetIO().MouseWheel > 0.5f)
			canvas_zoom += 1;
		else if (GetIO().MouseWheel < -0.5f)
			canvas_zoom -= 1;
		if (canvas_zoom < 1)
			canvas_zoom = 1;
		if (canvas_zoom > 8)
			canvas_zoom = 8;

		ImVec2 delta(0.f, 0.f);
		float multiplier = 300.f;
		if (input::getkey(SDL_SCANCODE_LSHIFT))
			multiplier = 600.f;
		if (input::getkey(SDL_SCANCODE_W))
			delta.y = -GetIO().DeltaTime * multiplier;
		if (input::getkey(SDL_SCANCODE_S))
			delta.y = GetIO().DeltaTime * multiplier;
		if (input::getkey(SDL_SCANCODE_A))
			delta.x = -GetIO().DeltaTime * multiplier;
		if (input::getkey(SDL_SCANCODE_D))
			delta.x = GetIO().DeltaTime * multiplier;
		// imgui bug thing
		if (delta.x >= 0.01f && delta.x <= 8.f)
			delta.x = 8.01f;
		if (delta.x <= -0.01f && delta.x >= -8.f)
			delta.x = -8.01f;
		if (delta.y >= 0.01f && delta.y <= 8.f)
			delta.y = 8.01f;
		if (delta.y <= -0.01f && delta.y >= -8.f)
			delta.y = -8.01f;
		SetScrollX(GetScrollX() + delta.x);
		SetScrollY(GetScrollY() + delta.y);
	}
	auto mul = canvas_zoom;

	BeginChild("texture_canvas_image", ImVec2(t.width * mul, t.height * mul), false);
	ImVec2 cursor_ref = GetCursorPos();

	texture_hovered = false;
	canvas_hovered = false;

	video::set_canvas_size(3, t.width, t.height);
	video::render(3);
	draw_texture_canvas();
	ImVec2 uv0(0.f, 1.f);
	ImVec2 uv1(1.f, 0.f);

	Image(video::canvas_texture[3].handle, ImVec2(t.width * mul, t.height * mul), uv0, uv1);
	SetCursorPos(cursor_ref);
	// TODO: clamp negative values
	mousex = (input::getmousex() - GetCursorScreenPos().x - GetScrollX()) / mul;
	mousey = (input::getmousey() - GetCursorScreenPos().y - GetScrollY()) / mul;
	int mousexabs = (input::getmousex() - GetCursorScreenPos().x - GetScrollX());
	int mouseyabs = (input::getmousey() - GetCursorScreenPos().y - GetScrollY());
	if (t.width > 0 && t.height > 0) {
		bool clicked = InvisibleButton("##atlastexturebutton", ImVec2(
			t.width * mul,
			t.height * mul
		));
		if ((clicked || GetIO().MouseClicked[1]) && !dragging) {
			selected_subtexture = "";
			atlas_input[0] = '\0';
			canvas_drag.x = 0;
			canvas_drag.y = 0;
			canvas_drag_max.x = 0;
			canvas_drag_max.y = 0;
		}
	}
	if (IsItemHovered()) {
		texture_hovered = true;
		if (GetIO().MouseClicked[0]) {
			clickx = mousexabs;
			clicky = mouseyabs;
		}
	}
	// drag
	// this logic fixes edge issues
	if (IsItemActive() && !dragging && IsMouseDragging(0, 2.f)) {
		//canvas_drag.x = int((mousexabs - GetMouseDragDelta(0, 2.f).x) / mul);
		//canvas_drag.y = int((mouseyabs - GetMouseDragDelta(0, 2.f).y) / mul);
		canvas_drag.x = int(clickx / mul);
		canvas_drag.y = int(clicky / mul);
		dragging = true;
	}
	if (dragging) {
		if (IsMouseDragging(0, 2.f)) {
			SetMouseCursor(ImGuiMouseCursor_Hand);
			int dx = GetMouseDragDelta(0, 0.f).x / mul;
			int dy = GetMouseDragDelta(0, 0.f).y / mul;
			//canvas_drag_max.x = canvas_drag.x + dx + 1;
			//canvas_drag_max.y = canvas_drag.y + dy + 1;
			canvas_drag_max.x = mousex + 1;
			canvas_drag_max.y = mousey + 1;

			if (valid_sub) {
				// modify
				video::subtexture &sub = t.subtextures[selected_subtexture];
				sub.uv0 = {
					float(canvas_drag.x) / t.width,
					float(canvas_drag.y) / t.height
				};
				sub.uv1 = {
					float(canvas_drag_max.x) / t.width,
					float(canvas_drag_max.y) / t.height
				};
				if (sub.uv0.x > sub.uv1.x) {
					std::swap(sub.uv0.x, sub.uv1.x);
					sub.uv0.x -= (1.f / t.width);
					sub.uv1.x += (1.f / t.width);
				}
				if (sub.uv0.y > sub.uv1.y) {
					std::swap(sub.uv0.y, sub.uv1.y);
					sub.uv0.y -= (1.f / t.height);
					sub.uv1.y += (1.f / t.height);
				}
			}
		} else {
			dragging = false;
		}
	}

	EndChild();
	EndChild();
}

void
graphics_w::draw_animation_canvas(void)
{
	video::set_color(0.225f, 0.225f, 0.225f, 1.f);
	video::draw_rect(256, 0, 256, 256);
	video::draw_rect(0, 256, 256, 256);
	video::set_color(0.275f, 0.275f, 0.275f, 1.f);
	video::draw_rect(0, 0, 256, 256);
	video::draw_rect(256, 256, 256, 256);
	video::set_color(1.f, 1.f, 1.f, 1.f);

	auto &seq = anim.seq[selected_layer];
	if (frame_valid()) {
		frame_t &f = seq.frames[selected_frame - 1];
		video::texture &t = *video::get_texture(f.texture_id);

		if (use_background && background_id[0] != '\0') {
			if (strcmp(background_id, "emulator") == 0) {
				video::draw_emulator(256 + background_x, 256 + background_y, background_alpha, 3);
			} else {
				video::set_color(1.f, 1.f, 1.f, background_alpha);
				// avoid invalid texture warning with get_texture
				video::draw_sprite(video::get_texture(background_id),256+background_x,256+background_y);
				video::set_color(1.f, 1.f, 1.f, 1.f);
			}
		}

		// shortcut to hide so you can see background
		if (input::getkey(SDL_SCANCODE_LCTRL))
			goto _flush;

		if (seq.playing) {
			selected_frame = seq.frame + 1;
			if (selected_frame > seq.frames.size())
				goto _flush;
			//seq.draw(256, 256);
			//return;
		}

		if (use_onion_skin && onion_skin_type == 1) {
			video::set_color(1.f, 1.f, 1.f, onion_alpha);
			if (frame_valid(selected_frame - 1))
				seq.draw_frame(selected_frame - 2, 256, 256);
			else if (selected_frame == 1 && seq.frames.size() > 1 && frame_valid(seq.frames.size()))
				seq.draw_frame(seq.frames.size() - 1, 256, 256);
		}

		if (use_onion_skin && onion_skin_type == 0) {
			for (int i = 0; i < anim.seq.size(); ++i) {
				video::set_color(1.f, 1.f, 1.f, i == selected_layer ? 1.f : onion_alpha);
				anim.seq[i].draw_frame((selected_frame - 1) % anim.seq[i].frames.size(), 256, 256);
			}
			video::set_color(1.f, 1.f, 1.f, 1.f);
		} else {
			video::set_color(1.f, 1.f, 1.f, 1.f);
			seq.draw_frame(selected_frame - 1, 256, 256);
		}

		if (texture_hovered || dragging) {
			video::set_color(1.f, 0.f, 1.f, 1.f);
			video::draw_rect_line(
				256 + f.offx - 1, 256 + f.offy - 1,
				t.subwidth(f.subtexture_id) + 2, t.subheight(f.subtexture_id) + 2
			);
		}
	}
_flush:
	video::flush();
}

static bool touched_anim_input = false;

static int
file_filter(ImGuiInputTextCallbackData *data)
{
	touched_anim_input = true;
	auto ch = data->EventChar;
	if (ch == '\\' || ch == '|' || (ch >= ':' && ch <= '?') || (ch >= ' ' && ch <= '/'))
		return 1;
	return 0;
}

bool
sort_alphabetical(std::string a, std::string b)
{
	return a < b;
}

void
graphics_w::refresh_anim_list(void)
{
	anim_list.clear();
	anim_list.push_back(anim.id);
	std::string dir = std::string("res/seq/").append(object_id);
	if (!std::filesystem::exists(dir))
		return;

	for (const auto &i : std::filesystem::directory_iterator(dir)) {
		if (i.is_directory() || (i.path().extension()!=".seq" && i.path().extension()!=".SEQ"))
			continue;
		if (i.path().stem() == anim.id)
			continue;

		anim_list.push_back(i.path().stem());
	}
	std::sort(anim_list.begin(), anim_list.end(), sort_alphabetical);

	// while we're at it
	anim_dir_list.clear();
	for (const auto &i : std::filesystem::directory_iterator("res/seq")) {
		if (!i.is_directory())
			continue;

		anim_dir_list.push_back(i.path().stem());
	}
}

void
graphics_w::draw_animation_pane(void)
{
	PushItemWidth(224);
	auto tflags = ImGuiInputTextFlags_CharsNoBlank
		| ImGuiInputTextFlags_EnterReturnsTrue
		| ImGuiInputTextFlags_CallbackCharFilter;
	InputTextWithHint("##aobjid","Object ID",object_id,IM_ARRAYSIZE(object_id),tflags,file_filter);
	sol::object v = lua["world"]["types"][object_id];
	if (!v.valid() &&
		std::find(anim_dir_list.begin(),anim_dir_list.end(),object_id)==anim_dir_list.end())
		return;
	if (touched_anim_input) {
		refresh_anim_list();
		touched_anim_input = false;
	}

	if (BeginCombo("##animcombo", anim.id.c_str())) {
		for (int i = 0; i < anim_list.size(); ++i) {
			PushID(i);
			if (Selectable(anim_list[i].c_str(), i == anim_combo)) {
				anim_combo = i;
				anim.id = anim_list[i];
				strncpy(anim_input, anim.id.c_str(), 47);
				anim_input[47] = '\0';
			}
			PopID();
		}
		ImGui::EndCombo();
	}
	PopItemWidth();

	InputTextWithHint("##aobjfile","Sequence",anim_input,IM_ARRAYSIZE(anim_input),tflags,file_filter);
	if (anim.id != std::string(anim_input))
		touched_anim_input = true;
	anim.id = anim_input;
	if (touched_anim_input) {
		refresh_anim_list();
		touched_anim_input = false;
	}
	SameLine();
	std::string dir = std::string("res/seq/").append(object_id);
	if (anim.id.size() == 0) {
		PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 0.75f));
		PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.5f, 0.5f, 0.75f));
		PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.5f, 0.5f, 0.75f));
	}
	if (Button(ICON_FA_FLOPPY_O "##saveanim") && anim.id.size() > 0) {
		if (!std::filesystem::exists(dir)) {
			std::filesystem::create_directory(dir);	
		}
		anim.save(dir + "/" + anim.id + ".seq");
	}
	SameLine();
	if (Button(ICON_FA_FOLDER_OPEN_O "##loadanim") && anim.id.size() > 0) {
		anim.load(dir + "/" + anim.id + ".seq");
	}
	if (anim.id.size() == 0)
		PopStyleColor(3);

	// layer buttons
	if (Button(ICON_FA_ANGLE_DOUBLE_LEFT "##layerleft")) {
		--selected_layer;
		if (selected_layer < 0)
			selected_layer = anim.seq.size() - 1;
		anim.seq[selected_layer].timer = anim.seq[selected_layer].frames[selected_frame - 1].time;
	}
	SameLine();
	PushFont(font_mono);
	SetCursorPosY(GetCursorPosY() + 3);
	Text("Layer %02d/%02d", selected_layer + 1, anim.seq.size());
	PopFont();
	SameLine();
	SetCursorPosY(GetCursorPosY() - 3);
	if (Button(ICON_FA_ANGLE_DOUBLE_RIGHT "##layerright")) {
		++selected_layer;
		if (selected_layer >= anim.seq.size())
			selected_layer = 0;
		anim.seq[selected_layer].timer = anim.seq[selected_layer].frames[selected_frame - 1].time;
	}
	SameLine();
	if (!anim.playing) {
		if (Button(ICON_FA_FORWARD "##playanimlayers", ImVec2(20, 24))) {
			bool can_play = false;
			for (int i = 0; i < anim.seq.size(); ++i) {
				if (anim.seq[i].frames.size() > 1)
					can_play = true;
			}
			if (can_play) {
				anim.reset();
				anim.play();
			}
		}
	} else if (Button(ICON_FA_STOP "##stopanimlayers", ImVec2(20, 24))) {
		anim.reset();
		anim.pause();
	}
	SameLine();
	if (Button(ICON_FA_PLUS "##insertlayer")) {
		sequence_t s;
		anim.seq.push_back(s);
	}
	SameLine();
	if (Button(ICON_FA_TRASH "##delanimlayer") && anim.seq.size() > 1)
		anim.seq.erase(anim.seq.begin() + selected_layer);

	if (selected_layer >= anim.seq.size())
		selected_layer = 0;
	if (anim.seq.size() == 0)
		return; // ???
	auto &seq = anim.seq[selected_layer];

	// TODO: layers as numbered tab bar
	//BeginTabBar("layer_tabs");
	//EndTabBar();

	// frame buttons
	if (Button(ICON_FA_ANGLE_DOUBLE_LEFT "##animleft")) {
		--selected_frame;
		if (selected_frame < 1)
			selected_frame = seq.frames.size();
		seq.timer = seq.frames[selected_frame - 1].time;
		seq.frame = selected_frame - 1;
	}
	SameLine();
	PushFont(font_mono);
	SetCursorPosY(GetCursorPosY() + 3);
	Text("Frame %02d/%02d", selected_frame, seq.frames.size());
	PopFont();
	SameLine();
	SetCursorPosY(GetCursorPosY() - 3);
	if (Button(ICON_FA_ANGLE_DOUBLE_RIGHT "##animright")) {
		++selected_frame;
		if (selected_frame > seq.frames.size())
			selected_frame = 1;
		seq.timer = seq.frames[selected_frame - 1].time;
		seq.frame = selected_frame - 1;
	}
	SameLine();
	if (!seq.playing) {
		if (Button(ICON_FA_PLAY "##playanimfromhere", ImVec2(20, 24)) && seq.frames.size() > 1)
			seq.playing = true;
	} else if (Button(ICON_FA_PAUSE "##pauseanim", ImVec2(20, 24))) {
		seq.reset(); // reset timer
		seq.frame = selected_frame - 1;
		seq.timer = seq.frames[selected_frame - 1].time;
		seq.frame = selected_frame - 1;
		seq.playing = false;
	}
	SameLine();
	if (Button(ICON_FA_PLUS "##insertframehere")) {
		frame_t frame;
		seq.frames.push_back(frame);
	}
	SameLine();
	if (Button(ICON_FA_TRASH "##delanimframe") && seq.frames.size() > 1)
		seq.frames.erase(seq.frames.begin() + selected_frame - 1);

	anim.update();
	if (seq.playing)
		selected_frame = seq.frame + 1;

	if (selected_frame > seq.frames.size())
		selected_frame = seq.frames.size();
	if (selected_frame == 0) {
		if (seq.frames.size() > 0)
			selected_frame = 1;
		else
			return; // better than segfaulting
	}

	uint8_t step8 = 1;
	// lol
	++seq.loop_frame;
	InputScalar("Loop frame", ImGuiDataType_U8, &seq.loop_frame, &step8);
	if (seq.loop_frame < 1)
		seq.loop_frame = 1;
	if (seq.loop_frame > seq.frames.size())
		seq.loop_frame = seq.frames.size();
	--seq.loop_frame;

	Separator();

	frame_t &frame = seq.frames[selected_frame - 1];
	CheckboxFlags("Horizontal flip", (unsigned int *)&frame.attrib, 0x01);
	SameLine();
	CheckboxFlags("Vertical flip", (unsigned int *)&frame.attrib, 0x02);

	int16_t step = 1;
	InputScalar("X##animoffx", ImGuiDataType_S16, &frame.offx, &step);
	SameLine();
	if (Button("Center##center_x")) {
		video::texture *t = video::get_texture(frame.texture_id);
		frame.offx = -t->subwidth(frame.subtexture_id) / 2;
	}
	InputScalar("Y##animoffy", ImGuiDataType_S16, &seq.frames[selected_frame - 1].offy, &step);
	SameLine();
	if (Button("Center##center_y")) {
		video::texture *t = video::get_texture(frame.texture_id);
		frame.offy = -t->subheight(frame.subtexture_id) / 2;
	}
	if (InputScalar("Length", ImGuiDataType_U8, &seq.frames[selected_frame - 1].time, &step)) {
		seq.timer = frame.time;
		seq.frame = selected_frame - 1;
	}
	if (frame.time < 1)
		frame.time = 1;

	Separator();

	if (BeginTabBar("animtexbar")) {
		if (BeginTabItem("Texture")) {
			PushFont(font_mono);
			// TODO: make this an inputtext
			Text("%s", frame.texture_id.c_str());
			PopFont();
			texture_filter.Draw("Look", GetWindowWidth() - 50.f);
			if (ListBoxHeader("##texture_id_list_anim", ImVec2(224, 133))) {
				for (std::pair<std::string, video::texture *> ii : video::texture_map) {
					video::texture &i = *ii.second;
					if (!texture_filter.PassFilter(ii.first.c_str()))
						continue;
					if (ii.first == "" || !bgfx::isValid(i.handle) || i.width < 1 || i.height < 1)
						continue;

					if (Selectable(ii.first.c_str(), frame.texture_id == ii.first))
						frame.texture_id = ii.first;
				}
				ListBoxFooter();
			}
			EndTabItem();
		}
		video::texture *t = video::get_texture(frame.texture_id);
		if (t->subtextures.size() > 0 && BeginTabItem("Subtexture")) {
			if (ListBoxHeader("##anim_sub_list", ImVec2(224, 133))) {
				for (std::pair<std::string, video::subtexture> ii : t->subtextures) {
					video::subtexture &i = ii.second;

					if (Selectable(ii.first.c_str(), frame.subtexture_id == ii.first))
						frame.subtexture_id = ii.first;
				}
				ListBoxFooter();
			}
			EndTabItem();
		} else if (t->subtextures.size() == 0) {
			frame.subtexture_id = "";
		}
		EndTabBar();
	}

	Separator();

	float astep = .1f;
	Checkbox("Onion skin", &use_onion_skin);
	if (use_onion_skin) {
		RadioButton("Layer", &onion_skin_type, 0);
		SameLine();
		RadioButton("Frame", &onion_skin_type, 1);
		InputScalar("Alpha##oalpha", ImGuiDataType_Float, &onion_alpha, &astep, &astep, "%.02f");
		if (onion_alpha < 0.f)
			onion_alpha = 0.f;
		if (onion_alpha > 1.f)
			onion_alpha = 1.f;
	}
	Checkbox("Background image", &use_background);
	if (!use_background)
		return;
	bool clicked = Combo("Source", &background_combo, "Texture\0Emulator\0Emulator (follow)");
	if (background_combo == 0) {
		if (clicked) {
			background_x = 0;
			background_y = 0;
		}
		InputTextWithHint("##animbg", "Texture ID", background_id, IM_ARRAYSIZE(background_id));
		InputScalar("X##animbgx", ImGuiDataType_S16, &background_x, &step);
		InputScalar("Y##animbgy", ImGuiDataType_S16, &background_y, &step);
	} else if (background_combo == 1) {
		if (clicked) {
			background_x = -128;
			background_y = -112;
		}
		strncpy(background_id, "emulator", strlen("emulator") + 1);
		Checkbox("Advance with emulator", &advance_with_emulator);
		InputScalar("X##animbgx", ImGuiDataType_S16, &background_x, &step);
		InputScalar("Y##animbgy", ImGuiDataType_S16, &background_y, &step);
	} else {
		strncpy(background_id, "emulator", strlen("emulator") + 1);
		Checkbox("Advance with emulator", &advance_with_emulator);
		draw_follow_options();
	}
	InputScalar("Alpha##aalpha", ImGuiDataType_Float, &background_alpha, &astep, &astep, "%.02f");
	if (background_alpha < 0.f)
		background_alpha = 0.f;
	if (background_alpha > 1.f)
		background_alpha = 1.f;
}

static std::string
object_id_to_name(uint8_t id)
{
	return lua["sfc"]["object_names"][2][id];
}

void
graphics_w::draw_follow_options(void)
{
	static int sel = 0;
	RadioButton("Player", &sel, 0);
	SameLine();
	RadioButton("Enemy", &sel, 1);
	uint16_t camx = emulator::read_u16_le(0x1E7A);
	uint16_t camy = emulator::read_u16_le(0x1E7C);
	if (sel == 0) {
		uint16_t px = emulator::read_u16_le(0x9FA);
		uint16_t py = emulator::read_u16_le(0x9FC);
		background_x = -px - -camx;
		background_y = -(py - camy);
	} else {
		uint16_t start = 0xD18;
		uint16_t base = start + (enemy_id * 0x40);
		uint16_t x = 0, y = 0;

		if (BeginCombo("##enemyfollowdropdown", object_id_to_name(emulator::read_u8(base + 0x0A)).c_str())) {
			for (int i = 0; i < 32; ++i) {
				base = start + (i * 0x40);
				if (emulator::read_u8(base) == 0 || emulator::read_u8(base + 0x27) == 128)
					continue;
				PushID(i);

				if (Selectable(object_id_to_name(emulator::read_u8(base + 0x0A)).c_str(), enemy_id == i)) {
					enemy_id = i;
				}
				PopID();
			}
			ImGui::EndCombo();
		}
		base = start + (enemy_id * 0x40);

		if (emulator::read_u8(base) == 0 || emulator::read_u8(base + 0x27) == 128) {
			enemy_id = 0;
			enemy_id = 0;
			for (int i = 0; i < 32; ++i) {
				base = start + (i * 0x40);
				if (emulator::read_u8(base) != 0 && emulator::read_u8(base + 0x27) != 128) {
					enemy_id = i;
					break;
				}
			}
			x = emulator::read_u16_le(0x9FA);
			y = emulator::read_u16_le(0x9FC);
			x = -x - -camx;
			y = -(y - camy);
		} else {
			x = -emulator::read_u16_le(base + 5) - -camx;
			y = -(emulator::read_u16_le(base + 8) - camy);
		}
		background_x = x;
		background_y = y;

		SameLine();
		Text("Anim %d", emulator::read_u8(base + 0x13));
	}
}

void
graphics_w::draw_animation_editor(void)
{
	int _h = GetWindowHeight() - 152;

	BeginChild("animation_left_pane", ImVec2(224, 0), false);
	if (frame_valid())
		draw_animation_pane();
	EndChild();

	SameLine();

	BeginGroup();

	auto &seq = anim.seq[selected_layer];

	auto flags = ImGuiWindowFlags_NoScrollbar| ImGuiWindowFlags_NoScrollWithMouse;
	BeginChild("animation_canvas", ImVec2(0, _h), true, flags);
	int scaled = 256 * canvas_scale;

	// unused
	/*PushItemWidth(-1);
	SliderInt("Scale  ", &canvas_scale, 1, 2, "%dx");
	PopItemWidth();*/

	ImVec2	 min = ImGui::GetWindowContentRegionMin();
	ImVec2 max = ImGui::GetWindowContentRegionMax();
	/*if (max.x - min.x > 512 and max.y - min.y > 512)
		canvas_scale = 2;
	else
		canvas_scale = 1;*/
	SetCursorPos(ImVec2(
		// needs to be rounded or we get artifacts
		uint32_t(min.x + (max.x / 2.f)) - (128.f * (float)canvas_scale),
		uint32_t(min.y + (max.y / 2.f)) - (128.f * (float)canvas_scale)
	));

	BeginChild("animation_canvas_image", ImVec2(scaled, scaled), false, flags);
	ImVec2 cursor_ref = GetCursorPos();
	video::set_canvas_size(3, 512, 512);
	video::render(3);
	draw_animation_canvas();
	ImVec2 uv0(0.f, 1.f);
	ImVec2 uv1(1.f, 0.f);
	if (canvas_zoom == 2) {
		uv0 = {.25f, .75f};
		uv1 = {.75f, .25f};
	} else if (canvas_zoom == 3) {
		uv0 = {.333333333f, .666666666f};
		uv1 = {.666666666f, .333333333f};
	} else if (canvas_zoom == 4) {
		uv0 = {.375f, .625f};
		uv1 = {.625f, .375f};
	}
	Image(video::canvas_texture[3].handle, ImVec2(scaled, scaled), uv0, uv1);

	texture_hovered = false;
	canvas_hovered = false;

	// drag
	if (frame_valid()) {
		frame_t &f = seq.frames[selected_frame - 1];
		video::texture *t = video::get_texture(f.texture_id);
		auto mul = canvas_zoom;
		SetCursorPos(ImVec2(256 + (f.offx * mul), 256 + (f.offy  * mul)));
		if (t->subwidth(f.subtexture_id) > 0 && t->subheight(f.subtexture_id) > 0) {
			InvisibleButton("##animtexturebutton", ImVec2(
				t->subwidth(f.subtexture_id) * mul,
				t->subheight(f.subtexture_id) * mul
			));
		}
		if (IsItemHovered())
			texture_hovered = true;
		// this logic fixes edge issues
		if (IsItemActive() && !dragging && IsMouseDragging(0, 0.f)) {
			canvas_drag.x = f.offx;
			canvas_drag.y = f.offy;
			dragging = true;
		}
		if (dragging) {
			if (IsMouseDragging(0, 0.f)) {
				SetMouseCursor(ImGuiMouseCursor_ResizeAll);
				int dx = GetMouseDragDelta(0, 0.f).x / (canvas_zoom);
				int dy = GetMouseDragDelta(0, 0.f).y / (canvas_zoom);
				f.offx = canvas_drag.x + dx;
				f.offy = canvas_drag.y + dy;
			} else {
				dragging = false;
			}
		}
	}

	SetCursorPos(cursor_ref);
	// hack to disable click-to-grab on the canvas
	if (InvisibleButton("##anim_canvas_hack", ImVec2(scaled, scaled))) {
		//
	}
	if (IsItemHovered() && frame_valid() && IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
		if (GetIO().MouseWheel > 0.5f)
			canvas_zoom += 1;
		else if (GetIO().MouseWheel < -0.5f)
			canvas_zoom -= 1;	
		if (canvas_zoom < 1)
			canvas_zoom = 1;
		if (canvas_zoom > 4)
			canvas_zoom = 4;
		if (IsKeyPressed(SDL_SCANCODE_UP))
			seq.frames[selected_frame - 1].offy -= 1;
		if (IsKeyPressed(SDL_SCANCODE_DOWN))
			seq.frames[selected_frame - 1].offy += 1;
		if (IsKeyPressed(SDL_SCANCODE_LEFT))
			seq.frames[selected_frame - 1].offx -= 1;
		if (IsKeyPressed(SDL_SCANCODE_RIGHT))
			seq.frames[selected_frame - 1].offx += 1;
		if (IsKeyPressed(SDL_SCANCODE_H))
			seq.frames[selected_frame - 1].attrib ^= 1 << 0;
		if (IsKeyPressed(SDL_SCANCODE_V))
			seq.frames[selected_frame - 1].attrib ^= 1 << 1;
		if (IsKeyPressed(SDL_SCANCODE_DELETE) && seq.frames.size() > 1) {
			seq.frames.erase(seq.frames.begin() + selected_frame - 1);
			if (seq.frames.size() == 0)
				selected_frame = 0;
		}
		if (IsKeyPressed(SDL_SCANCODE_BACKSLASH)) {
			anim.play();
			anim.update();
			selected_frame = anim.seq[selected_layer].frame + 1;
			anim.pause();
			if (use_background && background_combo > 0 && advance_with_emulator) {
				emulator::pause();
				++emulator::run_frames;
			}
		}
		canvas_hovered = true;
	}

	EndChild();
	EndChild();

	BeginChild("animation_timeline", ImVec2(0, 80), true, ImGuiWindowFlags_HorizontalScrollbar);
	int n = 0;
	for (auto &i : seq.frames) {
		video::texture *t = video::get_texture(i.texture_id);
		bool sel = (n == selected_frame - 1);
		if (sel)
			PushStyleColor(ImGuiCol_Button, GetStyleColorVec4(ImGuiCol_ButtonActive));
		else
			PushStyleColor(ImGuiCol_Button, ImVec4(.4f, .4f, .4f, .8f));
			//PushStyleColor(ImGuiCol_Button, ImVec4(.2f, .31f, .44f, .8f));
		PushID(n);
		ImVec2 uv0(0.f, 0.f);
		ImVec2 uv1(1.f, 1.f);
		if (i.subtexture_id!="" && t->subtextures.find(i.subtexture_id)!=t->subtextures.end()) {
			uv0 = {
				t->subtextures[i.subtexture_id].uv0.x,
				t->subtextures[i.subtexture_id].uv0.y
			};
			uv1 = {
				t->subtextures[i.subtexture_id].uv1.x,
				t->subtextures[i.subtexture_id].uv1.y
			};
		}
		if (ImageButton(t->handle, ImVec2(58, 58), uv0, uv1)) {
			selected_frame = n + 1;
			seq.timer = seq.frames[n].time;
			seq.frame = selected_frame - 1;
		}
		PopStyleColor();
		if (BeginPopupContextItem(("##timelinemenu" + std::to_string(n)).c_str())) {
			if (MenuItem("Copy"))
				frame_clipboard = seq.frames[n];
			if (MenuItem("Paste") && n <= seq.frames.size() - 1)
				seq.frames[n] = frame_clipboard;
			Separator();
			if (MenuItem("Insert duplicate") && n <= seq.frames.size() - 1)
				seq.frames.insert(seq.frames.begin() + n, seq.frames[n]);
			if (MenuItem("Push back duplicate") && n <= seq.frames.size() - 1)
				seq.frames.push_back(seq.frames[n]);
			Separator();
			if (MenuItem("Move left") && n > 0) {
				std::iter_swap(seq.frames.begin() + n, seq.frames.begin() + n - 1);
				if (n == selected_frame - 1)
					--selected_frame;
				/*else if (n == selected_frame)
					++selected_frame;*/
			}
			if (MenuItem("Move right") && n < seq.frames.size() - 1) {
				std::iter_swap(seq.frames.begin() + n, seq.frames.begin() + n + 1);
				if (n == selected_frame - 1)
					++selected_frame;
				/*else if (n == selected_frame)
					--selected_frame;*/
			}
			EndPopup();
		}
		PopID();
		SameLine();
		++n;
	}
	//PushStyleColor(ImGuiCol_Button, ImVec4(.209f, .310f, .435f, .75f));
	if (Button(ICON_FA_PLUS, ImVec2(64, 64))) {
		frame_t frame;
		seq.frames.push_back(frame);
		if (seq.frames.size() == 1)
			selected_frame = 1;
	}
	//PopStyleColor();
	EndChild();

	EndGroup();
}

void
graphics_w::draw(void)
{
	Begin(ICON_FA_PICTURE_O " Graphics", &visible);

	BeginTabBar("editor_tabs");

	if (BeginTabItem("Textures")) {
		draw_viewer();
		EndTabItem();
	}

	if (BeginTabItem("Animations")) {
		draw_animation_editor();
		EndTabItem();
	}

	EndTabBar();

	End();

	file_select.draw();
	if (file_select.opened_file != "") {
		// TODO: scroll to pos
		std::string id = "unmapped.";
		id.append(std::filesystem::path(file_select.opened_file).stem());
		video::load_texture(file_select.opened_file, id);
		selected_texture = id;
		// automatic now
		/*std::filesystem::path p = video::get_texture("UNMAPPED").filename;
		p.replace_extension(".uv");
		if (std::filesystem::exists(p)) {
			video::load_atlas(video::get_texture("UNMAPPED"));
		}*/
		file_select.opened_file = "";
	}
}

} // namespace
