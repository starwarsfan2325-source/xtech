#ifndef XPP_XGUI_HPP
#define XPP_XGUI_HPP

#include "../core.hpp"
#include "../video.hpp"
#include "../examples/common/imgui/imgui.h"
#include "../imgui/examples/imgui_impl_sdl.h"
#include "../sol/sol.hpp"
//#include "widgets.hpp"

namespace xpp::xgui {
	struct window_t;
	extern std::vector<window_t *> windows;
	extern ImFont *font_h1;
	extern ImFont *font_h2;
	extern ImFont *font_h3;
	extern ImFont *font_mono;
	extern int game_scale;
	extern bool game_visible;
	extern bool emu_run;
	extern float emu_alpha;

	void init(void);
	void begin(void);
	void end(void);
	void draw_windows(void);
	window_t *get_window(std::string title);
	ImFont *imgui_load_font(std::string filename, float size);
	void cleanup(void);

	template<typename T> T *
	add_window()
	{
		T *w = new T();
		windows.push_back(w);
		return w;
	}
}

#endif
