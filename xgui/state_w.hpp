#ifndef XPP_XGUI_STATES_HPP
#define XPP_XGUI_STATES_HPP

#include "window.hpp"
#include <vector>
#include <string>
#include <stdint.h>

namespace xpp::xgui {
	struct save_state_t {
		void *data;
		size_t size;
		std::string notes;
	};

	struct state_w : window_t {
		std::vector<save_state_t> save_states;
		int selected_state_id = -1;
		state_w(void);
		~state_w(void);
		void draw(void);
		void draw_manager(void);
		void new_savestate(std::string notes = "...");
		void load_savestate(int state);
		void export_savestates(std::string filename);
		void import_savestates(std::string filename);
		void clear_savestates(void);
	};

}

#endif
