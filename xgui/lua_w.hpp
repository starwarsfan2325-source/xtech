#ifndef XPP_XGUI_LUA_HPP
#define XPP_XGUI_LUA_HPP

#include "window.hpp"
#include <filesystem>

namespace xpp::xgui {
	struct lua_w : window_t {
		ImGuiTextFilter filter;
		size_t recurse_inc = 0;
		std::string selected_script = "./main.lua";
		bool hex_values = false;
		bool match_keys = true;
		bool match_values = true;
		bool hide_input_events = true;

		lua_w(void);
		~lua_w(void);
		void draw(void);
		void draw_lua_code(void);
		void draw_scripts(void);
		void draw_watches(void);
		void draw_globals(void);
		void draw_events(void);
		void table_recurse(sol::table &parent, sol::object const &key, sol::object &value);
		bool table_recurse_test(const int level, sol::table &parent, sol::object const &key, sol::object &value);
		bool table_recurse_look(const int level, sol::table &parent, sol::object const &key, sol::object &value);
		void goto_error(std::string file, int line);
	};
}

#endif
