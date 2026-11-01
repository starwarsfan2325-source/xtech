#ifndef XPP_XGUI_GRAPHICS_HPP
#define XPP_XGUI_GRAPHICS_HPP

#include "window.hpp"
#include "file_select_w.hpp"
#include "../animation.hpp"

namespace xpp::xgui {
	struct graphics_w : window_t {
		ImGuiTextFilter texture_filter;
		std::string selected_texture = "";
		std::string selected_subtexture = "";
		animation_t anim;
		int16_t selected_frame = 0;
		uint8_t selected_layer = 0;
		int gfx_scale = 1;
		int canvas_scale = 2;
		int canvas_zoom = 1;
		bool canvas_hovered = false;
		bool texture_hovered = false;
		bool dragging = false;
		bool use_onion_skin = false;
		int onion_skin_type = 0;
		float onion_alpha = .5f;
		bool use_background = false;
		int16_t background_x = -128;
		int16_t background_y = -112;
		float background_alpha = .3f;
		int background_combo = 0;
		bool advance_with_emulator = true;
		ImVec2 canvas_offset = {0.f, 0.f};
		ImVec2 canvas_drag = {0.f, 0.f};
		ImVec2 canvas_drag_max = {0.f, 0.f};
		char background_id[48] = {'\0'};
		char object_id[48] = {'\0'};
		char anim_input[48] = {'\0'};
		char atlas_input[48] = {'\0'};
		char map_input[48] = {'\0'};
		std::vector<std::string> anim_list;
		std::vector<std::string> anim_dir_list;
		int anim_combo = 0;
		uint8_t enemy_id = 0;
		frame_t frame_clipboard;
		int mousex = 0;
		int mousey = 0;
		int clickx = 0;
		int clicky = 0;

		file_select_w file_select;

		graphics_w(void);
		~graphics_w(void);
		// HACK: we can't use members as default arguments, so...
		bool frame_valid(int16_t f = -15997);
		void draw_image(video::texture &t);
		void draw_texture_canvas(void);
		void draw_viewer(void);
		void draw_animation_canvas(void);
		void refresh_anim_list(void);
		void draw_animation_pane(void);
		void draw_follow_options(void);
		void draw_animation_editor(void);
		void draw(void);
	};
}

#endif
