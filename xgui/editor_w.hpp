#ifndef XPP_XGUI_EDITOR_HPP
#define XPP_XGUI_EDITOR_HPP

#include "window.hpp"
#include <filesystem>

namespace xpp::xgui {
	struct editor_w : window_t {
		int drag_ox = 0;
		int drag_oy = 0;
		int drag_object = -1;
		int tileset_scale = 3;

		editor_w(void);
		~editor_w(void);
		void draw(void);
		void draw_canvas(void);
		void draw_canvas_objects(ImVec2 &cursor_ref);
		void draw_canvas_tiles(ImVec2 &cursor_ref);
		void draw_tile_tab(int layer);
		void draw_object_tab(void);
	};
}

#endif
