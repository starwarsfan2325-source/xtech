#include "editor_w.hpp"
#include "../editor.hpp"
#include "../input.hpp"
#include "../event.hpp"
#include <fstream>

using namespace ImGui;
using namespace xpp::editor;

namespace xpp::xgui {

editor_w::editor_w(void) : window_t("Level Editor")
{
	//_t = video::load_texture("image.png");
}

editor_w::~editor_w(void)
{
}

void
editor_w::draw_canvas_objects(ImVec2 &cursor_ref)
{
	// iterate in reverse order, if we don't the objects in the back get priority
	//for (auto &i : editor::level.objects) {
	int n = level.objects.size() - 1;
	auto &io = GetIO();
	for (auto i = level.objects.rbegin(); i != level.objects.rend(); ++i) {
		auto &obj = *i;
		if (object_list.find(obj.id) == object_list.end())
			continue; // TODO: show empty objects

		auto &entry = object_list[obj.id];
		int ow = entry.level_preview.width;
		int oh = entry.level_preview.height;
		std::string id = "##obj_button" + std::to_string(n);
		std::string popup_id = id + "popup";

		SetCursorPos(ImVec2(cursor_ref.x + (obj.x * 2), cursor_ref.y + (obj.y * 2)));
		// TODO: select back-and-forth between two items on top of eachother
		//if (editor::selected_object != n)
		InvisibleButton(id.c_str(), ImVec2(ow * 2, oh * 2));
		// select on press instead of release
		if (IsItemHovered() && IsMouseClicked(0))
			selected_object = n;

		//printc("a %d", io.MouseClickedPos[0]);
		// kinda works but not on the edges, maybe use dummy
		// or ImGui::IsItemHoveredRect(), ImGui::ButtonBehavior() itemHovered
		// https://github.com/ocornut/imgui/issues/1013

		// this logic fixes edge issues
		if (mode == MODE_OBJECT) {
			if (IsItemHovered() && drag_object == -1 && IsMouseDragging(0)) {
				drag_ox = obj.x;
				drag_oy = obj.y;
				drag_object = n;
			}
			if (drag_object == n) {
				if (IsMouseDragging(0)) {
					SetMouseCursor(ImGuiMouseCursor_ResizeAll);
					int dx = GetMouseDragDelta().x / 2;
					int dy = GetMouseDragDelta().y / 2;
					// no snap if alt is held
					if (input::getkey(SDL_SCANCODE_LALT)) {
						obj.x = drag_ox + dx;
						obj.y = drag_oy + dy;
					} else {
						obj.x = (drag_ox / 16 * 16) + (dx / 16 * 16);
						obj.y = (drag_oy / 16 * 16) + (dy / 16 * 16);
					}
					// TODO: don't let it out of upper bounds
					if (obj.x < 0)
						obj.x = 0;
					if (obj.y < 0)
						obj.y = 0;
				} else {
					drag_object = -1;
				}
			}
		}
		if (BeginPopupContextItem(popup_id.c_str())) {
			if (MenuItem("Flip", "H"))
				obj.flip = !obj.flip;
			if (MenuItem("Respawn", NULL, obj.respawn))
				obj.respawn = !obj.respawn;
			if (MenuItem("Die off-screen", NULL, true))
				obj.respawn = !obj.respawn;
			Separator();
			if (MenuItem("Edit events")) // [re]spawn script, die script, and properties
				{};
			EndPopup();
		}

		--n;
	}
}

void
editor_w::draw_canvas_tiles(ImVec2 &cursor_ref)
{
	// TODO: eyedropper cursor, use ctrl for block select too
	int tmousex = (editor::mousex + scrollx) / 16;
	int tmousey = (editor::mousey + scrolly) / 16;
	bool ctrl = input::getkey(SDL_SCANCODE_LCTRL);
	if (ctrl && mouse_in_canvas()) {
		SetMouseCursor(ImGuiMouseCursor_Hand);
		if (IsMouseClicked(0)) {
			// yikes
			selected_tile = tiles->get_tile(tmousex, tmousey) - 1;
			selected_tile_max = 0;
			selected_level_tile = tiles->map_xy_to_index(tmousex, tmousey);
			selected_level_tile_max = -1;
			drag_ox = tmousex;
			drag_oy = tmousey;
			selecting_block = false;
		}
	}
	if (ctrl && mouse_in_canvas() && !selecting_block && IsMouseDragging(0)) {
		selecting_block = true;
	}
	if (selecting_block) {
		drag_object = -1;
		if (ctrl && mouse_in_canvas() && IsMouseDragging(0)) {
			int div = 16 * 2;
			int _ox, offx;
			if (tmousex < drag_ox) {
				offx = 0;
				_ox = drag_ox + 1;
			} else {
				offx = 1;
				_ox = drag_ox;
			}
			int _oy, offy;
			if (tmousey < drag_oy) {
				offy = 0;
				_oy = drag_oy + 1;
			} else {
				offy = 1;
				_oy = drag_oy;
			}
			selected_level_tile = tiles->map_xy_to_index(_ox, _oy);
			selected_level_tile_max = tiles->map_xy_to_index(tmousex + offx, tmousey + offy);
		} else {
			selecting_block = false;
			// cancel / don't use block in case of 1x1 selection
			if (selected_level_tile_max - selected_level_tile == tiles->map_width + 1)
				selected_level_tile_max = -1;
			else
				block_clipboard = block_from_map(selected_level_tile, selected_level_tile_max);
		}
	}
}

void
editor_w::draw_canvas(void)
{
	video::set_canvas_size(0, tiles->map_width * 16, tiles->map_height * 16);
	BeginChild("editor_canvas");
	uint16_t cw = video::canvas_texture[0].width;
	uint16_t ch = video::canvas_texture[0].height;
	int _scrollx = 0, _scrolly = 0;
	// hack to disable click-to-grab on the canvas
	ImVec2 cursor_ref = GetCursorPos();

	BeginChild("editor_canvas_image", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
	_scrollx = GetScrollX();
	_scrolly = GetScrollY();
	float scroll_x = ImGui::GetScrollX();
	float scroll_max_x = ImGui::GetScrollMaxX();

	if (mouse_in_canvas()) {
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
		SetScrollX(GetScrollX() + delta.x);
		SetScrollY(GetScrollY() + delta.y);
	}

	// TODO: clamp negative values
	mousex = (input::getmousex() - GetCursorScreenPos().x - _scrollx) / 2;
	mousey = (input::getmousey() - GetCursorScreenPos().y - _scrolly) / 2;
	scroll_mousex = (input::getmousex() - GetCursorScreenPos().x) / 2;
	scroll_mousey = (input::getmousey() - GetCursorScreenPos().y) / 2;
	// but what if it's on an odd number scroll?
	editor::scrollx = (_scrollx) / 2;
	editor::scrolly = (_scrolly) / 2;

	editor::draw();
	Image(video::canvas_texture[0].handle, ImVec2(cw * 2, ch * 2), ImVec2(0, 1), ImVec2(1, 0));
	// -6: scrollbar hitbox is ~6px wide/tall
	editor::canvas_window_width = (GetWindowSize().x / 2) - 6;
	editor::canvas_window_height = (GetWindowSize().y / 2) - 6;

	if (mode == MODE_OBJECT) {
		draw_canvas_objects(cursor_ref);
	} else if (mode == MODE_TILE) {
		draw_canvas_tiles(cursor_ref);
	}

	SetCursorPos(cursor_ref);
	// can't use as button event due to imgui ordering...
	if (InvisibleButton("##canvas_hack", ImVec2(cw * 2, ch * 2))) {
		event::fire<int>("editor.canvas_click", 0);
	}

	// TODO: use x text rendering
	SetCursorPos(ImVec2(GetScrollX() + 4, GetScrollY() + 4));
	Text("%.0f\%, %.0f\%", GetScrollX()/GetScrollMaxX()*100, GetScrollY()/GetScrollMaxY()*100);

	EndChild();
	//

	//
	//SetCursorPos(ImVec2(cursor_ref.x, cursor_ref.y + (224 * 2) + 4));
	//Text("%d, %d, %d", editor::mousex, editor::mousey, editor::selected_object);
	EndChild();
}

void
editor_w::draw(void)
{
	Begin(ICON_FA_CUBES " Level Editor", &visible);

	BeginChild("editor_left_pane", ImVec2(320, 0), false);
	BeginTabBar("editor_tabs");

	if (BeginTabItem("Properties")) {
		EndTabItem();
	}

	if (BeginTabItem("Tiles")) {
		mode = MODE_TILE;
		BeginChild("editor_tile_tab");

		draw_tile_tab(0);

		EndChild();
		EndTabItem();
	}

	if (BeginTabItem("Objects")) {
		mode = MODE_OBJECT;
		BeginChild("editor_object_tab");

		draw_object_tab();

		EndChild();
		EndTabItem();
	}

	if (BeginTabItem("Events")) {
		mode = MODE_EVENT;
		EndTabItem();
	}

	EndTabBar();
	EndChild();

	SameLine();

	draw_canvas();

	End();
}

void
editor_w::draw_tile_tab(int layer)
{
	video::texture &tileset = tiles->tileset;
	float tsw = tileset.width * tileset_scale;
	float tsh = tileset.height * tileset_scale;

	Button(ICON_FA_PLUS " Open Tileset");

if (CollapsingHeader(tileset.filename.c_str(), ImGuiTreeNodeFlags_None)) {
	BeginChild("editor_tileimg", ImVec2(0, tsh+(8*tileset_scale)), true, ImGuiWindowFlags_NoScrollbar);
	SetCursorPos(ImVec2((GetWindowSize().x - tsw) / 2, (GetWindowSize().y - tsh) / 2));
	ImVec2 cursor_ref = GetCursorPos();
	ImVec2 screen_ref = GetCursorScreenPos();
	int _scrollx = GetScrollX();
	int _scrolly = GetScrollY();
	int div = 16 * tileset_scale;
	ImDrawList *draw_list = GetWindowDrawList();
	// TODO: dynamic resize
	if (bgfx::isValid(tileset.handle)) {
		ImVec2 uv0(0, 0);
		ImVec2 uv1(1, 0.25f);
		Image(tileset.handle, ImVec2(tsw, tsh), uv0, uv1);
		SetCursorPos(cursor_ref);
		InvisibleButton("invis_tileset_button", ImVec2(tsw, tsh));
		ImVec2 after_cursor = GetCursorPos();
		int offx = (input::getmousex() - screen_ref.x - _scrollx);
		int offy = (input::getmousey() - screen_ref.y - _scrolly);

		if (IsItemHovered() && IsMouseClicked(0)) {
			selected_level_tile_max = -1; // reset block
			selected_tile = tiles->tile_xy_to_index(offx / div, offy / div);
			selected_tile_max = -1;
		}
		if (IsItemHovered() && drag_object == -1 && IsMouseDragging(0)) {
			// index_to_xy fixes issue when grabbing around tile edges
			drag_object = 1;
		}
		if (drag_object != -1) {
			if (IsItemHovered() && IsMouseDragging(0)) {
				// yikes
				int dx = offx / div;
				int dy = offy / div;
				int otx = tiles->tile_index_to_xy(selected_tile).x;
				int oty = tiles->tile_index_to_xy(selected_tile).y;
				if (dx < otx)
					dx = otx;
				if (dy < oty)
					dy = oty;
				uint16_t t = tiles->tile_xy_to_index(dx, dy);
				if (t >= selected_tile)
					selected_tile_max = t;
				if (selected_tile_max == selected_tile)
					selected_tile_max = -1;
			} else {
				drag_object = -1;
			}
		}

		SetCursorPos(after_cursor);
	}

	vec2_16 tmin = tiles->tile_index_to_xy(selected_tile);
	vec2_16 tmax;
	if (selected_tile_max > selected_tile)
		tmax = tiles->tile_index_to_xy(selected_tile_max);
	else
		tmax = tmin;
	ImVec2 rect_min(
		screen_ref.x + (tmin.x * div) - 1,
		screen_ref.y + (tmin.y * div) - 1
	);
	ImVec2 rect_max(
		screen_ref.x + (tmax.x * div) + div + 1,
		screen_ref.y + (tmax.y * div) + div + 1
	);
	if (selected_tile != -1 && selected_level_tile_max == -1) {
		draw_list->AddRect(rect_min, rect_max, IM_COL32(255, 255, 0, 255));
		rect_min.x -= 1;
		rect_min.y -= 1;
		rect_max.x += 1;
		rect_max.y += 1;
		draw_list->AddRect(rect_min, rect_max, IM_COL32(0, 0, 0, 255));
	}

	EndChild();

	PushFont(font_mono);
	ImVec2 text_size = CalcTextSize("Selected: 00");
	SetCursorPosX((GetWindowSize().x - text_size.x) / 2);
	if (selected_level_tile_max == -1 && selected_tile_max == -1) {
		Text("Selected: %02d", selected_tile + 1);
	} else {
		Text("Selected: ++");
	}
	PopFont();

	CheckboxFlags("Horizontal flip", (unsigned int *)&attrib, 0x01);
	SameLine();
	CheckboxFlags("Vertical flip", (unsigned int *)&attrib, 0x02);

	//

	Separator();
	Text("Stamps");
	BeginChild("editor_tilestamp", ImVec2(0, 80), true);
	if (Button(ICON_FA_PLUS, ImVec2(64, 64)) && selected_level_tile_max != -1) {
		stamps.push_back(block_from_map(selected_level_tile, selected_level_tile_max));
	}
	int n = 0;
	for (auto &i : stamps) {
		SameLine();
		Button(std::to_string(n).c_str(), ImVec2(64, 64));
		++n;
	}
	EndChild();
	Button("Edit stamp");
}
}

void
editor_w::draw_object_tab(void)
{
	//BeginChild("editor_object_picker", ImVec2(320 - 12, 144), true);
	const char *listbox_items[] = {
		"x", "scriver", "capsule"
	};
	static int listbox_item_current = 1;
	PushItemWidth(320 - 80);
	ListBox("##tilesetpick", &listbox_item_current, listbox_items, IM_ARRAYSIZE(listbox_items), 3);
	selected_object_entry = listbox_items[listbox_item_current];
	PopItemWidth();
	SameLine();
	auto &obj = object_list[selected_object_entry];
	float w = obj.preview.width;
	float h = obj.preview.height;
	ImVec2 uv0(0, 0);
	ImVec2 uv1(32.f / w, 32.f / h);
	if (ImageButton(obj.preview.handle, ImVec2(64, 64), uv0, uv1))
		selected_object_entry = obj.id;

	int n = 0;
	// TODO: tree view based on types
	/*for (auto &i : object_list) {
		auto &obj = i.second;
		float w = obj.preview.width;
		float h = obj.preview.height;
		ImVec2 uv0(0, 0);
		ImVec2 uv1(32.f / w, 32.f / h);
		if (ImageButton(obj.preview.handle, ImVec2(64, 64), uv0, uv1))
			selected_object_entry = obj.id;
		if (n % 3 != 2)
			SameLine();
		++n;
	}*/
	//EndChild();

	if (selected_object_entry != "empty" &&
		object_list.find(selected_object_entry) != object_list.end()) {

		auto &obj = object_list[selected_object_entry];

		PushFont(xgui::font_h1);
		Text(" %s #1", obj.name.c_str());
		PopFont();
		//SameLine();
		//SetCursorPos(ImVec2(GetCursorPos().x, GetCursorPos().y + 4));
		//Button("Edit Object");
		//Button("Edit events");
		Separator();

		//CollapsingHeader((obj.name + " graphics").c_str());
		//CollapsingHeader((obj.name + " events").c_str());

		if (CollapsingHeader((obj.name + " properties").c_str())) {
			//PushFont(xgui::font_h3);
			//TextUnformatted(" Properties");
			//PopFont();
			//PushFont(xgui::font_mono);
			BeginChild("editor_object_properties_child", ImVec2(0, 0), true);
			Columns(2, "editor_object_properties_columns", true);
			n = 0;
			for (auto &i : obj.properties) {
				Text("%s", i.name.c_str());
				NextColumn();
				std::string id("##objcombo_" + i.name + std::to_string(n));
				SetNextItemWidth(-1);
				if (i.type == "combo") {
					Combo(id.c_str(), &i.combo_id, i.combo);
				} else if (i.type == "boolean") {
					Checkbox(id.c_str(), &i.boolean);
				} else if (i.type == "string") {
					InputText(id.c_str(), i.input, IM_ARRAYSIZE(i.input));
				} else {
					TextUnformatted("Invalid property type");
					Separator();
				}
				NextColumn();
				++n;
			}
			EndChild();
			//PopFont();
			Columns(1);
		}
	}
	//Text("AAAA\nYYY");
}

} // namespace
